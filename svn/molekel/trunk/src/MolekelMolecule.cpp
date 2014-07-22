//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//
// $Author$
// $Date$
// $Revision$
//

//GL required for GLSL
#include <GL/glew.h> // has to be first
#include <GL/gl.h>


// VTK
#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkRenderer.h>
#include <vtkWindow.h>
#include <vtkAssembly.h>
#include <vtkOutlineCornerFilter.h>
#include <vtkOutlineFilter.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkProperty.h>
#include <vtkMarchingCubes.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataMapper.h>
#include <vtkCellArray.h>
#include <vtkPropCollection.h>
#include <vtkReverseSense.h>
#include <vtkMatrix4x4.h>
#include <vtkLookupTable.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkArrowSource.h>
#include <vtkTransformFilter.h>
#include <vtkTransform.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkLoopSubdivisionFilter.h>

// Inventor
#include <Inventor/SoDB.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoMaterial.h>


//OpenMOIV
#include <ChemKit2/ChemData.h>
#include <ChemKit2/ChemAssociatedData.h>
#include <ChemKit2/util/ChemPDBImporter.h>
#include <ChemKit2/ChemInit.h>
#include <ChemKit2/ChemDisplayParam.h>
#include <ChemKit2/ChemDisplay.h>
#include <ChemKit2/ChemSelection.h>
#include <ChemKit2/ChemColor.h>
#include <ChemKit2/ChemRadii.h>
#include <ChemKit2/util/ChemFileImporter.h>
#include <ChemKit2/util/ChemMOLImporter.h>
#include <ChemKit2/ChemConnollyDot.h>

//OpenBabel
#include <openbabel/mol.h>
#include <openbabel/obconversion.h>
#include <openbabel/obmolecformat.h>

// STD
#include <cmath>
#include <string>
#include <cctype>
#include <cstdio>
#include <map>
#include <functional>
#include <sstream>

// Molekel
#include "utility/vtkSoMapper.h"
#include "utility/Geometry.h"
#include "old/molekeltypes.h"
#include "utility/OBGridData.h"
#include "utility/OBT41Data.h"
#include "old/constant.h"
#include "MolekelMolecule.h"
#include "MolekelException.h"
#include "MoleculeActorPickCommand.h"
#include "utility/ElementTable.h"
#include "utility/MolekelChemPDBImporter.h"
#include "utility/vtkGLSLShaderActor.h"
#include "utility/Timer.h"
#include "utility/System.h"
#include "utility/vtkMSMSReader.h"
#include "utility/vtkOpenGLGlyphMapper.h"

using namespace std;
using namespace OpenBabel;

extern bool GLSLShadersSupported(); // used to check if GLSL shaders are supported
                                    // this function is invoked whenever a new vtkActor
                                    // is created to decide if a vtkGLSLShaderActor or
                                    // simple actor should be created

// use old Molekel code to compute electron density
extern vtkImageData* vtk_process_calc( Molecule *mol, float *dim, int *ncubes, int key,
                                       void ( *progressCBack )( int, int, void* ) = 0,
                                       void* cbackData = 0 );

/// Class used to find in a collection a vtkSmartPointer< T > where
/// vtkSmartPointer< T type >.GetPointer() == T type *
template < class T > class FindVtkPointer
{
    T* p_;
public:
    FindVtkPointer( T* p ) : p_( p ) {}
    //FindVtkPointer( const FindVtkPointer< T >& vp ) : p_( vp.p_ ) {}
    bool operator()( const vtkSmartPointer< T >& a ) const { return a.GetPointer() == p_; }
};


/// @note using Qt4 with MinGW on windows separators seem to be automatically
/// set to '/'
static const char EXTENSION_SEPARATOR = '.';
//#ifdef WIN32
//	static const char PATH_SEPARATOR = '\\';
//#else
    static const char PATH_SEPARATOR = '/';
//#endif


//------------------------------------------------------------------------------
MolekelMolecule::MolekelMolecule() : chemData_( 0 ),
                        chemSelection_( 0 ),
                        chemDisplayParam_( 0 ),
                        chemDisplay_( 0 ),
                        chemRadii_( 0 ),
                        chemColor_( 0 ),
                        obMol_( 0 ),
                        molekelMol_( 0 ),
                        updater_( 0 ),
                        dipoleMomentArrowLength_( 2. ),
                        connollyDot_( 0 ),
                        sesSwitch_( 0 ),
                        stopSASComputation_( false ),
                        stopSESComputation_( false ),
                        stopSESMSComputation_( false )

{
    // initialize shader program objects to default
    ShaderProgram sp;
    shaderSurfaceMap_[ MOLECULE_SURFACE ] = sp;
    shaderSurfaceMap_[ SAS_SURFACE ] = sp;
    shaderSurfaceMap_[ SES_SURFACE ] = sp;
    shaderSurfaceMap_[ SESMS_SURFACE ] = sp;
    shaderSurfaceMap_[ GRID_DATA_SURFACE ] = sp;
    shaderSurfaceMap_[ DENSITY_MATRIX_SURFACE ] = sp;
    shaderSurfaceMap_[ ORBITAL_NEGATIVE_SURFACE ] = sp;
    shaderSurfaceMap_[ ORBITAL_NODAL_SURFACE ] = sp;
    shaderSurfaceMap_[ ORBITAL_POSITIVE_SURFACE ] = sp;
}



//------------------------------------------------------------------------------

// temporary, used to profile MolekelMolecule::New()
// MinGW's gprof output always returns a time of 0.0 ms for MolekelMolecule::New()
#ifdef TMP_MOLEKEL_PROFILE

namespace
{
    class TimerFun
    {
        const char* msg_;
    public:
        TimerFun( const char* msg = 0 ) : msg_( msg ) {}
        void operator()( double elapsed ) const
        { printf( "\n %s %fs\n", msg_, elapsed ); }
    };
}

#endif

MolekelMolecule* MolekelMolecule::New( const char* fname,
                                       const char* format,
                                       ILoadMoleculeCallback* cb,
                                       bool computeBonds )
{

#ifdef TMP_MOLEKEL_PROFILE
    Timer< TimerFun > t( TimerFun( "OpenBabel + MOIV load time:" ) );
#endif
    string obformat = format;
    string fn( fname );

    // read with OpenBabel
    OBConversion obConversion;
    // disable bond computation for pdbs, for 1AON.pdb it cuts load time
    // from 30 to 10s (Intel P4 Xeon 3Ghz, 4GB RAM, Win XP Pro sp2)
    // compute bonds only in case of multi-molecule files

    if( obformat == "pdb" ) obConversion.AddOption( "b", OBConversion::INOPTIONS );
    obConversion.SetInFormat( obformat.c_str() );

    StopWatch stopWatch;
    if( cb ) stopWatch.Start();
    MolekelMolecule* mol = new MolekelMolecule; // return this
    int frameCounter = 0;
    {
#ifdef TMP_MOLEKEL_PROFILE
        Timer< TimerFun > t1( TimerFun( "OpenBabel load time:" ) );
#endif
        bool ok = false;
        // multi-molecule format
        if( obformat == "pdb" || obformat == "xyz" )
        {
            ifstream in( fname );
            // read each molecule in the file and add it ot the frames_ array
            if( cb ) cb->StatusMessage( "Reading molecule from file " + string( fname ) + "..." );
            do
            {
                OBMol* obm = new OBMol;
                ok = obConversion.Read( obm, &in );
                if( !ok ) delete obm;
                else
                {
                    ++frameCounter;
                    mol->frames_.push_back( obm );
                    ostringstream msg;
                    msg << "Read frame " << frameCounter;
                    if( cb ) cb->StatusMessage( msg.str() );
                }
            } while( ok );
        }
        else // single molecule format
        {
            OBMol* obm = new OBMol;
            ok = obConversion.ReadFile( obm, fname );
            if( !ok ) delete obm;
            else mol->frames_.push_back( obm );
        }
    }
    // nothing read from file: delete molecule and throw exception
    if( mol->GetNumberOfFrames() == 0 )
    {
        delete mol;
        cb->StatusMessage( string( "Error reading file " ) + fname );
        throw MolekelException( "Cannot read file: " + fn + " with OpenBabel" );
    }

    mol->obMol_ = mol->frames_[ 0 ];

    // in case the format is pdb bond computation is turned off, have OB recompute bonds
    // in this case
    frameCounter = 0;
    if( mol->GetNumberOfFrames() > 1 && obformat == "pdb" && computeBonds )
    {
       for( Frames::iterator i = mol->frames_.begin();
             i != mol->frames_.end();
             ++i )
        {
            ++frameCounter;
            ostringstream msg;
            msg << "Computing bonds for frame " << frameCounter;
            cb->StatusMessage( msg.str() );
            ( *i )->ConnectTheDots();
            ( *i )->PerceiveBondOrders();
        }
    }

    /// @warning hack to address Inventor initialization
    /// it's cleaner to move code in some constructor
    static bool inited = false;
    if( !inited )
    {
        SoDB::init();
        SoNodeKit::init();
        ChemInit::initClasses();
        ChemConnollyDot::initClass();

        glewInit(); // for GLSL shader support

        inited = true;
    }

    // if GLSL shaders are supported create GLSL shader actor, default actor otherwise
    /// @todo Consider adding a boolean parameter to New() function to disable GLSL shaders
    /// a per-instance boolean flag is required to decide how to create per-surface actors:
    /// if GLSL is disabled at creation time do not create GLSL enabled actors for surfaces/
    if( !GLSLShadersSupported() ) mol->actor_ = vtkActor::New();
    else
    {
        mol->shaderSurfaceMap_[ MOLECULE_SURFACE ].actors.push_back( vtkGLSLShaderActor::New() );
        mol->actor_ = mol->shaderSurfaceMap_[ MOLECULE_SURFACE ].actors.back();
    }


    // Create inventor SceneGraph
    // Acquire ownership of created objects: this is required
    // in case they have to be deleted before being added
    // to the scenegraph since calling
    // unref() on an object with zero referece count works but
    // causes Inventor to issue a warning.
    SoSeparator *root = new SoSeparator;
    root->ref();
    root->setName( "root" );
    SoSeparator* moiv = new ChemSelection;
    moiv->ref();
    mol->chemData_ = new ChemData;
    mol->chemData_->ref();
    mol->chemAssociatedData_ = new ChemAssociatedData;
    mol->chemAssociatedData_->ref();

    // if extension != pdb or mol use openbabel to convert
    // file into mol then load file with OpenMOIV

    // if extension == .log also load molecule with old
    // molekel source code for further processing of
    // dynamics and MO


    ChemFileImporter* fi = 0;
    //if( obformat == "pdb" ) fi = new ChemPDBImporter;
    // use Molekel's version of ChemPDBImporter: copied from ChemPDBImporter
    // note that if bonds have already been computed for molecule they will be recomputed
    // here (for the first frame only though).
    if( obformat == "pdb" ) fi = new MolekelChemPDBImporter;
    else if( obformat == "mol" ) fi = new ChemMOLImporter;

    if( fi == 0 ) // no pdb nor mol --> convert from loaded OBMol to OpenMOIV
    {
        extern void OpenBabelToMOIV( OBMol*, ChemData*, ChemAssociatedData* );
        OpenBabelToMOIV( mol->obMol_, mol->chemData_, mol->chemAssociatedData_ );
    }
    else
    {
        if( !fi->openFile( fname, mol->chemData_, mol->chemAssociatedData_ ) )
        {
            mol->chemAssociatedData_->unref();
            mol->chemData_->unref();
            moiv->unref();
            delete mol->obMol_;
            delete mol;
            root->unref();
            throw MolekelException( "Cannot read file: " + fn + " with OpenMOIV" );
        }
    }

    delete fi;

    // read with old molekel code
    if( obformat == "g98" || obformat == "g03" )
    {

        extern Molecule *read_gauss( const char *name );

        mol->molekelMol_ = read_gauss( fname );

        if( mol->molekelMol_ == 0 )
        {
            mol->chemAssociatedData_->unref();
            mol->chemData_->unref();
            moiv->unref();
            delete mol;
            root->unref();
            throw MolekelException( "Cannot read: " + fn + " with Molekel 4.6 read_gauss() function" );
        }
    }
    else if( obformat == "gam" || obformat == "gamout" )
    {

        extern Molecule *read_gamess( const char *name );

        mol->molekelMol_ = read_gamess( fname );

        if( mol->molekelMol_ == 0 )
        {
            mol->chemAssociatedData_->unref();
            mol->chemData_->unref();
            moiv->unref();
            delete mol;
            root->unref();
            throw MolekelException( "Cannot read: " + fn + " with Molekel 4.6 read_gamess() function" );
        }
    }
    else if ( obformat == "molden" )
    {
        extern Molecule *read_molden( const char *name );

        // will try to read additional information from the molden file
        mol->molekelMol_ = read_molden( fname );
//
//        if( mol->molekelMol_ == 0 )
//        {
//            mol->chemAssociatedData_->unref();
//            mol->chemData_->unref();
//            moiv->unref();
//            delete mol;
//            root->unref();
//            throw MolekelException( "Cannot read: " + fn + " with Molekel 4.6 read_molden() function" );
//        }
    }

    if( cb )
    {
        stopWatch.Stop();
        ostringstream oss;
        oss << "Molecule loaded (" << stopWatch.GetElapsedTime() << "s)";
        cb->StatusMessage( oss.str() );
    }
    //////////////////////////
    // If this point is reached it means the molecule contains all the data
    // stored in OpenMOIV, OpenBabel & Molekel structures.
    mol->chemDisplayParam_ = new ChemDisplayParam();
    mol->chemDisplay_ = new ChemDisplay;
    mol->chemColor_   = new ChemColor;
    mol->chemRadii_	  = new ChemRadii;

    // Set radii to proper values: OpenMOIV sets most of the atom radii to 1 this means that a lot
    // of metal atoms have radii with the same size as the Hydrogen atom!
    const int ATOMS = GetElementTableSize();
	std::vector< float > radii( ATOMS );
    const MolekelElement* pe = GetElementTable();
    for( int r = 0; r != ATOMS; ++r ) radii[ r ] = pe[ r ].vdwRadius;
	mol->chemRadii_->atomRadii.setValues( 0, ATOMS, &( *radii.begin() ) );

    // Set default parameters to minimum detail to speed up first rendering
    // (as done in most of the Molecular Visualization Packages e.g. VMD)
	static const int WIREFRAME_THRESHOLD = 1000; 
    mol->chemDisplayParam_->displayStyle.setValue( 
		mol->GetNumberOfAtoms() < WIREFRAME_THRESHOLD ? ChemDisplayParam::DISPLAY_BALLSTICK 
		: ChemDisplayParam::DISPLAY_WIREFRAME );
    mol->chemDisplayParam_->residueInterpolateColor.setValue( false );
    mol->chemColor_->residueColorBinding.setValue( ChemColor::RESIDUE_PER_INDEX );
    mol->chemDisplayParam_->showMultipleBonds.setValue( true );
    mol->chemDisplayParam_->solidRibbonSmoothNormals.setValue( false );


    // default value for ballStickSphereScaleFactor == 0.2
    //mol->chemDisplayParam_->ballStickSphereScaleFactor.setValue( 0.3 );
    // default value for bondCylinderRadius == 0.15
    //mol->chemDisplayParam_->bondCylinderRadius.getValue()

    string::size_type path_Separator = fn.rfind( PATH_SEPARATOR );
    if( path_Separator != string::npos ) ++path_Separator;
    mol->fname_ = string( fn, path_Separator );
    mol->path_ = fn;
    mol->format_ = format;

    /// @warning '.' is not an allowed character for SoNode::setName() method
    /// name is not currently used anyway, if needed strip off .extension
    //mol->chemData_->setName( mol->fname_.c_str() );
    mol->material_ = new SoMaterial;
    mol->material_->ref();
    mol->material_->ambientColor.setValue( SbColor( 0.f, 0.f, 0.f ) );
    mol->material_->diffuseColor.setValue( SbColor( 0.8f, 0.8f, 0.8f ) );
    mol->material_->specularColor.setValue( SbColor( 0.8f, 0.8f, 0.8f ) );
    mol->material_->shininess.setValue( 1.f );
    moiv->addChild( mol->material_ );
    moiv->addChild( mol->chemData_ );
    moiv->addChild( mol->chemColor_ );
    moiv->addChild( mol->chemRadii_ );
    moiv->addChild( mol->chemDisplayParam_ );
    moiv->addChild( mol->chemDisplay_ );

    // create node for connolly surface
    mol->connollyDot_ = new ChemConnollyDot;
    // Connolly Dot Fields:
    //    SoSFFloat probeRadius;
    //    SoSFFloat densityOfPoints;
    //    SoSFEnum  colorBinding;
    //    SoSFColor overallSurfaceColor;
    //    SoSFColor contactSurfaceColor;
    //    SoSFColor saddleSurfaceColor;
    //    SoSFColor concaveSurfaceColor;
    mol->connollyDot_->colorBinding = ChemConnollyDot::BY_ATOM;
    // create switch node to show/hide connolly dot surface
    mol->sesSwitch_ = new SoSwitch;
    mol->sesSwitch_->whichChild = SO_SWITCH_NONE;
    mol->sesSwitch_->addChild( mol->connollyDot_ );
    moiv->addChild( mol->sesSwitch_ );
    // add whole molecule into scenegraph
    root->addChild( moiv );
    mol->chemSelection_ = ( ChemSelection* ) root->getChild( 0 );

    // create vtkSoMapper instance, use sphere as input, all events
    // received by sphere will then be routed to Inventor root node
    vtkSoMapper* m = vtkSoMapper::New();
    /// @todo remove all initialization code below and put it into
    /// vtkSoMapper or create new class vtkSoActor
    m->SetImmediateModeRendering( true ); // <- won't work properly without this
    m->SetRoot( root );

    // release references
    mol->chemData_->unref();
    mol->chemAssociatedData_->unref();
    moiv->unref();

    // create box object with same size as molecule bounding box;
    // the box will not be shown but used to interact with user events
    // sent through VTK;
    // the molecule will be entirely contained inside the sphere
    mol->boundingBox_ = vtkCubeSource::New();
    mol->boundingBox_->SetCenter( .5 * ( m->GetBounds()[ 0 ] +  m->GetBounds()[ 1 ] ),
                   .5 * ( m->GetBounds()[ 2 ] +  m->GetBounds()[ 3 ] ),
                   .5 * ( m->GetBounds()[ 4 ] +  m->GetBounds()[ 5 ] ) );

    mol->boundingBox_->SetBounds( m->GetBounds() );
    m->SetInputConnection( mol->boundingBox_->GetOutputPort() );
    m->Update();

    // create iso box object
    mol->isoBoundingBox_ = vtkCubeSource::New();
    mol->isoBoundingBox_->SetCenter( .5 * ( m->GetBounds()[ 0 ] +  m->GetBounds()[ 1 ] ),
                   .5 * ( m->GetBounds()[ 2 ] +  m->GetBounds()[ 3 ] ),
                   .5 * ( m->GetBounds()[ 4 ] +  m->GetBounds()[ 5 ] ) );
    mol->isoBoundingBox_->SetBounds( m->GetBounds() );

    // set actor-mapper
    mol->actor_->SetMapper( m );
    m->SetActor( mol->actor_ );
    //vtkSmartPointer< vtkProperty > actorProperty = vtkProperty::New();
    //actorProperty->SetOpacity( 1.0 );
    //mol->actor_->SetProperty( actorProperty );
    // create bounding box
    mol->bbox_ = vtkActor::New();
    vtkSmartPointer< vtkOutlineCornerFilter > cs( vtkOutlineCornerFilter::New() );
    cs->SetInput( mol->boundingBox_->GetOutput() );
    vtkSmartPointer< vtkPolyDataMapper > bbmapper( vtkPolyDataMapper::New() );
    bbmapper->SetInputConnection( cs->GetOutputPort() );
    bbmapper->Update();
    mol->bbox_->SetMapper( bbmapper );
    mol->bbox_->SetVisibility( false );
    mol->bbox_->SetPickable( false );

    // create bounding box for isosurface computation
    mol->isoBBox_ = vtkActor::New();
    vtkSmartPointer< vtkOutlineFilter > of( vtkOutlineFilter::New() );
    of->SetInput( mol->isoBoundingBox_->GetOutput() );
    vtkSmartPointer< vtkPolyDataMapper > isoBBmapper( vtkPolyDataMapper::New() );
    isoBBmapper->SetInputConnection( of->GetOutputPort() );
    isoBBmapper->Update();
    mol->isoBBox_->SetMapper( isoBBmapper );
    mol->isoBBox_->SetVisibility( false );
    mol->isoBBox_->SetPickable( false );
    mol->isoBBox_->GetProperty()->SetLineStipplePattern( 0xAA );

    // add objects to assembly
    mol->assembly_ = vtkAssembly::New();
    mol->assembly_->AddPart( mol->actor_ );
    mol->assembly_->AddPart( mol->bbox_ );
    mol->assembly_->AddPart( mol->isoBBox_ );


    return mol;

}


