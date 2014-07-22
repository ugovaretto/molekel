#ifndef MOLEKELMOLECULE_H_
#define MOLEKELMOLECULE_H_
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

// VTK
#include <vtkActor.h>
#include <vtkAssembly.h>
#include <vtkSmartPointer.h>
#include <vtkCubeSource.h>

// STD
#include <string>
#include <map>
#include <vector>

// Forward declaration
class ChemData;
class ChemAssociatedData;
class ChemDisplayParam;
class ChemDisplay;
class ChemRadii;
class ChemColor;
class ChemSelection;
class vtkCommand;
class vtkImageData;
class vtkArrowSource;
class vtkLookupTable;

namespace OpenBabel
{
    class OBMol;
}

struct Molecule;
class MoleculeVtkCommand;
struct MolecularOrbital;
class MolekelMolecule;
class ChemConnollyDot;
class SoSwitch;
class SoMaterial;

/// Progress call back function invoked by methods computing
/// grid data to report progress.
typedef void ( *ProgressCallback )( int currentStep, int totalSteps, void* cbData );

/// Updater interface: implementations of this interface
/// are used to update the molecule's structure for animation
/// purposes.
/// A typical implementation of this interface will:
/// - save the atoms positions when Init() is called
/// - update the atoms positions when Update() is called
/// - restore the original atoms positions when Reset() is called
/// Methods will usually be invoked from within MolekelMolecule's
/// methods.
/// e.g.
/// @code
/// void StartAnimation()
/// {
///
///   MolekelMolecule* mol = GetAnimatingMolecule();
///   mol->InitUpdate();
///   StartTimer();
/// }
///
/// void TimerTimeoutCallback()
/// {
///   ...
///   MolekelMolecule* mol = GetAnimatingMolecule();
///   mol->Update();
///   ...
/// }
///
/// void StopAnimation()
/// {
///   StopTimer();
///   MolekelMolecule* mol = GetAnimatingMolecule();
///	  mol->ResetUpdate();
///   ...
/// }
/// @endcode
class IMoleculeUpdater
{
public:
    virtual void SetMolecule( MolekelMolecule* ) = 0;
    virtual MolekelMolecule* GetMolecule() const  = 0;
    virtual void Init() = 0;
    virtual void Update( bool ) = 0;
    virtual void Reset() = 0;
    virtual void SetEnable( bool ) = 0;
    virtual bool GetEnable() const = 0;
    virtual ~IMoleculeUpdater() {}
};

///Callback invoked by MolekelMolecule::New methods
///to report status.
class ILoadMoleculeCallback
{
public:
    virtual void StatusMessage( const std::string& msg ) = 0;
    virtual ~ILoadMoleculeCallback() {}
};

/// Progress callback function invoked from within MolekelMolecule::GenerateMOGridData
/// and MolekelMolecule::AddOrbital methods.
/// @todo modify MolekelMolecule methods to support progress callback.
typedef void ( *MOGridDataProgressCallback )( int completedStep, void* cbackData );

class vtkGLSLShaderActor;

/// This class will hold the follwing members until the
/// code for reading Gaussian files and compute molecular
/// orbitals is replaced or added into OpenBabel:
/// - old Molekel Molecule class: used to store
///   Gaussian data in a format suitable for computation
///   of MO using old Molekel code; this data member will be
///   used only when reading .log file and computing MO
/// - vtkActor containing a vtkSoMapper which contains
///   Molecular Inventor data
/// - vtkActor containing molecule's bounding box
/// - vtkAssembly containing bounding box and molecule
/// - an OpenBabel molecule currently used for testing; will
///   be removed after OpenMOIV - OpenBabel is completed if
///   all the required data are accessible from OpenMOIV.
class MolekelMolecule
{

public:
	
	//@{ Orbital types
	static const int ORBITAL_MINUS = 0x00000001;
	static const int ORBITAL_NODAL = 0x00000010;
	static const int ORBITAL_PLUS  = 0x00000100;
	static const int ORBITAL_ALL   = ORBITAL_PLUS | ORBITAL_NODAL | ORBITAL_MINUS;
	//@}

    /// Simple struct holding shader program info.
    struct ShaderProgram
    {
        /// Vertex shader file name.
        std::string vertexShaderFileName;
        /// Fragment shader file name.
        std::string fragmentShaderFileName;
        /// Shader parameter file: used to for persistent storage of shader parameters.
        std::string parametersFile;
        typedef std::vector< vtkSmartPointer< vtkGLSLShaderActor > > Actors;
        ///
        Actors actors;
        /// GL vertex shader id.
        unsigned int vertexShader;
        /// GL fragment shader id.
        unsigned int fragmentShader;
        /// GL shader program id.
        unsigned int program;
        /// Default constructor.
        ShaderProgram() : vertexShader( 0 ), fragmentShader( 0 ), program( 0 ) {}
        /// Construct instance from file names.
        ShaderProgram( const std::string& v,
                       const std::string& f,
                       const std::string& p ) :
                       vertexShaderFileName( v ),
                       fragmentShaderFileName( f ),
                       parametersFile( p ),
                       vertexShader( 0 ),
                       fragmentShader( 0 ),
                       program( 0 ) {}
        /// Construct instance from file names and pre-allocated program ids.
        ShaderProgram( const std::string& v,
                       const std::string& f,
                       const std::string& p,
                       unsigned int vs,
                       unsigned int fs,
                       unsigned int sp ) :
                       vertexShaderFileName( v ),
                       fragmentShaderFileName( f ),
                       parametersFile( p ),
                       vertexShader( vs ),
                       fragmentShader( fs ),
                       program( sp ) {}

    };

    /// Materials.
    ///@warning OPAQUE is #defined somewhere else.
    typedef enum { OPAQUE_MATERIAL, SHINY_MATERIAL, INVALID_MATERIAL } MaterialType;

    /// Rendering style for surfaces.
    typedef enum { POINTS, WIREFRAME, SOLID, TRANSPARENT_SOLID,
                   INVALID_RENDERING_STYLE } RenderingStyle;

    /// Surface types, used in GLSL shaders.
    typedef enum { MOLECULE_SURFACE,
                   SAS_SURFACE,
                   SES_SURFACE,
                   SESMS_SURFACE,
                   GRID_DATA_SURFACE,
                   DENSITY_MATRIX_SURFACE,
                   ORBITAL_NEGATIVE_SURFACE,
                   ORBITAL_NODAL_SURFACE,
                   ORBITAL_POSITIVE_SURFACE,
                   INVALID_SURFACE } SurfaceType;

