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

#include "dialogs/ShadersDialog.h"

// QT
#include <QMainWindow>
#include <QFileDialog>
#include <QVTKWidget.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QStatusBar>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QDir>
#include <QSettings>
#include <QColorDialog>
#include <QDockWidget>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QThread>
#include <QVBoxLayout>
#include <QPushButton>
#include <QByteArray>
#include <QToolBar>
#include <QCoreApplication>
#include <QTextEdit>
#include <QTextDocument>
#include <QWhatsThis>

#if QT_VERSION >= 0x040200
#include <QDesktopServices>
#include <QUrl>
#endif

// VTK
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyleTrackballActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkWindowToImageFilter.h>
#include <vtkTIFFWriter.h>
#include <vtkAxes.h>
#include <vtkPolyDataMapper.h>
#include <vtkAxesActor.h>
#include <vtkImageData.h>
#include <vtkImagePlaneWidget.h>
#include <vtkLookupTable.h>
#include <vtkOutputWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkScalarBarWidget.h>
#include <vtkPNGWriter.h>
#include <vtkProperty.h>
#include <vtkMapper2D.h>
#include <vtkPointData.h>
#include <vtkGL2PSExporter.h>
#include <vtkRenderLargeImage.h>

#ifdef WIN32 // AVI output enabled on windows only
#include <vtkAVIWriter.h>
#endif

// INVENTOR
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/nodes/SoNode.h>

// MOIV
#include <ChemKit2/ChemData.h>
#include <ChemKit2/ChemDetail.h>
#include <ChemKit2/ChemDisplayPath.h>
#include <ChemKit2/ChemSelection.h>
#include <ChemKit2/ChemDisplayPathList.h>
#include <ChemKit2/ChemDisplayParam.h>

// STD
#include <cassert>
#include <string>
#include <sstream>
#include <stdexcept>

// Molekel
#include "MolekelData.h"
#include "MainWindow.h"
#include "MolekelException.h"
#include "MoleculeActorPickCommand.h"
#include "widgets/MoleculeRenderingStyleWidget.h"
#include "widgets/WorkspaceTreeWidget.h"
#include "widgets/DisplayPropertyWidget.h"
#include "AtomAnimation.h"
#include "dialogs/SasDialog.h"
#include "utility/events/EventRecorderWidget.h"
#include "utility/events/EventPlayerWidget.h"
#include "utility/qtfileutils.h"
#include "AtomAnimation.h"
#include "dialogs/GridDataSurfaceDialog.h"
#include "dialogs/ImagePlaneProbeDialog.h"
#include <openbabel/mol.h>
#include "utility/Timer.h"
#include "utility/System.h"
#include "dialogs/ExportAnimationDialog.h"
#include "dialogs/MoleculeAnimationDialog.h"
#include "dialogs/TimeStepDialog.h"
#include "dialogs/ComputeElDensSurfaceDialog.h"
#include "versioninfo.h"
#include "widgets/SpectrumWidget.h"

//#define ENABLE_DEPTH_PEELING


#if VTK_MAJOR_VERSION > 5 || ( VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0 )    
#ifdef ENABLE_DEPTH_PEELING   // REQUIRES VTK VERSION >= 5.1
#define DEPTH_PEELING_ENABLED
#endif
#endif


using namespace std;

// keys used in persistent settings
const string MainWindow::IN_DATA_DIR_KEY = "io/indatadir";
const string MainWindow::OUT_DATA_DIR_KEY = "io/outdatadir";
const string MainWindow::OUT_SNAPSHOTS_DATA_DIR_KEY = "io/outsnapshotsdatadir";
const string MainWindow::OUT_EVENTS_DIR_KEY = "io/outeventsdir";
const string MainWindow::IN_EVENTS_DIR_KEY = "io/ineventsdir";
const string MainWindow::PLUGIN_DIR_KEY = "plugin/dir";
const string MainWindow::MAIN_WINDOW_LAYOUT_KEY = "gui/mainwinlayout";
const string MainWindow::MAIN_WINDOW_WIDTH_KEY = "gui/mainwinwidth";
const string MainWindow::MAIN_WINDOW_HEIGHT_KEY = "gui/mainwinheight";
const string MainWindow::MSMS_EXECUTABLE_KEY = "msmsexecutable";
const string MainWindow::MAIN_WINDOW_X_KEY = "gui/mainwinx";
const string MainWindow::MAIN_WINDOW_Y_KEY = "gui/mainwiny";
const string MainWindow::SHADERS_DIR_KEY = "io/shaders";
const string MainWindow::BACKGROUND_COLOR_KEY = "gui/bkcolor";
const string MainWindow::MOLECULE_DISPLAY_STYLE_KEY = "appearance/display_style";
const string MainWindow::MOLECULE_ATOM_DISPLAY_STYLE_KEY = "appearance/atom_display_style";
const string MainWindow::MOLECULE_BOND_DISPLAY_STYLE_KEY = "appearance/bond_display_style";
const string MainWindow::MOLECULE_ATOM_SCALING_KEY = "appearance/atom_size";
const string MainWindow::MOLECULE_BOND_SCALING_KEY = "appearance/bond_size";
const string MainWindow::MOLECULE_ATOM_DETAIL_KEY = "appearance/atom_detail";
const string MainWindow::MOLECULE_BOND_DETAIL_KEY = "appearance/bond_detail";
const string MainWindow::MOLECULE_ATOM_COLORS_FILE_KEY = "appearance/atom_colors_file";

// molecule properties
static const char PROPS_TITLE[] = "Title";
static const char PROPS_NUMBER_OF_FRAMES[] = "Number of Frames";
static const char PROPS_NUMBER_OF_ATOMS[] = "Atoms";
static const char PROPS_NUMBER_OF_BONDS[] = "Bonds";
static const char PROPS_NUMBER_OF_NON_H_ATOMS[] = "Non-H bonds";
static const char PROPS_NUMBER_OF_RESIDUES[] =  "Residues";
static const char PROPS_NUMBER_OF_ROTORS[] = "Rotors";
static const char PROPS_NUMBER_OF_CONFORMERS[] = "Conformers";
static const char PROPS_FORMULA[] = "Formula";
static const char PROPS_ENERGY[] = "Energy";
static const char PROPS_STD_MOLAR_MASS[] = "Molar Mass";
static const char PROPS_EXACT_MASS[] = "Exact Mass";
static const char PROPS_TOTAL_CHARGE[] =  "Charge";
static const char PROPS_TOTAL_SPIN_MULT[] = "Spin Multiplicity";
static const char PROPS_TRAJECTORY[] = "Trajectory";
static const char PROPS_VIBRATION[] = "Vibration Frequencies";
static const char PROPS_DIPOLE_MOMENT[] = "Dipole Moment";

// toolbar
#ifndef __APPLE_CC__
#if defined(_MSC_VER) && defined(MOLEKEL_DEVELOPMENT) // executable is inside a Debug or Release folder
static const char TOOLBAR_ICONS_RELATIVE_PATH[] = "/../dist/resources/toolbar/";
#else
static const char TOOLBAR_ICONS_RELATIVE_PATH[] = "/../resources/toolbar/";
#endif
#else
static const char TOOLBAR_ICONS_RELATIVE_PATH[] = "/../Resources/toolbar/";
#endif                                                                            //USED   
static const char TOOLBAR_OPEN_FILE_ICON[] = "folder_open_24.png";                //[x]
static const char TOOLBAR_SAVE_FILE_ICON[] = "save_24.png";                       //[x]
static const char TOOLBAR_SNAPSHOT_ICON[] = "photo_camera_24.png";                //[x]
static const char TOOLBAR_CLEAR_ALL_ICON[] = "recycle_bin_24.png";                //[x]
static const char TOOLBAR_DELETE_MOLECULE_ICON[] = "delete_x_24.png";             //[x]
static const char TOOLBAR_RESET_CAMERA_ICON[] = "home_purple_24.png";             //[x]
static const char TOOLBAR_RESET_MOLECULE_ICON[] = "reset_molecule2.png";          //[x]
static const char TOOLBAR_TOGGLE_PLANE_PROBE_ICON[] = "plane_probe_24.png";       //[x]
static const char TOOLBAR_PLANE_PROBE_ICON[] = "plane_probe_24.png";              //
static const char TOOLBAR_ELDENS_SURFACE_ICON[] = "test_24.png";                  //
static const char TOOLBAR_SES_ICON[] = "test_24.png";                             //
static const char TOOLBAR_GRID_SURFACE_ICON[] = "test_24.png";                    //
static const char TOOLBAR_ATOM_DISTANCE_ICON[] = "test_24.png";                   //
static const char TOOLBAR_ATOM_ANGLE_ICON[] = "test_24.png";                      //
static const char TOOLBAR_ATOM_DIANGLE_ICON[] = "test_24.png";                    //
static const char TOOLBAR_MOLECULE_INTERACTION_ICON[] = "select_molecule2_24.png";//[x]
static const char TOOLBAR_CAMERA_INTERACTION_ICON[] = "select_camera_24.png";     //[x]
static const char TOOLBAR_PICK_MOLECULE_ICON[] = "test_24.png";                   //
static const char TOOLBAR_PICK_ATOM_ICON[] = "molecule_pick_atom2.png";           //[x]
static const char TOOLBAR_MOLECULE_DISPLAY_SETTINGS_ICON[] = "molecule2.png";     //[x]
static const char TOOLBAR_MEASURE_ICON[] = "measure_24.png";                      //[x]
static const char TOOLBAR_NEXT_FRAME_ICON[] = "forward_24.png";                   //[x]
static const char TOOLBAR_PREV_FRAME_ICON[] = "backward_24.png";                  //[x]
static const char TOOLBAR_PLAY_ICON[] = "play_24.png";                            //[x]
static const char TOOLBAR_STOP_ICON[] = "stop_24.png";                            //[x]
static const char TOOLBAR_PAUSE_ICON[] = "pause_24.png";                          //[x]
static const char TOOLBAR_REMOVE_BOND_ICON[] = "remove_bond_24.png";              //[x]
static const char TOOLBAR_ADD_BOND_ICON[] = "add_bond_24.png";                    //[x]


// Image formats.
static const char PNG_FILE_FORMAT[]  = "png - Portable Network Graphics - (*.png *.PNG)";
static const char TIFF_FILE_FORMAT[] = "tiff - Tag Image File Format - (*.tiff *.TIFF *.tif *.TIF)";


extern bool GLSLShadersSupported();