//------------------------------------------------------------------------------
MolekelMolecule* MolekelMolecule::New( const char* fname,
									   ILoadMoleculeCallback* cb,
									   bool computeBonds )
{
    // extract extension and convert to lowercase
    string fn( fname );
    string::size_type dot = fn.rfind( EXTENSION_SEPARATOR );
    if( dot == string::npos ) throw MolekelException( "Unknown file type: " + fn );
    ++dot;
    string format( fn, dot );

    transform( format.begin(), format.end(), format.begin(),( int ( * )( int ) ) tolower );

    // if extension == .log set extension to g98 before
    // reading with OpenBabel
    string obformat = format == "log" ? "g98" : format;
    return New( fname, obformat.c_str(), cb, computeBonds );

}

//------------------------------------------------------------------------------
void MolekelMolecule::Save( const char* fname ) const
{
    // extract extension and convert to lowercase
    string fn( fname );
    string::size_type dot = fn.rfind( EXTENSION_SEPARATOR );
    if( dot == string::npos ) throw MolekelException( "Unknown file type: " + fn );
    ++dot;
    string format( fn, dot );

    transform( format.begin(), format.end(), format.begin(),( int ( * )( int ) ) tolower );

    // if extension == .log set extension to g98 before
    // reading with OpenBabel
    string obformat = format == "log" ? "g98" : format;

    // write with OpenBabel
    OBConversion obConversion;
    obConversion.SetOutFormat( obformat.c_str() );
    if( !obConversion.WriteFile( obMol_, fname ) )
    {
         throw MolekelException( "Cannot write file " + string( fname ) + " with OpenBabel" );
    }
}


//------------------------------------------------------------------------------
void MolekelMolecule::Save( const char* fname, const char* fmt ) const
{
    // extract extension and convert to lowercase
    string fn( fname );
    string format( fmt );
    transform( format.begin(), format.end(), format.begin(),( int ( * )( int ) ) tolower );

    // write with OpenBabel
    OBConversion obConversion;
    obConversion.SetOutFormat( format.c_str() );
    if( !obConversion.WriteFile( obMol_, fname ) )
    {
         throw MolekelException( "Cannot write file " + string( fname ) + " with OpenBabel" );
    }
}


//------------------------------------------------------------------------------
MolekelMolecule::VtkObserverIdType
MolekelMolecule::AddVtkObserver( VtkEventIdType e,
                                 MoleculeVtkCommand* c )
{

    c->SetActor( actor_ );
    VtkObserverIdType oid = c->GetActor()->AddObserver( e, c );
    vtkObserverEventMap_[ e ] = oid;
    return oid;
}

//------------------------------------------------------------------------------
void MolekelMolecule::SetPickMode( PickMode p )
{
    VtkObserverEventMap::iterator i =
                        vtkObserverEventMap_.find( vtkCommand::PickEvent );
    assert( i != vtkObserverEventMap_.end() && "PickEvent observer not registered" );
    MoleculeActorPickCommand* pc = dynamic_cast< MoleculeActorPickCommand* >( actor_->GetCommand( i->second ) );
    assert( pc && "Wrong vtkCommand type" );
    if( p == PICK_ATOM_BOND_RESIDUE ) pc->SetPickAtoms( true );
    else if( p == PICK_MOLECULE )
    {
        pc->SetPickAtoms( false );
        pc->SetAllowActorOnlyPick( true );
    }
}

//-------------------------------------------------------------------------------
void MolekelMolecule::SetVisible( bool visible )
{
    assembly_->SetVisibility( visible );
}

//-------------------------------------------------------------------------------
void MolekelMolecule::SetBBoxVisible( bool visible )
{
    if( visible )
    {
        bbox_->SetVisibility( true );
        assembly_->AddPart( bbox_ );
    }
    else
    {
        bbox_->SetVisibility( false );
        assembly_->RemovePart( bbox_ );
    }
}

//-------------------------------------------------------------------------------
void MolekelMolecule::SetIsoBBoxVisible( bool visible )
{
    if( visible )
    {
        isoBBox_->SetVisibility( true );
        isoBBox_->GetProperty()->SetColor( bbox_->GetProperty()->GetColor() );
        assembly_->AddPart( isoBBox_ );
    }
    else
    {
        isoBBox_->SetVisibility( false );
        assembly_->RemovePart( isoBBox_ );
    }
}

//-------------------------------------------------------------------------------
bool MolekelMolecule::GetBBoxVisible() const
{
    return bbox_->GetVisibility() != 0;
}

//-------------------------------------------------------------------------------
bool MolekelMolecule::GetVisible() const
{
    return assembly_->GetVisibility() != 0;
}

//-------------------------------------------------------------------------------
void MolekelMolecule::RecomputeBBox()
{
    SaveTransform();
    ResetTransform();
    vtkSoMapper* som = dynamic_cast< vtkSoMapper* >( actor_->GetMapper() );
    assert( som && "Wrong mapper" );
    som->ComputeBBox();
    som->Update();
    vtkProp3D* m = assembly_;

    // bounding box is inside same assembly as other
    // molecule actors: set to zero before computing bounding box! keep
    // bounding box sparate from other actors.
    boundingBox_->SetBounds( 0, 0, 0, 0, 0, 0 );
    boundingBox_->Update();
    boundingBox_->SetCenter( .5 * ( m->GetBounds()[ 0 ] +  m->GetBounds()[ 1 ] ),
                   .5 * ( m->GetBounds()[ 2 ] +  m->GetBounds()[ 3 ] ),
                   .5 * ( m->GetBounds()[ 4 ] +  m->GetBounds()[ 5 ] ) );
    boundingBox_->SetBounds( m->GetBounds() );
    boundingBox_->Update();
    RestoreTransform();
    //if( isoBoundingBox_->GetXLength() == 0. && isoBoundingBox_->GetYLength() == 0. )
    //{
    //    isoBoundingBox_->SetXLength( boundingBox_->GetXLength() );
    //    isoBoundingBox_->SetYLength( boundingBox_->GetYLength() );
    //    isoBoundingBox_->SetZLength( boundingBox_->GetZLength() );
    //    isoBoundingBox_->SetCenter( boundingBox_->GetCenter() );
    //}
}