    /// Type to represent VTK events.
    typedef unsigned long VtkEventIdType;
    /// Type to represent
    typedef unsigned long VtkObserverIdType;
    /// Pick mode.
    typedef enum
    {
        PICK_MOLECULE,
        PICK_ATOM_BOND_RESIDUE
    } PickMode;

    /// String list with grid data surface names.
    typedef std::vector< std::string > SurfaceLabels;

    /// Factory method: constructs a molecule instance, reading
    /// from a file; file format is inferred from filename extension.
    /// @param fname file name.
    /// @return MolekelMolecule instance created with operator new.
    /// @throw MolekelException in case a problem occurs.
    /// This functions works as follows:
    /// - in case the file format is pdb or mol the molecule is
    ///   read with OpenMOIV and OpenBabel separately.
    /// - in case the file format is not pdb, mol or log the molecule
    ///   is read with OpenBabel, converted to mol format and read
    ///   with OpenMOIV.
    /// - in case the format is log the molecule is read with the old
    ///   Molekel's Gaussian reader, then with OpenBabel and finally
    ///   converted to mol and read with OpenMOIV.
    /// Gaussian data required for display of molecular orbitals and
    /// dynamics are not available in OpenBabel 2.0 but are in the
    /// OpenBabel roadmap and OpenMOIV is being integrated with OpenBabel
    /// (as of August 2006) in the future only one data type should be
    /// required to store all the needed information.
    static MolekelMolecule* New( const char* fname,
    							 ILoadMoleculeCallback* cb,
    							 bool computeBonds );
    /// Factory method accepting a format parameter to explicitly specify the
    /// format.
    static MolekelMolecule* New( const char* fname,
                                 const char* format,
                                 ILoadMoleculeCallback* cb,
                                 bool computeBonds );
    /// Destructor
    ~MolekelMolecule();
    /// Returns true if molecule has trajectory data, false otherwise.
    bool HasTrajectoryData() const;
    /// Returns true if molecule has vibration data, false otherwise.
    bool HasVibrationData() const;
    /// Save molecule to file using OpenBabel; format is inferred
    /// from filename extension.
    void Save( const char* fname ) const;
    /// Save molecule to file using OpenBabel.
    void Save( const char* fname, const char* fmt ) const;
    /// Returns reference to actor containing OpenMOIV scenegraph.
    vtkActor* GetActor() { return actor_; }
    /// Returns reference to molecule bounding box.
    vtkActor* GetBBoxActor() { return bbox_; }
    /// Returns reference to bounding box used for isosurface computation.
    vtkActor* GetIsoBBoxActor() { return isoBBox_; }
    /// Returns root of VTK scenegraph.
    vtkAssembly* GetAssembly() { return assembly_; }
    /// Returns reference to OpenBabel data.
    OpenBabel::OBMol* GetOpenBabelMolecule() { return obMol_; }
    /// Returns reference to old Molekel molecule.
    Molecule* GetMolekelMolecule() { return molekelMol_; }
    /// Returns reference to OpenMOIV ChemData node.
    ChemData* GetChemData() { return chemData_; }
    /// Returns reference to OpenMOIV display parameters.
    ChemDisplayParam* GetChemDisplayParam() { return chemDisplayParam_; }
    /// Returns filename.
    const std::string& GetFileName() const { return fname_; }
    /// Returns format.
    const std::string& GetFormat() const { return format_; }
    /// Returns reference to OpenMOIV ChemSelection node.
    ChemSelection* GetChemSelection() { return chemSelection_; }
    /// Adds vtkCommand associated to a specific VTK event type to actor.
    VtkObserverIdType AddVtkObserver( VtkEventIdType e, MoleculeVtkCommand* c );
    //void RemoveVtkObserver( VtkObserverIdType oid );
    /// Sets pick mode to either molecule or atoms/bonds/residues.
    void SetPickMode( PickMode p );
    /// Set molecule visibility.
    void SetVisible( bool );
    /// Set bounding box visibility.
    void SetBBoxVisible( bool );
    /// Set visibility of box used for isosurface computation.
    void SetIsoBBoxVisible( bool );
    /// Returns molecule visibility.
    bool GetVisible() const;
    /// Returns bounding box visibility.
    bool GetBBoxVisible() const;
    /// Recomputes the bounding box, useful when changing representation or
    /// adding orbitals.
    void RecomputeBBox();
    /// Sets the updater: each updater is OWNED by a specific
    /// MolekelMolecule instance which will delete the updater in its
    /// destructor.	In case an updater instance has previously been
    /// assigned, that instance is deleted and replaced with a new one.
    void SetUpdater( IMoleculeUpdater* u )
    {
        if( updater_ ) delete updater_;
        if( u ) u->SetMolecule( this );
        updater_ = u;
    }

    /// Returns updater.
    IMoleculeUpdater* GetUpdater() const { return updater_; }

    /// Initialize updater.
    void InitUpdate() { if( updater_ ) updater_->Init(); }
    /// Call updater's Update() methods.
    void Update( bool on ) { if( updater_ && updater_->GetEnable() ) updater_->Update( on ); }
    /// Resets updater.
    void ResetUpdate() { if( updater_ ) updater_->Reset(); }

    /// Returns bounding box size along axes.
    void GetBoundingBoxSize( double& dx, double& dy, double& dz ) const;

    /// Returns the bounding box center.
    void GetBoundingBoxCenter( double& x, double& y, double& z ) const;

    /// Returns bounding box size along axes of bounding box used for isosurface computation.
    void GetIsoBoundingBoxSize( double& dx, double& dy, double& dz ) const;

    /// Returns the bounding box center of bounding box used for isosurface computation.
    void GetIsoBoundingBoxCenter( double& x, double& y, double& z ) const;

    /// Set size of box used for isosurface computation.
    void SetIsoBoundingBoxSize( double dx, double dy, double dz );

    /// Set center of box used for isosurface computation.
    void SetIsoBoundingBoxCenter( double x, double y, double z );

    /// Resets vtkActor transform.
    void ResetTransform();