//-----------------------------------------------------------------------------
MainWindow::MainWindow() : fileMenu_( 0 ),
                           editMenu_( 0 ),
                           interactionMenu_( 0 ),
                           helpMenu_( 0 ),
                           animationMenu_( 0 ),
                           viewMenu_( 0 ),
                           vtkWidget_( 0 ),
                           vtkRenderer_( 0 ),
                           vtkAxesRenderer_( 0 ),
                           data_( 0 ),
                           modified_( false ),
                           timestep_( 50 ),
                           lastSelectedMolecule_( MolekelData::InvalidIndex() ),
                           interactionMode_( INTERACT_WITH_CAMERA ),
                           pickingMode_( PICK_MOLECULE ),
                           show3DViewSize_( true ),
                           loadFileMessageBox_( 0 ),
                           fileLoadThread_( 0 ),
			               recordEventsDlg_( new EventRecorderWidget, this, Qt::Tool ),
                           playEventsDlg_( new EventPlayerWidget, this, Qt::Tool ),
                           exportAnimationInProgress_( false ),
                           frameCounter_( 0 )
{

    recordEventsDlg_.setWindowTitle( "Record GUI Events" );
    qobject_cast< EventRecorderWidget* >( recordEventsDlg_.GetWidget() )->SetSettingsKey( OUT_EVENTS_DIR_KEY.c_str() );
	qobject_cast< EventRecorderWidget* >( recordEventsDlg_.GetWidget() )->SetObjectToIgnore( &recordEventsDlg_ );

    playEventsDlg_.setWindowTitle( "Playback GUI Events" );
    qobject_cast< EventPlayerWidget* >( playEventsDlg_.GetWidget() )->SetSettingsKey( IN_EVENTS_DIR_KEY.c_str() );

    // First of all: stop VTK at each error/warning.
    vtkOutputWindow::GetInstance()->PromptUserOn();

    //-------------------------------------------------------------
    // Internal class.
    /// Used to always return pointer to main renderer whenever
    /// @code FindPokedRenderer(int, int) @endcode is called;
    /// this is required to disable any interaction with the
    /// axes renderer.
    class RenderInteractor : public vtkRenderWindowInteractor
    {
        /// Pointer to returned renderer.
        vtkRenderer* ren_;
		RenderInteractor() {};
    public:
        /// Constructor.
        RenderInteractor( vtkRenderer* ren ) : ren_( ren ) {}
        /// Overridden method: always return pointer to
        /// renderer stored in internal data member.
        vtkRenderer* FindPokedRenderer( int, int )
        {
            assert( ren_ && "Null renderer" );
            return ren_;
        }
    };
    //--------------------------------------------------------------

    //--------------------------------------------------------------
    // Internal class.
    /// Called each time the main renderer's camera is modified.
    /// Used to align the axes orientation with the camera.
    class CameraModifiedCallback : public vtkCommand
    {
        /// Reference to vtkAxesActor to align with camera.
        vtkSmartPointer< vtkAxesActor > actor;
    public:
        /// Constructor.
        CameraModifiedCallback( const vtkSmartPointer< vtkAxesActor >& a) : actor( a ) {}
        /// Overridden execute method.
        /// - copy camera transform to internal object
        /// - set translation components to zero
        /// - normalize eigenvectors (rows or upper 3x3 matrix)
        /// - assign transform to axes actor
        void Execute( vtkObject *obj, unsigned long id, void *data )
        {
            vtkCamera* cam = dynamic_cast< vtkCamera* >( obj );
            // get camera matrix
            vtkSmartPointer< vtkMatrix4x4 > m4x4( vtkMatrix4x4::New() );
            vtkMatrix4x4& m = *m4x4;
            m.DeepCopy( cam->GetViewTransformMatrix() );
            // set translation to zero
            m[ 0 ][ 3 ] = 0; m[ 1 ][ 3 ] = 0; m[ 2 ][ 3 ] = 0;
            m[ 3 ][ 0 ] = 0; m[ 3 ][ 1 ] = 0; m[ 3 ][ 2 ] = 0; m[ 3 ][ 3 ] = 1;
            // normalize vectors
            Normalize( m[ 0 ] );
            Normalize( m[ 1 ] );
            Normalize( m[ 2 ] );

            // uncommenting the following line creates the inverse
            // transform which causes the axes to be displayed as they
            // would be seen through the main renderer's camera
            //TransposeUpper3x3( m );

            // assign to actor
            actor->SetUserMatrix( m4x4 );

        }
        /// Normalizes a 3d vector.
        void Normalize( double v[ 3 ] )
        {
            double n = std::sqrt( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
            if( n > 0. ) n = 1 / n;
            v[ 0 ] *= n;
            v[ 1 ] *= n;
            v[ 2 ] *= n;
        }
        /// Transposes the upper 3x3 part of a 4x4 matrix.
        void TransposeUpper3x3( vtkMatrix4x4& m )
        {
            std::swap( m[ 0 ][ 1 ], m[ 1 ][ 0 ] );
            std::swap( m[ 0 ][ 2 ], m[ 2 ][ 0 ] );
            std::swap( m[ 1 ][ 2 ], m[ 2 ][ 1 ] );
        }
    };
    //-------------------------------------------------------------

    /// QVTKWidget
    // widget
    vtkWidget_ = new QVTKWidget( this );
    vtkWidget_->setObjectName( "QVTKWidget" );
    vtkWidget_->setAccessibleName( vtkWidget_->objectName() );
    setCentralWidget( vtkWidget_ );
    // render window    
    vtkRenderWindow_ = vtkRenderWindow::New();

#if VTK_MAJOR_VERSION > 5 || ( VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0 )       
    vtkRenderWindow_->SetMultiSamples( 1 );
#endif    
    
    vtkRenderWindow_->SetAlphaBitPlanes( 1 );
    // renderer
    vtkRenderer_ = vtkRenderer::New();
    vtkRenderWindow_->AddRenderer( vtkRenderer_ );
    // DO NOT DO THIS:  vtkRenderWindow_ = vtkWidget_->GetRenderWindow();
    // a new vtkRenderWindow is created each time QVTKWidget::GetRenderWindow() is
    // invoked!
    vtkWidget_->SetRenderWindow( vtkRenderWindow_ );
    
    /// Toolbars
    mainToolBar_ = addToolBar( "Toolbar" );
    mainToolBar_->setObjectName( "MainToolbar" );
    CreateMenus();
    CreateQtActions();
    statusBar()->showMessage( QString( "Ready" ) );
    data_ = new MolekelData; // <- use auto_ptr
    trackBallActor_  = vtkInteractorStyleTrackballActor::New();
    trackBallCamera_ = vtkInteractorStyleTrackballCamera::New();
    interactorStyle_ = trackBallCamera_;
    
    /// Interactors
    vtkRenderWindow_->SetInteractor( new RenderInteractor( vtkRenderer_ ) );
    vtkRenderWindow_->GetInteractor()->SetInteractorStyle( interactorStyle_ );
    vtkRenderWindow_->GetInteractor()->Initialize();

    /// additional widgets

    // create tree widget
    workspaceTreeDockWidget_ = new WorkspaceTreeDockWidget( this );
    workspaceTreeDockWidget_->setWindowTitle( "Workspace" );
    addDockWidget( Qt::LeftDockWidgetArea, workspaceTreeDockWidget_ );

    // create molecule property widget
    QStringList molProps;
    molProps << QString( PROPS_TITLE )
             <<	QString( PROPS_NUMBER_OF_ATOMS )
             << QString( PROPS_NUMBER_OF_FRAMES )
             << QString( PROPS_NUMBER_OF_BONDS )
             << QString( PROPS_NUMBER_OF_NON_H_ATOMS )
             << QString( PROPS_NUMBER_OF_RESIDUES )
   // Disabled: takes too long with molecules having thousands of atoms
   //        << QString( PROPS_NUMBER_OF_ROTORS )
             << QString( PROPS_NUMBER_OF_CONFORMERS )
    // Disabled: might crash on linux: with some pdb files a call to OBMol::GetFormula() causes the program to crash
    //       << QString( PROPS_FORMULA )
             << QString( PROPS_ENERGY )
             << QString( PROPS_STD_MOLAR_MASS )
             << QString( PROPS_EXACT_MASS )
             << QString( PROPS_TOTAL_CHARGE )
             << QString( PROPS_TOTAL_SPIN_MULT )
             << QString( PROPS_TRAJECTORY )
             << QString( PROPS_VIBRATION )
             << QString( PROPS_DIPOLE_MOMENT );

    moleculePropertyDockWidget_ = new MoleculePropertyDockWidget( molProps, this, this );
    moleculePropertyDockWidget_->setWindowTitle( "Molecule Properties" );
    addDockWidget( Qt::LeftDockWidgetArea, moleculePropertyDockWidget_ );

    /// Axes
    axesViewport_[ 0 ] = 0.93;
    axesViewport_[ 1 ] = 0.;
    axesViewport_[ 2 ] = 1;
    axesViewport_[ 3 ] = 0.07;

    vtkAxesRenderer_ = vtkRenderer::New();
    vtkSmartPointer< vtkAxesActor > axes( vtkAxesActor::New() );

    vtkRenderer_->GetActiveCamera()->AddObserver( vtkCommand::ModifiedEvent,
                                                  new CameraModifiedCallback( axes ) );
    vtkAxesRenderer_->AddActor( axes );
    vtkRenderWindow_->AddRenderer( vtkAxesRenderer_ );
    vtkAxesRenderer_->SetViewport( axesViewport_[ 0 ], axesViewport_[ 1 ],
                                   axesViewport_[ 2 ], axesViewport_[ 3 ] );


    /// @todo movee code to set LUT, text and interpolation method into image plane
    /// probe dialog.

    /// Initialize analysis widgets.
    // Image plane
    vtkImagePlaneWidget_ = vtkImagePlaneWidget::New();
    vtkImagePlaneWidget_->SetUseContinuousCursor( true );
    vtkImagePlaneWidget_->SetInteractor( vtkRenderWindow_->GetInteractor() );
    vtkImagePlaneWidget_->SetDisplayText( true );
    vtkImagePlaneWidget_->TextureVisibilityOn();
    vtkImagePlaneWidget_->TextureInterpolateOn();
    //vtkImagePlaneWidget_->SetResliceInterpolateToLinear();
    //generate lookup table
    /// @todo add option to load custom LUTs
    defaultLUT_ = vtkLookupTable::New();
    probeLUT_ = vtkLookupTable::New();
    probeLUT_->DeepCopy( defaultLUT_ );
    mepLUT_ = vtkLookupTable::New();
    mepLUT_->DeepCopy( defaultLUT_ );
    vtkImagePlaneWidget_->SetLookupTable( probeLUT_ );
    // default behavior for vtkImagePlaneWidget is to have the middle mouse
    // button move the plane, on Apple however this button is usually associated with
    // the dashboard. Temporary solution: use left button to move the plane on Apple,
    // will have to determine if cursor on plane is required at all
#ifdef __APPLE_CC__
#if VTK_MAJOR_VERSION > 5 || ( VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0 )
    vtkImagePlaneWidget_->SetLeftButtonAction( vtkImagePlaneWidget::VTK_SLICE_MOTION_ACTION );
#else    
    vtkImagePlaneWidget_->SetLeftButtonAction( vtkImagePlaneWidget::SLICE_MOTION_ACTION );
#endif
#endif
    /// @todo finish widgets initialization.

    /// Scalar bars
    vtkMEPScalarBarWidget_ = vtkScalarBarWidget::New();
    vtkMEPScalarBarWidget_->SetInteractor( vtkRenderWindow_->GetInteractor() );
    vtkSmartPointer< vtkScalarBarActor > sba( vtkScalarBarActor::New() );
    sba->SetLookupTable( mepLUT_ );
    sba->SetTitle( "Electrostatic Potential" );
    vtkMEPScalarBarWidget_->SetScalarBarActor( sba );

    vtkProbeScalarBarWidget_ = vtkScalarBarWidget::New();
    vtkProbeScalarBarWidget_->SetInteractor( vtkRenderWindow_->GetInteractor() );
    vtkSmartPointer< vtkScalarBarActor > sba2( vtkScalarBarActor::New() );
    sba2->SetLookupTable( probeLUT_ );
    vtkProbeScalarBarWidget_->SetScalarBarActor( sba2 );

    /// Create thread and message box used in file loading operations
    loadFileMessageBox_ = new QDialog( this );
    QVBoxLayout* loadFileLayout = new QVBoxLayout;
    QPushButton* abortButton = new QPushButton( QString( "Abort loading" ) );
    connect( abortButton, SIGNAL( released() ), loadFileMessageBox_, SLOT( reject() ) );
    loadFileLayout->addWidget( abortButton );
    loadFileMessageBox_->setLayout( loadFileLayout );

    setObjectName( "MainWindow" );
    setAccessibleName( objectName() );

#ifdef DEPTH_PEELING_ENABLED
    // depth peeling:
    vtkRenderer_->SetUseDepthPeeling( 1 ); // enable depth peeling
#endif  
    
    QSettings s;
    SetBkColor( s.value( BACKGROUND_COLOR_KEY.c_str(), QColor() ).value< QColor >() ); 
}

//------------------------------------------------------------------------------
void MainWindow::AddActionToToolBar( QAction* a, const QString& iconName )
{
    assert( a && "NULL Action" );
    assert( mainToolBar_ && "NULL Toolbar" );
    const QString iconFilePath( QCoreApplication::applicationDirPath() +
                                TOOLBAR_ICONS_RELATIVE_PATH + iconName );
    a->setIcon( QIcon( iconFilePath ) );
    mainToolBar_->addAction( a );
}

//-----------------------------------------------------------------------------
/*inline*/ void ReplaceActionIcon( QAction* a, const QString& iconName )
{
    const QString iconFilePath( QCoreApplication::applicationDirPath() +
                                TOOLBAR_ICONS_RELATIVE_PATH + iconName );
    a->setIcon( QIcon( iconFilePath ) );
}


//------------------------------------------------------------------------------
void MainWindow::CreateQtActions()
{
	menuBar()->setObjectName( "TheMenuBar" );
    // actions that do not require check/enable are
    // not stored inside MainWindow class

    // File menu actions:
    // Open, Save, Save Image, Save PS, Save PDF, Exit
    QAction* openAct = new QAction( QString("&Open..."), this );
    openAct->setShortcut( QString( "Ctrl+O" ) );
    openAct->setStatusTip( QString( "Load molecule" ) );
    connect( openAct, SIGNAL( triggered() ), this, SLOT( LoadFileSlot() ) );
    AddActionToToolBar( openAct, TOOLBAR_OPEN_FILE_ICON );

    saveAction_ = new QAction( QString("&Save..."), this);
    saveAction_->setShortcut( QString( "Ctrl+S" ) );
    saveAction_->setStatusTip( QString( "Save molecule" ) );
    // disable action on startup since there is nothing to save
    saveAction_->setEnabled( false );
    connect( saveAction_, SIGNAL( triggered() ), this, SLOT( SaveFileSlot() ) );
    AddActionToToolBar( saveAction_, TOOLBAR_SAVE_FILE_ICON );

    QAction* saveImageAct = new QAction( QString( "Save &Image..." ), this );
    saveImageAct->setShortcut( QString( "Ctrl+I" ) );
    saveImageAct->setStatusTip( QString( "Save snapshot" ) );
    connect( saveImageAct, SIGNAL( triggered() ), this, SLOT( SaveSnapshotSlot() ) );
    AddActionToToolBar( saveImageAct, TOOLBAR_SNAPSHOT_ICON );

    QAction* savePSAct = new QAction( QString( "Save to PostScript..." ), this );
    savePSAct->setStatusTip( QString( "Save snapshot to PostScript" ) );
    connect( savePSAct, SIGNAL( triggered() ), this, SLOT( SaveToPSSlot() ) );

    QAction* saveEPSAct = new QAction( QString( "Save to EPS..." ), this );
    saveEPSAct->setStatusTip( QString( "Save snapshot to Encapsulated PostScript" ) );
    connect( saveEPSAct, SIGNAL( triggered() ), this, SLOT( SaveToEPSSlot() ) );

    QAction* savePDFAct = new QAction( QString( "Save to PDF..." ), this );
    savePDFAct->setStatusTip( QString( "Save snapshot to PDF" ) );
    connect( savePDFAct, SIGNAL( triggered() ), this, SLOT( SaveToPDFSlot() ) );

    QAction* saveTexAct = new QAction( QString( "Save to TeX..." ), this );
    saveTexAct->setStatusTip( QString( "Save snapshot to TeX" ) );
    connect( saveTexAct, SIGNAL( triggered() ), this, SLOT( SaveToTeXSlot() ) );

    QAction* saveOrbitalPicturesAct = new QAction( QString( "Save Orbital Pictures..." ), this );
    saveOrbitalPicturesAct->setStatusTip( QString( "Save one snapshot per orbital" ) );
    connect( saveOrbitalPicturesAct, SIGNAL( triggered() ), this, SLOT( SaveOrbitalSnapshotsSlot() ) );

    QAction* exitAct = new QAction( QString( "E&xit" ), this );
    exitAct->setShortcut( tr ( "Ctrl+Q" ) );
    exitAct->setStatusTip( QString( "Exit the application" ) );
    connect( exitAct, SIGNAL( triggered() ), this, SLOT( CloseSlot() ) );
    addAction( exitAct );

    // Edit menu actions
    // Unselect, Delete Molecule, Clear, Reset Molecule, Reset Camera
    mainToolBar_->addSeparator();

    unselectAllAction_ = new QAction( QString( "Unselect" ), this );
    unselectAllAction_->setStatusTip( QString( "Unselect all" ) );
    unselectAllAction_->setShortcut( tr ( "Ctrl+U" ) );
    unselectAllAction_->setEnabled( false );
    connect( unselectAllAction_, SIGNAL( triggered() ), this, SLOT( UnselectAllSlot() ) );

    deleteMoleculeAction_ = new QAction( QString( "Delete Molecule" ), this );
    deleteMoleculeAction_->setStatusTip( QString( "Delete selected molecule" ) );
    deleteMoleculeAction_->setShortcut( Qt::Key_Delete );
    deleteMoleculeAction_->setEnabled( false );
    connect( deleteMoleculeAction_, SIGNAL( triggered() ), this, SLOT( DeleteMoleculeSlot() ) );
    AddActionToToolBar( deleteMoleculeAction_, TOOLBAR_DELETE_MOLECULE_ICON );

    clearAction_ = new QAction( QString( "Clear" ), this );
    clearAction_->setStatusTip( QString( "Delete all molecules" ) );
    clearAction_->setEnabled( false );
    connect( clearAction_, SIGNAL( triggered() ), this, SLOT( ClearSlot() ) );
    AddActionToToolBar( clearAction_, TOOLBAR_CLEAR_ALL_ICON );

    resetMoleculeAction_ = new QAction( QString( "Reset Molecule" ), this );
    resetMoleculeAction_->setStatusTip( QString( "Reset selected molecule" ) );
    resetMoleculeAction_->setEnabled( false );
    connect( resetMoleculeAction_, SIGNAL( triggered() ), this, SLOT( ResetMoleculeSlot() ) );
    AddActionToToolBar( resetMoleculeAction_, TOOLBAR_RESET_MOLECULE_ICON );

    QAction* resetCameraAct = new QAction( QString( "Reset Camera" ), this );
    resetCameraAct->setStatusTip( QString( "Reset camera" ) );
    connect( resetCameraAct, SIGNAL( triggered() ), this, SLOT( ResetCameraSlot() ) );
    AddActionToToolBar( resetCameraAct, TOOLBAR_RESET_CAMERA_ICON );

    // Display menu actions
    // Molecule, Background, Show Plane Probe, Show MEP Scalar Bar, Show Probe Scalar Bar
    mainToolBar_->addSeparator();
    moleculeDisplayAction_ = new QAction( QString( "&Molecule..." ), this );
    // nothing is selected on startup
    moleculeDisplayAction_->setEnabled( false );
    moleculeDisplayAction_->setStatusTip( QString( "Molecule display properties" ) );
    moleculeDisplayAction_->setToolTip( QString( "Molecule display properties" ) );
    moleculeDisplayAction_->setShortcut( QString( "Shift+S" ) );
    connect( moleculeDisplayAction_, SIGNAL( triggered() ), this, SLOT( MoleculeDisplaySlot() ) );
    AddActionToToolBar( moleculeDisplayAction_, TOOLBAR_MOLECULE_DISPLAY_SETTINGS_ICON );

    QAction* backGroundColorAct_ = new QAction( QString( "Background..." ), this );
    backGroundColorAct_->setStatusTip( QString( "Change background color" ) );
    connect( backGroundColorAct_, SIGNAL( triggered() ), this, SLOT( BackgroundColorSlot() ) );

    planeProbeVisibilityAction_ = new QAction( QString( "Show Plane Probe" ), this );
    planeProbeVisibilityAction_->setStatusTip( QString( "Show/Hide plane probe" ) );
    planeProbeVisibilityAction_->setShortcut( QString( "Shift+P" ) );
    connect( planeProbeVisibilityAction_, SIGNAL( triggered() ), this, SLOT( TogglePlaneProbeSlot() ) );
    AddActionToToolBar( planeProbeVisibilityAction_, TOOLBAR_TOGGLE_PLANE_PROBE_ICON );

    mepScalarBarAction_ = new QAction( QString( "Show MEP Scalar Bar" ), this );
    mepScalarBarAction_->setStatusTip( QString( "Show/Hide MEP scalar bar" ) );
    connect( mepScalarBarAction_, SIGNAL( triggered() ), this, SLOT( ToggleMEPScalarBarSlot() ) );

    probeScalarBarAction_ = new QAction( QString( "Show Probe Scalar Bar" ), this );
    probeScalarBarAction_->setStatusTip( QString( "Show/Hide probe scalar bar" ) );
    connect( probeScalarBarAction_, SIGNAL( triggered() ), this, SLOT( ToggleProbeScalarBarSlot() ) );

    axesAction_ = new QAction( QString( "Hide Axes" ), this );
    axesAction_->setStatusTip( QString( "Show/Hide axes" ) );
    connect( axesAction_, SIGNAL( triggered() ), this, SLOT( AxesSlot() ) );
    
    bboxAction_ = new QAction( QString( "Hide Bounding Boxes" ), this );
    bboxAction_->setStatusTip( QString( "Show/Hide molecules' bounding box" ) );
    connect( bboxAction_, SIGNAL( triggered() ), this, SLOT( BBoxSlot() ) );

    QAction* editViewPropertiesAct = new QAction( "3D View Properties...", this );
    connect( editViewPropertiesAct, SIGNAL( triggered() ), this, SLOT( Edit3DViewPropertiesSlot() ) );

    shadersAction_ =  new QAction( QString( "Shaders..." ), this );
    shadersAction_->setStatusTip( QString( "Assign GLSL shaders to surfaces" ) );
    shadersAction_->setEnabled( false );
    connect( shadersAction_, SIGNAL( triggered() ), this, SLOT( ShadersSlot() ) );

    // Interaction menu actions
    // Camera, Molecule, Pick Atom/Bond
    mainToolBar_->addSeparator();
    cameraInteractAction_ = new QAction( QString( "Camera" ), this );
    cameraInteractAction_->setStatusTip( QString( "Interact with camera" ) );
    cameraInteractAction_->setToolTip( QString( "Interact with camera" ) );
    cameraInteractAction_->setShortcut( QString( "Shift+C" ) );
    cameraInteractAction_->setCheckable( true );
    cameraInteractAction_->setChecked( interactionMode_ == INTERACT_WITH_CAMERA );
    connect( cameraInteractAction_, SIGNAL( triggered() ),
             this, SLOT( SetCameraInteractionSlot() ) );
    AddActionToToolBar( cameraInteractAction_, TOOLBAR_CAMERA_INTERACTION_ICON );

    moleculeInteractAction_ = new QAction( QString( "Molecule" ), this );
    moleculeInteractAction_->setStatusTip( QString( "Interact with molecule" ) );
    moleculeInteractAction_->setToolTip( QString( "Interact with molecule" ) );
    moleculeInteractAction_->setShortcut( QString( "Shift+M" ) );
    moleculeInteractAction_->setCheckable( true );
    moleculeInteractAction_->setChecked( interactionMode_ == INTERACT_WITH_MOLECULE );
    connect( moleculeInteractAction_, SIGNAL( triggered() ),
             this, SLOT( SetMoleculeInteractionSlot() ) );
    AddActionToToolBar( moleculeInteractAction_, TOOLBAR_MOLECULE_INTERACTION_ICON );

    pickAtomAction_ = new QAction( QString( "Pick Atom/Bond" ), this );
    pickAtomAction_->setStatusTip( QString( "Pick atom/bond" ) );
    pickAtomAction_->setCheckable( true );
    pickAtomAction_->setChecked( pickingMode_ == PICK_ATOMS_AND_BONDS );
    pickAtomAction_->setEnabled( interactionMode_ == INTERACT_WITH_MOLECULE );
    connect( pickAtomAction_, SIGNAL( toggled( bool ) ),
             this, SLOT( AtomPickingToggledSlot( bool ) ) );
    AddActionToToolBar( pickAtomAction_, TOOLBAR_PICK_ATOM_ICON );

    // Animation menu actions
    // Start Animation, Next, Previous, Timestep, Per-Molecule Settings, Export Animation.
    mainToolBar_->addSeparator();
    animationAction_ = new QAction( QString( "Start Animation" ), this );
    animationAction_->setStatusTip( QString( "Start/Stop animation" ) );
    connect( animationAction_, SIGNAL( triggered() ), this, SLOT( AnimationSlot() ) );
    AddActionToToolBar( animationAction_, TOOLBAR_PLAY_ICON );

    previousFrameAction_ = new QAction( QString( "Previous" ), this );
    previousFrameAction_->setStatusTip( QString( "Advance to previous animation frame" ) );
    previousFrameAction_->setToolTip( QString( "Advance to previous animation frame" ) );
    connect( previousFrameAction_, SIGNAL( triggered() ), this, SLOT( PreviousFrameSlot() ) );
    AddActionToToolBar( previousFrameAction_, TOOLBAR_PREV_FRAME_ICON );

    nextFrameAction_ = new QAction( QString( "Next" ), this );
    nextFrameAction_->setStatusTip( QString( "Advance to next animation frame" ) );
    nextFrameAction_->setToolTip( QString( "Advance to next animation frame" ) );
    connect( nextFrameAction_, SIGNAL( triggered() ), this, SLOT( NextFrameSlot() ) );
    AddActionToToolBar( nextFrameAction_, TOOLBAR_NEXT_FRAME_ICON );

    QAction* timestepAct = new QAction( QString( "Timestep" ), this );
    timestepAct->setStatusTip( QString( "Change animation time step" ) );
    connect( timestepAct, SIGNAL( triggered() ), this, SLOT( TimestepSlot() ) );

    moleculeAnimationAction_ = new QAction( QString( "Per-Molecule Settings..." ), this );
    moleculeAnimationAction_->setStatusTip( QString( "Change per-molecule settings" ) );
    connect( moleculeAnimationAction_, SIGNAL( triggered() ),
             this, SLOT( MoleculeAnimationSlot() ) );
    moleculeAnimationAction_->setEnabled( false );

    exportVideoAction_ = new QAction( QString( "Export Animation..." ), this );
    exportVideoAction_->setStatusTip( QString( "Export animation" ) );
    connect( exportVideoAction_, SIGNAL( triggered() ),
             this, SLOT( ExportVideoSlot() ) );

    // Surfaces menu actions
    // Electron Density, Grid Data, SAS, SES
    orbitalsAction_ = new QAction( QString( "Electron Density..." ), this );
    orbitalsAction_->setStatusTip( QString( "Generate electron density surface" ) );
    connect( orbitalsAction_, SIGNAL( triggered() ), this, SLOT( OrbitalsSlot() ) );
    orbitalsAction_->setEnabled( false );

    gridDataSurfaceAction_ = new QAction( QString( "Grid Data..." ), this );
    gridDataSurfaceAction_->setStatusTip( QString( "Generate surface from grid data" ) );
    gridDataSurfaceAction_->setEnabled( false );
    connect( gridDataSurfaceAction_, SIGNAL( triggered() ), this, SLOT( GridDataSurfaceSlot() ) );

    connollySurfaceAction_ = new QAction( QString( "Solvent Excluded Surface..." ), this );
    connollySurfaceAction_->setStatusTip( QString( "Generate Solvent Excluded Surface" ) );
    connollySurfaceAction_->setEnabled( false );
    connect( connollySurfaceAction_, SIGNAL( triggered() ),
             this, SLOT( ConnollySurfaceSlot() ) );

    sasAction_ = new QAction( QString( "Solvent Accessible Surface..." ), this );
    sasAction_->setStatusTip( QString( "Generate Solvent Accessible Surface" ) );
    sasAction_->setEnabled( false );
    connect( sasAction_, SIGNAL( triggered() ), this, SLOT( SasSlot() ) );


    // Analysis menu actions
    // Plane Probe, Distance, Angle, Dihedral Angle
    mainToolBar_->addSeparator();
    imagePlaneProbeAction_ = new QAction( QString( "Plane Probe..." ), this );
    imagePlaneProbeAction_->setStatusTip( QString( "Plane probe settings" ) );
    imagePlaneProbeAction_->setEnabled( false );
    connect( imagePlaneProbeAction_, SIGNAL( triggered() ),
             this, SLOT( ImagePlaneProbeSlot() ) );
    positionProbeAction_ = new QAction( QString( "Position Probe" ), this );
    positionProbeAction_->setEnabled( false ); // not implemented yet

    distanceAction_ = new QAction( QString( "Distance" ), this );
    distanceAction_->setStatusTip( QString( "Compute distance between two atoms" ) );
    distanceAction_->setEnabled( false ); // not implemented yet
    connect( distanceAction_, SIGNAL( triggered() ),
             this, SLOT( DistanceSlot() ) );

    angleAction_ = new QAction( QString( "Angle" ), this );  // angle among three atoms
    angleAction_->setStatusTip( QString( "Compute angle among three atoms" ) );
    angleAction_->setEnabled( false ); // not implemented yet
    connect( angleAction_, SIGNAL( triggered() ),
             this, SLOT( AngleSlot() ) );

    dihedralAngleAction_ = new QAction( QString( "Dihedral Angle" ), this );
    dihedralAngleAction_->setEnabled( false );
    dihedralAngleAction_->setStatusTip( QString( "Compute dihedral angle" ) );
    connect( dihedralAngleAction_, SIGNAL( triggered() ),
             this, SLOT( DihedralAngleSlot() ) );

    measureAction_ = new QAction( QString( "Measure distances and (dihedral) angles" ), this );
    measureAction_->setEnabled( false );
    measureAction_->setStatusTip( "Measure distances and (dihedral) angles" );
    measureAction_->setToolTip( "Measure distances and (dihedral) angles" );
    connect( measureAction_, SIGNAL( triggered() ), this, SLOT( MeasureSlot() ) );
    AddActionToToolBar( measureAction_, TOOLBAR_MEASURE_ICON );

    irSpectrumAction_ = new QAction( QString( "Radiation Spectrum" ), this );
    irSpectrumAction_->setEnabled( false );
    irSpectrumAction_->setStatusTip( QString( "Display IR spectrum" ) );
    connect( irSpectrumAction_, SIGNAL( triggered() ),
                this, SLOT( SpectrumSlot() ) );
        
    clipPlaneAction_ = new QAction( QString( "Clip Plane" ), this );
    clipPlaneAction_->setEnabled( false ); // not implemented yet

	// ADD/REMOVE BONDS
	addRemoveBondAction_ = new QAction( QString( "Add/Remove bond" ), this );
    addRemoveBondAction_->setEnabled( false );
    addRemoveBondAction_->setStatusTip( "Add/Remove bonds" );
    addRemoveBondAction_->setToolTip( "Add/Remove bond" );
    connect( addRemoveBondAction_, SIGNAL( triggered() ), this, SLOT( AddRemoveBondSlot() ) );
    AddActionToToolBar( addRemoveBondAction_, TOOLBAR_REMOVE_BOND_ICON );	


    // View menu actions
    // Workspace, Molecule Properties, Toolbar
    viewWorkspaceAction_ = new QAction( QString( "Workspace" ), this );
    viewWorkspaceAction_->setStatusTip( QString( "Show/Hide workspace window" ) );
    viewWorkspaceAction_->setCheckable( true );
    viewWorkspaceAction_->setChecked( true );
    connect( viewWorkspaceAction_, SIGNAL( toggled( bool ) ), this, SLOT( ViewWorkspaceSlot( bool ) ) );

    viewMoleculePropAction_ = new QAction( QString( "Molecule Properties" ), this );
    viewMoleculePropAction_->setCheckable( true );
    viewMoleculePropAction_->setStatusTip( QString( "Show/Hide properties window" ) );
    viewMoleculePropAction_->setChecked( true );
    connect( viewMoleculePropAction_, SIGNAL( toggled( bool ) ), this, SLOT( ViewMoleculePropSlot( bool ) ) );

    viewToolbarAction_ = mainToolBar_->toggleViewAction();
    viewToolbarAction_->setStatusTip( QString( "Show/Hide toolbar" ) );
    viewToolbarAction_->setCheckable( true );
    viewToolbarAction_->setChecked( true );
    connect( viewToolbarAction_, SIGNAL( toggled( bool ) ), this, SLOT( ViewToolbarSlot( bool ) ) );

    // Help menu actions
    // Documentation, Molecule Web Page, About, License
    QAction* docAct = new QAction( QString( "Documentation" ), this );
    docAct->setStatusTip( "Open documentation" );
    connect( docAct, SIGNAL( triggered() ), this, SLOT( DocSlot() ) );

    QAction* molekelWebsiteAct = new QAction( QString( "Molekel Web Page" ), this );
    molekelWebsiteAct->setStatusTip( "Open Molekel Web Page url" );
    connect( molekelWebsiteAct, SIGNAL( triggered() ), this, SLOT( MolekelWebsiteSlot() ) );

    QAction* aboutAct = new QAction( QString( "&About" ), this );
    aboutAct->setStatusTip( QString( "Show the application's About box" ) );
    connect( aboutAct, SIGNAL( triggered() ), this, SLOT( AboutSlot() ) );

    QAction* licenseAct = new QAction( QString( "License" ), this );
    licenseAct->setStatusTip( QString( "Show license" ) );
    connect( licenseAct, SIGNAL( triggered() ), this, SLOT( LicenseSlot() ) );

    ///////////////////////////////////////////
    // update menus
    assert( fileMenu_ && "fileMenu_ is NULL" );
    assert( editMenu_ && "editMenu_ is NULL" );
    assert( helpMenu_ && "helpMenu_ is NULL" );
    assert( interactionMenu_ && "interactionMenu_ is NULL" );
    assert( displayMenu_ && "displayMenu_ is NULL" );
    assert( animationMenu_ && "animationMenu_ is NULL" );
    assert( viewMenu_ && "viewMenu_  is NULL" );
    assert( surfacesMenu_ && "surfacesMenu_ is NULL" );
    assert( analysisMenu_ && "analysisMenu_ is NULL" );

    // File
    fileMenu_->addAction( openAct );
    fileMenu_->addAction( saveAction_ );
    fileMenu_->addSeparator();
    fileMenu_->addAction( saveImageAct );
    fileMenu_->addAction( savePSAct );
    fileMenu_->addAction( saveEPSAct );
    fileMenu_->addAction( savePDFAct );
    // Disable TeX for now
    //fileMenu_->addAction( saveTexAct );
    fileMenu_->addSeparator();
    fileMenu_->addAction( saveOrbitalPicturesAct );
    fileMenu_->addSeparator();
    fileMenu_->addAction( exitAct );

    // Edit
    editMenu_->addAction( unselectAllAction_ );
    editMenu_->addAction( deleteMoleculeAction_ );
    editMenu_->addAction( clearAction_ );
    editMenu_->addSeparator();
    editMenu_->addAction( resetMoleculeAction_ );
    editMenu_->addAction( resetCameraAct );

    // Interaction
    interactionMenu_->setObjectName( "Interaction Menu" );
	cameraInteractAction_->setObjectName( "Camera Interaction" );
	moleculeInteractAction_->setObjectName( "Molecule Interaction" );
	interactionMenu_->addAction( cameraInteractAction_ );
    interactionMenu_->addAction( moleculeInteractAction_ );
    interactionMenu_->addAction( pickAtomAction_ );
    interactionMenu_->addSeparator();

    QAction* action = 0;
    action = interactionMenu_->addAction(tr("Event Recorder..."));
    connect(action, SIGNAL(triggered()), this, SLOT(ScriptRecordSlot()));

    action = interactionMenu_->addAction(tr("Event Player..."));
    connect(action, SIGNAL(triggered()), this, SLOT(ScriptPlaySlot()));


    // Display
    displayMenu_->addAction( moleculeDisplayAction_ );
    displayMenu_->addAction( backGroundColorAct_ );
    displayMenu_->addAction( editViewPropertiesAct );
    displayMenu_->addAction( planeProbeVisibilityAction_ );
    displayMenu_->addAction( mepScalarBarAction_ );
    displayMenu_->addAction( probeScalarBarAction_ );
    displayMenu_->addAction( axesAction_ );
    displayMenu_->addAction( bboxAction_ );
    displayMenu_->addSeparator();
    displayMenu_->addAction( shadersAction_ );

    // Quick display
    QMenu* qdisp = new QMenu( "Display style" );
    QAction* qd = new QAction( "Spacefill", this );
    qdisp->addAction( qd );
    connect( qd, SIGNAL( triggered() ), this, SLOT( QuickDisplaySpacefillSlot() ) );
    qd = new QAction( "Liquorice", this );
    qdisp->addAction( qd );
    connect( qd, SIGNAL( triggered() ), this, SLOT( QuickDisplayLiquoriceSlot() ) );
    qd = new QAction( "Ball and stick", this );
    qdisp->addAction( qd );
    connect( qd, SIGNAL( triggered() ), this, SLOT( QuickDisplayBStickSlot() ) );
    qd = new QAction( "Wireframe", this );
    qdisp->addAction( qd );
    connect( qd, SIGNAL( triggered() ), this, SLOT( QuickDisplayWireframeSlot() ) );
    styleButton_ = new QPushButton( "Style" );
    styleButton_->setMenu( qdisp );
    mainToolBar_->addSeparator();
    mainToolBar_->addWidget( styleButton_ );
    styleButton_->setEnabled( false );

    // Animation
    animationMenu_->addAction( animationAction_ );
    animationMenu_->addAction( nextFrameAction_ );
    animationMenu_->addAction( previousFrameAction_ );
    animationMenu_->addSeparator();
    animationMenu_->addAction( timestepAct );
    animationMenu_->addAction( moleculeAnimationAction_ );
    animationMenu_->addAction( exportVideoAction_ );

    // Surfaces
    surfacesMenu_->addAction( orbitalsAction_ );
    surfacesMenu_->addAction( gridDataSurfaceAction_ );
    surfacesMenu_->addAction( sasAction_ );
    surfacesMenu_->addAction( connollySurfaceAction_ );
    /// @warning seems not to work on Mac with top menubar items
    /// surfacesMenu_->setEnabled( false );

    // Analysis
    analysisMenu_->addAction( imagePlaneProbeAction_ );
    //analysisMenu_->addAction( positionProbeAction_ );
    analysisMenu_->addAction( distanceAction_ );
    analysisMenu_->addAction( angleAction_ );
    analysisMenu_->addAction( dihedralAngleAction_ );
    //analysisMenu_->addAction( clipPlaneAction_ );
    /// @warning seems not to work on Mac with top menubar items
    //analysisMenu_->setEnabled( false );
    analysisMenu_->addAction( irSpectrumAction_ );
    // View menu
    viewMenu_->addAction( viewWorkspaceAction_ );
    viewMenu_->addAction( viewMoleculePropAction_ );
    viewMenu_->addAction( viewToolbarAction_ );

    // Help
    helpMenu_->addAction( docAct );
    helpMenu_->addAction( molekelWebsiteAct );
    helpMenu_->addSeparator();
    helpMenu_->addAction( aboutAct );
    helpMenu_->addAction( licenseAct );

}

//------------------------------------------------------------------------------
void MainWindow::CreateMenus()
{
    fileMenu_ = menuBar()->addMenu( QString( "&File" ) );
    menuBar()->addSeparator();
    editMenu_ = menuBar()->addMenu( QString( "&Edit" ) );
    menuBar()->addSeparator();
    displayMenu_ = menuBar()->addMenu( QString( "Display" ) );
    assert( displayMenu_ );
    displayMenu_->setTearOffEnabled( true );
    menuBar()->addSeparator();
    interactionMenu_ = menuBar()->addMenu( QString( "&Interaction" ) );
    assert( interactionMenu_ );
    interactionMenu_->setTearOffEnabled( true );
    menuBar()->addSeparator();
    animationMenu_ = menuBar()->addMenu( QString( "Animation" ) );
    assert( animationMenu_ );
    animationMenu_->setTearOffEnabled( true );
    menuBar()->addSeparator();
    surfacesMenu_ = menuBar()->addMenu( QString( "Surfaces" ) );
    menuBar()->addSeparator();
    analysisMenu_ = menuBar()->addMenu( QString( "Analysis" ) );
    menuBar()->addSeparator();
    viewMenu_ = menuBar()->addMenu( QString( "View" ) );
    menuBar()->addSeparator();
    helpMenu_ = menuBar()->addMenu( QString( "&Help" ) );

}

//------------------------------------------------------------------------------
void MainWindow::MoleculeDisplaySlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    typedef MoleculeRenderingStyleWidget::PersistentSettingsKeys PSK;
    MoleculeRenderingStyleWidget mw( this,
                                     data_->GetMolecule( lastSelectedMolecule_ ),
                                     PSK( MOLECULE_ATOM_DETAIL_KEY,
                                          MOLECULE_BOND_DETAIL_KEY,
                                          MOLECULE_ATOM_SCALING_KEY,
                                          MOLECULE_BOND_SCALING_KEY,
                                          MOLECULE_DISPLAY_STYLE_KEY,
                                          MOLECULE_ATOM_DISPLAY_STYLE_KEY,
                                          MOLECULE_BOND_DISPLAY_STYLE_KEY),
                                     this );
    mw.setWindowTitle( QString( "Display - " ) +
            data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() );
    mw.exec();
}