//--------------------------------------------------------------------------------
MolekelMolecule::~MolekelMolecule()
{
    delete molekelMol_;
    delete updater_;
    // will be removed after adding smart pointers
    for( Frames::iterator i = frames_.begin(); i != frames_.end(); ++i ) delete *i;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetBoundingBoxSize( double& dx, double& dy, double& dz ) const
{
    const double* bounds = assembly_->GetBounds();
    assert( bounds && "NULL BBox" );
    dx = bounds[ 1 ] - bounds[ 0 ];
    dy = bounds[ 3 ] - bounds[ 2 ];
    dz = bounds[ 5 ] - bounds[ 4 ];
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetBoundingBoxCenter( double& x, double& y, double& z ) const
{
    const double* bounds = assembly_->GetBounds();
    assert( bounds && "NULL BBox" );
    x = .5 * ( bounds[ 1 ] + bounds[ 0 ] );
    y = .5 * ( bounds[ 3 ] + bounds[ 2 ] );
    z = .5 * ( bounds[ 5 ] + bounds[ 4 ] );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetIsoBoundingBoxSize( double& dx, double& dy, double& dz ) const
{
    dx = isoBoundingBox_->GetXLength();
    dy = isoBoundingBox_->GetYLength();
    dz = isoBoundingBox_->GetZLength();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetIsoBoundingBoxCenter( double& x, double& y, double& z ) const
{
    x = isoBoundingBox_->GetCenter()[ 0 ];
    y = isoBoundingBox_->GetCenter()[ 1 ];
    z = isoBoundingBox_->GetCenter()[ 2 ];
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetIsoBoundingBoxSize( double dx, double dy, double dz )
{
    isoBoundingBox_->SetXLength( dx );
    isoBoundingBox_->SetYLength( dy );
    isoBoundingBox_->SetZLength( dz );
    isoBoundingBox_->Update();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetIsoBoundingBoxCenter( double x, double y, double z )
{
    isoBoundingBox_->SetCenter( x, y, z );
    isoBoundingBox_->Update();
}


//---------------------------------------------------------------------------------
namespace
{
    //--------------------------------------------------------------------------------
    /// Free function: checks validity of Molekel molecule, this is required since
    /// the Molecule class is actually a struct + constructor/destructor without any
    /// getters/setters doing checks on the correctness of stored data. This is currently
    /// a simple consistency check on orbitals.
    void CheckMolecule( const Molecule* mol )
    {
        if( mol->nMolecularOrbitals != 0 )
        {
            if( mol->alphaBeta && ( !mol->alphaOrbital || !mol->betaOrbital ) )
            {
                throw MolekelException( "alphaBeta == true & NULL orbitals" );
            }
            if( !mol->alphaBeta && !mol->alphaOrbital )
            {
                throw MolekelException( "alphaBeta == false & NULL alpha orbitals" );
            }
        }
    }
}

//--------------------------------------------------------------------------------
int MolekelMolecule::GetNumberOfOrbitals() const
{
    if( molekelMol_ == 0 ) return 0;
    return molekelMol_->nMolecularOrbitals;
}

namespace
{
    //----------------------------------------------------------------------------
    /// Free function returning a reference to orbital.
    MolecularOrbital& GetOrbital( int index, const Molecule* mol )
    {
        if( !mol ) throw MolekelException( "NULL molecule" );
        CheckMolecule( mol );
        if( index < 0 || index >= mol->nMolecularOrbitals )
        {
            throw MolekelException( "Invalid orbital index" );
        }

        if( mol->alphaBeta )
        {
            // beta
            if( index % 2 )
            {
                return 	mol->betaOrbital[ index / 2 ];
            }
            else // alpha
            {
                return 	mol->alphaOrbital[ index / 2 ];
            }
        }
        else return mol->alphaOrbital[ index ];
    }
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasBetaOrbitals() const
{
    CheckMolecule( molekelMol_ );
    return molekelMol_->alphaBeta != 0;
}


//--------------------------------------------------------------------------------
double MolekelMolecule::GetOrbitalEigenValue( int orbitalIndex ) const
{
    if( molekelMol_ == 0 ) throw MolekelException( "Null Molekel molecule" );
    return GetOrbital( orbitalIndex, molekelMol_ ).eigenvalue;
}

//--------------------------------------------------------------------------------
double MolekelMolecule::GetOrbitalOccupation( int orbitalIndex ) const
{
    if( molekelMol_ == 0 ) throw MolekelException( "Null Molekel molecule" );
    return GetOrbital( orbitalIndex, molekelMol_ ).occ;
}

//--------------------------------------------------------------------------------
const char* MolekelMolecule::GetOrbitalType( int orbitalIndex ) const
{
    if( molekelMol_ == 0 ) throw MolekelException( "Null Molekel molecule" );
    return GetOrbital( orbitalIndex, molekelMol_ ).type;
}


namespace
{
    //--------------------------------------------------------------------------------
    /// Free function making a shiny material.
    void MakeShinyMaterialType( vtkProperty* p )
    {
        assert( p && "NULL property" );
        p->SetInterpolationToPhong();
        p->SetSpecularColor( .5f, .5f, .5f );
        p->SetSpecular( 1.0f );
        p->SetSpecularPower( 100 ); // VTK 5.0.2 clamps to 100 (?)
    }

    //--------------------------------------------------------------------------------
    /// Free function used to make a given material opaque.
    void MakeOpaqueMaterialType( vtkProperty* p )
    {
        assert( p && "NULL property" );
        p->SetSpecularColor( .0f, .0f, .0f );
        p->SetSpecular( .0f );
        p->SetSpecularPower( 0 ); // VTK 5.0.2 clamps to 100 (?)
    }

    //--------------------------------------------------------------------------------
    /// Free function returning a vtkActor containing an iso-surface generated
    /// from grid data and value.
    vtkActor* GenerateIsoSurfaceActor( vtkImageData* data, double value )
    {
        assert( data );
        vtkSmartPointer< vtkMarchingCubes > mc( vtkMarchingCubes::New() );
        mc->SetInput( data );
        mc->ComputeNormalsOn();
        mc->GenerateValues( 1, value, value );
        mc->Update();
        // Copy poly data and use copy as mapper input:
        // this is required for further modification of
        // polydata through vtkActor::GetMapper::GetInput().
        vtkSmartPointer< vtkPolyData > pd( vtkPolyData::New() );
        pd->DeepCopy( mc->GetOutput() );
        if( !pd || pd->GetNumberOfCells() == 0 ) return 0;
        vtkSmartPointer< vtkPolyDataMapper > mapper( vtkPolyDataMapper::New() );
        mapper->ScalarVisibilityOff();

        // rverse normals if value < 0
        if( value < 0. )
        {
            vtkSmartPointer< vtkReverseSense > reverse( vtkReverseSense::New() );
            reverse->SetInput( pd );
            reverse->ReverseNormalsOn();
            mapper->SetInputConnection( reverse->GetOutputPort() );
        }
        else mapper->SetInput( pd );
        vtkActor* actor = GLSLShadersSupported() ? vtkGLSLShaderActor::New() :  vtkActor::New();
        actor->SetMapper( mapper );
        return actor;
    }


    //--------------------------------------------------------------------------------
    /// Free function returning an actor containing a molecular orbital surface
    /// generated form grid data and value.
    vtkActor* GenerateMOActor( vtkImageData* data, double value )
    {
        vtkActor* actor = GenerateIsoSurfaceActor( data, value );
        if( !actor ) return 0;
        vtkProperty* p = actor->GetProperty();
        if( value < 0 )	p->SetColor( 1, 0.2, 0.2 ); // red
        else if( value > 0 ) p->SetColor( 0.2, 0.2, 1 ); // blue
        else p->SetColor( 0.8, 0.8, 0.8 ); // light grey
        MakeShinyMaterialType( p );
        return actor;
    }
}

//------------------------------------------------------------------------------
/// Molecular orbital to compute.
extern  MolecularOrbital* molOrb;

vtkImageData* MolekelMolecule::GenerateMOGridData( int orbitalIndex,
                                                   double bboxSize[ 3 ],
                                                   int steps[ 3 ],
                                                   ProgressCallback cb,
                                                   void* cbData ) const
{
    int ftype = CALC_ORB; // use EL_DENS for density matrix
    float dim[6];
//    int   ncub[3];
    molOrb = &GetOrbital( orbitalIndex, molekelMol_ );
    double x, y, z;
    GetIsoBoundingBoxCenter( x, y, z );
    dim[ 0 ] =  float( x - bboxSize[ 0 ] * .5 );
    dim[ 1 ] =  float( x + bboxSize[ 0 ] * .5 );
    dim[ 2 ] =  float( y - bboxSize[ 1 ] * .5 );
    dim[ 3 ] =  float( y + bboxSize[ 1 ] * .5 );
    dim[ 4 ] =  float( z - bboxSize[ 2 ] * .5 );
    dim[ 5 ] =  float( z + bboxSize[ 2 ] * .5 );

    vtkImageData* data =
        vtk_process_calc( molekelMol_, dim, steps, ftype, cb, cbData );
    if( data == 0 ) throw MolekelException( "Error computing Molecular Orbital" );

    return data;
}

//------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GenerateMOGridData( int orbitalIndex,
                                                   double step,
                                                   ProgressCallback cb,
                                                   void* cbData ) const
{
    assert( step > 0. && "Invalid step" );
    double dx, dy, dz;
    GetBoundingBoxSize( dx, dy, dz );
    double bboxSize[ 3 ];
    bboxSize[ 0 ] = dx;
    bboxSize[ 1 ] = dy;
    bboxSize[ 2 ] = dz;
    int steps[ 3 ];
    steps[ 0 ] = int( dx / step + .5 );
    steps[ 1 ] = int( dy / step + .5 );
    steps[ 2 ] = int( dz / step + .5 );
    molOrb = &GetOrbital( orbitalIndex, molekelMol_ );
    return GenerateMOGridData( orbitalIndex, bboxSize, steps, cb, cbData );
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::AddOrbitalSurface( int orbitalIndex,
                                         double bboxSize[ 3 ],
                                         int steps[ 3 ],
                                         double value,
                                         bool bothSigns,
                                         bool nodalSurface,
                                         ProgressCallback cb,
                                         void* cbData )
{
    // check if index is valid
    GetOrbital( orbitalIndex, molekelMol_ );
    // check if orbital already in map
    if( orbitalActorMap_.find( orbitalIndex ) != orbitalActorMap_.end() ) return false;

    SaveTransform(); // push current transform
    ResetTransform(); // set to default (identity)

    // create surface from data
    vtkSmartPointer< vtkActor > minusActor( 0 );
    vtkSmartPointer< vtkActor > zeroActor( 0 );
    vtkSmartPointer< vtkActor > plusActor( 0 );

    vtkSmartPointer< vtkImageData > data(
                        GenerateMOGridData( orbitalIndex, bboxSize, steps, cb, cbData ) );


    if( !bothSigns )
    {
        if( value < 0 )
        {
             minusActor = GenerateMOActor( data, value );
        }
        else plusActor = GenerateMOActor( data, value );

    }
    else
    {
        minusActor = GenerateMOActor( data, -abs( value ) );
        // how do we clone a vtkActor ?
        plusActor = GenerateMOActor( data, abs( value ) );
    }
    if( nodalSurface )
    {
         zeroActor = GenerateMOActor( data, 0 );
         // zeroActor = GenerateMOActor( data, value );
         // how do we clone a vtkActor ?
    }

    vtkSmartPointer< vtkAssembly > assembly( vtkAssembly::New() );

    int typeMask = 0;
    if( minusActor != 0 )
    {
    	assembly->AddPart( minusActor );
    	typeMask |= ORBITAL_MINUS;
    }
    if( zeroActor  != 0 )
    {
    	assembly->AddPart( zeroActor );
    	typeMask |= ORBITAL_NODAL;
    }
    if( plusActor  != 0 )
    {
    	typeMask |= ORBITAL_PLUS;
    	assembly->AddPart( plusActor );
    }

    if( GLSLShadersSupported() ) // in this case the actors are vtkGLSLShaderActors
    {
        vtkGLSLShaderActor* ma = dynamic_cast< vtkGLSLShaderActor* >( minusActor.GetPointer() );
        vtkGLSLShaderActor* za = dynamic_cast< vtkGLSLShaderActor* >( zeroActor.GetPointer() );
        vtkGLSLShaderActor* pa = dynamic_cast< vtkGLSLShaderActor* >( plusActor.GetPointer() );
        if( ma )
        {
            shaderSurfaceMap_[ ORBITAL_NEGATIVE_SURFACE ].actors.push_back( ma );
            ma->SetShaderProgramId( shaderSurfaceMap_[ ORBITAL_NEGATIVE_SURFACE ].program );
        }
        if( za )
        {
            shaderSurfaceMap_[ ORBITAL_NODAL_SURFACE ].actors.push_back( za );
            za->SetShaderProgramId( shaderSurfaceMap_[ ORBITAL_NODAL_SURFACE ].program );
        }
        if( pa )
        {
            shaderSurfaceMap_[ ORBITAL_POSITIVE_SURFACE ].actors.push_back( pa );
            pa->SetShaderProgramId( shaderSurfaceMap_[ ORBITAL_POSITIVE_SURFACE ].program );
        }
    }


    if( minusActor != 0 ||
        plusActor  != 0 ||
        zeroActor  != 0 )
    {
        // add actor to map for future reference
        orbitalActorMap_[ OrbitalIndex( orbitalIndex, typeMask ) ] = assembly;
        // add assembly to molecule assembly_
        assembly_->AddPart( assembly );
        RecomputeBBox();
        RestoreTransform();
        return true;
    }
    else
    {
        return false;
    }
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveOrbitalSurface( int orbitalIndex )
{
    if( orbitalActorMap_.find( orbitalIndex ) == orbitalActorMap_.end() )
    {
        throw MolekelException( "Invalid orbital index" );
    }

    if( GLSLShadersSupported() )
    {
        vtkSmartPointer< vtkPropCollection > pc( vtkPropCollection::New() );
        orbitalActorMap_[ orbitalIndex ]->GetActors( pc );
        pc->InitTraversal();
        vtkGLSLShaderActor* a = 0;
        typedef FindVtkPointer< vtkGLSLShaderActor > Finder;
        while( a = dynamic_cast< vtkGLSLShaderActor* >( pc->GetNextProp() ) )
        {
            typedef ShaderProgram::Actors SA;
            SA::iterator b = shaderSurfaceMap_[ ORBITAL_POSITIVE_SURFACE ].actors.begin();
            SA::iterator e = shaderSurfaceMap_[ ORBITAL_POSITIVE_SURFACE ].actors.end();
            SA::iterator i = find_if( b, e, Finder( a ) );
            if( i != e )
            {
                shaderSurfaceMap_[ ORBITAL_POSITIVE_SURFACE ].actors.erase( i );
                continue;
            }
            b = shaderSurfaceMap_[ ORBITAL_NODAL_SURFACE ].actors.begin();
            e = shaderSurfaceMap_[ ORBITAL_NODAL_SURFACE ].actors.end();
            i = find_if( b, e, Finder( a ) );
            if( i != e )
            {
                shaderSurfaceMap_[ ORBITAL_NODAL_SURFACE ].actors.erase( i );
                continue;
            }
            b = shaderSurfaceMap_[ ORBITAL_NEGATIVE_SURFACE ].actors.begin();
            e = shaderSurfaceMap_[ ORBITAL_NEGATIVE_SURFACE ].actors.end();
            i = find_if( b, e, Finder( a ) );
            if( i != e )
            {
                shaderSurfaceMap_[ ORBITAL_NEGATIVE_SURFACE ].actors.erase( i );
                continue;
            }
        }
    }


    assembly_->RemovePart( orbitalActorMap_[ orbitalIndex ] );
    orbitalActorMap_.erase( orbitalIndex );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasOrbitalSurface( int orbitalIndex ) const
{
    if( orbitalActorMap_.find( orbitalIndex ) == orbitalActorMap_.end() )
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::IsAlphaOrbital( int index ) const
{
    if( !molekelMol_ ) return false;
    if( index < 0 || index >= molekelMol_->nMolecularOrbitals ) return false;
    if( !molekelMol_->alphaBeta ) return true;
    return ( index %2 ) == 0;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::IsBetaOrbital( int index ) const
{
    if( !molekelMol_ ) return false;
    if( !molekelMol_->alphaBeta ) return false;
    if( index < 0 || index >= molekelMol_->nMolecularOrbitals ) return false;
    return ( index %2 ) != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetOrbitalRenderingStyle( int orbitalIndex, RenderingStyle rs )
{
    if( orbitalActorMap_.find( orbitalIndex ) == orbitalActorMap_.end() )
    {
        return;
    }

    vtkSmartPointer< vtkPropCollection > pc( vtkPropCollection::New() );
    orbitalActorMap_[ orbitalIndex ]->GetActors( pc );
    pc->InitTraversal();
    vtkActor* a = 0;
    while( a = dynamic_cast< vtkActor* >( pc->GetNextProp() ) )
    {
        vtkProperty* p = a->GetProperty();
        switch( rs )
        {
            case POINTS: p->SetRepresentationToPoints();
                         p->SetOpacity( 1.0f );
                         break;
            case WIREFRAME: p->SetRepresentationToWireframe();
                            p->SetOpacity( 1.0f );
                            break;
            case SOLID: p->SetRepresentationToSurface();
                        p->SetOpacity( 1.0f );
                        break;
            case TRANSPARENT_SOLID: p->SetRepresentationToSurface();
                                    p->SetOpacity( 0.7f );
                                    break;
            default: break;
        }

    }
}

//--------------------------------------------------------------------------------
void MolekelMolecule::ResetTransform()
{
    assembly_->SetPosition( 0, 0, 0 );
    assembly_->SetScale( 1, 1, 1 );
    assembly_->SetOrientation( 0, 0, 0 );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::StopMOGridDataGeneration()
{
extern void StopProcessCalc();
    StopProcessCalc();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::MOGridDataGenerationStopped()
{
extern bool ProcessCalcStopped();
extern int 	GetProcessCalcDataType();
    return ProcessCalcStopped() && GetProcessCalcDataType() == CALC_ORB;
}


//--------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GridDataToVtkImageData( const std::string& label,
                                                       int stepMultiplier,
                                                       ProgressCallback cb,
                                                       void* cbData ) const
{
    if( !HasGridData() ) return 0;


    if( format_ != "t41" )
    {
        const OBGridData* gd = dynamic_cast< const OBGridData* >( obMol_->GetData( "GridData" ) );
        assert( gd && "Invalid OBGridData" );

        if( stepMultiplier <= 0 ) throw MolekelException( "Invalid step" );

        // 1) retrieve nx, ny, nz, steps and values
        double xAxis[ 3 ];
        double yAxis[ 3 ];
        double zAxis[ 3 ];
        gd->GetAxes( xAxis, yAxis, zAxis );
        const double xStep = xAxis[ 0 ] * stepMultiplier;
        const double yStep = yAxis[ 1 ] * stepMultiplier;
        const double zStep = zAxis[ 2 ] * stepMultiplier;
        if( xStep < 0. || yStep < 0. || zStep < 0. ) return 0;
        double origin[ 3 ];
        gd->GetOrigin( origin );
        int npx, npy, npz;
        gd->GetNumberOfPoints( npx, npy, npz );
        // 2) create vtkImageData
        vtkImageData* grid = vtkImageData::New();
        grid->SetDimensions( npx / stepMultiplier, npy / stepMultiplier , npz / stepMultiplier );
        grid->SetOrigin( origin );
        grid->SetSpacing( xStep, yStep, zStep );
        const int totalSteps = npx * npy * npz;
        //initialize callback
        if( cb ) cb( 0, totalSteps, cbData );
        int i, j, k;
        for( i = 0; i < npx; i += stepMultiplier )
        {
            for( j = 0; j < npy; j += stepMultiplier )
            {
                for( k = 0; k < npz; k += stepMultiplier )
                {
                    const double s = gd->GetValue( i, j, k );
                    grid->SetScalarComponentFromDouble( i / stepMultiplier,
                                                        j / stepMultiplier,
                                                        k / stepMultiplier, 0, s );
                }
            }
            const int idx = ( i + 1 ) * ( npz * npy );
            if( cb ) cb( idx, totalSteps, cbData );
        }

        return grid;
    }
    else
    {
        const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
        assert( gd && "Invalid OBT41Data" );

        if( stepMultiplier <= 0 ) throw MolekelException( "Invalid step" );


        // 1) retrieve nx, ny, nz, steps and values
        double xAxis[ 3 ];
        double yAxis[ 3 ];
        double zAxis[ 3 ];
        gd->GetAxes( xAxis, yAxis, zAxis );
        const double xStep = xAxis[ 0 ] * stepMultiplier;
        const double yStep = yAxis[ 1 ] * stepMultiplier;
        const double zStep = zAxis[ 2 ] * stepMultiplier;
        if( xStep < 0. || yStep < 0. || zStep < 0. ) return 0;
        double origin[ 3 ];
        gd->GetStartPoint( origin );
        int npx, npy, npz;
        gd->GetNumberOfPoints( npx, npy, npz );
        // 2) create vtkImageData
        vtkImageData* grid = vtkImageData::New();
        grid->SetDimensions( npx / stepMultiplier, npy / stepMultiplier , npz / stepMultiplier );
        grid->SetOrigin( origin );
        grid->SetSpacing( xStep, yStep, zStep );
        const int totalSteps = npx * npy * npz;
        //initialize callback
        if( cb ) cb( 0, totalSteps, cbData );
        int i, j, k;
        for( i = 0; i < npx; i += stepMultiplier )
        {
            for( j = 0; j < npy; j += stepMultiplier )
            {
                for( k = 0; k < npz; k += stepMultiplier )
                {
                    const double s = gd->GetValue( label, i, j, k );
                    grid->SetScalarComponentFromDouble( i / stepMultiplier,
                                                        j / stepMultiplier,
                                                        k / stepMultiplier, 0, s );
                }
            }
            const int idx = ( i + 1 ) * ( npz * npy );
            if( cb ) cb( idx, totalSteps, cbData );
        }

        return grid;
    }
    return 0;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GenerateGridDataSurface( const std::string& label,
                                               double value, int stepMultiplier,
                                               int iterations, double relaxationFactor,
                                               ProgressCallback cb, void* cbData )
{

    RemoveGridDataSurface( label );

    SaveTransform(); // push current transform
    ResetTransform(); // set to default (identity)

    vtkSmartPointer< vtkImageData > grid =
                                GridDataToVtkImageData( label, stepMultiplier, cb, cbData );

    if( grid == 0 ) return false;

    if( cb ) cb( 0, 2, cbData );

    // 3 )use vtkMarchingCubes to generate the iso-surface
    vtkSmartPointer< vtkMarchingCubes > mc( vtkMarchingCubes::New() );
    mc->SetInput( grid );
    mc->GenerateValues( 1, value, value );
    mc->ComputeNormalsOn();
    mc->Update();
    if( cb ) cb( 1, 2, cbData );

    // 4 )create mapper and actor and add to molecule scenegraph
    if( mc->GetOutput()->GetNumberOfCells() == 0 ) return false;
    vtkSmartPointer< vtkPolyDataMapper > mapper( vtkPolyDataMapper::New() );
    mapper->ScalarVisibilityOff();
    // reverse normals if value < 0
    if( value < 0. )
    {
        vtkSmartPointer< vtkReverseSense > reverse( vtkReverseSense::New() );
        reverse->SetInputConnection( mc->GetOutputPort() );
        reverse->ReverseNormalsOn();
        // default VTK relaxation factor is 0.01
        if( iterations > 0 )
        {
             vtkSmartPointer< vtkSmoothPolyDataFilter > pf( vtkSmoothPolyDataFilter::New() );
             pf->SetInputConnection( reverse->GetOutputPort() );
             pf->SetNumberOfIterations( iterations );
             pf->SetRelaxationFactor( relaxationFactor );
             vtkSmartPointer< vtkPolyDataNormals > pn( vtkPolyDataNormals::New() );
             pn->SetInputConnection( pf->GetOutputPort() );
             pn->FlipNormalsOn();
             mapper->SetInputConnection( pn->GetOutputPort() );
        }
        else mapper->SetInputConnection( reverse->GetOutputPort() );
    }
    else
    {
        if( iterations > 0 )
        {
            vtkSmartPointer< vtkSmoothPolyDataFilter > pf( vtkSmoothPolyDataFilter::New() );
            pf->SetInputConnection( mc->GetOutputPort() );
            pf->SetNumberOfIterations( iterations );
            pf->SetRelaxationFactor( relaxationFactor );
            vtkSmartPointer< vtkPolyDataNormals > pn( vtkPolyDataNormals::New() );
            pn->SetInputConnection( pf->GetOutputPort() );
            pn->FlipNormalsOn();
            mapper->SetInputConnection( pn->GetOutputPort() );
        }
        else mapper->SetInputConnection( mc->GetOutputPort() );
    }
    if( cb ) cb( 2, 2, cbData );

    // Actor
    vtkSmartPointer< vtkActor > actor;
    if( !GLSLShadersSupported() )
    {
		gridDataActor_ = vtkActor::New();
		actor = gridDataActor_;
	}
    else
    {
       shaderSurfaceMap_[ GRID_DATA_SURFACE ].actors.push_back( vtkGLSLShaderActor::New() );
       actor =  shaderSurfaceMap_[ GRID_DATA_SURFACE ].actors.back();
       shaderSurfaceMap_[ GRID_DATA_SURFACE ].actors.back()
        ->SetShaderProgramId( shaderSurfaceMap_[ GRID_DATA_SURFACE ].program );
    }

    vtkProperty* p = actor->GetProperty();
    p->SetColor( 0.9, 0.9, 0.4 ); // yellowish
    actor->SetMapper( mapper );
    MakeShinyMaterialType( p );

    //Subdivision TBD
//    const int subdiv = 2;
//    if( subdiv > 0 )
//    {
//        vtkSmartPointer< vtkLoopSubdivisionFilter > l( vtkLoopSubdivisionFilter::New() );
//        l->SetInputConnection( mapper->GetInputConnection( 0, 0 ) );
//        l->SetNumberOfSubdivisions( subdiv );
//        mapper->SetInputConnection( l->GetOutputPort() );
//    }
    if( format_ != "t41" ) gridDataActor_ = actor;
    else
    {

        gridActorMap_[ label ] = actor;
        //set transform
//        double x[ 3 ];
//        double y[ 3 ];
//        double z[ 3 ];
//        double o[ 3 ];
//        const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
//        assert( gd && "Invalid OBT41Data" );
//      Setup transform according to information in T41 file
//        gd->GetAxes( x, y, z );
//        gd->GetStartPoint( o );
//        vtkSmartPointer< vtkMatrix4x4 > m( vtkMatrix4x4::New() );
//
//        vtkMatrix4x4& mat = *( m.GetPointer() );
//        mat[ 0 ][ 0 ] = x[ 0 ];  mat[ 0 ][ 1 ] = x[ 1 ]; mat[ 0 ][ 2 ] = x[ 2 ]; mat[ 0 ][ 3 ] = 0.0;
//        mat[ 1 ][ 0 ] = y[ 0 ];  mat[ 1 ][ 1 ] = y[ 1 ]; mat[ 1 ][ 2 ] = y[ 2 ]; mat[ 1 ][ 3 ] = 0.0;
//        mat[ 2 ][ 0 ] = z[ 0 ];  mat[ 2 ][ 1 ] = z[ 1 ]; mat[ 2 ][ 2 ] = z[ 2 ]; mat[ 2 ][ 3 ] = 0.0;
//        mat[ 3 ][ 0 ] = o[ 0 ];  mat[ 3 ][ 1 ] = o[ 1 ]; mat[ 3 ][ 2 ] = o[ 2 ]; mat[ 3 ][ 3 ] = 1.0;
//        mat.Transpose();
//      actor->SetUserMatrix( m );
    }
    assembly_->AddPart( actor );
    RecomputeBBox();
    RestoreTransform();
    return true;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveGridDataSurface( const std::string& label )
{
    if( format_ != "t41" )
    {
        if( gridDataActor_ == 0 ) return;
        assembly_->RemovePart( gridDataActor_ );
        gridDataActor_ = 0;
    }
    else
    {
        if( gridActorMap_.find( label ) == gridActorMap_.end() ) return;
        assembly_->RemovePart( gridActorMap_[ label ] );
        gridActorMap_.erase( label );
    }

    if( GLSLShadersSupported() )
    {
        vtkGLSLShaderActor* a = format_ != "t41" ?
                                dynamic_cast< vtkGLSLShaderActor* >( gridDataActor_.GetPointer() ) :
                                dynamic_cast< vtkGLSLShaderActor* >( gridActorMap_[ label ].GetPointer() );

        typedef FindVtkPointer< vtkGLSLShaderActor > Finder;
        typedef ShaderProgram::Actors SA;
        SA::iterator b = shaderSurfaceMap_[ GRID_DATA_SURFACE ].actors.begin();
        SA::iterator e = shaderSurfaceMap_[ GRID_DATA_SURFACE ].actors.end();
        SA::iterator i = find_if( b, e, Finder( a ) );
        if( i != e )
        {
            shaderSurfaceMap_[ GRID_DATA_SURFACE ].actors.erase( i );
        }
    }

    RecomputeBBox();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetGridDataSurfaceRenderingStyle( RenderingStyle rs,
                                                        const std::string& label )
{

    vtkProperty* p = 0;
    if( format_ != "t41" )
    {
        if( gridDataActor_ == 0 ) return;
        p = gridDataActor_->GetProperty();
    }
    else
    {
        if( gridActorMap_.find( label ) == gridActorMap_.end() ) return;
        p = gridActorMap_[ label ]->GetProperty();
    }
    switch( rs )
    {
        case POINTS: p->SetRepresentationToPoints();
                     p->SetOpacity( 1.0f );
                     break;
        case WIREFRAME: p->SetRepresentationToWireframe();
                        p->SetOpacity( 1.0f );
                        break;
        case SOLID: p->SetRepresentationToSurface();
                    p->SetOpacity( 1.0f );
                    break;
        case TRANSPARENT_SOLID: p->SetRepresentationToSurface();
                                p->SetOpacity( 0.7f );
                                break;
        default: break;
    }


}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasGridData() const
{
    if( !obMol_ ) return false;
    return obMol_->HasData( "GridData" ) || obMol_->HasData( "T41Data" );
}

//--------------------------------------------------------------------------------
double MolekelMolecule::GetGridDataMax( const std::string& label ) const
{
    if( !HasGridData() ) return numeric_limits< double >::min();
    double m = 0;
    if( format_ != "t41" )
    {
        const OBGridData* gd = dynamic_cast< const OBGridData* >( obMol_->GetData( "GridData" ) );
        m = gd->GetMaxValue();
    }
    else
    {
        const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
        m = gd->GetMaxValue( label );
    }
    return m;
}

//--------------------------------------------------------------------------------
double MolekelMolecule::GetGridDataMin( const std::string& label ) const
{
    if( !HasGridData() ) return numeric_limits< double >::max();
    double m = 0;
    if( format_ != "t41" )
    {
        const OBGridData* gd = dynamic_cast< const OBGridData* >( obMol_->GetData( "GridData" ) );
        m = gd->GetMinValue();
    }
    else
    {
        const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
        m = gd->GetMinValue( label );
    }
    return m;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetGridDataNumSteps( int steps[ 3 ] ) const
{
    steps[ 0 ] = -1;
    steps[ 1 ] = -1;
    steps[ 2 ] = -1;
    if( !HasGridData() ) return;
    if( format_ != "t41" )
    {
        const OBGridData* gd = dynamic_cast< const OBGridData* >( obMol_->GetData( "GridData" ) );
        gd->GetNumberOfSteps( steps );
    }
    else
    {
        const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
        gd->GetNumberOfSteps( steps );
    }
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetGridDataStepSize( double ssize[ 3 ] ) const
{
    ssize[ 0 ] = -1.;
    ssize[ 1 ] = -1.;
    ssize[ 2 ] = -1.;
    if( !HasGridData() ) return;
    double xa[ 3 ];
    double ya[ 3 ];
    double za[ 3 ];

    if( format_ != "t41" )
    {
        const OBGridData* gd = dynamic_cast< const OBGridData* >( obMol_->GetData( "GridData" ) );
        gd->GetAxes( xa, ya, za );
        ssize[ 0 ] = xa[ 0 ];
        ssize[ 1 ] = ya[ 1 ];
        ssize[ 2 ] = za[ 2 ];
    }
    else
    {
        const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
        gd->GetAxes( xa, ya, za );
        ssize[ 0 ] = xa[ 0 ];
        ssize[ 1 ] = ya[ 1 ];
        ssize[ 2 ] = za[ 2 ];
    }
}

//------------------------------------------------------------------------------
bool MolekelMolecule::HasAlphaDensity() const
{
    return(  molekelMol_ && molekelMol_->alphaDensity );
}

//------------------------------------------------------------------------------
bool MolekelMolecule::HasBetaDensity() const
{
    return(  molekelMol_ && molekelMol_->betaDensity );
}


//------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GenerateDensityData( int ftype,
                                                    double bboxSize[ 3 ],
                                                    int steps[ 3 ],
                                                    ProgressCallback cb,
                                                    void* cbData ) const
{
    float dim[6];
//    int   ncub[3];
    double x, y, z;
    GetIsoBoundingBoxCenter( x, y, z );
    dim[ 0 ] =  float( x - bboxSize[ 0 ] * .5 );
    dim[ 1 ] =  float( x + bboxSize[ 0 ] * .5 );
    dim[ 2 ] =  float( y - bboxSize[ 1 ] * .5 );
    dim[ 3 ] =  float( y + bboxSize[ 1 ] * .5 );
    dim[ 4 ] =  float( z - bboxSize[ 2 ] * .5 );
    dim[ 5 ] =  float( z + bboxSize[ 2 ] * .5 );

    vtkImageData* data =
        vtk_process_calc( molekelMol_, dim, steps, ftype, cb, cbData );
    if( data == 0 ) throw MolekelException( "Error computing density data" );
    return data;
}

//------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GenerateDensityData( int ftype,
                                                    const double& step,
                                                    double& minValue,
                                                    double& maxValue,
                                                    ProgressCallback cb,
                                                    void* cbData ) const
{
    assert( step > 0. && "Invalid step" );
    double dx, dy, dz;
    GetBoundingBoxSize( dx, dy, dz );
    double bboxSize[ 3 ];
    bboxSize[ 0 ] = dx;
    bboxSize[ 1 ] = dy;
    bboxSize[ 2 ] = dz;
    int steps[ 3 ];
    steps[ 0 ] = int( dx / step + .5 );
    steps[ 1 ] = int( dy / step + .5 );
    steps[ 2 ] = int( dz / step + .5 );
    vtkImageData* data = GenerateDensityData( ftype, bboxSize, steps, cb, cbData );
extern void GetProcessCalcMinMax( double&, double& );
    GetProcessCalcMinMax( minValue, maxValue );
    return data;
}


//------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GenerateSpinDensityData( const double& step,
                                                        double& minValue,
                                                        double& maxValue,
                                                        ProgressCallback cb,
                                                        void* cbData ) const
{
    return GenerateDensityData( SPIN_DENS, step, minValue, maxValue, cb, cbData );
}

//------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GenerateElectronDensityData( const double& step,
                                                            double& minValue,
                                                            double& maxValue,
                                                            ProgressCallback cb,
                                                            void* cbData ) const
{
    return GenerateDensityData( EL_DENS, step, minValue, maxValue, cb, cbData );
}

//------------------------------------------------------------------------------
vtkImageData* MolekelMolecule::GenerateMEPData( const double& step,
                                                double& minValue,
                                                double& maxValue,
                                                ProgressCallback cb,
                                                void* cbData ) const
{
    return GenerateDensityData( MEP, step, minValue, maxValue, cb, cbData );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::StopMEPDataGeneration() const
{
extern void StopProcessCalc();
    StopProcessCalc();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::MEPDataGenerationStopped() const
{
extern bool ProcessCalcStopped();
extern int 	GetProcessCalcDataType();
    return ProcessCalcStopped() && GetProcessCalcDataType() == MEP;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::StopElectronDensityDataGeneration() const
{
extern void StopProcessCalc();
    StopProcessCalc();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::ElectronDensityDataGenerationStopped() const
{
extern bool ProcessCalcStopped();
extern int 	GetProcessCalcDataType();
    return ProcessCalcStopped() && GetProcessCalcDataType() == EL_DENS;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::StopSpinDensityDataGeneration() const
{
extern void StopProcessCalc();
    StopProcessCalc();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::SpinDensityDataGenerationStopped() const
{
extern bool ProcessCalcStopped();
extern int 	GetProcessCalcDataType();
    return ProcessCalcStopped() && GetProcessCalcDataType() == SPIN_DENS;
}

namespace
{
    //-----------------------------------------------------------------------------
    vtkActor* GenerateElDensSurfaceActor( vtkImageData* data, double value )
    {
        vtkActor* actor = GenerateIsoSurfaceActor( data, value );
        if( !actor ) return 0;
        actor->GetProperty()->SetColor( 0.9, 0.3, 0.9 ); // purple
        return actor;
    }

    //-----------------------------------------------------------------------------
    vtkActor* GenerateSpinDensSurfaceActor( vtkImageData* data, double value )
    {
        vtkActor* actor = GenerateIsoSurfaceActor( data, value );
        if( !actor ) return 0;
        actor->GetProperty()->SetColor( 0.9, 0.4, 0.2 ); // orange
        return actor;
    }
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::AddElectronDensitySurface( double bboxSize[ 3 ],
                                                 int steps[ 3 ],
                                                 double value,
                                                 ProgressCallback cb,
                                                 void* cbData )
{


    if( !CanComputeElectronDensity() ) return false;
    RemoveElectronDensitySurface();
    SaveTransform(); // push current transform
    ResetTransform(); // set to default (identity)
    vtkSmartPointer< vtkImageData > data(
                            GenerateDensityData( EL_DENS, bboxSize, steps, cb, cbData ) );
    elDensSurfaceActor_ = GenerateElDensSurfaceActor( data, value );
    if( elDensSurfaceActor_ == 0 ) return false;
    if( GLSLShadersSupported() )
    {
        vtkGLSLShaderActor* a = dynamic_cast< vtkGLSLShaderActor* >( elDensSurfaceActor_.GetPointer() );
        if( a ) shaderSurfaceMap_[ DENSITY_MATRIX_SURFACE ].actors.push_back( a );
        a->SetShaderProgramId( shaderSurfaceMap_[ DENSITY_MATRIX_SURFACE ].program );
    }

    MakeShinyMaterialType( elDensSurfaceActor_->GetProperty() );
    assembly_->AddPart( elDensSurfaceActor_ );
    RecomputeBBox();
    RestoreTransform(); // pop transform
    return true;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::AddSpinDensitySurface( double bboxSize[ 3 ],
                                             int steps[ 3 ],
                                             double value,
                                             ProgressCallback cb,
                                             void* cbData )
{


    if( !CanComputeSpinDensity() ) return false;
    RemoveSpinDensitySurface();
    SaveTransform(); // push current transform
    ResetTransform(); // set to default (identity)
    vtkSmartPointer< vtkImageData > data(
                            GenerateDensityData( SPIN_DENS, bboxSize, steps, cb, cbData ) );
    spinDensSurfaceActor_ = GenerateSpinDensSurfaceActor( data, value );
    if( spinDensSurfaceActor_ == 0 ) return false;
    assembly_->AddPart( spinDensSurfaceActor_ );
    RecomputeBBox();
    RestoreTransform(); // pop transform
    return true;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveElectronDensitySurface()
{
    if( elDensSurfaceActor_ == 0 ) return;
    if( GLSLShadersSupported() )
    {
        vtkGLSLShaderActor* a = dynamic_cast< vtkGLSLShaderActor* >( elDensSurfaceActor_.GetPointer() );
        typedef FindVtkPointer< vtkGLSLShaderActor > Finder;
        typedef ShaderProgram::Actors SA;
        SA::iterator b = shaderSurfaceMap_[ DENSITY_MATRIX_SURFACE ].actors.begin();
        SA::iterator e = shaderSurfaceMap_[ DENSITY_MATRIX_SURFACE ].actors.end();
        SA::iterator i = find_if( b, e, Finder( a ) );
        if( i != e )
        {
            shaderSurfaceMap_[ DENSITY_MATRIX_SURFACE ].actors.erase( i );
        }
    }
    assembly_->RemovePart( elDensSurfaceActor_ );
    elDensSurfaceActor_ = 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveSpinDensitySurface()
{
    if( spinDensSurfaceActor_ == 0 ) return;
    assembly_->RemovePart( spinDensSurfaceActor_ );
    spinDensSurfaceActor_ = 0;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::CanComputeMEP() const
{
    return molekelMol_ && molekelMol_->Atoms.size();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetElDensSurfaceRenderingStyle( RenderingStyle rs )
{
    if( elDensSurfaceActor_ == 0 ) return;
    vtkProperty* p = elDensSurfaceActor_->GetProperty();
    switch( rs )
    {
        case POINTS: p->SetRepresentationToPoints();
                     p->SetOpacity( 1.0f );
                     break;
        case WIREFRAME: p->SetRepresentationToWireframe();
                        p->SetOpacity( 1.0f );
                        break;
        case SOLID: p->SetRepresentationToSurface();
                    p->SetOpacity( 1.0f );
                    break;
        case TRANSPARENT_SOLID: p->SetRepresentationToSurface();
                                p->SetOpacity( 0.7f );
                                break;
        default: break;
    }


}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasVibrationData() const
{
    return ( molekelMol_ && molekelMol_->vibration.size() );
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasTrajectoryData() const
{
    return ( molekelMol_ && molekelMol_->dynamics.ntotalsteps > 1 );
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetVibrationVectorsVisibility() const
{
    return ( vibrationVectorsActor_ != 0 && vibrationVectorsActor_->GetVisibility() );
}

//--------------------------------------------------------------------------------
namespace
{
    /// Free function: returns actor containing glyph mapper to map
    /// vibration vectors to arrows.
    vtkActor* CreateVibrationVectorsActor( Molecule* mol,
                                           double length = 1.,
                                           double tipLength = 0.25,
                                           double tipRadius = 0.13,
                                           double shaftRadius = 0.03,
                                           int tipResolution = 12,
                                           int shaftResolution = 12 )
    {
        vtkSmartPointer< vtkArrowSource > as( vtkArrowSource::New() );
        // create arrow source
        as->SetTipLength( tipLength );
        as->SetTipRadius( tipRadius );
        as->SetShaftRadius( shaftRadius );
        as->SetTipResolution( tipResolution );
        as->SetShaftResolution( shaftResolution );
        // rotate 90 degrees about z axis: vtkOpenGLGlyphMapper assumes object
        // to be aligned along y axis while vtkArrowSource is aligned along
        // x axis
        vtkSmartPointer< vtkTransformFilter > tf( vtkTransformFilter::New() );
        vtkSmartPointer< vtkTransform > t( vtkTransform::New() );
        t->RotateZ( 90. );
        t->Scale( length, 1, 1 );
        tf->SetTransform( t );
        tf->SetInput( as->GetOutput() );
        // create vtkGlyphMapper & reserve space: number of matrices = number of atoms
        vtkSmartPointer< vtkOpenGLGlyphMapper > gm( vtkOpenGLGlyphMapper::New() );
        // set input to arrow source
        gm->SetInputConnection( tf->GetOutputPort() );
        gm->Allocate( mol->Atoms.size() );
        vtkActor* a = vtkActor::New();
        a->SetMapper( gm );
        a->GetProperty()->SetColor( 0, 1, 1 );
        return a;
    }


    /// Free function: returns actor containing glyph mapper to map
    /// vibration vectors to arrows.
    void CreateArrowActor( vtkActor* a,
                           double start[ 3 ],
                           double end[ 3 ],
                           double length = 2.,
                           double tipLength = 0.27,
                           double tipRadius = 0.12,
                           double shaftRadius = 0.03,
                           int tipResolution = 32,
                           int shaftResolution = 32 )
    {
        assert( a && "vtkActor is NULL" );
        // create arrow source
        vtkSmartPointer< vtkArrowSource > as( vtkArrowSource::New() );
        as->SetTipLength( tipLength );
        as->SetTipRadius( tipRadius );
        as->SetShaftRadius( shaftRadius );
        as->SetTipResolution( tipResolution );
        as->SetShaftResolution( shaftResolution );
        // rotate 90 degrees about z axis: vtkOpenGLGlyphMapper assumes object
        // to be aligned along y axis while vtkArrowSource is aligned along
        // x axis
        vtkSmartPointer< vtkTransformFilter > tf( vtkTransformFilter::New() );
        vtkSmartPointer< vtkTransform > t( vtkTransform::New() );
        t->RotateZ( 90. );
        t->Scale( length, 1, 1 );
        tf->SetTransform( t );
        tf->SetInput( as->GetOutput() );
        // create vtkGlyphMapper & reserve space: number of matrices = number of atoms
        vtkSmartPointer< vtkOpenGLGlyphMapper > gm( vtkOpenGLGlyphMapper::New() );
        // set input to arrow source
        gm->SetInputConnection( tf->GetOutputPort() );
        gm->Add( start, end );
        a->SetMapper( gm );
        a->GetProperty()->SetColor( 1, 0.4, 0.3 );
    }


}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetVibrationVectorsVisibility( bool v )
{
    if( !HasVibrationData() ) return;
    if( vibrationVectorsActor_ == 0 )
    {

        vibrationVectorsActor_ = CreateVibrationVectorsActor( molekelMol_,
                                                              2.5, // length
                                                              0.30, // tip length
                                                              0.03, // tip radius
                                                              0.02, // shaft radius
                                                              16, // tip resolution
                                                              16 ); // shaft resolution
        assembly_->AddPart( vibrationVectorsActor_ );
    }
    vibrationVectorsActor_->SetVisibility( v );
}


//--------------------------------------------------------------------------------
void MolekelMolecule::SetVibrationVector( int a, double p0[ 3 ], double p1[ 3 ], bool scale )
{
    if( !HasVibrationData() || vibrationVectorsActor_ == 0  ) return;
    vtkOpenGLGlyphMapper* gm = dynamic_cast< vtkOpenGLGlyphMapper* >( vibrationVectorsActor_->GetMapper() );
    gm->Set( a, p0, p1, 0., scale );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetDipoleMomentVisibility( bool on )
{
    if( !HasDipoleMoment() ) return;
    if( dipoleMomentActor_ == 0 )
    {
        double start[ 3 ];
        double end[ 3 ];
        start[ 0 ] = molekelMol_->dipole->start[ 0 ];
        start[ 1 ] = molekelMol_->dipole->start[ 1 ];
        start[ 2 ] = molekelMol_->dipole->start[ 2 ];
        end[ 0 ] = molekelMol_->dipole->end[ 0 ];
        end[ 1 ] = molekelMol_->dipole->end[ 1 ];
        end[ 2 ] = molekelMol_->dipole->end[ 2 ];
        dipoleMomentActor_ = vtkActor::New();
        CreateArrowActor( dipoleMomentActor_, start, end, dipoleMomentArrowLength_ );
        assembly_->AddPart( dipoleMomentActor_ );
    }
    dipoleMomentActor_->SetVisibility( on );
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetDipoleMomentVisibility() const
{
    if( !HasDipoleMoment() || dipoleMomentActor_ == 0 ) return false;
    return dipoleMomentActor_->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasDipoleMoment() const
{
    return molekelMol_ != 0 && molekelMol_->dipole != 0;
}

//--------------------------------------------------------------------------------
double MolekelMolecule::GetDipoleMomentArrowLength() const
{
    return dipoleMomentArrowLength_;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetDipoleMomentArrowLength( double l )
{
    dipoleMomentArrowLength_ = l;
    if( dipoleMomentActor_ == 0 ) return;
    double start[ 3 ];
    double end[ 3 ];
    start[ 0 ] = molekelMol_->dipole->start[ 0 ];
    start[ 1 ] = molekelMol_->dipole->start[ 1 ];
    start[ 2 ] = molekelMol_->dipole->start[ 2 ];
    end[ 0 ] = molekelMol_->dipole->end[ 0 ];
    end[ 1 ] = molekelMol_->dipole->end[ 1 ];
    end[ 2 ] = molekelMol_->dipole->end[ 2 ];
    dipoleMomentActor_ = vtkActor::New();
    CreateArrowActor( dipoleMomentActor_, start, end, dipoleMomentArrowLength_ );
}

//--------------------------------------------------------------------------------
extern double calc_mep( const Mol *mol, const double xyz[ 3 ] );
namespace
{
    /// Free function, maps scalar values from vtkImageData to vtkPolyData.
    /// @param id [in]  image data whose value will be used to color code the surface
    /// @param pd [out] surface to be color coded
    /// @param minValue [out] minimum value of scalar assigned to vertices
    /// @param maxValue [out] maximum value of scalar assigned to vertices
    void MapImageDataToPolyDataScalars( vtkImageData* id, vtkPolyData* pd,
                                        double& minValue, double& maxValue )
    {
        assert( id && "NULL vtkImageData" );
        assert( pd && "NULL vtkPolyData" );
        vtkPoints* points = pd->GetPoints();
        if( !points ) return;
        vtkSmartPointer< vtkDoubleArray > scalars( vtkDoubleArray::New() );
        const int sz = points->GetNumberOfPoints();
        scalars->Allocate( sz );
        minValue = std::numeric_limits< double >::max();
        maxValue = std::numeric_limits< double >::min();
        const int* dim = id->GetDimensions();
        for( int i = 0; i != sz; ++i )
        {
            double* p = points->GetPoint( i );
            int voxel[ 3 ];
            double coord[ 3 ];
            id->ComputeStructuredCoordinates( p, voxel, coord );
            voxel[ 0 ] = std::max( 0, std::min( voxel[ 0 ], dim[ 0 ] - 1 ) );
            voxel[ 1 ] = std::max( 0, std::min( voxel[ 1 ], dim[ 1 ] - 1 ) );
            voxel[ 2 ] = std::max( 0, std::min( voxel[ 2 ], dim[ 2 ] - 1 ) );
            const double v = id->GetScalarComponentAsDouble( voxel[ 0 ],
                                                             voxel[ 1 ],
                                                             voxel[ 2 ],
                                                             0 );
            if( v < minValue ) minValue = v;
            if( v > maxValue ) maxValue = v;
            scalars->InsertValue( i, v );
        }
        pd->GetPointData()->SetScalars( scalars );
     }

    /// Free function, generates per-vertex MEP values.
    /// @param mol [in]  molecule
    /// @param pd [in/out] surface whose scalars will be generate through
    ///        usage of FunT functor/function.
    /// @param minValue [out] minimum value of scalar assigned to vertices
    /// @param maxValue [out] maximum value of scalar assigned to vertices
    /// The functor template parameter FunT has to support
    /// double operator()( const double point[ 3 ] ) const.
    template < class FunT >
    void GeneratePolyDataScalarValues( const FunT& f, vtkPolyData* pd,
                                       double& minValue, double& maxValue )
    {
        assert( pd && "NULL vtkPolyData" );
        vtkPoints* points = pd->GetPoints();
        if( !points ) return;
        vtkSmartPointer< vtkDoubleArray > scalars( vtkDoubleArray::New() );
        const int sz = points->GetNumberOfPoints();
        scalars->Allocate( sz );
        minValue = std::numeric_limits< double >::max();
        maxValue = std::numeric_limits< double >::min();

        for( int i = 0; i != sz; ++i )
        {
            double* p = points->GetPoint( i );
            const double v = f( p[ 0 ] );
            if( v < minValue ) minValue = v;
            if( v > maxValue ) maxValue = v;
            scalars->InsertValue( i, v );
        }
        pd->GetPointData()->SetScalars( scalars );
    }

    /// Free function, generates per-vertex MEP values.
    /// @param mol [in]  molecule
    /// @param pd [in/out] surface to be color coded
    /// @param minValue [out] minimum value of scalar assigned to vertices
    /// @param maxValue [out] maximum value of scalar assigned to vertices
    void MapMEPToPolyDataScalars( const Molecule* mol, vtkPolyData* pd,
                                  double& minValue, double& maxValue )
    {
        assert( pd && "NULL vtkPolyData" );
        assert( mol && "NULL molecule" );
        vtkPoints* points = pd->GetPoints();
        if( !points ) return;
        vtkSmartPointer< vtkDoubleArray > scalars( vtkDoubleArray::New() );
        const int sz = points->GetNumberOfPoints();
        scalars->Allocate( sz );
        minValue = std::numeric_limits< double >::max();
        maxValue = std::numeric_limits< double >::min();

        for( int i = 0; i != sz; ++i )
        {
            double* p = points->GetPoint( i );
            const double v = calc_mep( mol, &p[ 0 ] );
            if( v < minValue ) minValue = v;
            if( v > maxValue ) maxValue = v;
            scalars->InsertValue( i, v );
        }
        pd->GetPointData()->SetScalars( scalars );
     }
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSurface( vtkActor* a,
                                       double bboxSize[ 3 ],
                                       int steps[ 3 ],
                                       const vtkLookupTable* lut,
                                       bool useGridData ) const
{
    if( a == 0 ) return;
    vtkPolyDataMapper* pdm = dynamic_cast< vtkPolyDataMapper* >( a->GetMapper() );
    assert( pdm && "Wrong vtkMapper type" );
    //vtkSmartPointer< vtkImageData > mep = GenerateDensityData( MEP, bboxSize, steps );
    if( useGridData && !HasGridData() ) return;
    double minv, maxv;

    if( useGridData )
    {
        vtkSmartPointer< vtkImageData > mep = GridDataToVtkImageData( "", 1, 0, 0 );
        MapImageDataToPolyDataScalars( mep, pdm->GetInput(), minv, maxv );
    }
    else MapMEPToPolyDataScalars( molekelMol_, pdm->GetInput(), minv, maxv );
    assert( a->GetMapper()->GetLookupTable() && "NULL LUT" );
    a->GetMapper()->SetScalarRange( minv, maxv );
    a->GetMapper()->ScalarVisibilityOn();
    if( lut )
    {
        vtkSmartPointer< vtkLookupTable > lt( vtkLookupTable::New() );
        // VTK requires the source (copy from) object not to be constant
        lt->DeepCopy( const_cast< vtkLookupTable* >( lut ) );
        lt->SetTableRange( minv, maxv );
        a->GetMapper()->SetLookupTable( lt );
        // record MEP LUT
        mepLUT_ = lt;
    }
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnElDensSurface( double bboxSize[ 3 ], int steps[ 3 ],
                                             const vtkLookupTable* lut )
{
    MapMEPOnSurface( elDensSurfaceActor_, bboxSize, steps, lut );
}

//--------------------------------------------------------------------------------
#include <vtkProp3DCollection.h>
void MolekelMolecule::MapMEPOnOrbitalSurface( int orbital, double bboxSize[ 3 ],
                                              int steps[ 3 ], const vtkLookupTable* lut )
{
    OrbitalActorMap::iterator i = orbitalActorMap_.find( orbital );
    if(  i == orbitalActorMap_.end() ) return;
    vtkSmartPointer< vtkAssembly >& assembly = i->second;
    vtkProp3DCollection* c = assembly->GetParts();
    assert( c && "Empty orbital assembly" );
    c->InitTraversal();
    while( vtkProp3D* p = dynamic_cast< vtkProp3D* >( c->GetNextItemAsObject() ) )
    {
        MapMEPOnSurface( dynamic_cast< vtkActor* >( p ), bboxSize, steps, lut );
    }
}

//--------------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetOrbitalSurfaceLUT( int orbitalIndex ) const
{
    if( !HasOrbitalSurface( orbitalIndex ) ) return 0;
    OrbitalActorMap::const_iterator i = orbitalActorMap_.find( orbitalIndex );
    const vtkSmartPointer< vtkAssembly >& assembly = i->second;
    vtkProp3DCollection* c = assembly->GetParts();
    assert( c && "Empty Orbital Surface Assembly" );
    c->InitTraversal();
    vtkActor* a = dynamic_cast< vtkActor* >( c->GetNextItemAsObject() );
    assert( a && "NULL Orbital Surface" );
    return dynamic_cast< vtkLookupTable* >( a->GetMapper()->GetLookupTable() );
}

//--------------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetElectronDensitySurfaceLUT() const
{
    if( !HasElectronDensitySurface() ) return 0;
    return dynamic_cast< vtkLookupTable* >( elDensSurfaceActor_->GetMapper()->GetLookupTable() );
}

//--------------------------------------------------------------------------------
int MolekelMolecule::GetNumberOfOrbitalSurfaces() const
{
    return int( orbitalActorMap_.size() );
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetOrbitalSurfaceVisibility( int orbital ) /*const*/
{
    if( !HasOrbitalSurface( orbital ) ) return false;
    return orbitalActorMap_[ orbital ]->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetOrbitalSurfaceVisibility( int orbital, bool on )
{
    if( !HasOrbitalSurface( orbital ) ) return;
    OrbitalActorMap::iterator i = orbitalActorMap_.find( orbital );
    if(  i == orbitalActorMap_.end() ) return;
    vtkSmartPointer< vtkAssembly >& assembly = i->second;
    vtkProp3DCollection* c = assembly->GetParts();
    assert( c && "Empty orbital assembly" );
    c->InitTraversal();
    while( vtkProp3D* p = dynamic_cast< vtkProp3D* >( c->GetNextItemAsObject() ) )
    {
        p->SetVisibility( on );
    }
    orbitalActorMap_[ orbital ]->SetVisibility( on );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetSpinDensitySurfaceVisibility() /*const*/
{
    if( !HasSpinDensitySurface() ) return false;
    return spinDensSurfaceActor_->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSpinDensitySurfaceVisibility( bool on )
{
    if( !HasSpinDensitySurface() ) return;
    spinDensSurfaceActor_->SetVisibility( on );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetElDensSurfaceVisibility() /*const*/
{
    if( !HasElectronDensitySurface() ) return false;
    return elDensSurfaceActor_->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetElDensSurfaceVisibility( bool on )
{
    if( !HasElectronDensitySurface() ) return;
    elDensSurfaceActor_->SetVisibility( on );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetGridDataSurfaceVisibility( const std::string& label ) /*const*/
{
    if( !HasGridDataSurface( label ) ) return false;

    bool vis = false;
    vtkActor* a = 0;
    if( format_ != "t41" ) a = gridDataActor_;
    else
    {
        if( gridActorMap_.find( label ) != gridActorMap_.end() ) a = gridActorMap_[ label ];
    }

    if( !a ) return false;
    return a->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetGridDataSurfaceVisibility( bool on, const std::string& label )
{
   if( !HasGridDataSurface( label ) ) return;
   if( format_ != "t41" ) gridDataActor_->SetVisibility( on );
   else
   {
        if( gridActorMap_.find( label ) == gridActorMap_.end() ) return;
        gridActorMap_[ label ]->SetVisibility( on );
   }
   RecomputeBBox();
}

//--------------------------------------------------------------------------------
double MolekelMolecule::ComputeDihedralAngle( int atom1, int atom2, int atom3, int atom4 ) const
{
    return obMol_->GetTorsion( atom1 + 1, atom2 + 1, atom3 + 1, atom4 + 1 );
}

//--------------------------------------------------------------------------------
double MolekelMolecule::ComputeAngle( int atom1Id, int atom2Id, int atom3Id ) const
{
    OBAtom* atom1 = obMol_->GetAtom( atom1Id + 1 );
    return atom1->GetAngle( atom2Id + 1, atom3Id + 1 );
}

//--------------------------------------------------------------------------------
double MolekelMolecule::ComputeDistance( int atom1Id, int atom2Id ) const
{
    OBAtom* atom1 = obMol_->GetAtom( atom1Id + 1 );
    return atom1->GetDistance( atom2Id + 1 );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveBond( int bondId ) 
{
	assert( chemData_ );
	if( !chemData_ ) return;
	if( chemData_->numberOfBonds.getValue() <= bondId ) return;
	if( bondId < 0 ) return;
	//std::ofstream os( "/tmp/molekel2.log" );
	//os << "BOND ID:   " << bondId << std::endl;
	int a1 = chemData_->getBondFrom( bondId ) + 1;
	int a2 = chemData_->getBondTo( bondId )  + 1;
	//os << "ATOMS IDs: " << a1 << ' ' << a2 << '\n';
	if( obMol_->GetBond( a1, a2 ) ) obMol_->DeleteBond( obMol_->GetBond( a1, a2 ) );	
	chemData_->bondFrom.deleteValues( bondId, 1 );
	//os << "bondFrom.deleteValues" << std::endl;
	chemData_->bondTo.deleteValues( bondId, 1 );
	//os << "bondTo.deleteValues" << std::endl;
	chemData_->bondIndex.deleteValues( bondId, 1 );
	//os << "bondIndex.deleteValues" << std::endl;
	chemData_->bondType.deleteValues( bondId, 1 );
	//os << "bondType.deleteValues" << std::endl;
	chemData_->numberOfBonds.setValue( chemData_->numberOfBonds.getValue() - 1 ); 
	chemData_->touch();
	//os << "BOND:      " << obMol_->GetBond( a1, a2 ) << ' ' << obMol_->GetBond( bondId ) << ' ' <<obMol_->GetBond( bondId + 1 ) << '\n';
	//return;
	
}

//--------------------------------------------------------------------------------
void MolekelMolecule::AddBond( int atom1Id, int atom2Id ) 
{
	assert( chemData_ );
	if( !chemData_ ) return;
	if( chemData_->numberOfAtoms.getValue() <= atom1Id ||
		chemData_->numberOfAtoms.getValue() <= atom2Id ) return;
	if( atom1Id < 0 || atom2Id < 0 ) return;

	//std::ofstream os( "/tmp/molekel.log" );
	//os << chemData_->bondIndex.getNum() << ' ' << chemData_->bondType.getNum() << std::endl;
	//for( int i = 0; i != chemData_->bondIndex.getNum(); ++i, os << chemData_->bondIndex[ i ] << std::endl );
	//os << "===================" << std::endl;
	//for( int i = 0; i != chemData_->bondType.getNum(); ++i, os << chemData_->bondType[ i ]  << std::endl );
	
	// if there are already bonds then add it; in some cases the OBMol is used only to hold non-bond
	// related information
	if( obMol_->NumBonds() ) obMol_->AddBond( atom1Id + 1, atom2Id + 1, 1 );
	const int T = chemData_->bondTo.getNum();
	const int F = chemData_->bondFrom.getNum();
	const int I = chemData_->bondIndex.getNum();
	const int E = chemData_->bondType.getNum();
	chemData_->numberOfBonds.setValue( chemData_->numberOfBonds.getValue() + 1 );
	chemData_->bondFrom.setNum( F + 1 );
	chemData_->bondTo.setNum( T + 1 );
	chemData_->bondIndex.setNum( I + 1 );
	chemData_->bondType.setNum( E + 1 );
	int32_t* from = chemData_->bondFrom.startEditing();
	assert( from && "NULL from bonds" );
	int32_t* to = chemData_->bondTo.startEditing();
	assert( to && "NULL to bonds" );
	int32_t* index = chemData_->bondIndex.startEditing();
	assert( index && "NULL bond index" );
	int* type = chemData_->bondType.startEditing();
	assert( type && "NULL bond types" );

	from[ F ] = atom1Id;
	to[ T ] = atom2Id;
	index[ I ] = I; //os << " index[ I ] = " << index[ I - 1 ] + 1 << std::endl;
	type[ E ] = 1;
	chemData_->bondFrom.finishEditing();
	chemData_->bondTo.finishEditing();
	chemData_->bondIndex.finishEditing();
	chemData_->bondType.finishEditing();
	chemData_->touch();

	//os << std::endl << "////////////////////////////////////" << std::endl;
	//os << chemData_->bondIndex.getNum() << ' ' << chemData_->bondType.getNum() << std::endl;
	//for( int i = 0; i != chemData_->bondIndex.getNum(); ++i, os << chemData_->bondIndex[ i ] << std::endl );
	//os << "===================" << std::endl;
	//for( int i = 0; i != chemData_->bondType.getNum(); ++i, os << chemData_->bondType[ i ]  << std::endl );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SaveTransform() /* const ?  does not change any state variable */
{
    assembly_->GetOrientation( savedTransform_.orientation );
    assembly_->GetPosition( savedTransform_.position );
    assembly_->GetScale( savedTransform_.scaling );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RestoreTransform()
{
    assembly_->SetOrientation( savedTransform_.orientation );
    assembly_->SetPosition( savedTransform_.position );
    assembly_->SetScale( savedTransform_.scaling );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::GetAtomPosition( int atomId, double pos[ 3 ] ) const
{
    assert( atomId >= 0 && atomId < GetNumberOfAtoms() );
    /// @note we could consider returning a const pointer to atom position since
    /// OB returns it.
    const double* p = obMol_->GetAtom( atomId + 1 )->GetCoordinate();
    pos[ 0 ] = p[ 0 ];
    pos[ 1 ] = p[ 1 ];
    pos[ 2 ] = p[ 2 ];
}


//--------------------------------------------------------------------------------
double MolekelMolecule::GetVdWRadius( int atomId ) const
{
    ///extern Element element[ 105 ];
    assert( atomId >= 0 && atomId < GetNumberOfAtoms() );
    /// @warning value returned by openbabel for vdw radius is always
    /// zero! even when environment variable BABEL_DATADIR is set;
    /// vdw radius is zero in Molekel as well.
    return GetElementTable()[ ( obMol_->GetAtom( atomId + 1 )->GetAtomicNum() ) ].vdwRadius;
    //extern Element element[ 105 ];
    //return element[ obMol_->GetAtom( atomId + 1 )->GetAtomicNum() ].rvdw; // <-- zero
}

//--------------------------------------------------------------------------------
double MolekelMolecule::GetCovalentRadius( int atomId ) const
{
    assert( atomId >= 0 && atomId < GetNumberOfAtoms() );
    return etab.GetCovalentRad( obMol_->GetAtom( atomId + 1 )->GetAtomicNum() );
}

//--------------------------------------------------------------------------------
double MolekelMolecule::GetDistanceFromAtom( int atomId, const double point[ 3 ] ) const
{
    assert( atomId >= 0 && atomId < GetNumberOfAtoms() );
    const double* p = obMol_->GetAtom( atomId + 1 )->GetCoordinate();
    const double dist = vdist( point, p );
    return ( dist - GetCovalentRadius( atomId ) );
}

//--------------------------------------------------------------------------------
// Following code used for experimenting only; to be removed in final version
namespace
{
    // square
    double Sq( double v ) { return v * v; }
    // squared distance
    double SqDist( const double p1[ 3 ], const double p2[ 3 ] )
    {
        return Sq( p1[ 0 ] - p2[ 0 ] ) + Sq( p1[ 1 ] - p2[ 1 ] ) + Sq( p1[ 2 ] - p2[ 2 ] );
    }
    // sphere equation
    double Sphere( const double center[ 3 ], double radius, const double point[ 3 ] )
    {
        return  SqDist( center, point ) - Sq( radius );
    }
    // blob equation
    double Blob( const double center[ 3 ], double radius, const double point[ 3 ] )
    {
        const double a = 1 ; // height - can be function of radius
        const double b = 1; // width
        return a * exp( -b * Sq( radius ) ) - a * exp( -b * SqDist( center, point ) );
    }
}

double MolekelMolecule::GetMinDistanceFromAtom( const double point[ 3 ],
                                                int& atomId,
                                                double dr ) const
{
    /// @todo add spatial partitioning to allow for faster searches
    /// of closest atoms
    atomId = -1;
    double minValue = numeric_limits< double >::max();
    for( int a = 0; a != GetNumberOfAtoms(); ++a )
    {
        /// @warning since both OpenBabel and the old molekel code do return a zero VdW radius
        /// the covalent radius is used instead of VdW to at least display something.
        const double d = Sphere( obMol_->GetAtom( a + 1 )->GetCoordinate(),
                                 GetVdWRadius( a ) + dr, point );
        if( d < minValue )
        {
            minValue = d;
            atomId = a;
        }
    }
    return minValue;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::AddSAS( double solventRadius, double step,
                              ProgressCallback cb, void* cbData )
{
    assert( step > 0. );
    stopSASComputation_ = false;
    RemoveSAS();
    // create vtkImageData with distances from surface:
    //   for each point in the grid store the signed distance
    //   between the grid point and the closest atom
    //   if stopSASComputation_ == true return.
    const int displayStyle = GetChemDisplayParam()->displayStyle.getValue();
    GetChemDisplayParam()->displayStyle.setValue( ChemDisplayParam::DISPLAY_CPK );
    SaveTransform();
    ResetTransform();
    RecomputeBBox();
    GetChemDisplayParam()->displayStyle.setValue( displayStyle );
    /// @warning using a VTK 5.0.2 vtkSmartPointer in this code called from a separate
    /// thread crashes; using a regular pointer and invoking Delete doensn't.
    vtkSmartPointer< vtkImageData > grid( vtkImageData::New() );
    const double db = solventRadius;
    double bounds[ 6 ];
    // get bounds and increase box by solventRadius size in each direction
    bounds[ 0 ] = assembly_->GetBounds()[ 0 ] - db;
    bounds[ 1 ] = assembly_->GetBounds()[ 1 ] + db;
    bounds[ 2 ] = assembly_->GetBounds()[ 2 ] - db;
    bounds[ 3 ] = assembly_->GetBounds()[ 3 ] + db;
    bounds[ 4 ] = assembly_->GetBounds()[ 4 ] - db;
    bounds[ 5 ] = assembly_->GetBounds()[ 5 ] + db;
    const int nx = int( ( bounds[ 1 ] - bounds[ 0 ] ) / step  + .5 );
    const int ny = int( ( bounds[ 3 ] - bounds[ 2 ] ) / step  + .5 );
    const int nz = int( ( bounds[ 5 ] - bounds[ 4 ] ) / step  + .5 );
    grid->SetDimensions( nx, ny, nz );
    grid->SetSpacing( step, step, step );
    grid->SetOrigin( bounds[ 0 ], bounds[ 2 ], bounds[ 4 ] );
    const int n = GetNumberOfAtoms();
    double point[ 3 ];
    int atomId;
    const int totalSteps = nx * ny * nz;
    if( cb ) cb( 0, totalSteps, cbData );
    double x = bounds[ 0 ];
    for( int i = 0; i != nx; ++i, x += step )
    {
        double y = bounds[ 2 ];
        for( int j = 0; j != ny ; ++j, y += step )
        {
            double z = bounds[ 4 ];
            for( int k = 0; k != nz; ++k, z += step )
            {
                point[ 0 ] = x; point[ 1 ] = y; point[ 2 ] = z;
                const double s = GetMinDistanceFromAtom( point, atomId, solventRadius );
                grid->SetScalarComponentFromDouble( i, j, k, 0, s );
            }
        }
        if( stopSASComputation_ ) break;
        const int currentStep = ( i + 1 ) * ny * nz;
        if( cb ) cb( currentStep, totalSteps, cbData );

    }
    RestoreTransform();
    if( stopSASComputation_ )
    {
        RecomputeBBox();
        grid->Delete();
        return;
    }
    // generate surface at distance <solvent radius> from VdW surface
    // use solventRadius = 0 for VdW
    sasActor_ = GenerateIsoSurfaceActor( grid, 0.  );
    if( GLSLShadersSupported() )
    {
        vtkGLSLShaderActor* a = dynamic_cast< vtkGLSLShaderActor* >( sasActor_.GetPointer() );
        if( a ) shaderSurfaceMap_[ SAS_SURFACE ].actors.push_back( a );
        a->SetShaderProgramId( shaderSurfaceMap_[ SAS_SURFACE ].program );
    }

    if( sasActor_ == 0 ) return;
    sasActor_->GetProperty()->SetColor( 0.1, 0.92, 0.92 ); // cyan
    MakeShinyMaterialType( sasActor_->GetProperty() );
    assembly_->AddPart( sasActor_ );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveSAS()
{
    if( sasActor_ == 0 ) return;
    if( GLSLShadersSupported() )
    {
        vtkGLSLShaderActor* a = dynamic_cast< vtkGLSLShaderActor* >( sasActor_.GetPointer() );
        typedef FindVtkPointer< vtkGLSLShaderActor > Finder;
        typedef ShaderProgram::Actors SA;
        SA::iterator b = shaderSurfaceMap_[ SAS_SURFACE ].actors.begin();
        SA::iterator e = shaderSurfaceMap_[ SAS_SURFACE ].actors.end();
        SA::iterator i = find_if( b, e, Finder( a ) );
        if( i != e )
        {
            shaderSurfaceMap_[ SAS_SURFACE ].actors.erase( i );
        }
    }

    assembly_->RemovePart( sasActor_ );
    sasActor_ = 0;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetSASVisibility() const
{
    if( !HasSAS() ) return false;
    return  sasActor_->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSASVisibility( bool on )
{
    if( !HasSAS() ) return;
    sasActor_->SetVisibility( on );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSASRenderingStyle( RenderingStyle rs )
{
    if( sasActor_ == 0 ) return;
    vtkProperty* p = sasActor_->GetProperty();
    switch( rs )
    {
        case POINTS: p->SetRepresentationToPoints();
                     p->SetOpacity( 1.0f );
                     break;
        case WIREFRAME: p->SetRepresentationToWireframe();
                        p->SetOpacity( 1.0f );
                        break;
        case SOLID: p->SetRepresentationToSurface();
                    p->SetOpacity( 1.0f );
                    break;
        case TRANSPARENT_SOLID: p->SetRepresentationToSurface();
                                p->SetOpacity( 0.7f );
                                break;
        default: break;
    }


}

//--------------------------------------------------------------------------------
int MolekelMolecule::GetNumberOfAtoms() const
{
    return obMol_->NumAtoms();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasSAS() const
{
    return sasActor_ != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSAS( double bboxSize[ 3 ], int steps[ 3 ],
                                   const vtkLookupTable* lut, bool useGridData )
{

    MapMEPOnSurface( sasActor_, bboxSize, steps, lut, useGridData );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSAS( double step, const vtkLookupTable* lut,
                                   bool useGridData )
{
    assert( step > .0 && "STEP < 0" );
    const double* bounds = assembly_->GetBounds();
    double bbs[ 6 ];
    bbs[ 0 ] = bounds[ 1 ] - bounds[ 0 ];
    bbs[ 1 ] = bounds[ 3 ] - bounds[ 2 ];
    bbs[ 2 ] = bounds[ 5 ] - bounds[ 4 ];
    int steps[ 3 ];
    steps[ 0 ] = int( bbs[ 0 ] / step + .5 );
    steps[ 1 ] = int( bbs[ 1 ] / step + .5 );
    steps[ 2 ] = int( bbs[ 2 ] / step + .5 );
    MapMEPOnSAS( bbs, steps, lut, useGridData );
}

//--------------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetSASLUT() const
{
    if( !HasSAS() ) return 0;
    return dynamic_cast< vtkLookupTable* >( sasActor_->GetMapper()->GetLookupTable() );
}

//--------------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetGridDataSurfaceLUT( const std::string& label ) const
{
    if( !HasGridDataSurface( label ) ) return 0;
    vtkLookupTable* lut = 0;
    if( format_ != "t41" )
    {
        lut  = dynamic_cast< vtkLookupTable* >( gridDataActor_->GetMapper()->GetLookupTable() );
    }
    else
    {
        if( gridActorMap_.find( label ) == gridActorMap_.end() ) return 0;
        lut = dynamic_cast< vtkLookupTable* >( gridActorMap_.find( label )->second->GetMapper()->GetLookupTable() );
    }
    return lut;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::AddSES( double solventRadius, double step,
                              ProgressCallback cb, void* cbData )
{
    connollyDot_->densityOfPoints.setValue( step );
    connollyDot_->probeRadius = solventRadius;
    sesSwitch_->whichChild = SO_SWITCH_ALL;
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasSES() const
{
    SoVertexProperty* vp = dynamic_cast< SoVertexProperty* >(
                                        connollyDot_->vertexProperty.getValue() );
    return vp->vertex.getNum() > 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveSES()
{

    if( !HasSES() ) return;

    // no easy way to get a node's parent in open inventor: nodes can
    // have multiple parents and is therefore required to edit the node
    // by finding the parent first and then calling parent->getChild( findChild( childPtr ) )

    // just reset data generated by connolly surface generation function
    SoVertexProperty* vp = dynamic_cast< SoVertexProperty* >(
                                        connollyDot_->vertexProperty.getValue() );
    vp->vertex.setNum( 0 );
    vp->normal.setNum( 0 );
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetSESVisibility() const
{
    return sesSwitch_->whichChild.getValue() == SO_SWITCH_ALL;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSESVisibility( bool on )
{
    if( on ) sesSwitch_->whichChild = SO_SWITCH_ALL;
    else sesSwitch_->whichChild = SO_SWITCH_NONE;
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSESRenderingStyle( RenderingStyle rs )
{
    // NOT IMPLEMENTED
}

//--------------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetSESLUT() const
{
    // NOT IMPLEMENTED ALWAYS RETURNS 0
    return 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSES( double bboxSize[ 3 ], int steps[ 3 ],
                                   const vtkLookupTable* lut )
{
    // NOT IMPLEMENTED
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSES( double step, const vtkLookupTable* lut )
{
    // NOT IMPLEMENTED
}

//--------------------------------------------------------------------------------
void MolekelMolecule::AddSESMS( double probeRadius,
                                double density,
                                const std::string& msmsExecutable,
                                const std::string& inputFileName,
                                const std::string& outputFileName,
                                ProgressCallback cb,
                                void* cbData )
{

    stopSESMSComputation_ = false;
    // get temporary file name, used for
    // @warning the same filename with different extensions is going to be used
    // for input and output files.
    const string msmsIn  = inputFileName.size()  == 0 ? GetTemporaryFileName() : inputFileName;
    const string msmsOut = outputFileName.size() == 0 ? GetTemporaryFileName() : outputFileName;
    if( msmsIn.size() == 0 || msmsOut.size() == 0 )
    {
        stopSESMSComputation_ = true;
        throw MolekelException(
        "Cannot generate temporary file for generating Connolly surface" );
    }
    if( msmsExecutable.size() == 0 )
    {
        stopSESMSComputation_ = true;
        throw MolekelException( "NULL MSMS executable" );
    }

    Save( msmsIn.c_str(), "msms" );
    ostringstream commandLine;
    commandLine << msmsExecutable << ' ' << "-if " << msmsIn << ' ' << "-of " << msmsOut
                << " -density " << density << " -probe_radius " << probeRadius << " -no_area";
    commandLine.flush();

    const int r = StartSyncProcess( commandLine.str() );
    if( r != 0 )
    {
        stopSESMSComputation_ = true;
        return;
    }
    RemoveSESMS();
    vtkSmartPointer< vtkMSMSReader > msmsReader( vtkMSMSReader::New() );
    msmsReader->SetFileName( msmsOut );
    msmsReader->Update();
    vtkSmartPointer< vtkPolyData > pd( vtkPolyData::New() );
    pd->DeepCopy( msmsReader->GetOutput() );
    if( inputFileName.size() == 0 ) DeleteFile( msmsIn );
    if( outputFileName.size() == 0 )
    {
        DeleteFile( msmsOut + ".vert" );
        DeleteFile( msmsOut + ".face" );
    }
    if( pd->GetNumberOfCells() == 0 ) return;
    vtkSmartPointer< vtkPolyDataMapper > mapper( vtkPolyDataMapper::New() );
    mapper->SetInput( pd );
    mapper->ScalarVisibilityOff();
    sesmsActor_ = vtkActor::New();
    if( !GLSLShadersSupported() ) sesmsActor_ = vtkActor::New();
    else
    {
       shaderSurfaceMap_[ SESMS_SURFACE ].actors.push_back( vtkGLSLShaderActor::New() );
       sesmsActor_ =  shaderSurfaceMap_[ SESMS_SURFACE ].actors.back();
       shaderSurfaceMap_[ SESMS_SURFACE ].actors.back()
        ->SetShaderProgramId( shaderSurfaceMap_[ SESMS_SURFACE ].program );
    }

    sesmsActor_->SetMapper( mapper );
    sesmsActor_->GetProperty()->BackfaceCullingOn();

    sesmsActor_->GetProperty()->SetColor( 0.8, 0.8, 0.8 );
    MakeShinyMaterialType( sesmsActor_->GetProperty() );

    assembly_->AddPart( sesmsActor_ );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::HasSESMS() const
{
    return sesmsActor_ != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RemoveSESMS()
{
    if( !HasSESMS() ) return;
    if( GLSLShadersSupported() )
    {
        vtkGLSLShaderActor* a = dynamic_cast< vtkGLSLShaderActor* >( sesmsActor_.GetPointer() );
        typedef FindVtkPointer< vtkGLSLShaderActor > Finder;
        typedef ShaderProgram::Actors SA;
        SA::iterator b = shaderSurfaceMap_[ SESMS_SURFACE ].actors.begin();
        SA::iterator e = shaderSurfaceMap_[ SESMS_SURFACE ].actors.end();
        SA::iterator i = find_if( b, e, Finder( a ) );
        if( i != e )
        {
            shaderSurfaceMap_[ SESMS_SURFACE ].actors.erase( i );
        }
    }

    assembly_->RemovePart( sesmsActor_ );
    sesmsActor_ = 0;
}

//--------------------------------------------------------------------------------
bool MolekelMolecule::GetSESMSVisibility() const
{
    if( !HasSESMS() ) return false;
    return sesmsActor_->GetVisibility() != 0;
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSESMSVisibility( bool on )
{
    if( !HasSESMS() ) return;
    sesmsActor_->SetVisibility( on );
    RecomputeBBox();
}

//--------------------------------------------------------------------------------
void MolekelMolecule::SetSESMSRenderingStyle( RenderingStyle rs )
{
    if( !HasSESMS() ) return;
    vtkProperty* p = sesmsActor_->GetProperty();
    switch( rs )
    {
        case POINTS: p->SetRepresentationToPoints();
                     p->SetOpacity( 1.0f );
                     break;
        case WIREFRAME: p->SetRepresentationToWireframe();
                        p->SetOpacity( 1.0f );
                        break;
        case SOLID: p->SetRepresentationToSurface();
                    p->SetOpacity( 1.0f );
                    break;
        case TRANSPARENT_SOLID: p->SetRepresentationToSurface();
                                p->SetOpacity( 0.7f );
                                break;
        default: break;
    }

}

//--------------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetSESMSLUT() const
{
     if( !HasSESMS() ) return 0;
     return dynamic_cast< vtkLookupTable* >( sesmsActor_->GetMapper()->GetLookupTable() );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSESMS( double bboxSize[ 3 ],
                                     int steps[ 3 ],
                                     const vtkLookupTable* lut,
                                     bool useGridData )
{
    if( !HasSESMS() ) return;
    if( !CanComputeMEP() ) return;
    MapMEPOnSurface( sesmsActor_, bboxSize, steps, lut, useGridData );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::MapMEPOnSESMS( double step, const vtkLookupTable* lut, bool useGridData )
{
    assert( step > .0 && "STEP < 0" );
    const double* bounds = assembly_->GetBounds();
    double bbs[ 6 ];
    bbs[ 0 ] = bounds[ 1 ] - bounds[ 0 ];
    bbs[ 1 ] = bounds[ 3 ] - bounds[ 2 ];
    bbs[ 2 ] = bounds[ 5 ] - bounds[ 4 ];
    int steps[ 3 ];
    steps[ 0 ] = int( bbs[ 0 ] / step + .5 );
    steps[ 1 ] = int( bbs[ 1 ] / step + .5 );
    steps[ 2 ] = int( bbs[ 2 ] / step + .5 );
    MapMEPOnSESMS( bbs, steps, lut, useGridData );
}

//--------------------------------------------------------------------------------
void MolekelMolecule::RecomputeSurfaces( double bboxFactor,
                                         double step,
                                         const string& msmsExecutable,
                                         const string& msmsInFileName,
                                         const string& msmsOutFileName,
                                         double probeRadius,
                                         double density,
                                         bool mapMepOnOrbitals,
                                         bool mapMepOnElDens,
                                         bool mapMepOnSES,
                                         bool bothSigns,
                                         bool nodalSurface )
{
    // all the iso surfaces are recomputed with default values, full control over
    // animation of surfaces can only be achieved through scripting

    // recomputes el dens, orbitals, SAS and connolly surface using up to date atom positions

    // recompute connolly surface if visible --> bounding box will be reset to new values
    if( GetSESMSVisibility() )
    {
        const RenderingStyle rs = GetSESMSRenderingStyle();
        AddSESMS( probeRadius, density, msmsExecutable, msmsInFileName, msmsOutFileName, 0, 0 );
        SetSESMSRenderingStyle( rs );
        if( mapMepOnSES ) MapMEPOnSESMS( step, 0 );
    }
    // increase bbox size by bboxFactor and compute steps
    assert( step > .0 && "STEP < 0" );
    const double* bounds = assembly_->GetBounds();
    double bbs[ 3 ];
    bbs[ 0 ] = bboxFactor * ( bounds[ 1 ] - bounds[ 0 ] );
    bbs[ 1 ] = bboxFactor * ( bounds[ 3 ] - bounds[ 2 ] );
    bbs[ 2 ] = bboxFactor * ( bounds[ 5 ] - bounds[ 4 ] );
    int steps[ 3 ];
    steps[ 0 ] = int( bbs[ 0 ] / step + .5 );
    steps[ 1 ] = int( bbs[ 1 ] / step + .5 );
    steps[ 2 ] = int( bbs[ 2 ] / step + .5 );

    // remove all orbitals keeping track of the rendering style.
    // iterate through orbitals and recompute all the already available and visible orbitals
    typedef std::map< int, RenderingStyle > RSMap;
    RSMap rsmap;
    OrbitalActorMap::iterator oi = orbitalActorMap_.begin();
    const OrbitalActorMap::iterator oe = orbitalActorMap_.end();

    // record rendering styles
    for( oi; oi != oe; ++oi )
    {
        if( GetOrbitalSurfaceVisibility( oi->first ) )
        {
           rsmap[ oi->first ] = GetOrbitalRenderingStyle( oi->first );
        }
    }

    // remove and add orbital surfaces (computed from updated coordinates)
    RSMap::const_iterator rsi = rsmap.begin();
    const RSMap::const_iterator rse = rsmap.end();
    for( ; rsi != rse; ++rsi )
    {
        RemoveOrbitalSurface( rsi->first );
        AddOrbitalSurface( rsi->first, bbs, steps, 0.05, bothSigns, nodalSurface );
    }

    // recompute density matrix surface if visible
    if( GetElDensSurfaceVisibility() )
    {
        const RenderingStyle rs = GetElDensSurfaceRenderingStyle();
        AddElectronDensitySurface( bbs, steps, 0.05 );
        SetElDensSurfaceRenderingStyle( rs );
        if( mapMepOnElDens ) MapMEPOnElDensSurface( bbs, steps );
    }
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetOrbitalRenderingStyle( int orbital ) /*const*/
{
    if( !HasOrbitalSurface( orbital ) ) return INVALID_RENDERING_STYLE;
    vtkSmartPointer< vtkAssembly >& oa = orbitalActorMap_[ orbital ];
    vtkProp3DCollection* c = oa->GetParts();
    assert( c && "Empty orbital assembly" );
    c->InitTraversal();
    vtkActor* a = dynamic_cast< vtkActor* >( c->GetNextItemAsObject() );
    return ( GetActorRenderingStyle( a ) == TRANSPARENT_SOLID ? SOLID : GetActorRenderingStyle( a ) );
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetSESMSRenderingStyle() const
{
    if( !HasSESMS() ) return INVALID_RENDERING_STYLE;
    return GetActorRenderingStyle( sesmsActor_ );
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetElDensSurfaceRenderingStyle() const
{
    if( !HasElectronDensitySurface() ) return INVALID_RENDERING_STYLE;
    return ( GetActorRenderingStyle( elDensSurfaceActor_ ) == TRANSPARENT_SOLID ? SOLID :
             GetActorRenderingStyle( elDensSurfaceActor_ ) );
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetGridDataSurfaceRenderingStyle( const std::string& label ) const
{
    if( !HasGridDataSurface( label ) ) return INVALID_RENDERING_STYLE;
    if( format_ != "t41" ) return GetActorRenderingStyle( gridDataActor_ );
    else
    {
        if( gridActorMap_.find( label ) == gridActorMap_.end() ) return INVALID_RENDERING_STYLE;
        return GetActorRenderingStyle( gridActorMap_.find( label )->second );
    }
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetSASRenderingStyle() const
{
    if( !HasSAS() ) return INVALID_RENDERING_STYLE;
    return GetActorRenderingStyle( sasActor_ );
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetSESRenderingStyle() const
{
    if( !HasSES() ) return INVALID_RENDERING_STYLE;
    return POINTS; //only supported style
}

//-----------------------------------------------------------------------------
MolekelMolecule::RenderingStyle MolekelMolecule::GetActorRenderingStyle( vtkActor* actor  ) const
{
    if( !actor ) return INVALID_RENDERING_STYLE;
    switch( actor->GetProperty()->GetRepresentation() )
    {
        case VTK_POINTS: return POINTS;
                         break;
        case VTK_WIREFRAME: return WIREFRAME;
                            break;
        case VTK_SURFACE:
            if( actor->GetProperty()->GetOpacity() < 1.0f ) return TRANSPARENT_SOLID;
            return SOLID;
            break;
        default: break;
    }

    return INVALID_RENDERING_STYLE; // we should never get here
}

//-----------------------------------------------------------------------------
int MolekelMolecule::GetNumberOfFrames() const
{
    return frames_.size();
}

//-----------------------------------------------------------------------------
OBMol* MolekelMolecule::GetOBMolAtFrame( int frame ) const
{
    assert( frame >= 0 && "frame < 0" );
    assert( frame < frames_.size() && "frame > <number of frames> - 1" );
    return frames_[ frame ];
}

//-----------------------------------------------------------------------------
void MolekelMolecule::CopyOBMolToChemData( OBMol* obMol )
{
    extern void OpenBabelToChemData( OBMol* mol, ChemData* chemdata );
    OpenBabelToChemData( obMol, chemData_ );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetColor( float r, float g, float b )
{
    material_->diffuseColor.setValue( SbColor( r, g, b ) );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::GetColor( float& r, float& g, float& b ) const
{
    const SbColor c =  material_->diffuseColor[ 0 ];
    r = c[ 0 ];
    g = c[ 1 ];
    b = c[ 2 ];
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetAtomLabelVisibility( bool on )
{
    chemDisplayParam_->showAtomLabels.setValue( on );
}

//-----------------------------------------------------------------------------
bool MolekelMolecule::GetAtomLabelVisibility() const
{
    return chemDisplayParam_->showAtomLabels.getValue() != 0;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetBondLabelVisibility( bool on )
{
    chemDisplayParam_->showBondLabels.setValue( on );
}

//-----------------------------------------------------------------------------
bool MolekelMolecule::GetBondLabelVisibility() const
{
    return chemDisplayParam_->showBondLabels.getValue() != 0;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetResidueLabelVisibility( bool on )
{
    chemDisplayParam_->showResidueLabels.setValue( on );
}

//-----------------------------------------------------------------------------
bool MolekelMolecule::GetResidueLabelVisibility() const
{
    return chemDisplayParam_->showResidueLabels.getValue() != 0;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetOrbitalSurfacesVisibility( bool on )
{
    for( int i = 0; i != GetNumberOfOrbitals(); ++i )
    {
        SetOrbitalSurfaceVisibility( i, on );
    }
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetOrbitalMaterialType( int orbitalIndex, MaterialType m )
{
    if( orbitalActorMap_.find( orbitalIndex ) == orbitalActorMap_.end() )
    {
        return;
    }

    vtkSmartPointer< vtkPropCollection > pc( vtkPropCollection::New() );
    orbitalActorMap_[ orbitalIndex ]->GetActors( pc );
    pc->InitTraversal();
    vtkActor* a = 0;
    while( a = dynamic_cast< vtkActor* >( pc->GetNextProp() ) )
    {
        vtkProperty* p = a->GetProperty();
        SetMaterialType( p, m );
    }
}

//------------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetOrbitalMaterialType( int orbitalIndex )
{
    if( !HasOrbitalSurface( orbitalIndex ) ) return INVALID_MATERIAL;
    vtkSmartPointer< vtkAssembly >& oa = orbitalActorMap_[ orbitalIndex ];
    vtkProp3DCollection* c = oa->GetParts();
    assert( c && "Empty orbital assembly" );
    c->InitTraversal();
    vtkActor* a = dynamic_cast< vtkActor* >( c->GetNextItemAsObject() );
    return GetMaterialType( a->GetProperty() );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetElDensSurfaceMaterialType( MaterialType m )
{
   if( elDensSurfaceActor_ == 0 ) return;
   SetMaterialType( elDensSurfaceActor_->GetProperty(), m );
}

//-----------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetElDensSurfaceMaterialType() const
{
   if( elDensSurfaceActor_ == 0 ) return INVALID_MATERIAL;
   return GetMaterialType( elDensSurfaceActor_->GetProperty() );
}


//-----------------------------------------------------------------------------
void MolekelMolecule::SetGridDataSurfaceMaterialType( MaterialType m, const std::string& label )
{
    if( format_ != "t41" )
    {
        if( gridDataActor_ == 0 ) return;
        SetMaterialType( gridDataActor_->GetProperty(), m );
    }
}

//-----------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetGridDataSurfaceMaterialType() const
{
    if( gridDataActor_ == 0 ) return INVALID_MATERIAL;
    return GetMaterialType( gridDataActor_->GetProperty() );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetSASMaterialType( MaterialType m )
{
    if( sasActor_ == 0 ) return;
    SetMaterialType( sasActor_->GetProperty(), m );
}

//-----------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetSASMaterialType() const
{
    if( sasActor_ == 0 ) return INVALID_MATERIAL;
    return GetMaterialType( sasActor_->GetProperty() );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetSESMaterialType( MaterialType m )
{
    // NOT IMPLEMENTED
}

//-----------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetSESMaterialType() const
{
    // NOT IMPLEMENTED
    return INVALID_MATERIAL;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetSESMSMaterialType( MaterialType m )
{
    if( sesmsActor_ == 0 ) return;
    SetMaterialType( sesmsActor_->GetProperty(), m );
}


//-----------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetSESMSMaterialType() const
{
    if( sesmsActor_ == 0 ) return INVALID_MATERIAL;
    return GetMaterialType( sesmsActor_->GetProperty() );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetMaterialType( vtkProperty* p, MaterialType m )
{
    switch( m )
    {
        case OPAQUE_MATERIAL: MakeOpaqueMaterialType( p );
                     break;
        case SHINY_MATERIAL:  MakeShinyMaterialType( p );
                     break;
        default:     break;
    }
}

//-----------------------------------------------------------------------------
MolekelMolecule::MaterialType MolekelMolecule::GetMaterialType( vtkProperty* p ) const
{
    if( p->GetSpecularColor()[ 0 ] == 0. &&
        p->GetSpecularColor()[ 1 ] == 0. &&
        p->GetSpecularColor()[ 2 ] == 0. ) return OPAQUE_MATERIAL;

    return SHINY_MATERIAL;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetAtomColors( const std::string& fileName )
{
    ifstream is( fileName.c_str() );
    string lineBuffer;
    const int NUM_ELEMENTS = 104;
    const int R = 0;
    const int G = 1;
    const int B = 2;
    float colors[ NUM_ELEMENTS ][ 3 ];
    int cnt = 0;
    colors[ 0 ][ R ] = 1.0f;
    colors[ 0 ][ G ] = 1.0f;
    colors[ 0 ][ B ] = 1.0f;
    while( getline( is, lineBuffer ) && cnt != NUM_ELEMENTS )
    {
        ++cnt;
//        float r, g, b;
        istringstream in( lineBuffer );
        in >> colors[ cnt ][ R ] >> colors[ cnt ][ G ] >> colors[ cnt ][ B ];
        lineBuffer = "";
    }
    if( cnt != NUM_ELEMENTS ) throw MolekelException( "Cannot read atom colors from file" );
    chemColor_->atomColor.setValues( 0, NUM_ELEMENTS, colors );
    atomColorFile_ = fileName;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetOBMolToFrame( int frame )
{
    assert( frame >=0 && frame < frames_.size() );
    obMol_ = frames_[ frame ];
}

//-----------------------------------------------------------------------------
bool MolekelMolecule::HasGridDataSurface( const std::string& label ) const
{
    if( format_ != "t41" ) return gridDataActor_ != 0;
    else return gridActorMap_.find( label ) != gridActorMap_.end();
}


//-----------------------------------------------------------------------------
bool MolekelMolecule::HasGridDataSurface() const
{
    return gridDataActor_ != 0 || gridActorMap_.size();
}

//-----------------------------------------------------------------------------
MolekelMolecule::SurfaceLabels MolekelMolecule::GetGridLabels() const
{
    if( format_ != "t41" ) return SurfaceLabels();
    const OBT41Data* gd = dynamic_cast< const OBT41Data* >( obMol_->GetData( "T41Data" ) );
    assert( gd && "Invalid OBT41Data" );
    return gd->GetGridLabels();
}

//-----------------------------------------------------------------------------
MolekelMolecule::SurfaceLabels MolekelMolecule::GetGridSurfaceLabels() const
{
    SurfaceLabels sl;
    if( format_ != "t41" ) return sl;
    sl.reserve( gridActorMap_.size() );
    GridActorMap::const_iterator i = gridActorMap_.begin();
    const GridActorMap::const_iterator end = gridActorMap_.end();
    for( ; i != end; ++i ) sl.push_back( i->first );
    return sl;
}

//-----------------------------------------------------------------------------
vtkActor* MolekelMolecule::GetGridDataSurfaceActor( const string& label ) const
{
    if( !HasGridDataSurface( label ) ) return 0;
    if( format_ != "t41" ) return gridDataActor_;
    else return gridActorMap_.find( label )->second;
}

//-----------------------------------------------------------------------------
void MolekelMolecule::GetGridDataSurfaceColor( float& r, float& g, float& b, const string& label ) const
{
    if( !HasGridDataSurface( label ) ) return;
    double c[ 3 ];
    GetGridDataSurfaceActor( label )->GetProperty()->GetColor( c );
    r = float( c[ 0 ] );
    g = float( c[ 1 ] );
    b = float( c[ 2 ] );
}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetGridDataSurfaceColor( float r, float g, float b, const string& label )
{
    if( !HasGridDataSurface( label ) ) return;
    GetGridDataSurfaceActor( label )->GetProperty()->SetColor( r, g, b );
}

//-----------------------------------------------------------------------------
vtkLookupTable* MolekelMolecule::GetMEPLookupTable() const
{
    return mepLUT_;
}

//------------------------------------------------------------------------------
extern GLuint CreateGLSLProgramFromFiles( const char* vert, const char* frag,
                                          GLuint& vs, GLuint& fs );
extern void SetShaderParametersFromFile( GLuint program, const char* fileName );
void MolekelMolecule::SetShaderProgram( const ShaderProgram& sp, SurfaceType st )
{
    if( shaderSurfaceMap_.find( st ) == shaderSurfaceMap_.end() )
    {
        throw MolekelException( "Wrong surface type" );
    }
    ShaderProgram& shaderProgram = shaderSurfaceMap_[ st ] ;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;

    DeleteShaderProgram( st );
    const char* v =  sp.vertexShaderFileName.size() ? sp.vertexShaderFileName.c_str() : 0;
    const char* f =  sp.fragmentShaderFileName.size() ? sp.fragmentShaderFileName.c_str() : 0;
    GLuint program = CreateGLSLProgramFromFiles( v,
                                                 f,
                                                 vertexShader,
                                                 fragmentShader );

    if( !program  ) throw MolekelException( "Cannot create shader program" );
    if( sp.parametersFile.size() > 0 ) ::SetShaderParametersFromFile( program, sp.parametersFile.c_str() );

    shaderProgram.vertexShaderFileName   = sp.vertexShaderFileName;
    shaderProgram.fragmentShaderFileName = sp.fragmentShaderFileName;
    shaderProgram.parametersFile = sp.parametersFile;
    shaderProgram.vertexShader = vertexShader;
    shaderProgram.fragmentShader = fragmentShader;
    shaderProgram.program = program;

    typedef ShaderProgram::Actors ShaderActors;
    if( shaderProgram.actors.size() )
    {
        for( ShaderActors::iterator i = shaderProgram.actors.begin();
             i != shaderProgram.actors.end();
             ++i )
        {
            if( *i == 0 ) continue;
            ( *i )->SetShaderProgramId( program );
        }
    }
}

//------------------------------------------------------------------------------
void MolekelMolecule::SetShaderParametersFromFile( const char* fileName, SurfaceType st )
{
    if( shaderSurfaceMap_.find( st ) == shaderSurfaceMap_.end() )
    {
        throw MolekelException( "Wrong surface type" );
    }
    ::SetShaderParametersFromFile( GetShaderProgramId( st ), fileName );
}

//------------------------------------------------------------------------------
unsigned int MolekelMolecule::GetShaderProgramId( SurfaceType st ) const
{
    if( shaderSurfaceMap_.find( st ) == shaderSurfaceMap_.end() )
    {
        throw MolekelException( "Wrong surface type" );
    }
    return shaderSurfaceMap_.find( st )->second.program;
}

//------------------------------------------------------------------------------
const MolekelMolecule::ShaderProgram& MolekelMolecule::GetShaderProgram( SurfaceType st ) const
{

    if( shaderSurfaceMap_.find( st ) == shaderSurfaceMap_.end() )
    {
        throw MolekelException( "Wrong surface type" );
    }
    return shaderSurfaceMap_.find( st )->second;
}

//------------------------------------------------------------------------------
extern void DeleteGLSLProgramAndShaders( GLuint p, GLuint v, GLuint s );
void MolekelMolecule::DeleteShaderProgram( SurfaceType st )
{
    if( shaderSurfaceMap_.find( st ) == shaderSurfaceMap_.end() )
    {
        throw MolekelException( "Wrong surface type" );
    }
    ShaderProgram& sp = shaderSurfaceMap_[ st ];
    typedef ShaderProgram::Actors ShaderActors;
    for( ShaderActors::iterator i = sp.actors.begin();
         i != sp.actors.end();
         ++i )
    {
        if( *i == 0 ) continue;
        ( *i )->SetShaderProgramId( 0 );
    }
    DeleteGLSLProgramAndShaders( sp.program, sp.vertexShader, sp.fragmentShader );
    sp.vertexShaderFileName = "";
    sp.fragmentShaderFileName = "";
    sp.parametersFile = "";
    sp.vertexShader = 0;
    sp.fragmentShader = 0;
    sp.program = 0;
}

//------------------------------------------------------------------------------
double MolekelMolecule::GetOrbitalOpacity( int orbital, int type ) /*const*/
{
    if( !HasOrbitalSurface( orbital ) ) return 1.0;
    vtkSmartPointer< vtkAssembly >& oa = orbitalActorMap_[ orbital ];
    vtkProp3DCollection* c = oa->GetParts();
    assert( c && "Empty orbital assembly" );
    const int bitMask = orbitalActorMap_.find( orbital )->first.TypeMask();
    c->InitTraversal();
    double opacity = 1.0;
    vtkActor* a = 0;
    switch( type & bitMask )
    {
    case ORBITAL_MINUS:
       	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
       	opacity = a->GetProperty()->GetOpacity();
       	break;
    case ORBITAL_NODAL:
       	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
       	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
       	opacity = a->GetProperty()->GetOpacity();
       	break;
    case ORBITAL_PLUS:
       	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
       	if( bitMask & ORBITAL_NODAL ) c->GetNextProp();
       	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
       	opacity = a->GetProperty()->GetOpacity();
       	break;
    default:
       	break;
    }
    return opacity;
}

//------------------------------------------------------------------------------
void MolekelMolecule::SetOrbitalOpacity( int orbital, double opacity, int type )
{
    if( !HasOrbitalSurface( orbital ) ) return;
    vtkSmartPointer< vtkAssembly >& oa = orbitalActorMap_[ orbital ];
    vtkProp3DCollection* c = oa->GetParts();
    assert( c && "Empty orbital assembly" );
    const int bitMask = orbitalActorMap_.find( orbital )->first.TypeMask();
    c->InitTraversal();
    vtkActor* a = 0;
    switch( type & bitMask )
    {
    case ORBITAL_MINUS:
      	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
      	a->GetProperty()->SetOpacity( opacity );
      	break;
    case ORBITAL_NODAL:
      	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
      	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
      	a->GetProperty()->SetOpacity( opacity );
      	break;
    case ORBITAL_PLUS:
      	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
      	if( bitMask & ORBITAL_NODAL ) c->GetNextProp();
      	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
      	a->GetProperty()->SetOpacity( opacity );
      	break;
    default:
      	break;
    }

}

//-----------------------------------------------------------------------------
void MolekelMolecule::SetElDensSurfaceOpacity( double alpha )
{
   if( elDensSurfaceActor_ == 0 ) return;
   elDensSurfaceActor_->GetProperty()->SetOpacity( alpha );
}

//-----------------------------------------------------------------------------
double MolekelMolecule::GetElDensSurfaceOpacity() /*const*/
{
   if( elDensSurfaceActor_ == 0 ) return 1.0;
   return elDensSurfaceActor_->GetProperty()->GetOpacity();
}


//------------------------------------------------------------------------------
void MolekelMolecule::GetOrbitalColor( int orbital, int type, double* color ) /*const*/
{
    if( !HasOrbitalSurface( orbital ) ) return;
    vtkSmartPointer< vtkAssembly >& oa = orbitalActorMap_[ orbital ];
    vtkProp3DCollection* c = oa->GetParts();
    assert( c && "Empty orbital assembly" );
    const int bitMask = orbitalActorMap_.find( orbital )->first.TypeMask();
    c->InitTraversal();
    vtkActor* a = 0;

    switch( type & bitMask )
    {
    case ORBITAL_MINUS:
    	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
    	a->GetProperty()->GetColor( color );
    	break;
    case ORBITAL_NODAL:
    	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
    	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
    	a->GetProperty()->GetColor( color );
    	break;
    case ORBITAL_PLUS:
    	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
    	if( bitMask & ORBITAL_NODAL ) c->GetNextProp();
    	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
    	a->GetProperty()->GetColor( color );
    	break;
    default:
    	break;
    }
}

//------------------------------------------------------------------------------
void MolekelMolecule::SetOrbitalColor( int orbital, int type, const double* color )
{
    if( !HasOrbitalSurface( orbital ) ) return;
    vtkSmartPointer< vtkAssembly >& oa = orbitalActorMap_[ orbital ];
    vtkProp3DCollection* c = oa->GetParts();
    assert( c && "Empty orbital assembly" );
    const int bitMask = orbitalActorMap_.find( orbital )->first.TypeMask();
    c->InitTraversal();
    vtkActor* a = 0;

    switch( type & bitMask )
    {
    case ORBITAL_MINUS:
    	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
    	a->GetProperty()->SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
    	break;
    case ORBITAL_NODAL:
    	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
    	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
    	a->GetProperty()->SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
    	break;
    case ORBITAL_PLUS:
    	if( bitMask & ORBITAL_MINUS ) c->GetNextProp();
    	if( bitMask & ORBITAL_NODAL ) c->GetNextProp();
    	a = dynamic_cast< vtkActor* >( c->GetNextProp() );
    	a->GetProperty()->SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
    	break;
    default:
    	break;
    }

}

//------------------------------------------------------------------------------
void MolekelMolecule::GetElDensSurfaceColor( double* color ) /*const*/
{
	if( elDensSurfaceActor_ == 0 ) return;
	elDensSurfaceActor_->GetProperty()->GetColor( color );
}

//------------------------------------------------------------------------------
void MolekelMolecule::SetElDensSurfaceColor( const double* color )
{
	if( elDensSurfaceActor_ == 0 ) return;
	elDensSurfaceActor_->GetProperty()->SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
}