    //@{ Molecular orbital handling methods; the methods not containing
    ///  the orbital type (alpha, beta) in their name operate as if
    ///  the orbitals were stored in one single array of
    ///  size == # alpha orbitals + # beta orbitals  laid out as
    ///  [ alpha orbital 0, alpha orbital 1,..., alpha orbital Na,
    ///    beta orbital 0, beta orbital 1, ..., beta orbital Nb ].
    /// @todo add support for changing bounding box and sampling interval.
    /// Returns number of orbitals alpha + beta.
    int GetNumberOfOrbitals() const;
    /// Returns true if beta orbitals present.
    bool HasBetaOrbitals() const;
    /// Returns eigenvalue of specific orbital.
    double GetOrbitalEigenValue( int orbitalIndex ) const;
    /// Returns occupation of specific orbital.
    double GetOrbitalOccupation( int orbitalIndex ) const;
    /// Returns orbital type.
    const char* GetOrbitalType( int orbitalIndex ) const;
    /// Computes 3D grid for specific orbital; each grid node
    /// is an electron density value.
    vtkImageData* GenerateMOGridData( int orbitalIndex,
                                      double bboxSize[ 3 ],
                                      int steps[ 3 ],
                                      ProgressCallback cb = 0,
                                      void* cbData = 0 ) const;
    /// Computes 3D grid for specific orbital; each grid node
    /// is an electron density value.
    vtkImageData* GenerateMOGridData( int orbitalIndex,
                                      double step,
                                      ProgressCallback cb = 0,
                                      void* cbData = 0 ) const;
    /// Stops orbital grid data generation, must be called from a thread
    /// different from the one that calls GenerateMOGridData().
    /// @note current MolekelMolecule operations are all synchronous
    /// mainly due to a lack of portable threading libraries in the used
    /// frameworks. This method currently simply calls the StopProcessCalc()
    /// function added into calcdens.cpp.
    void StopMOGridDataGeneration();
    /// Returns true if last grid data generation was interrupted.
    bool MOGridDataGenerationStopped();
    /// Adds orbital surface into VTK renderer.
    /// orbitalIndex must fall inside the interval [0, # alpha + # beta orbitals - 1];
    /// - if orbitalIndex is in the interval [0, # alpha orbitals - 1] it is assumed to be
    ///   associated with the alpha orbital of index orbitalIndex;
    /// - if orbitalIndex is in the interval [# alpha orbitals, # alpha + # beta orbitals - 1]
    ///   it is assumed to be associated with the beta orbital of index orbitalIndex.
    bool AddOrbitalSurface( int orbitalIndex,
                            double bboxSize[ 3 ],
                            int steps[ 3 ],
                            double value = 0.05,
                            bool bothSigns = true,
                            bool nodalSurface = false,
                            ProgressCallback cb = 0,
                            void* cbData = 0 );
    /// Adds surface computed from density matrix into VTK renderer.
    ///void AddDensityMatrixSurface( int orbitalIndex, double value = 0.05 );
    /// Removes orbital surface.
    /// orbitalIndex must fall inside the interval [0, # alpha + # beta orbitals - 1];
    /// - if orbitalIndex is in the interval [0, # alpha orbitals - 1] it is assumed to be
    ///   associated with the alpha orbital of index orbitalIndex;
    /// - if orbitalIndex is in the interval [# alpha orbitals, # alpha + # beta orbitals - 1]
    ///   it is assumed to be associated with the beta orbital of index orbitalIndex.
    void RemoveOrbitalSurface( int orbitalIndex );
    /// Returns true if orbital already generated.
    bool HasOrbitalSurface( int orbitalIndex ) const;
    /// Returns the number of generated orbital surfaces.
    int GetNumberOfOrbitalSurfaces() const;
    /// Returns orbital surface visibility.
    /// @warning vtkProp::GetVisibility() NOT const.
    bool GetOrbitalSurfaceVisibility( int orbital ) /*const*/;
    /// Sets orbital surface visibility.
    void SetOrbitalSurfaceVisibility( int orbital, bool on );
    /// Returns true if index maps to alpha orbital index; false otherwise.
    bool IsAlphaOrbital( int index ) const;
    /// Returns true if index maps to beta orbital index; false otherwise.
    bool IsBetaOrbital( int index ) const;
    /// Sets the orbital's rendering style.
    void SetOrbitalRenderingStyle( int orbitalIndex, RenderingStyle rs );
    /// Sets opacity for specific orbital.
    /// @param orbital zero-based orbital index.
    /// @param opacity alpha value between 0 and 1.
    /// @param typeMask bit mask containing the affected orbital types
    void SetOrbitalOpacity( int orbital, double alpha, int typeMask = ORBITAL_ALL );
    /// Returns opacity (alpha) for specific orbital.
    /// @param orbital orbital index
    /// @param typeMask orbital type (-/0/+)
    double GetOrbitalOpacity( int orbital, int typeMask = ORBITAL_ALL );
    /// Sets opacity for density matrix surface.
    /// @param opacity alpha value between 0 and 1.
    void SetElDensSurfaceOpacity( double alpha );
    /// Returns opacity (alpha) for density matrix surface.
    double GetElDensSurfaceOpacity();
    /// Sets the orbital's material.
    void SetOrbitalMaterialType( int orbitalIndex, MaterialType m );
    /// Returns lookup table associated with orbital surface.
    /// @note It returns the lookup table associated with the first
    /// actor in the orbital surface vtkAssembly.
    vtkLookupTable* GetOrbitalSurfaceLUT( int orbitalIndex ) const;
    /// Sets visibility flag for all the generated orbital surfaces.
    void SetOrbitalSurfacesVisibility( bool on );
    //@}

    // @{ Methods for handling surfaces generated from OBGridData; @see OBGridData.