//------------------------------------------------------------------------------
void MainWindow::ShadersSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    ShadersDialog d( this, data_->GetMolecule( lastSelectedMolecule_ ), this );
    d.setWindowTitle( QString( "Shaders - " ) +
            data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() );
    d.exec();
}


//------------------------------------------------------------------------------
/// Class used to load molecular data and add data to the database in a separate
/// thread. This is required because both OpenBabel and OpenMOIV are slow at
/// loading data (most of the time spent in computing bonds).
/// On a Xeon 3.0 Ghz 4GB RAM, 1AON.pdb:
/// - OpenBabel 2.0.1: 30s to load and compute bonds.
/// - OpenMOIV 1.0.3: 20s to load and compute bonds.
class FileLoadThread : public QThread
{
    /// Reference to molecular database.
    MolekelData* data_;
    /// Index of last loaded molecule.
    MolekelData::IndexType loadedMoleculeId_;
    /// Path of last loaded file.
    string fileName_;
    /// Format.
    string format_;
    /// Reference to renderer into which molecule will be added.
    vtkRenderer* renderer_;
public:
    FileLoadThread( QObject* parent,
                    MolekelData* data )
                            : QThread( parent ),
                              data_( data ),
                              loadedMoleculeId_( MolekelData::InvalidIndex() )
                    { setTerminationEnabled(); }
    /// Overridden run() method: this is the code that runs in a separate thread.
    void run()
    {
        loadedMoleculeId_ = MolekelData::InvalidIndex();
        if( format_.size() )
        {
            loadedMoleculeId_ = data_->AddMolecule( fileName_.c_str(), format_.c_str(), renderer_, 0, true );
        }
        else loadedMoleculeId_ = data_->AddMolecule( fileName_.c_str(), renderer_, 0, true );
    }

    /// Set reference to data.
    void SetData( MolekelData* data ) { data_ = data; }
    /// Returns id of last loaded molecule: this method is intended to be invoked
    /// in the slot invoked upon thread termination.
    MolekelData::IndexType GetLoadedMoleculeId() const { return loadedMoleculeId_; }
    /// Returns file name of last loaded file.
    const string& GetFileName() const { return fileName_; }
    /// Initializes data and start thread execution.
    void LoadMolecule( const string& fname, const string& format, vtkRenderer* renderer )
    {
        fileName_ = fname;
        format_ = format;
        renderer_ = renderer;
        start();
    }
};

//------------------------------------------------------------------------------
void MainWindow::LoadFileSlot()
{
    assert( data_ && "data_ is NULL" );
    // do not allow file loading if animtion in progress.
    if( AnimationStarted() )
    {
        QMessageBox::information( this, QString( "Load File" ), QString( "Animation in progress: stop animation first" ) );
        return;
    }
    QSettings settings;
    QString dir = settings.value( IN_DATA_DIR_KEY.c_str(),
                                  QCoreApplication::applicationDirPath() ).toString();

    const MolekelData::FileFormats& ff = data_->GetSupportedFileFormats();
    ostringstream stringBuf;
    const char separator[] = " - ";
    for( MolekelData::FileFormats::const_iterator ffi = ff.begin();
         ffi != ff.end();
         ++ffi )
    {
        if( !ffi->CanRead() ) continue;
        stringBuf << ffi->GetFormat() 	   << separator
                  << ffi->GetDescription() << separator
                  << '('
                  << ffi->GetExtensionsAsString()
                  << ')'
                  << ";;";
    }
    const string defaultFilter( "All Files - (*.*)" );
    stringBuf << defaultFilter;
    QString selectedFilter( defaultFilter.c_str() );
    QString fileName = GetOpenFileName( this,
                                        QString( "Load molecule" ),
                                        dir,
                                        stringBuf.str().c_str(),
                                        &selectedFilter,
                                        0,
                                        defaultFilter.c_str() );
    if( !fileName.isEmpty() )
    {
        settings.setValue( IN_DATA_DIR_KEY.c_str(), DirPath( fileName ) );
        LoadMolecule( fileName, selectedFilter );
    }
}

//------------------------------------------------------------------------------
void MainWindow::LoadMolecule( const QString& fileName,
                               const QString& filter )
{

    try
    {
        const char separator[] = " - ";
        const QString selectedFilter = filter.size() ? filter : "All Files - (*.*)";
        statusBar()->showMessage( QString( "Loading file %1..." ).arg( fileName ) );
        // find format
        const string filterString = selectedFilter.toStdString();
        const string formatString( filterString, 0, selectedFilter.toStdString().find( separator ) );
        // add molecule
        /// @warning No other option than explicitly terminating the thread to stop loading;
        /// OpenBabel and OpenMOIV do not support multithreaded operations with options
        /// to interrupt the computation.
        /// @warning on Mac OS X the following code doesn't work, the thread doesn't terminate
        /// and wait() waits until the run() method returns, basically the terminate() method
        /// seems to return immediately without stopping the thread.
        /// @code
        /// fileLoadThread_->terminate();
        /// // wait for thread to terminate
        /// fileLoadThread_->wait();
        /// @endcode
        /// Removing the wait() calls works but the thread is not actually terminated and upon exit
        /// the following message is printed to standard output:
        /// @code "QThread object destroyed while thread is still running." @endcode
        /// @warning On Mac OS X on top of the terminate()/wait() problem there is another issue;
        /// when creating the molecule in MolekelMolecule::New() in a separate thread the program
        /// crashes with a "bus error" message: a working solution is to perform a two step initialization:
        /// - load molecule in separate thread but do not add objects into OpenInventor scenegraph and
        ///   VTK renderer
        /// - emit signal which will be received by MainWindow::MoleculeLoadedSlot()
        /// - call MolekelMolecule::Initialize() method to create IV scenegraph and add object to VTK renderer.
        /// this solution has been tested and works but since the thread cannot be terminated on Mac OS X anyway
        /// multithreaded loading of molecules will be disabled on Mac OS X.
        /// @warning on SuSE Linux 10.1 Intel, 32bit problems similar to Mac OS X arise: most of the time the
        /// program crashes while loading the molecule printing the following message:
        /// @code
        /// FATAL: exception not rethrown
        /// ABORT
        /// @endcode
        /// A two steps initialization would probably work
        /// To enable multithreaded loading #define MOLEKEL_MULTITHREADED_DATA_LOADING.
#ifdef MOLEKEL_MULTITHREADED_DATA_LOADING
        if( fileLoadThread_ == 0 )
        {
            fileLoadThread_ = new FileLoadThread( this, data_ );
            connect( fileLoadThread_, SIGNAL( finished() ), this, SLOT( MoleculeLoadedSlot() ),
                     Qt::QueuedConnection ); // we want MoleculeLoadedSlot() to run in main thread
        }
        else
        {
            fileLoadThread_->setParent( this );
            fileLoadThread_->SetData( data_ );
        }

        if( formatString != "All Files" )
        {
            fileLoadThread_->LoadMolecule( fileName.toStdString().c_str(), formatString.c_str(), vtkRenderer_ );
        }
        else fileLoadThread_->LoadMolecule( fileName.toStdString().c_str(), string(), vtkRenderer_ );
        if( loadFileMessageBox_->exec() == QDialog::Rejected )
        {
            // no other option than explicitly terminaring the thread, OpenBabel and OpenMOIV
            // do not support multithreaded operations with options to interrupt the computation
            fileLoadThread_->terminate();
            // wait for thread to terminate
            fileLoadThread_->wait();
            // in case thread was terminated just after the molecule was added do not display
            // any message
            if( fileLoadThread_->GetLoadedMoleculeId() == MolekelData::InvalidIndex() )
            {
                 statusBar()->showMessage( QString( "Stopped loading %1" ).arg( fileName ) );

            }
        }
#else // MOLEKEL_MULTITHREADED_DATA_LOADING
        MolekelData::IndexType i = MolekelData::InvalidIndex();

        // implement callback to pass to data loading method
        class LoadMoleculeCB : public ILoadMoleculeCallback
        {
            MainWindow* mw_;
        public:
            LoadMoleculeCB( MainWindow* mw ) : mw_( mw ) {}
            void StatusMessage( const string& msg ) { mw_->DisplayStatusMessage( msg.c_str() ); }
        } cb( this );

        if( formatString != "All Files" )
        {
            i = data_->AddMolecule( fileName.toStdString().c_str(), formatString.c_str(),
                                    vtkRenderer_, &cb, true );
        }
        else i = data_->AddMolecule( fileName.toStdString().c_str(), vtkRenderer_, &cb, true );
        if( i != MolekelData::InvalidIndex() ) MoleculeLoaded( i, fileName );

#endif // MOLEKEL_MULTITHREADED_DATA_LOADING
    }
    catch( const exception& ex )
    {
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                               QMessageBox::Ok, QMessageBox::NoButton );

        statusBar()->showMessage( QString( "Error loading file" ) );
    }
}

//------------------------------------------------------------------------------
#ifdef MOLEKEL_MULTITHREADED_DATA_LOADING
void MainWindow::MoleculeLoadedSlot()
#else // MOLEKEL_MULTITHREADED_DATA_LOADING
void MainWindow::MoleculeLoaded( MolekelData::IndexType i, const QString& fileName )
#endif // MOLEKEL_MULTITHREADED_DATA_LOADING
{

#ifdef MOLEKEL_MULTITHREADED_DATA_LOADING
    assert( loadFileMessageBox_ );
    loadFileMessageBox_->done( QMessageBox::Yes );
    const MolekelData::IndexType i = fileLoadThread_->GetLoadedMoleculeId();
    // this happens when thread is terminated by user
    if( i == MolekelData::InvalidIndex() ) return;
    QString fileName( fileLoadThread_->GetFileName().c_str() );
#endif // MOLEKEL_MULTITHREADED_DATA_LOADING

    MolekelMolecule* mol = data_->GetMolecule( i );
    SelectMoleculeCB* smcb = new SelectMoleculeCB( this );
    typedef MoleculeActorPickCommand MolActorCmd;
    vtkSmartPointer< MolActorCmd > mc( MolActorCmd::New() );
    mc->SetMoleculeId( i );
    mc->SetCallback( smcb );
    mc->SetRenderWindowInteractor( vtkRenderer_->GetRenderWindow()->GetInteractor() );
    /// @note vtkSmartPointer uses static_cast internally !!! why ?
    /// use GetPointer() to get actual type
    mol->AddVtkObserver( vtkCommand::PickEvent, mc.GetPointer() );

    // add animators
    typedef AbstractAtomAnimator::AnimationMode AnimationMode;
    SwitchAtomAnimator< AnimationMode >* aa =
        new SwitchAtomAnimator< AnimationMode >();
    if( mol->GetNumberOfFrames() > 1 )
    {
        aa->SetAnimator( new AtomFramesAnimator, AbstractAtomAnimator::TRAJECTORY );
    }
    else
    {
        aa->SetAnimator( new AtomPositionAnimator, AbstractAtomAnimator::TRAJECTORY );
    }
    aa->SetAnimator( new AtomVibrationAnimator, AbstractAtomAnimator::VIBRATION );
    aa->SetCurrent( AbstractAtomAnimator::TRAJECTORY );
    mol->SetUpdater( aa );

    // set default appearance
    QSettings settings;
    mol->GetChemDisplayParam()->atomSphereComplexity.setValue(
        float( settings.value( MOLECULE_ATOM_DETAIL_KEY.c_str(),
                        mol->GetChemDisplayParam()->atomSphereComplexity.getValue() ).toDouble() )
                        );
    mol->GetChemDisplayParam()->bondCylinderComplexity.setValue(
        float( settings.value( MOLECULE_BOND_DETAIL_KEY.c_str(),
                        mol->GetChemDisplayParam()->bondCylinderComplexity.getValue() ).toDouble() )
                        );
    mol->GetChemDisplayParam()->ballStickSphereScaleFactor.setValue(
        float( settings.value( MOLECULE_ATOM_SCALING_KEY.c_str(),
                        mol->GetChemDisplayParam()->ballStickSphereScaleFactor.getValue() ).toDouble() )
                        );
    mol->GetChemDisplayParam()->bondCylinderRadius.setValue(
        float( settings.value( MOLECULE_BOND_SCALING_KEY.c_str(),
                        mol->GetChemDisplayParam()->bondCylinderRadius.getValue() ).toDouble() )
                        );
    if( mol->GetChemDisplayParam()->displayStyle.getValue() !=
            ChemDisplayParam::DISPLAY_WIREFRAME )
    {
         mol->GetChemDisplayParam()->displayStyle.setValue(
              ( settings.value( MOLECULE_DISPLAY_STYLE_KEY.c_str(),
                    mol->GetChemDisplayParam()->displayStyle.getValue() ).toInt() )
                    );
    }
    mol->GetChemDisplayParam()->atomSphereDisplayStyle.setValue(
        ( settings.value( MOLECULE_ATOM_DISPLAY_STYLE_KEY.c_str(),
               mol->GetChemDisplayParam()->atomSphereDisplayStyle.getValue() ).toInt() )
                        );
    mol->GetChemDisplayParam()->bondCylinderDisplayStyle.setValue(
        ( settings.value( MOLECULE_BOND_DISPLAY_STYLE_KEY.c_str(),
               mol->GetChemDisplayParam()->bondCylinderDisplayStyle.getValue() ).toInt() )
                        );

    
    // enable menu items
    clearAction_->setEnabled( true );
    UnselectAll();
    // following line not required anymore since status bar update is performed
    // in ILoadMoleculeCallback implementation passed to MolekelData::AddMolecule
    //DisplayStatusMessage( QString( "Loaded file %1" ).arg( fileName ) );

    // add item into tree widget if visible
    workspaceTreeDockWidget_->GetTreeWidget()->AddMolecule( mol, i );
    // refresh view
    vtkRenderer_->ResetCamera();

	// force camera interaction
	SetCameraInteractionSlot();

	// always select latest loaded molecule
	/*if( data_->GetNumberOfMolecules() == 1 )*/ SelectMolecule( i, 0, false, true );

    Refresh();
  
}

//------------------------------------------------------------------------------
void MainWindow::SaveFileSlot()
{
    /// @todo disable menu item if nothing selected
    // menu action calling this function has to be disabled if
    // no molecule is selected, it is therefore an error
    // if this function is called without selecting a molecule first
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex()
            && "No molecule selected" );
    QString selectedFilter;
    const MolekelData::FileFormats& ff = data_->GetSupportedFileFormats();
    ostringstream stringBuf;
    const char separator[] = " - ";
    for( MolekelData::FileFormats::const_iterator ffi = ff.begin();
         ffi != ff.end();
         ++ffi )
    {
        if( !ffi->CanWrite() ) continue;
        stringBuf << ffi->GetFormat() 	   << separator
                  << ffi->GetDescription() << separator
                  << '('
                  << ffi->GetExtensionsAsString()
                  << ')'
                  << ";;";
    }
    try
    {
        // open file dialog and get output file name with extension
        // default should be same extension as selected molecule
        QSettings settings;
        QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
        QString selectedFilter;
        QString fileName = GetSaveFileName( this, "Save molecule", dir, stringBuf.str().c_str(), &selectedFilter );
        if( fileName.isEmpty() ) return;
        if( selectedFilter.isEmpty() )
        {
        	data_->SaveMolecule( lastSelectedMolecule_,
                                 fileName.toStdString().c_str() );
        }
        else
        {
        	QStringList sl = selectedFilter.split( " ", QString::SkipEmptyParts );
        	data_->SaveMolecule( lastSelectedMolecule_,
                                 fileName.toStdString().c_str(),
                                 sl.begin()->toStdString().c_str() );
        }
        // save current directory path
        settings.setValue( OUT_DATA_DIR_KEY.c_str(), DirPath( fileName ) );    }

    catch( const exception& ex )
    {
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                               QMessageBox::Ok, QMessageBox::NoButton );
    }
}