    /// Generates iso-surface from OBGridData if data is available inside OBMol instance.
    /// It is possible to perform a smoothing of the generated surface by specifying
    /// a value greater than zero for the smoothingIteration parameter.
    /// Google for "Laplacian Smoothing" for additional information on the smoothing
    /// method or have a look at VTK's vtkSmoothPolyDataFilter API reference page.
    /// Surface is generated and added to the molecule scenegraph as a vtkActor.
    /// @param value isosurface value.
    /// @param stepMultiplier step multiplier: step is multiplied by this value to
    ///        decrease the amount of generated data; stepMultiplier must fall in the
    ///        range [ 1, min{ x steps, y steps, z steps } ].
    /// @param smoothingIterations number of performed smoothing iterations
    /// @param relaxationFactor relaxation factor used in each smoothing iteration
    bool GenerateGridDataSurface( const std::string& label,
                                  double value,
                                  int stepMultiplier = 1,
                                  int smoothingIterations = 0,
                                  double relaxationFactor = 0.01,
                                  ProgressCallback cb = 0,
                                  void* cbData = 0  );
    /// Generates a vtkImageData object from OBGridData.
    vtkImageData* GridDataToVtkImageData( const std::string& label,
                                          int stepMultiplier = 1,
                                          ProgressCallback cb = 0,
                                          void* cbData = 0 ) const;
    /// Removes iso-surface generated from OBGridData.
    void RemoveGridDataSurface( const std::string& label = "" );
    /// Returns true if grid data surface was added to the molecule scene graph.
    bool HasGridDataSurface( const std::string& label ) const;
    /// Returns true if at least one data surface has been generated from grid data.
    bool HasGridDataSurface() const;
    /// Returns grid data actor's LUT.
    vtkLookupTable* GetGridDataSurfaceLUT( const std::string& label = "" ) const;
    /// Sets rendering style for Grid Data surface.
    void SetGridDataSurfaceRenderingStyle( RenderingStyle rs, const std::string& label = "" );
    /// Sets material for Grid Data surface.
    void SetGridDataSurfaceMaterialType( MaterialType m, const std::string& label = "" );
    /// Returns grid data surface visibility.
    /// @warning vtkProp::GetVisibility() NOT const.
    bool GetGridDataSurfaceVisibility( const std::string& label = "" ) /*const*/;
    /// Sets grid data surface visibility.
    void SetGridDataSurfaceVisibility( bool on, const std::string& label = "" );
    /// Returns true if Grid Data available; false otherwise.
    bool HasGridData() const;
    /// Returns max grid value or std::numeric_limits< double >::min()
    /// if no grid data available.
    double GetGridDataMax( const std::string& label = "" ) const;
    /// Returns min grid value or std::numeric_limits< double >::max()
    /// if no grid data is available.
    double GetGridDataMin( const std::string& label = "" ) const;
    /// Returns number of steps along x, y, z axes; values are set to -1
    /// if no data is available.
    void GetGridDataNumSteps( int steps[ 3 ] ) const;
    /// Returns step size along x, y and z axes; steps are set to 0
    /// if no data is available.
    void GetGridDataStepSize( double steps[ 3 ] ) const;
    /// Returns the names of the generated grid data surfaces.
    SurfaceLabels GetGridSurfaceLabels() const;
    /// Returns the names of the grid data sets.
    SurfaceLabels GetGridLabels() const;
    // @}

    // @{ Spin and Electron density
    /// Returns true if data for alpha density matrix are available, false otherwise.
    bool HasAlphaDensity() const;
    /// Returns true if data for beta density matrix are available, false otherwise.
    bool HasBetaDensity() const;
    /// Returns true if spin density can be computed, false otherwise.
    bool CanComputeSpinDensity() const { return HasAlphaDensity() && HasBetaDensity(); }
    /// Returns true if electron density can be computed from density matrix, false otherwise.
    bool CanComputeElectronDensity() const 
	{ return HasAlphaDensity() || HasBetaDensity() || GetNumberOfOrbitals() > 0; }
    /// Generates vtkImageData from electron density. Returns NULL if electron density cannot
    /// be computed.
    /// @param minValue method returns min electron density value in this parameter
    /// @param maxValue method returns max electron density value in this parameter
    vtkImageData* GenerateElectronDensityData( const double& step,
                                               double& minValue,
                                               double& maxValue,
                                               ProgressCallback cb = 0,
                                               void* cbData = 0 ) const;
    /// Generates vtkImageData from spin density. Returns NULL if spin density cannot
    /// be computed.
    /// @param minValue method returns min electron density value in this parameter
    /// @param maxValue method returns max electron density value in this parameter
    vtkImageData* GenerateSpinDensityData( const double& step,
                                           double& minValue,
                                           double& maxValue,
                                           ProgressCallback cb = 0,
                                           void* cbData = 0 ) const;
    /// Stops electron density data generation.
    void StopElectronDensityDataGeneration() const;
    /// Returns true if last electron density data generation was interrupted.
    bool ElectronDensityDataGenerationStopped() const;
    /// Stops spin density data generation.
    void StopSpinDensityDataGeneration() const;
    /// Returns true if last spin density data generation was interrupted.
    bool SpinDensityDataGenerationStopped() const;
    /// Adds iso-surface generated from electron density data.
    bool AddElectronDensitySurface( double bboxSize[ 3 ], int steps[ 3 ], double value,
                                    ProgressCallback cb = 0, void* cbData = 0 );
    /// Adds iso-surface generated from spin density data.
    bool AddSpinDensitySurface( double bboxSize[ 3 ], int steps[ 3 ], double value,
                                ProgressCallback cb = 0, void* cbData = 0 );
    /// Removes electron density surface if present.
    void RemoveElectronDensitySurface();
    /// Removes spin density surface if present.
    void RemoveSpinDensitySurface();
    /// Returns true if electron density surface generated, false otherwise.
    bool HasElectronDensitySurface()const { return elDensSurfaceActor_ != 0; }
    /// Returns true if spin density surface generated, false otherwise.
    bool HasSpinDensitySurface() const { return spinDensSurfaceActor_ != 0; }
    /// Sets rendering style of electron density surface.
    void SetElDensSurfaceRenderingStyle( RenderingStyle );
    /// Sets material of electron density surface.
    void SetElDensSurfaceMaterialType( MaterialType m );
    /// Sets rendering style of spin density surface.
    void SetSpinDensSurfaceRenderingStyle( RenderingStyle );
    /// Returns lookup table associated with electron density surface.
    vtkLookupTable* GetElectronDensitySurfaceLUT() const;
    /// Returns spin density surface visibility.
    /// @warning vtkProp::GetVisibility() NOT const.
    bool GetSpinDensitySurfaceVisibility() /*const*/;
    /// Sets spin density surface visibility.
    void SetSpinDensitySurfaceVisibility( bool on );
    /// Returns electron density surface visibility.
    /// @warning vtkProp::GetVisibility() NOT const.
    bool GetElDensSurfaceVisibility() /*const*/;
    /// Sets electron density surface visibility.
    void SetElDensSurfaceVisibility( bool on );
    // @}

    /// Generates molecular electrostatic potential data.
    vtkImageData* GenerateMEPData( const double& step,
                                   double& minVal,
                                   double& maxValue,
                                   ProgressCallback cb = 0,
                                   void* cbData = 0 ) const;
    /// Stops generation of MEP data.
    void StopMEPDataGeneration() const;
    /// Returns true if last MEP data generation stopped.
    bool MEPDataGenerationStopped() const;
    /// Returns true if MEP can be computed, false otherwise.
    /// @todo use OpenBabel to retrieve atom charge.
    bool CanComputeMEP() const;

    // @{ Vibration vectors.
    /// Returns true if vibration vectors are being displayed, false otherwise.
    bool GetVibrationVectorsVisibility() const;
    /// Enables/disables display of vibration vectors.
    void SetVibrationVectorsVisibility( bool );
    /// Sets position and orientation of vibration vector for specific atom.
    /// @param atom atom index.
    /// @param p0 first point of vector p0 -> p1.
    /// @param p1 secod point of vector p0 -> p1.
    /// @param scale if true arrow is scaled by |p1 - p0| if false no scaling is
    /// performed.
    void SetVibrationVector( int atom, double p0[ 3 ], double p1[ 3 ], bool scale );
    // @}

    // @{ Dipole moment
    /// Sets dipole moment visibility.
    void SetDipoleMomentVisibility( bool on );
    /// Returns true if dipole moment is visible, false otherwise.
    bool GetDipoleMomentVisibility() const;
    /// Returns true if dipole moment present, false otherwise.
    bool HasDipoleMoment() const;
    /// Returns dipole moment's scaling factor.
    double GetDipoleMomentArrowLength() const;
    /// Sets dipole moment's scaling factor.
    void SetDipoleMomentArrowLength( double );
    // @}

    // @{ Color code surfaces with MEP
    /// Generates MEP grid data and assigns MEP values to vertices of electron density
    /// surface.
    void MapMEPOnElDensSurface( double bboxSize[ 3 ], int steps[ 3 ],
                                const vtkLookupTable* lut = 0 );
    /// Generates MEP grid data and assigns MEP values to vertices of orbital surface.
    void MapMEPOnOrbitalSurface( int orbitalIndex, double bboxSize[ 3 ], int steps[ 3 ],
                                 const vtkLookupTable* lut = 0 );
    // @}

	/// Add Bond.
	void AddBond( int atom1Id, int atom2Id );
	/// Remove Bond.
	void RemoveBond( int bondId );
    /// Computes dihedral angle.
    double ComputeDihedralAngle( int atom1Id, int atom2Id, int atom3Id, int atom4Id ) const;
    /// Computes angle between ( < atom1 position > - < atom2 position > )
    /// and ( < atom3 position > - < atom2 position > )
    double ComputeAngle( int atom1Id, int atom2Id, int atom3Id ) const;
     /// Computes distance between two atoms.
    double ComputeDistance( int atom1Id, int atom2Id ) const;
    /// Return atom's covalent radius.
    double GetCovalentRadius( int atomId ) const;
    /// Returns atom's Van der Waals radius.
    double GetVdWRadius( int atomId ) const;
    /// Returns atom's position
    void GetAtomPosition( int atomId, double pos[ 3 ] ) const;
    /// Returns signed distance of point from atom's surface.
    /// - <  0 --> point inside atom
    /// - == 0 --> point on atom's surface
    /// - >  0 --> point outside atom
    double GetDistanceFromAtom( int atomId, const double point[ 3 ] ) const;
    /// Returns minimum SIGNED distance from atoms.
    /// @todo add spatial partitioning to speed up computation;
    /// right now every time this method is called a distance between
    /// the point and each atom in the molecule is computed while it should
    /// be computed the distance between the point and the atoms in a
    /// specific bounded region (e.g. an octree node) only.
    /// The atom id corresponding to the closest atom is returned; note
    /// that there could be more than one atom at the same distance but
    /// only one is returned.
    /// @param point 3d point
    /// @param id returned id of atom
    /// @param dr value added to the Van der Waals radius of each atom;
    /// used to e.g. compute SAS
    double GetMinDistanceFromAtom( const double point[ 3 ],
                                   int& atomId,
                                   double dr = 0.  ) const;
    /// Add Solvent Accessible Surface.
    /// Web reference: http://www.netsci.org/Science/Compchem/feature14e.html
    void AddSAS( double solventRadius, double step, ProgressCallback cb = 0, void* cbData = 0 );
    /// Issues a request to stop computation of SAS.
    /// @see SASComputationStopped().
    void StopSASComputation() { stopSASComputation_ = true; }
    /// Returns true if SAS computation was stopped.
    /// @note Do not use this method before having checked that the thread
    /// is actually terminated, this method returns a value that represents
    /// a request made by a client to stop the computation and not the actual
    /// state of of the computation:
    /// - check if thread is finished
    /// - check value returned by SASComputationStopped()
    ///   - if thread is finished AND  SASComputationStopped() returned true
    ///     it means that the computation was actually stopped
    ///   - if thread is still running AND SASComputationStopped() returns
    ///     true it means that a client has requested for termination.
    bool SASComputationStopped() const { return stopSASComputation_; }
    /// Returns true if SAS in scenegraph; false otherwise.
    bool HasSAS() const;
    /// Remove SAS;
    void RemoveSAS();
    /// Return SAS visibility.
    bool GetSASVisibility() const;
    /// Sets SAS visibility.
    void SetSASVisibility( bool on );
    /// Set SAS rendering style.
    void SetSASRenderingStyle( RenderingStyle rs );
    /// Sets SAS material.
    void SetSASMaterialType( MaterialType m );
    /// Returns SAS' LUT.
    vtkLookupTable* GetSASLUT() const;
    /// Map MEP on SAS.
    void MapMEPOnSAS( double bboxSize[ 3 ], int steps[ 3 ],
                      const vtkLookupTable* lut, bool useGridData = false  );
    /// Map MEP on SAS using current bounding box.
    void MapMEPOnSAS( double step, const vtkLookupTable* lut, bool useGridData = true );
    /// Returns number of atoms in molecule.
    int GetNumberOfAtoms() const;