//------------------------------------------------------------------------------
void MainWindow::SetCameraInteractionSlot()
{
    SetInteractionMode( INTERACT_WITH_CAMERA );
    cameraInteractAction_->setChecked( true );
    moleculeInteractAction_->setChecked( false );
    pickAtomAction_->setEnabled( false );
    DisplayStatusMessage( "Camera interaction" );
}

//------------------------------------------------------------------------------
void MainWindow::SetMoleculeInteractionSlot()
{
    SetInteractionMode( INTERACT_WITH_MOLECULE );
    cameraInteractAction_->setChecked( false );
    moleculeInteractAction_->setChecked( true );
    pickAtomAction_->setEnabled( true );
    DisplayStatusMessage( "Molecule interaction" );
}

//------------------------------------------------------------------------------
void MainWindow::AtomPickingToggledSlot( bool on )
{
    if( on ) SetPickingMode( PICK_ATOMS_AND_BONDS );
    else SetPickingMode( PICK_MOLECULE );
}

//------------------------------------------------------------------------------
void MainWindow::AboutSlot()
{
    int maj, min, build, patch;
    GetMolekelVersionInfo( maj, min, patch, build );
    const char* type = GetMolekelVersionType();
    const char* copyright = GetMolekelCopyrightInfo();
    QMessageBox::about( this,
                        QString( "Molekel %1.%2" ).arg( maj ).arg( min ),
                        QString( "<b>Molekel: %1.%2.%3.%4 %5</b><p>" ).arg( maj )
                                                     .arg( min )
                                                     .arg( patch )
                                                     .arg( build )
                                                     .arg(type) + QString( "Build date: " ) +
                                                     GetMolekelBuildDate() +
                                                     QString( "<p>" ) + copyright );
}

//------------------------------------------------------------------------------
namespace
{
    struct RemoveFromRenderer
    {
        vtkRenderer* const pr;
        RemoveFromRenderer( vtkRenderer* r ) : pr( r ) {}
        void operator()( MolekelMolecule* m ) const { pr->RemoveActor( m->GetAssembly() ); }
    };
}
void MainWindow::ClearSlot()
{
    assert( data_ && "NULL data" );
    const int clear = QMessageBox::question( this, QString( "Clear" ), QString( "Delete all molecules ?" ),
                                           QMessageBox::Yes, QMessageBox::No );
    if( clear == QMessageBox::Yes )
    {
        UnselectAll();
        data_->Apply( ( RemoveFromRenderer( vtkRenderer_ ) ) );
        data_->Clear();
        workspaceTreeDockWidget_->GetTreeWidget()->Clear();
        if( AnimationStarted() ) AnimationSlot(); // stop animation
    }
    Refresh();
}

//------------------------------------------------------------------------------
void MainWindow::UnselectAllSlot()
{
    UnselectAll();
    Refresh();
}

//------------------------------------------------------------------------------
void MainWindow::DeleteMoleculeSlot()
{
    assert( data_ && "NULL data" );
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() && "Invalid selection" );
    MolekelMolecule* mol = data_->GetMolecule( lastSelectedMolecule_ );

    const int del = QMessageBox::question( this, QString( "Delete Molecule" ),
                                            QString( "Delete molecule %1 ?" ).arg( mol->GetFileName().c_str() ),
                                            QMessageBox::Yes, QMessageBox::No );

    if( del != QMessageBox::Yes ) return;

    vtkRenderer_->RemoveActor( mol->GetAssembly() );
    data_->RemoveMolecule( lastSelectedMolecule_ );

    workspaceTreeDockWidget_->GetTreeWidget()->RemoveMolecule( lastSelectedMolecule_ );

    UnselectAll();

    if( data_->GetNumberOfMolecules() == 0 )
    {
        clearAction_->setEnabled( false );
        unselectAllAction_->setEnabled( false );
        if( AnimationStarted() ) AnimationSlot(); // stop animation if last molecule
    }
    Refresh();
}


//------------------------------------------------------------------------------
void MainWindow::OrbitalsSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex()
            && "Invalid molecule index" );
    // show orbital dialog
    ComputeElDensSurfaceDialog dlg( data_->GetMolecule( lastSelectedMolecule_ ), this,
                                    this, mepLUT_ );
    dlg.setWindowTitle( QString( "Electron Density - %1" )
        .arg( data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() ) );
    dlg.exec();
    workspaceTreeDockWidget_->GetTreeWidget()->UpdateMoleculeItem( lastSelectedMolecule_ );
}


//------------------------------------------------------------------------------
void MainWindow::SaveSnapshotSlot()
{

    try
    {
    
    	QSettings settings;
    	QString dir = settings.value( OUT_SNAPSHOTS_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
    	
    	ostringstream os;
    	os << PNG_FILE_FORMAT << ";;" << TIFF_FILE_FORMAT << ";;"; 
    	QString selectFilter( PNG_FILE_FORMAT );
        unsigned magFactor = 1;
    	QString fileName = GetSaveImageFileName( magFactor, this, "Save image", dir,
    										os.str().c_str(),
    										&selectFilter,
    										0,
    										selectFilter );
    	
    	if( fileName.isEmpty() ) return;
    	if( selectFilter.startsWith( "png" ) ) SaveSnapshot( fileName, "png", magFactor );
    	else if( selectFilter.startsWith( "tiff" ) ) SaveSnapshot( fileName, "tiff", magFactor );
    	else throw std::runtime_error( "Invalid file format selected" );
    	settings.setValue( OUT_SNAPSHOTS_DATA_DIR_KEY.c_str(), DirPath( fileName ) );
    }
    catch( const exception& ex )
    {
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                                   QMessageBox::Ok, QMessageBox::NoButton );
    }
}

//------------------------------------------------------------------------------
void MainWindow::SaveToPSSlot()
{

    try
    {
        // open file dialog and get output file name with extension
        // default should be same extension as selected molecule
        QSettings settings;
        QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
        //QString fileName = QFileDialog::getSaveFileName( this, "Save to postscript", dir );
        QString fileName = GetSaveFileName( this, "Save to postscript", dir );
        if( fileName.isEmpty() ) return;
        DisplayStatusMessage( QString( "Exporting PostScript to file %1..." ).arg( fileName ) );
        vtkSmartPointer< vtkGL2PSExporter > psexp( vtkGL2PSExporter::New() );
        psexp->SetRenderWindow( vtkRenderWindow_ );
        psexp->SetFileFormatToPS();
        psexp->CompressOff();
        psexp->SetSortToBSP();
        if( fileName.endsWith( ".ps" ) || fileName.endsWith( ".PS" ) )
        {
           fileName.chop( 3 );
        }
        psexp->SetFilePrefix( fileName.toStdString().c_str() );
        Refresh();
        psexp->Write();
        DisplayStatusMessage( QString( "Exported PostScript to file %1" ).arg( fileName ) );
    }
    catch( const exception& ex )
    {
        DisplayStatusMessage( QString( "Error exporting PostScript" ) );
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                                   QMessageBox::Ok, QMessageBox::NoButton );
    }
}

//------------------------------------------------------------------------------
void MainWindow::SaveToPDFSlot()
{

    try
    {
        // open file dialog and get output file name with extension
        // default should be same extension as selected molecule
        QSettings settings;
        QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
        //QString fileName = QFileDialog::getSaveFileName( this, "Save to pdf", dir );
        QString fileName = GetSaveFileName( this, "Save to pdf", dir );
        if( fileName.isEmpty() ) return;
        DisplayStatusMessage( QString( "Exporting PDF to file %1..." ).arg( fileName ) );
        vtkSmartPointer< vtkGL2PSExporter > psexp( vtkGL2PSExporter::New() );
        psexp->SetRenderWindow( vtkRenderWindow_ );
        psexp->SetFileFormatToPDF();
        psexp->CompressOff();
        psexp->SetSortToBSP();
        if( fileName.endsWith( ".pdf" ) || fileName.endsWith( ".PDF" ) )
        {
           fileName.chop( 4 );
        }
        psexp->SetFilePrefix( fileName.toStdString().c_str() );
        Refresh();
        psexp->Write();
        DisplayStatusMessage( QString( "Exported PDF to file %1" ).arg( fileName ) );
    }
    catch( const exception& ex )
    {
        DisplayStatusMessage( QString( "Error exporting PDF" ) );
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                                   QMessageBox::Ok, QMessageBox::NoButton );
    }
}

//------------------------------------------------------------------------------
void MainWindow::SaveToEPSSlot()
{

    try
    {
        // open file dialog and get output file name with extension
        // default should be same extension as selected molecule
        QSettings settings;
        QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
        //QString fileName = QFileDialog::getSaveFileName( this, "Save to EPS", dir );
        QString fileName = GetSaveFileName( this, "Save to EPS", dir );
        if( fileName.isEmpty() ) return;
        DisplayStatusMessage( QString( "Exporting Encapsulated PostScript to file %1..." ).arg( fileName ) );
        vtkSmartPointer< vtkGL2PSExporter > psexp( vtkGL2PSExporter::New() );
        psexp->SetRenderWindow( vtkRenderWindow_ );
        psexp->SetFileFormatToEPS();
        psexp->CompressOff();
        psexp->SetSortToBSP();
        if( fileName.endsWith( ".eps" ) || fileName.endsWith( ".EPS" ) )
        {
           fileName.chop( 4 );
        }
        psexp->SetFilePrefix( fileName.toStdString().c_str() );
        Refresh();
        psexp->Write();
        DisplayStatusMessage( QString( "Exported Encapsulated PostScript to file %1" ).arg( fileName ) );
    }
    catch( const exception& ex )
    {
        DisplayStatusMessage( QString( "Error exporting Encapsulated PostScript" ) );
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                                   QMessageBox::Ok, QMessageBox::NoButton );
    }
}


//------------------------------------------------------------------------------
void MainWindow::SaveToTeXSlot()
{

    try
    {
        // open file dialog and get output file name with extension
        // default should be same extension as selected molecule
        QSettings settings;
        QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
        //QString fileName = QFileDialog::getSaveFileName( this, "Save to TeX", dir );
        QString fileName = GetSaveFileName( this, "Save to TeX", dir );
        if( fileName.isEmpty() ) return;
        DisplayStatusMessage( QString( "Exporting TeX to file %1..." ).arg( fileName ) );
        vtkSmartPointer< vtkGL2PSExporter > psexp( vtkGL2PSExporter::New() );
        psexp->SetRenderWindow( vtkRenderWindow_ );
        psexp->SetFileFormatToTeX();
        psexp->CompressOff();
        psexp->SetSortToBSP();
        psexp->SetFilePrefix( fileName.toStdString().c_str() );
        Refresh();
        psexp->Write();
        DisplayStatusMessage( QString( "Exported TeX to file %1" ).arg( fileName ) );
    }
    catch( const exception& ex )
    {
        DisplayStatusMessage( QString( "Error exporting TeX" ) );
        QMessageBox::critical( this, QString( "I/O Error" ), QString( ex.what() ),
                                   QMessageBox::Ok, QMessageBox::NoButton );
    }
}
//------------------------------------------------------------------------------
void MainWindow::BackgroundColorSlot()
{
    const double* bk = vtkRenderer_->GetBackground();
    static const int CMAX = 255;
    QColor bc( int( bk[ 0 ] * CMAX ), int( bk[ 1 ] * CMAX ) , int( bk[ 2 ] * CMAX ) );
    QColor bkColor = QColorDialog::getColor( bc, this );
    if( !bkColor.isValid() ) return;
    QSettings s;
    s.setValue( BACKGROUND_COLOR_KEY.c_str(), bkColor );
    SetBkColor( bkColor );
}

//------------------------------------------------------------------------------
void MainWindow::AxesSlot()
{
    // if axes visible showAxes_ == true and menu item == "Hide Axes"
    if( axesAction_->text() == QString( "Hide Axes" ) )
    {
        vtkRenderWindow_->RemoveRenderer( vtkAxesRenderer_ );
        axesAction_->setText( QString( "Show Axes" ) );
    }
    else
    {
        vtkRenderWindow_->AddRenderer( vtkAxesRenderer_ );
        axesAction_->setText( QString( "Hide Axes" ) );
    }
    Refresh();
}

//------------------------------------------------------------------------------
namespace
{
    struct HideMoleculeBBox
    {
        void operator()( MolekelMolecule* m ) const
        {
            m->SetBBoxVisible( false );            
        }
    };
}

void MainWindow::BBoxSlot()
{
    // hide
    if( bboxAction_->text() == QString( "Hide Bounding Boxes" ) )
    {
       bboxAction_->setText( QString( "Show Bounding Boxes" ) );
       data_->Apply( HideMoleculeBBox() );
    }
    else // show
    {
       bboxAction_->setText( QString( "Hide Bounding Boxes" ) );
       if( lastSelectedMolecule_ != MolekelData::InvalidIndex() )
       {
    	   data_->GetMolecule( lastSelectedMolecule_ )->SetBBoxVisible( true );
       }
    }
    Refresh();
}

//------------------------------------------------------------------------------
bool MainWindow::ShowBoundingBox() const
{
	return bboxAction_->text() == QString( "Hide Bounding Boxes" );
}


//------------------------------------------------------------------------------
void MainWindow::AnimationSlot()
{
    if( !AnimationStarted() )
    {
        StartAnimation( true ); // init and start timer
        nextFrameAction_->setEnabled( false );
        previousFrameAction_->setEnabled( false );
    }
    else
    {
        StopAnimation();
        nextFrameAction_->setEnabled( true );
        previousFrameAction_->setEnabled( true );
        Refresh();
    }
}

//------------------------------------------------------------------------------
void MainWindow::NextFrameSlot()
{
    if( !AnimationStarted() ) StartAnimation( false );
    static const bool FORWARD = true;
    UpdateAnimation( FORWARD );
}

//------------------------------------------------------------------------------
void MainWindow::PreviousFrameSlot()
{
    if( !AnimationStarted() ) StartAnimation( false );
    static const bool BACKWARD = false;
    UpdateAnimation( BACKWARD );
}


//------------------------------------------------------------------------------
void MainWindow::TimestepSlot()
{
    TimeStepDialog d( this, this );
    d.setWindowTitle( QString( "Animation Time Step" ) );
    d.exec();
}

//------------------------------------------------------------------------------
void MainWindow::MoleculeAnimationSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() &&
            "Invalid molecule index" );
    typedef SwitchAtomAnimator< AbstractAtomAnimator::AnimationMode > AtomAnimType;
    AtomAnimType* saa = dynamic_cast< AtomAnimType* >(
             data_->GetMolecule( lastSelectedMolecule_ )->GetUpdater() );
    assert( saa );
    AtomTrajectoryAnimator* ata = dynamic_cast< AtomTrajectoryAnimator* >(
        saa->GetAnimator( AbstractAtomAnimator::TRAJECTORY ) );
    assert( ata );
    AtomVibrationAnimator* ava = dynamic_cast< AtomVibrationAnimator* >(
        saa->GetAnimator( AbstractAtomAnimator::VIBRATION ) );
    assert( ava );
    MoleculeAnimationDialog d( this, data_->GetMolecule( lastSelectedMolecule_ ),
                               saa, ava, ata, this );

    d.setWindowTitle( QString( "Animation Preferences" ) + QString( " - " ) +
                      data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() );
    d.exec();

}

//------------------------------------------------------------------------------
/// Surface updater: simply invokes <code>MolekelMolecule::RecomputeSurfaces()</code>
/// each time operator() is called.
class UpdateSurfaces
{
    double bboxFactor_;
    double step_;
    string msmsExecutable_;
    string msmsInFile_;
    string msmsOutFile_;
    double probeRadius_;
    double density_;
    bool mapMepOnOrbitals_;
    bool mapMepOnElDens_;
    bool mapMepOnSES_;
    bool bothSigns_;
    bool nodalSurface_;
public:
    UpdateSurfaces( const string& msms,
                    const string& msmsInFile,
                    const string& msmsOutFile,
                    double bbf = 1.0, double step = 0.1,
                    double pr = 1.4, double d = 10., bool mo = false, bool me = true,
                    bool ms = true, bool bs = false, bool ns = false )
                    : bboxFactor_( bbf ), step_( step ), msmsExecutable_( msms),
                      msmsInFile_( msmsInFile ), msmsOutFile_( msmsOutFile ),
                      probeRadius_( pr ), density_( d ), mapMepOnOrbitals_( mo ),
                      mapMepOnElDens_( me ), mapMepOnSES_( ms ),
                      bothSigns_( bs ), nodalSurface_( ns ) {}