    // @{ SES (internal)
    /// Add Solvent Excluded Surface aka Connolly Surface.
    /// Web reference: http://www.netsci.org/Science/Compchem/feature14e.html
    void AddSES( double solventRadius, double step, ProgressCallback cb = 0, void* cbData = 0 );
    /// Issues a request to stop computation of SES.
    /// @see SESComputationStopped().
    void StopSESComputation() { stopSESComputation_ = true; }
    /// Returns true if SES computation was stopped.
    /// @note Do not use this method before having checked that the thread
    /// is actually terminated, this method returns a value that represents
    /// a request made by a client to stop the computation and not the actual
    /// state of of the computation:
    /// - check if thread is finished
    /// - check value returned by SESComputationStopped()
    ///   - if thread is finished AND  SESComputationStopped() returned true
    ///     it means that the computation was actually stopped
    ///   - if thread is still running AND SESComputationStopped() returns
    ///     true it means that a client has requested for termination.
    bool SESComputationStopped() const { return stopSESComputation_; }
    /// Returns true if SES in scenegraph; false otherwise.
    bool HasSES() const;
    /// Remove SES;
    void RemoveSES();
    /// Return SES visibility.
    bool GetSESVisibility() const;
    /// Sets SAS visibility.
    void SetSESVisibility( bool on );
    /// Set SES rendering style.
    void SetSESRenderingStyle( RenderingStyle rs );
    /// Sets SES material.
    void SetSESMaterialType( MaterialType m );
    /// Returns SES' LUT.
    vtkLookupTable* GetSESLUT() const;
    /// Map MEP on SES.
    void MapMEPOnSES( double bboxSize[ 3 ], int steps[ 3 ], const vtkLookupTable* lut = 0 );
    /// Map MEP on SES using current bounding box.
    void MapMEPOnSES( double step, const vtkLookupTable* lut = 0 );
    // @}

    // @{ SES (external, saves molecule data to M.F. Sanner's MSMS input format, runs
    ///   the msms program and reads the result back through a vtkMSMSReader object.

    /// Add Solvent Excluded Surface aka Connolly Surface using M.F. Sanner's MSMS.
    /// Web reference: http://www.netsci.org/Science/Compchem/feature14e.html
    /// Web reference: http://www.scripps.edu/~sanner/
    void AddSESMS( double solventRadius,
                   double density,
                   const std::string& msmsExecutable,
                   const std::string& msmsInFileName = "",
                   const std::string& msmsOutFileName = "",
                   ProgressCallback cb = 0,
                   void* cbData = 0 );
    /// Issues a request to stop computation of SES.
    /// @see SESComputationStopped().
    void StopSESMSComputation() { stopSESMSComputation_ = true; }
    /// Returns true if SES computation through MSMS was stopped.
    /// @note Do not use this method before having checked that the thread
    /// is actually terminated, this method returns a value that represents
    /// a request made by a client to stop the computation and not the actual
    /// state of of the computation:
    /// - check if thread is finished
    /// - check value returned by SESMSComputationStopped()
    ///   - if thread is finished AND  SESMSComputationStopped() returned true
    ///     it means that the computation was actually stopped
    ///   - if thread is still running AND SESMSComputationStopped() returns
    ///     true it means that a client has requested for termination.
    bool SESMSComputationStopped() const { return stopSESMSComputation_; }
    /// Returns true if SES in scenegraph; false otherwise.
    bool HasSESMS() const;
    /// Remove SES;
    void RemoveSESMS();
    /// Return SES visibility.
    bool GetSESMSVisibility() const;
    /// Sets SES visibility.
    void SetSESMSVisibility( bool on );
    /// Set SES rendering style.
    void SetSESMSRenderingStyle( RenderingStyle rs );
    /// Set SES material.
    void SetSESMSMaterialType( MaterialType m );
    /// Returns SAS' LUT.
    vtkLookupTable* GetSESMSLUT() const;
    /// Map MEP on SES.
    void MapMEPOnSESMS( double bboxSize[ 3 ], int steps[ 3 ],
                        const vtkLookupTable* lut, bool useGridData = false );
    /// Map MEP on SES using current bounding box.
    void MapMEPOnSESMS( double step, const vtkLookupTable* lut, bool useGridData = false );
    // @}

    // @{ Rendering style info, used in RecomputeSurface method.
    // @note cannot find a workaround for making vtkSmartPointer work with constant objects.
    RenderingStyle GetOrbitalRenderingStyle( int orbital ) /*const*/;
    RenderingStyle GetSESMSRenderingStyle() const;
    RenderingStyle GetSASRenderingStyle() const;
    RenderingStyle GetSESRenderingStyle() const;
    RenderingStyle GetGridDataSurfaceRenderingStyle( const std::string& label = "" ) const;
    RenderingStyle GetElDensSurfaceRenderingStyle() const;
    // @}

    // @{ MaterialType type info
    MaterialType GetOrbitalMaterialType( int orbital );
    MaterialType GetSESMSMaterialType() const;
    MaterialType GetSASMaterialType() const;
    MaterialType GetGridDataSurfaceMaterialType() const;
    MaterialType GetElDensSurfaceMaterialType() const;
    MaterialType GetSESMaterialType() const;
    // @}

    /// Recomputes visible orbitals, electron density surface and SES (with MSMS);
    /// a default value of 0.05 is used for isosurface computation.
    /// Temporary solution until QSA scripting becomes available.
    /// Since atom positions are updated at each animation step it is possible
    /// to export animations where the surfaces are animated calling this method
    /// after updating the atom positions.
    /// After the execution of this algorithm all the visible surfaces will be changed
    /// according to the values passed to this method; the changed will affect:
    /// - surface resolution
    /// - bounding box
    /// - MEP mapping
    /// Rendering styles and all the non-visible surfaces will not be affected.
    /// @param bboxFactor factor by which current bounding box size is expanded.
    /// @param step step used in isosorface generation.
    /// @param probeRadius probe radius passed to SES computation algorithm.
    /// @param density density passed to SES computation algorithm.
    /// @param mapMepOnOrbitals if true maps MEP on all the visible orbital surfaces
    /// @param mapMepOnElDens if true maps MEP on the surface generated from
    ///        density matrix information.
    /// @param mapMepOnSES if true maps MEP on SES surface.
    /// @param bothSigns if true computes orbitals using +/- isosurface values
    /// @param nodalSurface if true adds a nodal surface to each orbital surface
    void RecomputeSurfaces( double bboxFactor,
                            double step,
                            const std::string& msmsExecutable,
                            const std::string& msmsInFileName,
                            const std::string& msmsOutFileName,
                            double probeRadius,
                            double density,
                            bool mapMepOnOrbitals,
                            bool mapMepOnElDens,
                            bool mapMepOnSES,
                            bool bothSigns,
                            bool nodalSurface );