    void operator()( MolekelMolecule* m ) const
    {
        m->RecomputeSurfaces( bboxFactor_, step_, msmsExecutable_,
                              msmsInFile_, msmsOutFile_,
                              probeRadius_, density_, mapMepOnOrbitals_,
                              mapMepOnElDens_, mapMepOnSES_,
                              bothSigns_, nodalSurface_ );
    }
};

/// Function object: returns the max number of frames to export.
class FindMaxFrames
{
    mutable unsigned int& frames_;
public:
    FindMaxFrames( unsigned int& frames ) : frames_( frames ) {}
    void operator()( MolekelMolecule* m ) const
    {
        const unsigned int f =  m->GetMolekelMolecule() ?
                                m->GetMolekelMolecule()->dynamics.ntotalsteps
                                : 0;
        frames_ = max( frames_, max( f, ( unsigned int )( m->GetNumberOfFrames() ) ) );
    }
};

/// @warning cannot use vtkMPEG2Writer (VTK 5.0.1) depending on the window size
/// it might fail with an error:
/// "...vtkDoubleArray (...): Unable to allocate ... elements of size 8 bytes."
/// @warning vtkAVIWriter always outputs uncompressed video at 15fps.
/// @note best solution to create animation is to save individual frames to a directory
/// and then use mencoder to generate the animation specifying frame rate, compression
/// parameters and codec on the command line, all options that are not available in VTK.
/// Sample command line to generate mpeg video from sequence of PNG images:
/// <code>C:\tmp\molekelvideo1>c:\programs\mplayer\mencoder.exe mf://c:\tmp\molekelvideo1\
/// *.png -o m.mpg -ovc lavc -lavcopts vcodec=mpeg2video -of mpeg</endcode>
/// Frame rate can be chaged with the -ofps options.
/// Usefule links:
/// - http://web.njit.edu/all_topics/Prog_Lang_Docs/html/mplayer/encoding.html
/// - http://www.cscs.ch/~mvalle/mencoder/index.html
void MainWindow::ExportVideoSlot()
{
    if( AnimationStarted() ) return;

    // find mulecule with maximum number of frames
    unsigned int f = 0;
    FindMaxFrames fmf( f );
    data_->Apply( fmf );
    const int defaultFPS = 15;
    // set default FPS to 15 and total time to FPS * Max Number Of Frames
    ExportAnimationDialog d( this, defaultFPS,  int( float( f / defaultFPS ) + 0.5f ) );
    d.setWindowTitle( QString( "Export Animation Options " ) );
    d.exec();

    if( d.result() == QDialog::Accepted )
    {
        QSettings settings;
        QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
        QString fileName;

        if( !d.SaveFrames() ) // on platform other than windows d.SaveFrames() is always true
        {
            //fileName = QFileDialog::getSaveFileName( this, "Save Video to AVI", dir );
            fileName = GetSaveFileName( this, "Save Video to AVI", dir );
            if( fileName.isEmpty() ) return;
        }
        else
        {
            //QString d = QFileDialog::getExistingDirectory( this, "Select directory where frames will be saved", dir );
            QString d = GetExistingDirectory( this, "Select directory where frames will be saved", dir );
            if( d.isEmpty() ) return;
            fileName = d + "/molekel_frame_";
        }
        const int numFrames =  int( d.GetTotalTime() * d.GetFPS() + 0.5 );
        if( !numFrames ) return;
        const string msmsInFilePath = GetTemporaryFileName();
        const string msmsOutFilePath = GetTemporaryFileName();
        // handle error
        if( msmsInFilePath.empty() || msmsOutFilePath.empty() )
        {
            QMessageBox::critical( this, "Error generating animation", "Cannot create temporary files" );
            return;
        }

        UpdateSurfaces updateSurfaces( GetMSMSExecutablePath().toStdString(),
                                       msmsInFilePath, msmsOutFilePath,
                                       d.GetBBoxScalingFactor(), d.GetStep(),
                                       d.GetSESProbeRadius(), d.GetSESDensity(),
                                       d.MapMEPOnOrbitals(), d.MapMEPOnElDensSurface(),
                                       d.MapMEPOnSES(), d.UseBothSigns(), d.GenerateNodalSurface() );
        StartAnimation( false );
        
        // Each element of the padding array is set to 6  - <num digits> so that if e.g. the frame
        // number is 22 (string length == 2) the selected 'padding' string
        // will be 0000 and the name will be something like 'molekel_frame_000022.png'
        static const char* PADDING[] = { "0", "00000", "0000", "000", "00", "0", "" };
        static const int MAX_PADDING_LENGTH = sizeof( PADDING ) / sizeof( const char* ) - 1;
        
        vtkSmartPointer< vtkPNGWriter > writer;
#ifdef WIN32 // AVI output enabled on windows only
        vtkSmartPointer< vtkAVIWriter > aviWriter;
        bool started = false;
#endif
        if( d.SaveFrames() )
        {
            writer = vtkPNGWriter::New();
        }
        else
        {
#ifdef WIN32 // on platform other than windows we never get to this point
            aviWriter = vtkAVIWriter::New();
            aviWriter->SetFileName( fileName.toStdString().c_str() );
#endif
        }
        // Set export animation flag; this flag can be set to false by clicking on the stop button
        // during an animation export. This flag is reset by StopAnimation
        exportAnimationInProgress_ = true;
        for( int i = 0; i != numFrames; ++i )
        {
            if( !exportAnimationInProgress_ ) break;
            statusBar()->showMessage( QString( "Taking snapshot of frame %1 of %2" )
                                        .arg( i + 1 )
                                        .arg( numFrames ) );
            static const bool FORWARD = true;
            UpdateAnimation( FORWARD );
            data_->Apply( updateSurfaces );
            Refresh();
            if( d.SaveFrames() )
            {
                writer->SetInput( GetSnapshot() );
                const QString num = QString( "%1" ).arg( i );
                const int paddingIndex = std::min( num.size(), MAX_PADDING_LENGTH ); 
                const QString fname = fileName + PADDING[ paddingIndex ] + num + ".png";
                writer->SetFileName( fname.toStdString().c_str() );
                writer->Update();
                writer->Write();
                Refresh();
            }
            else
            {
#ifdef WIN32 // enabled on windows only, on platforms other than windows
             // we never get here
                aviWriter->SetInput( GetSnapshot() );
                if( started == false )
                {
                    aviWriter->Start();
                    started = true;
                }
                aviWriter->Write();
                Refresh();
#endif
            }
            // process pending events this allows the user to stop the animation export
            QCoreApplication::processEvents();
        }
#ifdef WIN32
        if( !d.SaveFrames() ) aviWriter->End();
#endif
        StopAnimation();
        data_->Apply( updateSurfaces );
        DeleteFile( msmsInFilePath );
        DeleteFile( msmsOutFilePath );
        Refresh();
    }
}


//------------------------------------------------------------------------------
void MainWindow::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
    if( show3DViewSize_ )
    {
        const int w = vtkWidget_->size().width();
        const int h = vtkWidget_->size().height();
        statusBar()->showMessage( QString( "3D View size: %1 x %2" ).arg( w ).arg( h ) );
        if( w < h ) vtkRenderer_->GetActiveCamera()->UseHorizontalViewAngleOn();
        else vtkRenderer_->GetActiveCamera()->UseHorizontalViewAngleOff();
    }
}

//------------------------------------------------------------------------------
void MainWindow::timerEvent( QTimerEvent *event )
{
   static const bool FORWARD = true;
   UpdateAnimation( FORWARD );
}

//------------------------------------------------------------------------------
void MainWindow::UpdateMoleculePropertyWidget( MolekelMolecule* mol )
{
    assert( moleculePropertyDockWidget_ );
    if( !mol || !mol->GetOpenBabelMolecule() )
    {
        moleculePropertyDockWidget_->setWindowTitle( "Molecule Properties" );
        moleculePropertyDockWidget_->GetPropWidget()->ClearValues();
        return;
    }
    moleculePropertyDockWidget_->setWindowTitle( mol->GetFileName().c_str() + QString( " properties" ) );
    OpenBabel::OBMol* m = mol->GetOpenBabelMolecule();
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_TITLE, m->GetTitle() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_FRAMES, mol->GetNumberOfFrames() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_ATOMS, m->NumAtoms() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_BONDS, m->NumBonds() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_NON_H_ATOMS, m->NumHvyAtoms() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_RESIDUES, m->NumResidues() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_ENERGY, m->GetEnergy() );

    // Disabled: might crash on linux: with some pdb files a call to OBMol::GetFormula() causes the program to crash;
    // see bug 1632403 on OpenBabel website
    // moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_FORMULA, m->GetFormula().c_str() );

    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_STD_MOLAR_MASS, m->GetMolWt() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_EXACT_MASS, m->GetExactMass() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_TOTAL_CHARGE, m->GetTotalCharge() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_TOTAL_SPIN_MULT, m->GetTotalSpinMultiplicity() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_CONFORMERS, m->NumConformers() );
    // this takes forever (120s for 1AON.pdb on a 3.00 GHz Xeon). Disable for now.
    //moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_NUMBER_OF_ROTORS, m->NumRotors() );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_TRAJECTORY, mol->HasTrajectoryData() ? "Yes" : "No" );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_VIBRATION, mol->HasVibrationData() ? "Yes" : "No" );
    moleculePropertyDockWidget_->GetPropWidget()->SetValue( PROPS_DIPOLE_MOMENT, mol->HasDipoleMoment() ? "Yes" : "No" );
}

//------------------------------------------------------------------------------
void MainWindow::ViewWorkspaceSlot( bool checked )
{
    assert( workspaceTreeDockWidget_ );
    if( checked ) workspaceTreeDockWidget_->setVisible( true );
    else workspaceTreeDockWidget_->setVisible( false );
}

//------------------------------------------------------------------------------
void MainWindow::ViewMoleculePropSlot( bool checked )
{
    assert( moleculePropertyDockWidget_ );
    if( checked ) moleculePropertyDockWidget_->setVisible( true );
    else moleculePropertyDockWidget_->setVisible( false );
}

//------------------------------------------------------------------------------
void MainWindow::ViewToolbarSlot( bool checked )
{
    assert( mainToolBar_ );
    if( checked ) mainToolBar_->setVisible( true );
    else mainToolBar_->setVisible( false );
}

//------------------------------------------------------------------------------
void MainWindow::WorkspaceCloseEvent()
{
    viewWorkspaceAction_->toggle();
}

//------------------------------------------------------------------------------
void MainWindow::MoleculePropCloseEvent()
{
    viewMoleculePropAction_->toggle();
}

//------------------------------------------------------------------------------
void MainWindow::ImagePlaneProbeSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() &&
            "Invalid molecule index" );

    ImagePlaneProbeDialog pd( vtkImagePlaneWidget_,
                              data_->GetMolecule( lastSelectedMolecule_ ), this );
    pd.setWindowTitle(
        QString( "Plane Probe - " ) +
        data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() );
    // if plane probe is visible, hide probe, the following code assumes
    // that plane visibility and visibility action are in sync
    if( vtkImagePlaneWidget_->GetEnabled() ) planeProbeVisibilityAction_->trigger();
    pd.exec();
}

//------------------------------------------------------------------------------
void MainWindow::ResetMoleculeSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex()
            && "Invalid molecule index" );
    data_->GetMolecule( lastSelectedMolecule_ )->ResetTransform();
    Refresh();
    DisplayStatusMessage( "Reset molecule" );
}

//------------------------------------------------------------------------------
void MainWindow::ResetCameraSlot()
{

    vtkRenderer_->ResetCamera();
    Refresh();
    DisplayStatusMessage( "Reset camera" );
}


//------------------------------------------------------------------------------
void MainWindow::LicenseSlot()
{
extern const char* GetMolekelLicense();

    class LicenseDialog : public QDialog
    {
    public:
        LicenseDialog( const QString& licenseText, QWidget* parent ) : QDialog( parent )
        {
            QVBoxLayout* mainLayout = new QVBoxLayout;
            QTextDocument* textDoc = new QTextDocument;
            textDoc->setPlainText( licenseText ) ;
            QTextEdit* textEdit = new QTextEdit;
            textEdit->setDocument( textDoc );
            textEdit->setReadOnly( true );
            mainLayout->addWidget( textEdit );
            QPushButton* ok = new QPushButton( QString( "Ok" ) );
            connect( ok, SIGNAL( released() ), this, SLOT( accept() ) );
            mainLayout->addWidget( ok );
            setLayout( mainLayout );
            setWindowTitle( QString( "Molekel License" ) );
        }
    } dlg( GetMolekelLicense(), this );
    dlg.exec();
}

//------------------------------------------------------------------------------
void MainWindow::DocSlot()
{
//#if QT_VERSION >= 0x040200
//   QDesktopServices::openUrl( QUrl( QCoreApplication::applicationDirPath() +
//   #ifndef __APPLE_CC__
//   "/../doc/index.html" ) );
//   #else
//   "/../../../Doc/index.html" ) ); // root folder, same level as Molekel.app
//   #endif
//#endif
#if QT_VERSION >= 0x040200
    QDesktopServices::openUrl( QUrl( "http://molekel.cscs.ch/wiki/pmwiki.php/ReferenceGuide" ) );
#endif
}

//------------------------------------------------------------------------------
void MainWindow::MolekelWebsiteSlot()
{
#if QT_VERSION >= 0x040200
   QDesktopServices::openUrl( QUrl( "http://molekel.cscs.ch" ) );
#endif
}

//------------------------------------------------------------------------------
void MainWindow::GridDataSurfaceSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex()
            && "Invalid molecule index" );
    MolekelMolecule* mol = data_->GetMolecule( lastSelectedMolecule_ );
    GridDataSurfaceDialog dlg( mol, this, this );
    dlg.setWindowTitle( QString( "%1 - Surface from Grid Data" ).arg( mol->GetFileName().c_str() ) );
    dlg.exec();
    workspaceTreeDockWidget_->GetTreeWidget()->UpdateMoleculeItem( lastSelectedMolecule_ );
}

//------------------------------------------------------------------------------
void MainWindow::ToggleMEPScalarBarSlot()
{
    if( mepScalarBarAction_->text() == QString( "Show MEP Scalar Bar" ) )
    {
        // scalar bar is hidden, show it
        vtkMEPScalarBarWidget_->SetEnabled( true );
        mepScalarBarAction_->setText( QString( "Hide MEP Scalar Bar" ) );
    }
    else
    {
        // scalar bar is displayed, hide it
        vtkMEPScalarBarWidget_->SetEnabled( false );
        mepScalarBarAction_->setText( QString( "Show MEP Scalar Bar" ) );
    }

    Refresh();
}

//------------------------------------------------------------------------------
void MainWindow::ToggleProbeScalarBarSlot()
{
    if( probeScalarBarAction_->text() == QString( "Show Probe Scalar Bar" ) )
    {
        // scalar bar is hidden, show it
        vtkProbeScalarBarWidget_->SetEnabled( true );
        probeScalarBarAction_->setText( QString( "Hide Probe Scalar Bar" ) );
    }
    else
    {
        // scalar bar is displayed, hide it
        vtkProbeScalarBarWidget_->SetEnabled( false );
        probeScalarBarAction_->setText( QString( "Show Probe Scalar Bar" ) );
    }

    Refresh();
}

//------------------------------------------------------------------------------
void MainWindow::DihedralAngleSlot()
{
    assert( selectedAtoms_.GetSize() == 4 && "Wrong size of atom selection list" );
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    assert( lastSelectedMolecule_ == selectedAtoms_[ 0 ].GetMoleculeId() &&
            lastSelectedMolecule_ == selectedAtoms_[ 1 ].GetMoleculeId() &&
            lastSelectedMolecule_ == selectedAtoms_[ 2 ].GetMoleculeId() &&
            lastSelectedMolecule_ == selectedAtoms_[ 3 ].GetMoleculeId() );

    DisplayStatusMessage(
        QString( "Dihedral angle: %1" )
            .arg(
                    data_->GetMolecule( lastSelectedMolecule_ )->ComputeDihedralAngle(
                        selectedAtoms_[ 0 ].GetId(),
                        selectedAtoms_[ 1 ].GetId(),
                        selectedAtoms_[ 2 ].GetId(),
                        selectedAtoms_[ 3 ].GetId()
                    )
            )
    );

}


//------------------------------------------------------------------------------
void MainWindow::AddRemoveBondSlot()
{
	if( lastSelectedMolecule_ == MolekelData::InvalidIndex() ) return;

	//remove
	if( selectedBonds_.GetSize() == 1 )
	{
		if( lastSelectedMolecule_ != selectedBonds_[ 0 ].GetMoleculeId() ) return;
		data_->GetMolecule( lastSelectedMolecule_ )->RemoveBond( selectedBonds_[ 0 ].GetId() );
		DisplayStatusMessage( QString( "Removed bond %1" ).arg( selectedBonds_[ 0 ].GetId() ) );
		selectedBonds_.Clear();
	}
	//add
	else
	{
		if( lastSelectedMolecule_ != selectedAtoms_[ 0 ].GetMoleculeId() ||
            lastSelectedMolecule_ != selectedAtoms_[ 1 ].GetMoleculeId() ) return;
		data_->GetMolecule( lastSelectedMolecule_ )->AddBond(
							selectedAtoms_[ 0 ].GetId(),
							selectedAtoms_[ 1 ].GetId() );
		DisplayStatusMessage( QString( "Added bond" ) );
		selectedAtoms_.Clear();
	}
	Refresh();
}

//------------------------------------------------------------------------------
void MainWindow::AngleSlot()
{
    assert( selectedAtoms_.GetSize() == 3 && "Wrong size of atom selection list" );
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    assert( lastSelectedMolecule_ == selectedAtoms_[ 0 ].GetMoleculeId() &&
            lastSelectedMolecule_ == selectedAtoms_[ 1 ].GetMoleculeId() &&
            lastSelectedMolecule_ == selectedAtoms_[ 2 ].GetMoleculeId() );

    DisplayStatusMessage(
        QString( "Angle: %1" )
            .arg(
                    data_->GetMolecule( lastSelectedMolecule_ )->ComputeAngle(
                        selectedAtoms_[ 0 ].GetId(),
                        selectedAtoms_[ 1 ].GetId(),
                        selectedAtoms_[ 2 ].GetId()
                    )
            )
    );
}

//------------------------------------------------------------------------------
void MainWindow::DistanceSlot()
{
    assert( selectedAtoms_.GetSize() == 2 && "Wrong size of atom selection list" );
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    assert( lastSelectedMolecule_ == selectedAtoms_[ 0 ].GetMoleculeId() &&
            lastSelectedMolecule_ == selectedAtoms_[ 1 ].GetMoleculeId() );

    DisplayStatusMessage(
        QString( "Distance: %1" )
            .arg(
                    data_->GetMolecule( lastSelectedMolecule_ )->ComputeDistance(
                        selectedAtoms_[ 0 ].GetId(),
                        selectedAtoms_[ 1 ].GetId()
                    )
            )
    );
}


//------------------------------------------------------------------------------
void MainWindow::MeasureSlot()
{
    if( distanceAction_->isEnabled() ) DistanceSlot();
    else if( angleAction_->isEnabled() ) AngleSlot();
    else if( dihedralAngleAction_->isEnabled() ) DihedralAngleSlot();
}


//------------------------------------------------------------------------------
void MainWindow::TogglePlaneProbeSlot()
{
    if( planeProbeVisibilityAction_->text() == QString( "Show Plane Probe" ) )
    {
        // plane is hidden, show it
        vtkImagePlaneWidget_->SetEnabled( true );
        planeProbeVisibilityAction_->setText( QString( "Hide Plane Probe" ) );
    }
    else
    {
        // plane is visible displayed, hide it
        vtkImagePlaneWidget_->SetEnabled( false );
        planeProbeVisibilityAction_->setText( QString( "Show Plane Probe" ) );
    }

    Refresh();
}

//------------------------------------------------------------------------------
#include "dialogs/SesDialog.h"
void MainWindow::ConnollySurfaceSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    SesDialog dlg( this, data_->GetMolecule( lastSelectedMolecule_ ), this );
    dlg.setWindowTitle( QString( "Solvent Excluded Surface - " ) +
            data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() );
    dlg.exec();
    workspaceTreeDockWidget_->GetTreeWidget()->UpdateMoleculeItem( lastSelectedMolecule_ );
}

//------------------------------------------------------------------------------
void MainWindow::SasSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    SasDialog dlg( this, data_->GetMolecule( lastSelectedMolecule_ ), this );
    dlg.setWindowTitle( QString( "Solvent Accessible Surface - " ) +
            data_->GetMolecule( lastSelectedMolecule_ )->GetFileName().c_str() );
    dlg.exec();
    workspaceTreeDockWidget_->GetTreeWidget()->UpdateMoleculeItem( lastSelectedMolecule_ );
}

//------------------------------------------------------------------------------
void MainWindow::CloseSlot()
{

    QSettings settings;
    settings.setValue( MAIN_WINDOW_LAYOUT_KEY.c_str(), saveState() );
    settings.setValue( MAIN_WINDOW_WIDTH_KEY.c_str(), width() );
    settings.setValue( MAIN_WINDOW_HEIGHT_KEY.c_str(), height() );
    settings.setValue( MAIN_WINDOW_X_KEY.c_str(), pos().x() );
    settings.setValue( MAIN_WINDOW_Y_KEY.c_str(), pos().y() );

    recordEventsDlg_.close();
    playEventsDlg_.close();

    close();
}

//------------------------------------------------------------------------------
// Interaction methods
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void MainWindow::SelectMolecule( MolekelData::IndexType molId,
                                 const SoPickedPoint* pp,
                                 bool multiSelection,
                                 bool notify )
{
    try
    {
        MolekelMolecule* mol = data_->GetMolecule( molId );

        if( molId != lastSelectedMolecule_ ) UnselectAll();

        if( ShowBoundingBox() ) mol->SetBBoxVisible( true );

        if( mol->GetMEPLookupTable() != 0 ) SetMEPScalarBarLUT( mol->GetMEPLookupTable() );

        QString statusMessage = mol->GetFileName().c_str();

        if( pp == 0 )
        {
             statusMessage += " selected";
        }
        else
        {

			typedef SelectionList::SelectionInfo SelInfo;
            ChemDetail* detail = ( ChemDetail* ) pp->getDetail();
            assert( detail && "ChemDetail is NULL" );
            assert( mol->GetChemData() && "ChemData is NULL" );
            if( detail->getAtomIndex() >= 0 )
            {
				selectedBonds_.Clear();
                statusMessage += " - Atom: ";
                statusMessage += mol->GetChemData()->getAtomName( detail->getAtomIndex() ).getString();
                const SbVec3f coords = mol->GetChemData()->getAtomCoordinates( detail->getAtomIndex() );
                // if multiple selection enabled add atom to selection list
                // currently only supported for atoms to compute distances and angles
                bool added = false;
				if( multiSelection || selectedAtoms_.GetSize() == 0 )
                {
					
                    added = selectedAtoms_.AddRemove( SelectionList::SelectionInfo( lastSelectedMolecule_,
                                                                                    SelInfo::ATOM,
                                                                                    detail->getAtomIndex() ) );
                }
				else selectedAtoms_.Clear();

                QString c = QString( "  x: %1 y: %2 z: %3" )
                            .arg( coords[ 0 ] )
                            .arg( coords[ 1 ] )
                            .arg( coords[ 2 ] );
                statusMessage += c;
                if( multiSelection  )
                {
                    statusMessage += added ? QString( " - Added to selection list" ) :
                                             QString( " - Removed from selection list" );
                }
            }
            else if( detail->getBondIndex() >= 0 )
            {
				selectedAtoms_.Clear();
                statusMessage += " - Bond: ";
                const int from = mol->GetChemData()->getBondFrom( detail->getBondIndex() );
                const int to = mol->GetChemData()->getBondTo( detail->getBondIndex() );

                const SbVec3f coordsFrom = mol->GetChemData()->getAtomCoordinates( from );
                const SbVec3f coordsTo =  mol->GetChemData()->getAtomCoordinates( to );

                const string nameFrom = mol->GetChemData()->getAtomName( from ).getString();
                const string nameTo = mol->GetChemData()->getAtomName( to ).getString();
                QString f = QString( " From %1 at (%2, %3, %4)" )
                            .arg( nameFrom.c_str() )
                            .arg( coordsFrom[ 0 ] )
                            .arg( coordsFrom[ 1 ] )
                            .arg( coordsFrom[ 2 ] );
                QString t = QString( " To %1 at (%2, %3, %4)" )
                            .arg( nameTo.c_str() )
                            .arg( coordsTo[ 0 ] )
                            .arg( coordsTo[ 1 ] )
                            .arg( coordsTo[ 2 ] );

                statusMessage += f;
                statusMessage += t;
				selectedBonds_.Clear();
				selectedBonds_.Add( SelectionList::SelectionInfo( lastSelectedMolecule_,
																  SelInfo::BOND,
                                                                  detail->getBondIndex() ) );


            }
            if( detail->getResidueIndex() >= 0 )
            {
                statusMessage += " - Residue: ";
                statusMessage += mol->GetChemData()->getResidueName( detail->getResidueIndex() ).getString();
                statusMessage += QString( " %1" ).arg( detail->getResidueIndex() );
            }
            if( detail->getSchematicIndex() >= 0 )
            {
                statusMessage += " - Schematic: ";
                statusMessage += QString( "%1" ).arg( detail->getSchematicIndex() );
            }
        }
        lastSelectedMolecule_ = molId;
        statusBar()->showMessage( statusMessage );
        saveAction_->setEnabled( true );
        moleculeDisplayAction_->setEnabled( true );
        deleteMoleculeAction_->setEnabled( true );
        unselectAllAction_->setEnabled( true );
        if( GLSLShadersSupported() ) shadersAction_->setEnabled( true );
        //surfacesMenu_->setEnabled( true ); // won't work on Mac
        //analysisMenu_->setEnabled( true ); // won't work on Mac
        orbitalsAction_->setEnabled( true );
        gridDataSurfaceAction_->setEnabled( true );
        imagePlaneProbeAction_->setEnabled( true );
        moleculeAnimationAction_->setEnabled( true );
        sasAction_->setEnabled( true );
        connollySurfaceAction_->setEnabled( true );
        resetMoleculeAction_->setEnabled( true );
        irSpectrumAction_->setEnabled( true );
        if( notify )
        {
            workspaceTreeDockWidget_->GetTreeWidget()->SelectMolecule( lastSelectedMolecule_, false );
        }

        UpdateMoleculePropertyWidget( mol );
        UpdateAnalysisActions();
		UpdateEditActions();
        styleButton_->setEnabled( true );

    }
    catch( const exception& ex )
    {
        QMessageBox::critical( this, QString( "SelectMolecule Error" ), QString( ex.what() ),
                                   QMessageBox::Ok, QMessageBox::NoButton );
    }
}

//------------------------------------------------------------------------------
namespace
{
    struct UnselectMolecule
    {
        void operator()( MolekelMolecule* m ) const
        {
            m->SetBBoxVisible( false );
            m->GetChemSelection()->deselectAll();
        }
    };
}

void MainWindow::UnselectAll()
{
    lastSelectedMolecule_ = MolekelData::InvalidIndex();
    statusBar()->showMessage( QString( "Ready" ) );
    saveAction_->setEnabled( false );
    moleculeDisplayAction_->setEnabled( false );
    shadersAction_->setEnabled( false );
    data_->Apply( ( UnselectMolecule() ) );
    deleteMoleculeAction_->setEnabled( false );
    unselectAllAction_->setEnabled( false );
    moleculeAnimationAction_->setEnabled( false );
    sasAction_->setEnabled( false );
    connollySurfaceAction_->setEnabled( false );
    imagePlaneProbeAction_->setEnabled( false );
    gridDataSurfaceAction_->setEnabled( false );
    orbitalsAction_->setEnabled( false );
    resetMoleculeAction_->setEnabled( false );
    irSpectrumAction_->setEnabled( false );
    workspaceTreeDockWidget_->GetTreeWidget()->UnselectMolecules();
    UpdateMoleculePropertyWidget( 0 );
    selectedAtoms_.Clear();
	selectedBonds_.Clear();
    UpdateAnalysisActions();
	UpdateEditActions();
    styleButton_->setEnabled( false );
}

//------------------------------------------------------------------------------
void MainWindow::SetInteractionMode( InteractionMode im )
{
    interactionMode_ = im;
    switch( interactionMode_ )
    {
    case INTERACT_WITH_CAMERA:
        assert( trackBallCamera_ != 0 && "NULL interactor style" );
        interactorStyle_ = trackBallCamera_;
        break;
    case INTERACT_WITH_MOLECULE:
        assert( trackBallActor_ != 0 && "NULL interactor style" );
        interactorStyle_ = trackBallActor_;
        break;
    default:
        break;
    }

    vtkRenderWindow_->GetInteractor()->SetInteractorStyle( interactorStyle_ );
}

//------------------------------------------------------------------------------
namespace
{
    struct SetPickMode
    {
        const MolekelMolecule::PickMode pm;
        SetPickMode( MolekelMolecule::PickMode m ) : pm( m ) {}
        void operator()( MolekelMolecule* m ) const { m->SetPickMode( pm ); }
    };
}