    //@{ Support for multi-molecule formats.
    int GetNumberOfFrames() const;
    OpenBabel::OBMol* GetOBMolAtFrame( int frame ) const;
    void CopyOBMolToChemData( OpenBabel::OBMol* obMol );
    //@}

    //@{ Set/Get molecule color: this is the color used when
    /// SoSphere and SoCylinder are used as display styles for atoms and bonds.
    void SetColor( float r, float g, float b );
    void GetColor( float& r, float& g, float& b ) const;
    //@}

    //@{ Set/Get atom label visibility.
    void SetAtomLabelVisibility( bool on );
    bool GetAtomLabelVisibility() const;
    //@}

    //@{ Set/Get bond label visibility.
    void SetBondLabelVisibility( bool on );
    bool GetBondLabelVisibility() const;
    //@}

    //@{ Set/Get residue label visibility.
    void SetResidueLabelVisibility( bool on );
    bool GetResidueLabelVisibility() const;
    //@}

    /// Read atom colors from file and sets the value of atomColorFile_ if operation
    /// successful.
    /// ASCII File Format:
    /// - no blank lines
    /// - one R G B color entry per line
    /// - one entry -> float float float (3 floats separated by blanks/tabs)
    /// - R,G,B values must be in the interval [0, 1];
    /// - 1-indexed line number == atomic number; first line is H, line two is He...
    void SetAtomColors( const std::string& fileName );

    /// Return file name of file from which atom colors were read.
    const std::string& GetAtomColorFileName() const { return atomColorFile_; }

    /// Makes the internal OBMol* pointer point to the OBMol at specific frame.
    void SetOBMolToFrame( int frame );

    /// Returns color of surface generated from grid data.
    void GetGridDataSurfaceColor( float& r, float& g, float& b, const std::string& label ) const;

    /// Sets color of surface generated from grid data.
    void SetGridDataSurfaceColor( float r, float g, float b, const std::string& label );

    /// Returns the last lookup table used for MEP color coding. Note that MEP lookup tables
    /// are set from client code when generating molecular surfaces or scalar fields.
    vtkLookupTable* GetMEPLookupTable() const;

    //@{ GLSL Shader handling
    /// Get shader program id.
    unsigned int GetShaderProgramId( SurfaceType surfaceType ) const;

    /// Set shader program id (shader must have been previously linked)
    /// for a specific surface type. If a given surface type is available
    /// the shader will be assigned to that surface immediately; if not
    /// it will be assigned to the surface at the first surface generation
    /// request.
    //void SetShaderProgramId( unsigned int program, SurfaceType surfaceType );

    /// Set shaders.
    void SetShaderProgram( const ShaderProgram&, SurfaceType st );

    /// Get shaders.
    const ShaderProgram& GetShaderProgram( SurfaceType st ) const;

    /// Set shader parameters.
    void SetShaderParametersFromFile( const char* fileName, SurfaceType st );

    /// Delete shader program.
    void DeleteShaderProgram( SurfaceType );

    /// Set vertex shader.
    //void SetVetexShader( const char* fileName, SurfaceType st = MOLECULE_SURFACE );

    /// Set vertex shader id.
    //void SetVetexShaderId( unsigned int, SurfaceType st = MOLECULE_SURFACE );

    /// Get vertex shader id.
    //unsigned int GetVetexShaderId( SurfaceType st = MOLECULE_SURFACE ) const;

    /// Set fragment shader.
    //void SetFragmentShader( const char* fileName, SurfaceType st = MOLECULE_SURFACE );

    /// Set fragment shader id.
    //void SetFragmentShaderId( unsigned int, SurfaceType st = MOLECULE_SURFACE );

    /// Get fragment shader id.
    //unsigned int GetFragmentShaderId( SurfaceType st = MOLECULE_SURFACE ) const;
    // @}
    /// Sets the color of one or more orbital surface types.
    void SetOrbitalColor( int orbital, int typeMask, const double* color );
    /// Returns the color of a specific orbital surface type.
    void GetOrbitalColor( int orbital, int typeMask, double* color );
    /// Sets the density matrix surface color.
    void SetElDensSurfaceColor( const double* color );
    /// Returns the density matrix surface color.
    void GetElDensSurfaceColor( double* color );

private:

    typedef std::vector< OpenBabel::OBMol* > Frames;

    /// Constructor.
    MolekelMolecule();

    /// Generates grid data.
    vtkImageData* GenerateDensityData( int type,
                                       double bboxSize[ 3 ],
                                       int steps[ 3 ],
                                       ProgressCallback cb = 0,
                                       void* cbData = 0 ) const;
    /// Generates density data usind the current molecule's bounding box.
    /// Value range is returned in minValue, maxValue parameters.
    vtkImageData* GenerateDensityData( int type, const double& step,
                                       double& minValue, double& maxValue,
                                       ProgressCallback cb = 0,
                                       void* cbData = 0 ) const;
    /// Generates and maps MEP on surface passed as an actor; if the lookup table
    /// parameter is non-null the passed lookup table will be used to map scalar
    /// values to colors.
    /// @warning vtkActor::GetMapper() must return a pointer to a vtkPolyDataMapper instance.
    void MapMEPOnSurface( vtkActor* a,
                          double bboxSize[ 3 ],
                          int steps[ 3 ],
                          const vtkLookupTable* lut = 0,
                          bool useGridData = false ) const;

    /// Returns a vtkActor's rendering style.
    RenderingStyle GetActorRenderingStyle( vtkActor* ) const;

    /// Save current transform.
    void SaveTransform();

    /// Restore transform.
    void RestoreTransform();

    /// Assigns material to vtkProperty.
    void SetMaterialType( vtkProperty* p, MaterialType m );

    /// Returns the material type from a vtkProperty.
    MaterialType GetMaterialType( vtkProperty* p ) const;

    /// Returns the suface actor from a grid label or NULL if surface not
    /// generated.
    vtkActor* GetGridDataSurfaceActor( const std::string& label ) const;
   
    
    /// First contains orbital index; second contains bitmask with orbital
    /// types.
    class OrbitalIndex
    {
    public:
    	OrbitalIndex( int index, int typeMask = ~0 ) :
    		index_( index ), typeMask_( typeMask )
    	{}
    	int TypeMask() const { return typeMask_; }
    	operator int() const { return index_; }
    	bool operator<( const OrbitalIndex& oi )
    	{
    		return index_ < oi.index_;
    	}
    private:
    	int index_;
    	int typeMask_;
    };
    
    typedef std::map< OrbitalIndex, vtkSmartPointer< vtkAssembly > >
        OrbitalActorMap;
    
    typedef std::map< VtkEventIdType, VtkObserverIdType >
        VtkObserverEventMap;
    
    typedef std::map< std::string, vtkSmartPointer< vtkActor > >
        GridActorMap;

    /// Orbital id -> vtkActor map.
    OrbitalActorMap orbitalActorMap_;
    /// Observer id -> event map
    VtkObserverEventMap vtkObserverEventMap_;
    /// Actor containing Molecular Inventor scenegraph
    /// stored inside vtkSoMapper
    vtkSmartPointer< vtkActor > actor_;
    /// Actor containing Molecule's bounding box
    vtkSmartPointer< vtkActor > bbox_;
    /// Actor containing bounding box used for isosurface computation.
    vtkSmartPointer< vtkActor > isoBBox_;
    /// Assembly containing bounding box and molecule geometry.
    vtkSmartPointer< vtkAssembly > assembly_;
    /// MaterialType used when atom display style == SoSphere and
    /// bond display style == SoCylinder.
    SoMaterial* material_;
    /// Reference to ChemSelection node
    ChemSelection* chemSelection_;
    /// Reference to ChemData node inside vtkSomMapper;
    ChemData* chemData_;
    /// Reference to ChemAssociatedData node; this node is NOT
    /// stored inside the Inventor Scenegraph.
    ChemAssociatedData* chemAssociatedData_;
    /// Reference to ChemDisplayParam node inside vtkSoMapper;
    /// this data member is used to change the visual appearance
    /// of molecules
    ChemDisplayParam* chemDisplayParam_;
    /// Reference to ChemDisplay node inside vtkSoMapper;
    /// used to change global display parameters
    ChemDisplay* chemDisplay_;
    /// Reference to atom radius table stored into vtkSoMapper;
    ChemRadii* chemRadii_;
    /// Reference to atom color table stored into vtkSoMapper;
    ChemColor* chemColor_;
    /// OpenBabel molecule
    OpenBabel::OBMol* obMol_;
    /// Old Molekel's Molecule class
    Molecule* molekelMol_;
    /// Molecule's file extension (used as a format identifier)
    std::string format_;
    /// Molecule's file name
    std::string fname_;
    /// Molecule's file path
    std::string path_;
    /// Bounding box
    vtkSmartPointer< vtkCubeSource > boundingBox_;
    /// Bounding box used for isosurface computation.
    vtkSmartPointer< vtkCubeSource > isoBoundingBox_;
    /// Class used to update this molecule each time Update() is
    /// called. Currently used for animation only.
    IMoleculeUpdater* updater_;
    /// Actor for surface generated from OBGridData.
    vtkSmartPointer< vtkActor > gridDataActor_;
    /// Electron density surface.
    vtkSmartPointer< vtkActor > elDensSurfaceActor_;
    /// Spin density surface.
    vtkSmartPointer< vtkActor > spinDensSurfaceActor_;
    /// Actor for vibration vectors.
    vtkSmartPointer< vtkActor > vibrationVectorsActor_;
    /// Dipole moment actor
    vtkSmartPointer< vtkActor > dipoleMomentActor_;
    /// (T41 ADF) grid data
    GridActorMap gridActorMap_;
    /// Dipole moment arrow length.
    double dipoleMomentArrowLength_;
    /// SAS.
    vtkSmartPointer< vtkActor > sasActor_;
    /// SES dot surface.
    ChemConnollyDot* connollyDot_;
    /// MSMS generated SES.
    vtkSmartPointer< vtkActor > sesmsActor_;
    /// SoSwitch node used to hide/show Connolly surface.
    SoSwitch* sesSwitch_;
    /// Inner class used to hold transform info.
    struct Transform
    {
        double position[ 3 ];
        double orientation[ 3 ];
        double scaling[ 3 ];
        Transform()
        {
            position[ 0 ] = 0.; position[ 1 ] = 0.; position[ 2 ] = 0.;
            scaling[ 0 ] = 1.; scaling[ 1 ] = 1.; scaling[ 2 ] = 1.;
            orientation[ 0 ] = 0.; orientation[ 1 ] = 0.; orientation[ 2 ] = 0.;
        }
    };

    /// Used to record information about the last lookup table assigned to a surface.
    mutable vtkSmartPointer< vtkLookupTable > mepLUT_;

    /// Saved transform.
    mutable Transform savedTransform_;

    // @{ Variables for thread control.
    /// Boolean variables used to control thread termination for methods that can
    /// be executed in their own thread these variables are:
    /// - set to false when a computation starts;
    /// - checked at each iteration by the active method; method exits if variable == true
    /// - set to true by the StopComputation methods
    /// - set to false upon successfull completion of an operation
    /// XXXStopped methods return the value of these variables to allow clients to detect
    /// if computation was stopped before completion; note that only part of the computation
    /// (the one carried on within MolekelMolecule methods ) can actually be stopped:
    /// Most of VTK and all OpenInventor methods do not have any support for stop operations.
    /// SAS.
    mutable bool stopSASComputation_;
    /// Connolly. Not Implemented yet as an interruptible operation.
    mutable bool stopSESComputation_;
    /// Connolly surface computed through M.F. Sanner's MSMS.
    mutable bool stopSESMSComputation_;
    // @}

    /// Frames: OBMol* sequence read from multi-molecule data formats.
    /// Pointers are freed in @code ~MolekelMolecule().
    Frames frames_;

    /// File from which atom color were read.
    std::string atomColorFile_;

    typedef std::map< SurfaceType, ShaderProgram > ShaderSurfaceMap;

    /// Shader program <--> surface map.
    ShaderSurfaceMap shaderSurfaceMap_;

};


#endif /*MOLEKELMOLECULE_H_*/