void MainWindow::SetPickingMode( PickingMode pm )
{
    if( pm == pickingMode_ ) return;
    pickingMode_ = pm;
    switch( pickingMode_ )
    {
    case PICK_MOLECULE:
        data_->Apply( ( SetPickMode( MolekelMolecule::PICK_MOLECULE ) ) );
        break;
    case PICK_ATOMS_AND_BONDS:
        data_->Apply( ( SetPickMode( MolekelMolecule::PICK_ATOM_BOND_RESIDUE ) ) );
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
void MainWindow::Refresh()
{
    assert( vtkWidget_ && "NULL QVTK widget" );
    vtkRenderer_->ResetCameraClippingRange();
    vtkRenderWindow_->Render();
}

//------------------------------------------------------------------------------
void MainWindow::SetMoleculeVisibility( MolekelData::IndexType i,
                                        bool visibility )
{
    MolekelMolecule* mol = data_->GetMolecule( i );
    mol->SetVisible( visibility );
    Refresh();
}

//------------------------------------------------------------------------------
MolekelMolecule* MainWindow::GetMolecule( MolekelData::IndexType i )
{
    assert( data_ );
    return data_->GetMolecule( i );
}

//------------------------------------------------------------------------------

namespace
{
    struct InitAnimation
    {
        void operator()( MolekelMolecule* m ) { m->InitUpdate(); }
    };

    struct ResetAnimation
    {
        void operator()( MolekelMolecule* m ) { m->ResetUpdate(); }

    };
}

//------------------------------------------------------------------------------
bool MainWindow::AnimationStarted() const
{
    return animationAction_->text() == QString( "Stop Animation" );
}

//------------------------------------------------------------------------------
void MainWindow::StartAnimation( bool startTimer = true )
{
    animationAction_->setText( QString( "Stop Animation" ) );
    ReplaceActionIcon( animationAction_, TOOLBAR_STOP_ICON );
    exportVideoAction_->setEnabled( false );
    data_->Apply( InitAnimation() );
    // start timer
    animationTimerId_ = 0;
    if( startTimer ) animationTimerId_ = this->startTimer( timestep_ );
    frameCounter_ = 0;
}

//------------------------------------------------------------------------------
void MainWindow::StopAnimation()

 {
    if( animationTimerId_ )
    {
        // stop timer
        killTimer( animationTimerId_ );
    }
    animationAction_->setText( QString( "Start Animation" ) );
    ReplaceActionIcon( animationAction_, TOOLBAR_PLAY_ICON );
    data_->Apply( ResetAnimation() );
    exportVideoAction_->setEnabled( true );
    exportAnimationInProgress_ = false;
}

//------------------------------------------------------------------------------
class UpdateMolecule
{
    bool forward_;
public:
    UpdateMolecule( bool forward = true ) : forward_( forward ) {}
    void operator()( MolekelMolecule* m ) const { m->Update( forward_ ); }
    void SetForward( bool on ) { forward_ = on; }
};

void MainWindow::UpdateAnimation( bool forward )
{
    static UpdateMolecule updater;
    updater.SetForward( forward );
    data_->Apply( updater );
    Refresh();
    if( AnimationStarted() ) UpdateFrameCounter( forward );
}

//-------------------------------------------------------------------------------
vtkImageData* MainWindow::GetSnapshot() const
{
    vtkSmartPointer< vtkWindowToImageFilter > w2i( vtkWindowToImageFilter::New() );
    w2i->SetInput( vtkRenderWindow_ );
    w2i->Update();
    return w2i->GetOutput();
}


//------------------------------------------------------------------------------
void MainWindow::SetMEPScalarBarLUT( vtkLookupTable* lut )
{

    // VTK requires non const pointers for Set methods' parameters
    assert( vtkMEPScalarBarWidget_ != 0 );
    assert( vtkMEPScalarBarWidget_->GetScalarBarActor() != 0 );
    vtkMEPScalarBarWidget_->GetScalarBarActor()
                          ->SetLookupTable( 0 );
    vtkMEPScalarBarWidget_->GetScalarBarActor()
                          ->SetLookupTable( lut );
}

//------------------------------------------------------------------------------
void MainWindow::SetProbeWidgetScalarBarLUT( vtkLookupTable* lut )
{

    // VTK requires non const pointers for Set methods' parameters
    assert( vtkProbeScalarBarWidget_ != 0 );
    assert( vtkProbeScalarBarWidget_->GetScalarBarActor() != 0 );
    vtkProbeScalarBarWidget_->GetScalarBarActor()
                            ->SetLookupTable( 0 );
    vtkProbeScalarBarWidget_->GetScalarBarActor()
                            ->SetLookupTable( lut );
}

//------------------------------------------------------------------------------
void MainWindow::SetProbeWidgetScalarBarRange( const double range[ 2 ] )
{

    // VTK requires non const pointers for Set methods' parameters
    assert( vtkProbeScalarBarWidget_ != 0 );
    assert( vtkProbeScalarBarWidget_->GetScalarBarActor() != 0 );
    vtkProbeScalarBarWidget_->GetScalarBarActor()
                            ->SetLookupTable( 0 );
    vtkLookupTable* lut = dynamic_cast< vtkLookupTable* >(
        vtkProbeScalarBarWidget_->GetScalarBarActor()->GetLookupTable() );
    assert( lut && "Wrong lookup table type" );
    lut->SetTableRange( range[ 0 ], range[ 1 ] );
}

//------------------------------------------------------------------------------
void MainWindow::SetProbeWidgetScalarBarTitle( const char* title )
{
    vtkProbeScalarBarWidget_->GetScalarBarActor()->SetTitle( title );
}

//------------------------------------------------------------------------------
void MainWindow::DisplayStatusMessage( const QString& msg )
{
    statusBar()->showMessage( msg );
}


//------------------------------------------------------------------------------
#include <ctime>
void MainWindow::ProgressCallback( int step, int totalSteps, void* cbData )
{
    if( !cbData ) return;
    MainWindow* mw = reinterpret_cast< MainWindow* >( cbData );
    if( step < 0 || totalSteps <= 0 )
    {
        mw->DisplayStatusMessage( QString( "Progress callback error" ) );
    }
    else
    {
        static clock_t start = clock_t();
        // if step == 0 initialize start time and return
        if( step == 0 ) start = clock();
        else
        {
            const int progress = int( ( 100. * step ) / totalSteps );
            const double elapsed = double( clock() - start ) / CLOCKS_PER_SEC;
            mw->DisplayStatusMessage( QString( "Computed step %1 of %2 - %3% - Elapsed time: %4s" )
                                                                         .arg( step )
                                                                         .arg( totalSteps )
                                                                         .arg( progress )
                                                                         .arg( elapsed ) );
        }
        
        //This makes the application responsive to user events while computation in progress.
        //Requires the code to be re-entrant so that computation can be stopped.
        //QCoreApplication::processEvents();
    }
}

//------------------------------------------------------------------------------
void MainWindow::UpdateAnalysisActions()
{
    distanceAction_->setEnabled( false );
    angleAction_->setEnabled( false );
    dihedralAngleAction_->setEnabled( false );
    measureAction_->setEnabled( false );
    switch( selectedAtoms_.GetSize() )
    {
        case 2:
            distanceAction_->setEnabled( true );
            break;
        case 3:
            angleAction_->setEnabled( true );
            break;
        case 4:
            dihedralAngleAction_->setEnabled( true );
            break;
        default:
            break;
    }
    if( distanceAction_->isEnabled() ||
        angleAction_->isEnabled()    ||
        dihedralAngleAction_->isEnabled() )
    {
        measureAction_->setEnabled( true );
    }
}

//------------------------------------------------------------------------------
void MainWindow::UpdateEditActions()
{
    addRemoveBondAction_->setEnabled( false );
	if( selectedAtoms_.GetSize() == 2 )
	{
		addRemoveBondAction_->setEnabled( true );
		ReplaceActionIcon( addRemoveBondAction_, TOOLBAR_ADD_BOND_ICON );
	}
	else if( selectedBonds_.GetSize() == 1 )
	{
		addRemoveBondAction_->setEnabled( true );
		ReplaceActionIcon( addRemoveBondAction_, TOOLBAR_REMOVE_BOND_ICON );
	}
}


//------------------------------------------------------------------------------
void MainWindow::SetMSMSExecutablePath( const QString& msmsPath )
{
    QSettings settings;
    settings.setValue( MSMS_EXECUTABLE_KEY.c_str(), msmsPath );
}

//------------------------------------------------------------------------------
QString MainWindow::GetMSMSExecutablePath() const
{
    QSettings settings;
    return settings.value( MSMS_EXECUTABLE_KEY.c_str(), QString( "" ) ).toString();
}

//------------------------------------------------------------------------------
void MainWindow::SaveSnapshot( const QString& fname, const QString& format, unsigned magFactor )
{
  vtkSmartPointer< vtkImageWriter > writer;
  if( format.compare( "png", Qt::CaseInsensitive ) == 0 ) writer = vtkPNGWriter::New();
  else if( format.compare( "tiff", Qt::CaseInsensitive) == 0 ) writer = vtkTIFFWriter::New();
  else throw std::invalid_argument( "Invalid image format" );
  vtkSmartPointer< vtkRenderLargeImage > lir = vtkRenderLargeImage::New();
  lir->SetMagnification( magFactor );
  lir->SetInput( vtkRenderer_ );
  writer->SetInputConnection( lir->GetOutputPort() );
  writer->SetFileName( fname.toStdString().c_str() );
  lir->Update();
  vtkRenderWindow_->Render();
  writer->Write();
}


//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    /// @todo verify the following
    /// - vtkWidget_ should be deleted by MainWindow
    /// - vtkRenderer_ should be deleted by vtkWidget_
    /// - actions should be deleted by menus
    /// - menus are deleted by MainWindow
    /// @todo use auto_ptr for data_, loadFileMessageBox_ and fileLoadThread_
    delete data_;
    delete loadFileMessageBox_;
    delete fileLoadThread_;
}

//------------------------------------------------------------------------------
void MainWindow::SaveOrbitalSnapshotsSlot()
{
    if( lastSelectedMolecule_ == MolekelData::InvalidIndex() )
    {
        QMessageBox::information( this, "Save Orbitals Snapshots", "Select a molecule first" );
        return;
    }

    QSettings settings;
    QString dir = settings.value( OUT_DATA_DIR_KEY.c_str(), QCoreApplication::applicationDirPath() ).toString();
    QString d = GetExistingDirectory( this, "Select directory where snapshots will be saved", dir );
    if( d.isEmpty() ) return;

    MolekelMolecule* mol = data_->GetMolecule( lastSelectedMolecule_ );

    UnselectAll();

    const QString prefix = d + '/' + QString( mol->GetFileName().c_str() ) + ".orbital_";

    typedef map< int, bool > VisibilityMap;
    VisibilityMap visibilityMap;

    // save visibility information
    for( int oi = 0; oi != mol->GetNumberOfOrbitals(); ++oi )
    {
        if( !mol->HasOrbitalSurface( oi ) ) continue;
        visibilityMap[ oi ] =  mol->GetOrbitalSurfaceVisibility( oi );
    }

    
    static const char* PADDING[] = { "0", "000", "00", "0", "" }; 
    static const int MAX_PADDING_LENGTH = sizeof( PADDING ) / sizeof( const char* ) - 1; 
    
    // save snapshots enabling one orbital at a time
    mol->SetOrbitalSurfacesVisibility( false );
    for( int i = 0; i != mol->GetNumberOfOrbitals(); ++i )
    {
        if( !mol->HasOrbitalSurface( i ) ) continue;
        mol->SetOrbitalSurfaceVisibility( i, true );
        const int paddingIndex = std::min( QString( "%1" ).arg( i ).size(), MAX_PADDING_LENGTH );
        const QString ofname = prefix + PADDING[ paddingIndex ] + QString( "%1" ).arg( i ) + ".png";
        statusBar()->showMessage( QString( "Taking snapshot of orbital %1" ).arg( i ) );
        SaveSnapshot( ofname, "png" );
        mol->SetOrbitalSurfaceVisibility( i, false );
        QCoreApplication::processEvents();
    }

    // restore visibility
    // save visibility information
    for( VisibilityMap::const_iterator vi = visibilityMap.begin();
         vi != visibilityMap.end();
         ++vi )
    {
        mol->SetOrbitalSurfaceVisibility( vi->first, vi->second );
    }

    Refresh();
}

//------------------------------------------------------------------------------
class ChangeBBoxColor
{
private:
    double r_;
    double g_;
    double b_;
public:
    ChangeBBoxColor() : r_( double() ), g_( double() ), b_( double() ) {}
    ChangeBBoxColor( double r, double g, double b ) : r_( r ), g_( g ), b_( b ) {}
    void operator()( MolekelMolecule* m ) const
    {
        assert( m );
        m->GetBBoxActor()->GetProperty()->SetColor( r_, g_, b_ );
        m->GetIsoBBoxActor()->GetProperty()->SetColor( r_, g_, b_ );
    }
};
void MainWindow::SetMoleculeBBoxColor( double r, double g, double b )
{
    data_->Apply( ChangeBBoxColor( r, g, b ) );
}

//------------------------------------------------------------------------------
#include "dialogs/ViewPropertiesDialog.h"
void MainWindow::Edit3DViewPropertiesSlot()
{
	ViewPropertiesDialog d( this, this );
    d.setWindowTitle( "3D View Properties" );
	d.exec();
}


//------------------------------------------------------------------------------
QSize MainWindow::Get3DViewSize() const
{
    return vtkWidget_->size();
}

//------------------------------------------------------------------------------
void MainWindow::ScriptRecordSlot()
{
    if( recordEventsDlg_.isVisible() ) return;
    recordEventsDlg_.show();

}

//------------------------------------------------------------------------------
void MainWindow::ScriptPlaySlot()
{
   if( playEventsDlg_.isVisible() ) return;
   playEventsDlg_.show();
}

//------------------------------------------------------------------------------
QString MainWindow::GetShadersDir() const
{
    QSettings settings;
    return settings.value( SHADERS_DIR_KEY.c_str(), "" ).toString();
}

//-----------------------------------------------------------------------------
void MainWindow::SetShadersDir( const QString& dir )
{
    QSettings settings;
    settings.setValue( SHADERS_DIR_KEY.c_str(), dir );
}

//-----------------------------------------------------------------------------
void MainWindow::PlayEvents( const char* filePath,
							 unsigned initialDelay,
							 int minDelay,
                             double timeScaling )
{
    EventPlayerWidget* pw = qobject_cast< EventPlayerWidget* >( playEventsDlg_.GetWidget() );
    assert( pw );
    pw->SetFilePath( filePath );
    pw->SetTimeScaling( timeScaling );
    pw->SetMinDelay( minDelay );
    QTimer::singleShot( initialDelay, this, SLOT( PlayEventsSlot() ) );
}

//-----------------------------------------------------------------------------
void MainWindow::PlayEventsSlot()
{
    EventPlayerWidget* pw = qobject_cast< EventPlayerWidget* >( playEventsDlg_.GetWidget() );
    assert( pw );
    pw->Play();
}

//-----------------------------------------------------------------------------
void MainWindow::SpectrumSlot()
{
    assert( lastSelectedMolecule_ != MolekelData::InvalidIndex() );
    DialogWidget* sd = new DialogWidget( 
    						new SpectrumWidget(
    								data_->GetMolecule( lastSelectedMolecule_ ), OUT_DATA_DIR_KEY.c_str() ),
    								this, Qt::Tool );
    sd->setWindowTitle( "Radiation Spectrum" );
    sd->resize( 450, 350 );
    sd->show();    
}

//-----------------------------------------------------------------------------
void MainWindow::SetDepthPeelingEnabled( bool on )
{
#ifdef DEPTH_PEELING_ENABLED	
	assert( vtkRenderer_ != 0 );
	vtkRenderer_->SetUseDepthPeeling( on );
	Refresh();
#endif	
}

//-----------------------------------------------------------------------------
bool MainWindow::DepthPeelingSupported() const
{
#ifdef DEPTH_PEELING_ENABLED	
	return true;
#else
	return false;
#endif	
}


//-----------------------------------------------------------------------------
bool MainWindow::GetDepthPeelingEnabled() const
{	
#ifdef DEPTH_PEELING_ENABLED	
	assert( vtkRenderer_ != 0 );
	return vtkRenderer_->GetUseDepthPeeling();
#else
	return false;
#endif	
}

//-----------------------------------------------------------------------------
void MainWindow::SetMaxNumberOfPeels( int peels )
{
#ifdef DEPTH_PEELING_ENABLED	
	assert( vtkRenderer_ != 0 );
	vtkRenderer_->SetMaximumNumberOfPeels( peels );
#endif	
}

//-----------------------------------------------------------------------------
int MainWindow::GetMaxNumberOfPeels() const
{
#ifdef DEPTH_PEELING_ENABLED		
	assert( vtkRenderer_ != 0 );
	return vtkRenderer_->GetMaximumNumberOfPeels();
#else
	return 0;
#endif	
}

//-----------------------------------------------------------------------------
void MainWindow::SetDPOcclusionRatio( double r )
{
#ifdef DEPTH_PEELING_ENABLED		
	assert( vtkRenderer_ != 0 );
	vtkRenderer_->SetOcclusionRatio( r );
#endif	
}

//-----------------------------------------------------------------------------
double MainWindow::GetDPOcclusionRatio() const
{
#ifdef DEPTH_PEELING_ENABLED		
	assert( vtkRenderer_ != 0 );
	return vtkRenderer_->GetOcclusionRatio();
#else
	return 0.0;
#endif	
}

//-----------------------------------------------------------------------------
void MainWindow::SetAAFrames( int f )
{
	assert( vtkRenderWindow_ != 0 );
	vtkRenderWindow_->SetAAFrames( f );
}

//-----------------------------------------------------------------------------
int MainWindow::GetAAFrames() const
{
	assert( vtkRenderWindow_ != 0 );
	return vtkRenderWindow_->GetAAFrames();
}

//-----------------------------------------------------------------------------
void MainWindow::SetBkColor( const QColor& bkColor )
{
	const int CMAX = 255;
	const double R = double( bkColor.red() ) / CMAX;
	const double G = double( bkColor.green() ) / CMAX;
	const double B = double( bkColor.blue() ) / CMAX;
	vtkRenderer_->SetBackground( R, G, B );
	vtkAxesRenderer_->SetBackground( R, G, B );
	SetMoleculeBBoxColor( 1.0 - R, 1.0 - G, 1.0 - B );
	Refresh();
}

//-----------------------------------------------------------------------------
void MainWindow::UpdateFrameCounter( bool forward )
{
    frameCounter_ += forward ? 1 : -1;
    statusBar()->showMessage( QString( "Frame %1" ).arg( frameCounter_ ) );
}

#include <ChemKit2/ChemDisplayParam.h>

//-----------------------------------------------------------------------------
void MainWindow::QuickDisplaySpacefillSlot()
{
    if( lastSelectedMolecule_ == MolekelData::InvalidIndex() ) return;
    MolekelMolecule* m = data_->GetMolecule( lastSelectedMolecule_ );
    m->GetChemDisplayParam()->displayStyle = ChemDisplayParam::DISPLAY_CPK;
    Refresh();
    QSettings settings;
    settings.setValue( MOLECULE_DISPLAY_STYLE_KEY.c_str(),
        m->GetChemDisplayParam()->displayStyle.getValue() );
}

//-----------------------------------------------------------------------------
void MainWindow::QuickDisplayBStickSlot()
{
    if( lastSelectedMolecule_ == MolekelData::InvalidIndex() ) return;
    MolekelMolecule* m = data_->GetMolecule( lastSelectedMolecule_ );
    m->GetChemDisplayParam()->displayStyle = ChemDisplayParam::DISPLAY_BALLSTICK;
    //m->GetChemDisplayParam()->bondCylinderDisplayStyle = ChemDisplayParam::BONDCYLINDER_NOCAP;
    Refresh();
    QSettings settings;
    settings.setValue( MOLECULE_DISPLAY_STYLE_KEY.c_str(),
        m->GetChemDisplayParam()->displayStyle.getValue() );
}

//-----------------------------------------------------------------------------
void MainWindow::QuickDisplayLiquoriceSlot()
{
    if( lastSelectedMolecule_ == MolekelData::InvalidIndex() ) return;
    MolekelMolecule* m = data_->GetMolecule( lastSelectedMolecule_ );
    m->GetChemDisplayParam()->displayStyle = ChemDisplayParam::DISPLAY_STICK;
    m->GetChemDisplayParam()->bondCylinderDisplayStyle = ChemDisplayParam::BONDCYLINDER_ROUNDCAP;
    Refresh();
    QSettings settings;
    settings.setValue( MOLECULE_DISPLAY_STYLE_KEY.c_str(),
        m->GetChemDisplayParam()->displayStyle.getValue() );
}

//-----------------------------------------------------------------------------
void MainWindow::QuickDisplayWireframeSlot()
{
    if( lastSelectedMolecule_ == MolekelData::InvalidIndex() ) return;
    MolekelMolecule* m = data_->GetMolecule( lastSelectedMolecule_ );
    m->GetChemDisplayParam()->displayStyle = ChemDisplayParam::DISPLAY_WIREFRAME;
    Refresh();
    QSettings settings;
    settings.setValue( MOLECULE_DISPLAY_STYLE_KEY.c_str(),
        m->GetChemDisplayParam()->displayStyle.getValue() );
}

//-----------------------------------------------------------------------------
void MainWindow::UpdateViewMenu()
{   
    viewToolbarAction_->setChecked( mainToolBar_->isVisible() );
    viewWorkspaceAction_->setChecked( workspaceTreeDockWidget_->isVisible() );
    viewMoleculePropAction_->setChecked( moleculePropertyDockWidget_->isVisible() );    
}