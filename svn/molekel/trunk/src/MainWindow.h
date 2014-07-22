#ifndef MAINWINDOW_H_
#define MAINWINDOW_H_
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

/// @todo add exception handling and smart pointers
/// @todo consider keeping a list of actions or map< action id/name, action>
/// it could be useful to access the same actions from scripts / plugins

// QT
#include <QMainWindow>
#include <QDockWidget>
#include <QAction>
#include <QSize>

// VTK
#include <vtkSmartPointer.h>

// STD
#include <string>
#include <cassert>
#include <fstream>

#include "MolekelData.h"
#include "MoleculeCallback.h"
#include "widgets/WorkspaceTreeWidget.h"
#include "widgets/DisplayPropertyWidget.h"
#include "Selection.h"
#include "utility/DialogWidget.h"

// Forward declarations
class QMenu;
class QTimerEvent;
class QResizeEvent;
class QVTKWidget;
class MolekelData;
class vtkRenderer;
class SoPickedPoint;
class vtkInteractorStyle;
class vtkInteractorStyleTrackballActor;
class vtkInteractorStyleTrackballCamera;
class vtkImagePlaneWidget;
class QStringList;
class QCloseEvent;
class vtkImageData;
class vtkLookupTable;
class vtkScalarBarWidget;
class FileLoadThread;
class QMessageBox;
class QToolBar;
class vtkRenderWindow;
class LogEventFilter;
class QColor;

/// Main window class; contains:
/// - vtk widget
/// - menu bar
/// - status bar
/// - docking widgets
/// - data (molecules)
///
/// This class is currently used for all VC (in an MVC architecture)
/// related tasks.
/// @note might be useful to build a stand-alone controller.
///
/// View role:
///   MainWindow presents the visual information to the user and
///   decides when to update the view based on GUI events and
///   Model state
///
/// Controller role:
///  All events are handled inside an instance of this class
///  which decides how to handle events depending on event and
///  current state
///
/// @warning even if MOLEKEL_MULTITHREADED_DATA_LOADING is defined
/// on the compiler command line moc won't generate the SLOT unless
/// #define MOLEKEL_MULTITHREADED_DATA_LOADING is placed in this
/// include file (thus resulting in a warning from the compiler:
/// "MOLEKEL_MULTITHREADED_DATA_LOADING redefined..." )
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //@{ Settings. Keys used to retrieve application settings.
    /// Input data directory.
    static const std::string IN_DATA_DIR_KEY;
    /// Output data directory.
    static const std::string OUT_DATA_DIR_KEY;
    /// Snapshots output directory
    static const std::string OUT_SNAPSHOTS_DATA_DIR_KEY;
    /// Plugin directory.
    static const std::string PLUGIN_DIR_KEY;
    /// GUI layout.
    static const std::string MAIN_WINDOW_LAYOUT_KEY;
    /// MainWindow's width
    static const std::string MAIN_WINDOW_WIDTH_KEY;
    /// MainWindow's height
    static const std::string MAIN_WINDOW_HEIGHT_KEY;
    /// M.F. Sanner's MSMS executable for generation of Connolly surfaces.
    static const std::string MSMS_EXECUTABLE_KEY;
    /// MainWindow x position.
    static const std::string MAIN_WINDOW_X_KEY;
    /// MainWindow y position.
    static const std::string MAIN_WINDOW_Y_KEY;
    /// Events output dir.
    static const std::string OUT_EVENTS_DIR_KEY;
    /// Events input dir.
    static const std::string IN_EVENTS_DIR_KEY;
    /// Shaders dir.
    static const std::string SHADERS_DIR_KEY;
    /// Background color.
    static const std::string BACKGROUND_COLOR_KEY;
    /// Molecule display style: spacefill, ball & stick, wireframe... .
    static const std::string MOLECULE_DISPLAY_STYLE_KEY;
    /// Atom display style.
    static const std::string MOLECULE_ATOM_DISPLAY_STYLE_KEY;
    /// Bond display style.
    static const std::string MOLECULE_BOND_DISPLAY_STYLE_KEY;
    /// Atom scaling factor. 
    static const std::string MOLECULE_ATOM_SCALING_KEY;
    /// Bond scaling factor.
    static const std::string MOLECULE_BOND_SCALING_KEY;
    /// Atom detail.
    static const std::string MOLECULE_ATOM_DETAIL_KEY;
    /// Bond detail.
    static const std::string MOLECULE_BOND_DETAIL_KEY;
    /// Atom colors file name.
    static const std::string MOLECULE_ATOM_COLORS_FILE_KEY;
    //@}

public:
    /// Constructor.
    MainWindow();
    /// Destructor. Deletes data_ member.
    ~MainWindow();

private:
    /// Creates QActions and adds actions to menus.
    void CreateQtActions();
    /// Ceates UI menus.
    void CreateMenus();
    /// Overridden method to display window's width and height in status bar.
    void resizeEvent( QResizeEvent* event );
    /// @warning temporary
    void timerEvent( QTimerEvent *event );
    /// Updates molecule property widget according to selection.
    void UpdateMoleculePropertyWidget( MolekelMolecule* mol );
    /// Adds action into toolbar loading the required icon.
    void AddActionToToolBar( QAction* a, const QString& iconName );
    /// Changes molecules' bounding box color.
    void SetMoleculeBBoxColor( double r, double g, double b );
    /// Returns true if selection bounding boxes are shown, false otherwise.
    bool ShowBoundingBox() const;
    
#ifndef MOLEKEL_MULTITHREADED_DATA_LOADING
    /// Method invoked by the file loading thread after a new
    /// molecule has been added to the database. @note The thread
    /// in which data loading is performed depends on the
    /// MOLEKEL_MULTITHREADED_DATA_LOADING definition:
    /// - if MOLEKEL_MULTITHREADED_DATA_LOADING is defined
    ///   data loading will be performed in a separate thread and the
    ///   user will be able to interrupt the operation
    /// - if MOLEKEL_MULTITHREADED_DATA_LOADING is not defined data
    ///   loading will be perfomed synchronously in the main application
    ///   thread.
    void MoleculeLoaded( MolekelData::IndexType, const QString& );
#endif // MOLEKEL_MULTITHREADED_DATA_LOADING

private slots:

#ifdef MOLEKEL_MULTITHREADED_DATA_LOADING
    /// Callback method invoked by the file loading thread after a new
    /// molecule has been added to the database. @note The thread
    /// in which data loading is performed depends on the
    /// MOLEKEL_MULTITHREADED_DATA_LOADING definition:
    /// - if MOLEKEL_MULTITHREADED_DATA_LOADING is defined
    ///   data loading will be performed in a separate thread and the
    ///   user will be able to interrupt the operation
    /// - if MOLEKEL_MULTITHREADED_DATA_LOADING is not defined data
    ///   loading will be perfomed synchronously in the main application
    ///   thread.
    void MoleculeLoadedSlot();
#endif // MOLEKEL_MULTITHREADED_DATA_LOADING
    /// Load molecule.
    void LoadFileSlot();
    /// Save molecule.
    void SaveFileSlot();
    /// About.
    void AboutSlot();
    /// Enable camera interaction and disable molecule interaction mode.
    /// @todo add accelerators.
    void SetCameraInteractionSlot();
    /// Enable molecule interaction mode and disable camera interaction mode.
    /// @todo add accelerators.
    void SetMoleculeInteractionSlot();
    /// Display display style dialog.
    void MoleculeDisplaySlot();
    /// Clear scene: remove all molecules.
    void ClearSlot();
    /// Unselect all molecules, atoms, bonds and residues.
    void UnselectAllSlot();
    /// Remove selected molecule.
    void DeleteMoleculeSlot();
    /// Save snapshot to file (TIFF).
    void SaveSnapshotSlot();
    /// Change background color.
    void BackgroundColorSlot();
    /// Show/hide axes.
    void AxesSlot();
    /// Start/stop animation.
    void AnimationSlot();
    /// Change time interval used for animation.
    void TimestepSlot();
    /// Change animation mode (vibration, trajectory, mixed, enable/disable)
    void MoleculeAnimationSlot();
    /// @todo add support for mixed mode animation + vibration
    /// # update atom positions with trajectory animator
    /// # update atom positions with vibration animator
    ///   this bejavior can be achieved by:
    /// # adding functionality into SwitchAtomAnimator making it a
    ///   CompositeAtomAnimator with the option of selecting an
    ///   animator or apply the Update() method of each of the animators
    /// # create a serparate CompositeAtomAnimator and switch animators
    ///   at run-time depending on user preferences
    /// # (worst) handle everything from within MainWindow, but we still
    ///   need to store per-molecule preferences.
    /// Toggles visibility of tree docking widget.
    void ViewWorkspaceSlot( bool );
    /// Toggles visibility of Molecule Property widget.
    void ViewMoleculePropSlot( bool );
    /// Opens orbitals dialog.
    void OrbitalsSlot();
    /// Opens image plane probe (vtkImagePlaneWidget) dialog.
    void ImagePlaneProbeSlot();
    /// Resets camera transform.
    void ResetCameraSlot();
    /// Resets molecule transform.
    void ResetMoleculeSlot();
    /// License slot: displays licence information.
    void LicenseSlot();
    /// Generate surface from grid data.
    void GridDataSurfaceSlot();
    /// Save video.
    void ExportVideoSlot();
    /// Show/hide MEP scalar bar widget.
    void ToggleMEPScalarBarSlot();
    /// Show/hide plane probe scalar bar widget.
    void ToggleProbeScalarBarSlot();
    /// Compute dihedral angle; enabled only when four atoms selected.
    void DihedralAngleSlot();
    /// Compute angle; enabled only when three atoms are selected.
    void AngleSlot();
    /// Compute distance between two atoms; enabled only when
    /// two atoms are selected.
    void DistanceSlot();
	/// Add/remove bond. Either one bond or two atoms bust be selected.
	void AddRemoveBondSlot();
    /// Toggles visibility of plane probe.
    void TogglePlaneProbeSlot();
    /// Opens Connolly surface dialog.
    void ConnollySurfaceSlot();
    /// Opens SAS dialog.
    void SasSlot();
    /// Save window layout and exit.
    void CloseSlot();
    /// Save to PS.
    void SaveToPSSlot();
    /// Save to EPS.
    void SaveToEPSSlot();
    /// Save to PDF.
    void SaveToPDFSlot();
    /// Save to TeX
    void SaveToTeXSlot();
    /// Toggle toolbar visibility.
    void ViewToolbarSlot( bool );
    /// Called when atom picking option is toggled:
    /// - if option enabled user can pick individual atoms and bonds
    /// - if option disabled user can interact with molecules only by
    ///   picking a molecule's bounding box.
    void AtomPickingToggledSlot( bool );
    /// Called when the toolbar's 'Measure' button is clicked. Depending
    /// on how many atoms are selected the following values will be computed:
    /// - 2 atoms --> distance
    /// - 3 atoms --> angle
    /// - 4 atoms --> dihedral angle
    void MeasureSlot();
    /// Advance to next animation frame if animation not started.
    void NextFrameSlot();
    /// Advance to previous animation frame if animation not started.
    void PreviousFrameSlot();
    /// Open local documentation.
    void DocSlot();
    /// Open Molekel's web site url.
    void MolekelWebsiteSlot();
    /// Save one snapshot per selected molecule orbital.
    void SaveOrbitalSnapshotsSlot();
	/// Change 3D View properties.
	void Edit3DViewPropertiesSlot();

    //@{ Events recording/playback; on Mac OS the eventFilter() method is never
    ///  invoked for menu events and accelerators and is therefore not possible to record
    ///  this event types on Mac OS as of Qt 4.3rc1. One option is to override the macEventFilter
    ///  method but this requires changing the record/playback architecture specifically for Mac OS.

    /// Start recording GUI events.
	void ScriptRecordSlot();
    /// Play recorded events.
    void ScriptPlaySlot();
    /// Open shaders dialog.
    void ShadersSlot();
    /// Play events slot; called by a single QTimer initialized in PlayEvents method.
    void PlayEventsSlot();
    //@}

    /// Hide/show molecule bounding box when molecule selected.
    void BBoxSlot();
    /// Display IR and Raman activities spectra.
    void SpectrumSlot();
    //@{ Quick display options
    void QuickDisplaySpacefillSlot();
    void QuickDisplayBStickSlot();
    void QuickDisplayLiquoriceSlot();
    void QuickDisplayWireframeSlot();
    //@}
    
private:

    //@{ Menus
    /// File.
    QMenu* fileMenu_;
    /// Edit.
    QMenu* editMenu_;
    /// Display preferences.
    QMenu* displayMenu_;
    /// Help.
    QMenu* helpMenu_;
    /// Interaction mode.
    QMenu* interactionMenu_;
    /// Animation
    QMenu* animationMenu_;
    /// View
    QMenu* viewMenu_;
    /// Orbitals, Density Matrix, Connolly
    QMenu* surfacesMenu_;
    /// Analysis.
    QMenu* analysisMenu_;
    //@}
    /// Toolbar.
    QToolBar* mainToolBar_;
    /// VTK Widget used to dsplay 3d geometry.
    QVTKWidget* vtkWidget_;
    /// VTK Renderer.
    vtkSmartPointer< vtkRenderer > vtkRenderer_;
    /// VTK Render Window
    vtkSmartPointer< vtkRenderWindow > vtkRenderWindow_;
    /// VTK Renderer for rendering Axes.
    vtkSmartPointer< vtkRenderer > vtkAxesRenderer_;
    /// Model: holds molecular data.
    MolekelData* data_;
    /// Set to true if data changed, @warning currently NOT used.
    bool modified_;
    /// Index/key of last selected molecule in data_.
    MolekelData::IndexType lastSelectedMolecule_;
    /// Default color lookup table for scalar to color mapping.
    vtkSmartPointer< vtkLookupTable > defaultLUT_;
    /// Lookup table used for MEP mapping.
    vtkSmartPointer< vtkLookupTable > mepLUT_;
    /// Lookup table used for probe widgets.
    vtkSmartPointer< vtkLookupTable > probeLUT_;
    /// Called by workspaceTreeDockWidget_ when it receives a close event.
    void WorkspaceCloseEvent();
    /// Workspace dock widget.
    class WorkspaceTreeDockWidget : public QDockWidget
    {
        WorkspaceTreeWidget* tw_;
        MainWindow* mw_;
    public:
        WorkspaceTreeDockWidget( MainWindow* mw )
            : QDockWidget( mw ), tw_( new WorkspaceTreeWidget( *mw ) ), mw_( mw )
        {
            setWidget( tw_ );
            setObjectName( "WorkspaceTreeDockWidget" );
            setAccessibleName( objectName() );
            tw_->setObjectName( "WorkspaceTreeWidget" );
            tw_->setAccessibleName( tw_->objectName() );
        }
        void closeEvent( QCloseEvent* e )
        {
            QDockWidget::closeEvent( e );
            mw_->WorkspaceCloseEvent();

        }
        WorkspaceTreeWidget* GetTreeWidget() { return tw_; }
    };
    WorkspaceTreeDockWidget* workspaceTreeDockWidget_;

    /// Called by moleculePropertyDockWidget_ when it receives a close event.
    void MoleculePropCloseEvent();

    /// Molecule Property dock widget.
    class MoleculePropertyDockWidget : public QDockWidget
    {
        DisplayPropertyWidget* dw_;
        MainWindow* mw_;
    public:
        MoleculePropertyDockWidget( const QStringList& propNames,
                                    MainWindow* mw,
                                    QWidget* parent )
         : QDockWidget( mw ),
           dw_( new DisplayPropertyWidget( propNames, parent ) ),
           mw_( mw )
         {
             setWidget( dw_ );
             setObjectName( "MoleculePropertyDockWidget" );
             setAccessibleName( objectName() );
             dw_->setObjectName( "MoleculePropertyWidget" );
             dw_->setAccessibleName( dw_->objectName() );
         }
        void closeEvent( QCloseEvent* e )
        {
            QDockWidget::closeEvent( e );
            mw_->MoleculePropCloseEvent();

        }
         DisplayPropertyWidget* GetPropWidget() { return dw_; }
    };
    MoleculePropertyDockWidget* moleculePropertyDockWidget_;

    //@{ Actions triggered by menu items.
    /// @todo consider using a container and access
    /// actions as actionContainer[ <ACTION_KEY> ]->...
	QAction* addRemoveBondAction_;
    QAction* cameraInteractAction_;
    QAction* moleculeInteractAction_;
    QAction* pickMoleculeAction_;
    QAction* pickAtomAction_;
    QAction* saveAction_;
    QAction* moleculeDisplayAction_;
    QAction* unselectAllAction_;
    QAction* clearAction_;
    QAction* deleteMoleculeAction_;
    QAction* axesAction_;
    QAction* animationAction_;
    QAction* moleculeAnimationAction_;
    QAction* exportVideoAction_;
    QAction* viewWorkspaceAction_;
    QAction* viewMoleculePropAction_;
    QAction* viewToolbarAction_;
    QAction* resetMoleculeAction_;

    QAction* orbitalsAction_;
    QAction* gridDataSurfaceAction_;

    QAction* imagePlaneProbeAction_;
    QAction* positionProbeAction_;
    QAction* distanceAction_;
    QAction* angleAction_;
    QAction* dihedralAngleAction_;
    QAction* clipPlaneAction_;

    QAction* planeProbeVisibilityAction_;
    QAction* mepScalarBarAction_;
    QAction* probeScalarBarAction_;

    QAction* connollySurfaceAction_;
    QAction* sasAction_;
    QAction* measureAction_;

    QAction* nextFrameAction_;
    QAction* previousFrameAction_;

    QAction* shadersAction_;
    
    QAction* bboxAction_;
    
    QAction* irSpectrumAction_;
    //@}

    
    //@{ Analysis widgets
    vtkSmartPointer< vtkImagePlaneWidget > vtkImagePlaneWidget_;
    //@}

    /// MEP Scalar bar: used to display colors associated with
    /// Molecular Electrostatic Potential.
    vtkSmartPointer< vtkScalarBarWidget > vtkMEPScalarBarWidget_;
    /// Probe Scalar bar: used to display colors associated
    /// with the currently active probe; e.g. plane probe showing
    /// electron density information.
    vtkSmartPointer< vtkScalarBarWidget > vtkProbeScalarBarWidget_;

    /// Viewport used to render axes; {min x, min y, max x, max y}
    /// all values have to be in the interval [0,1].
    double axesViewport_[ 4 ];

    /// Animation time step (milliseconds) == timer timeout
    unsigned int timestep_;
  
    /// If true displays size of 3d view when main window is resized.
    /// @note it won't display the correct size when other child controls
    /// are resized; the only way to display the correct size when the 3D
    /// view is resized is to subclass QVTK widget and override QWidget::sizeEvent().
    /// @todo add GUI controls to toggle this value.
    bool show3DViewSize_;

    /// Thread used to asynchronously load files.
    FileLoadThread* fileLoadThread_;

    /// Dialog box used to interrupt fileLoadThread_ operations.
    QDialog* loadFileMessageBox_;

    /// Record events dialog.
    DialogWidget recordEventsDlg_;

    /// Playback events dialog.
    DialogWidget playEventsDlg_;

    /// Set to true if an animation export is in progress, false otherwise.
    bool exportAnimationInProgress_;

    /// Frame counter
    int frameCounter_;
    
    /// Style button for quick switch of display style
    QPushButton* styleButton_;

public:

    /// Interaction mode.
    typedef enum { INTERACT_WITH_CAMERA, INTERACT_WITH_MOLECULE }
        InteractionMode;

    /// Picking mode.
    typedef enum { PICK_MOLECULE, PICK_ATOMS_AND_BONDS }
        PickingMode;

private:
    /// Animation timer.
    int animationTimerId_;
    /// Current interaction mode.
    InteractionMode interactionMode_;
    /// Current picking mode.
    PickingMode pickingMode_;
    /// Pointer to current interactor style. Points to trackBallActor_ or
    /// trackBallCamera_ depending on interaction mode.
    vtkSmartPointer< vtkInteractorStyle > interactorStyle_;
    /// Trackball actor interactor style; selected when INTERACT_WITH_MOLECULE selected.
    vtkSmartPointer< vtkInteractorStyleTrackballActor > trackBallActor_;
    /// Trackbal camera interactor style; selected when INTERACT_WITH_CAMERA selected.
    vtkSmartPointer< vtkInteractorStyleTrackballCamera > trackBallCamera_;
    /// List of last 4 selected atoms.
    SelectionList selectedAtoms_;
	/// List of selected bonds; we really need to keep track of the last selected bond only.
    SelectionList selectedBonds_;

public:
//@{ Interaction methods (public interface)
    /// Select and highlight molecule.
    /// This method is usually called from a callback class when
    /// a molecule is picked.
    /// @param id picked molecule.
    /// @param pp picked point; in case atom/bonds/residue picking is
    /// turned off or bounding box is picked this parameter is null.
    /// @param notify if true all observers are notified, @note
    /// child widget are not yet implemented as observers; therefore
    /// setting @code notify = true; @endcode has currently the effect of
    /// notifying the workspace widget only.
    void SelectMolecule( MolekelData::IndexType id,
                         const SoPickedPoint* pp,
                         bool multiSelection,
                         bool notify );
    /// Unselect all.
    void UnselectAll();
    /// Returns current interaction mode.
    InteractionMode GetInteractionMode() const { return interactionMode_; }
    /// Returns current picking mode.
    PickingMode GetPickingMode() const { return pickingMode_; }
    /// Sets interaction mode.
    void SetInteractionMode( InteractionMode im );
    /// Sets picking mode.
    void SetPickingMode( PickingMode pm );
    /// Redraws content of VTK widget and automatically adjusts
    /// clipping planes.
    void Refresh();
    /// Sets visibility property of a molecule.
    void SetMoleculeVisibility( MolekelData::IndexType i, bool visibility );
    /// Sets animation time step and restarts timer.
    void SetTimeStep( unsigned int ts )
    {
        timestep_ = ts;
        // if animating reset timer
        if( animationAction_->text() == tr( "Stop Animation" ) )
        {
            killTimer( animationTimerId_ );
            animationTimerId_ = startTimer( timestep_ );
        }
    }
    /// Sets animation time step.
    unsigned int GetTimeStep() const { return timestep_; }
    /// Returns molecule.
    MolekelMolecule* GetMolecule( MolekelData::IndexType i );
    /// Returns true if animation was started, false otherwise.
    bool AnimationStarted() const;
    /// Advance to next or previous animation frame and refresh 3d view.
    void UpdateAnimation( bool forward );
    /// Starts animation.
    /// @param startTimer if true a timer is started, if false
    /// no timer is started.
    void StartAnimation( bool startTimer );
    /// Stop animation and kills timer if timer was started.
    void StopAnimation();
    /// Returns snapshot of 3d view.
    vtkImageData* GetSnapshot() const;
    /// Sets lookup table for scalar bar.
    void SetMEPScalarBarLUT( vtkLookupTable* lut );
    /// Sets lookup table for probe widget scalar bar.
    void SetProbeWidgetScalarBarLUT( vtkLookupTable* lut );
    /// Sets range of probe widget's scalar bar.
    void SetProbeWidgetScalarBarRange( const double range[ 2 ] );
    /// Sets title of probe widget's scalar bar.
    void SetProbeWidgetScalarBarTitle( const char* title );
    /// Callback function used to report progress.
    /// Surface/Analysis dialog boxes can pass this function to
    /// MolekelMolecule methods to show progress in MainWindow's status bar.
    static void ProgressCallback( int step, int totalSteps, void* cbData );
    /// Updates analysis menu action depending on current state; this method
    /// currently only checks the size of the atom selection list and enables
    /// disables actions accordingly.
    void UpdateAnalysisActions();
	/// Updates toolbar edit actions depending on current state; this method
    /// currently only checks the size of the atom and bond selection list
	/// and enables disables actions accordingly.
    void UpdateEditActions();
    /// Displays message in status bar.
    void DisplayStatusMessage( const QString& );
    /// Set MSMS executable path.
    void SetMSMSExecutablePath( const QString& );
    /// Get MSMS executable path.
    QString GetMSMSExecutablePath() const;
    /// Load molecule.
    void LoadMolecule( const QString& fileName, const QString& format = QString() );
    /// Saves current 3D view snapshot to png or tiff file.
    void SaveSnapshot( const QString& fname, const QString& format = "png", unsigned scaling = 1 );
    /// Return size of 3D view
    QSize Get3DViewSize() const;
    /// Get shaders dir.
    QString GetShadersDir() const;
    /// Set shaders dir.
    void SetShadersDir( const QString& );
    /// Play events.
    /// @param filePath file path of events file
    /// @param initialDelay initial delay before first event in milliseconds
    /// @param minDelay minimum delay between subsequent events
    /// @param timeScaling time scale factor
    void PlayEvents( const char* filePath, unsigned initialDelay, int minDelay, double timeScaling );
    //@{ Depth peeling.
    /// Returns true if depth peeling is supported by VTK.
    bool DepthPeelingSupported() const;
    /// Enable/disable depth-peeling.
    void SetDepthPeelingEnabled( bool );
    /// Returns depth-peeling status.
    bool GetDepthPeelingEnabled() const;
    /// Set max peels. Zero means no maximum.
    void SetMaxNumberOfPeels( int );
    /// Get max peels.
    int GetMaxNumberOfPeels() const;
    /// Set occlusion ratio.
    /// In case of use of depth peeling technique for rendering translucent material,
    /// define the threshold under which the algorithm stops to iterate over peel layers.
    /// This is the ratio of the number of pixels that have been touched by the last layer
    /// over the total number of pixels of the viewport area. Initial value is 0.0, meaning
    /// rendering have to be exact.
    void SetDPOcclusionRatio( double );
    /// Return occlusion ratio.
    double GetDPOcclusionRatio() const;
    //@}
    /// Set number of anti-aliasing frames.
    void SetAAFrames( int );
    /// Get number of anti-aliasing frames.
    int GetAAFrames() const;
    /// Sets the background color.
    void SetBkColor( const QColor& );
    /// Forward call to CloseSlot()
    void closeEvent( QCloseEvent* ) { CloseSlot(); }
    /// Reset frame counter
    void ResetFrameCounter() { frameCounter_ = 0; }
    /// Update frame counter and display value in status bar.
    void UpdateFrameCounter( bool /* forward */ );
    /// Clear all
    void Clear() { ClearSlot(); }
    /// Delete selected molecule
    void DeleteMolecule() { DeleteMoleculeSlot(); }
    //@{ Menus
    QMenu* GetDisplayMenu() const { return displayMenu_; }
    QMenu* GetSurfacesMenu() const { return surfacesMenu_; }
    QMenu* GetAnalysisMenu() const { return analysisMenu_; }
    QMenu* GetStyleMenu() const { return styleButton_->menu(); }
    //@}
    /// Update view menu items according to visibility of contained widgets
    void UpdateViewMenu();
//@}

//@{ callbacks invoking interaction methods
private:

    //--------------------------------------------------------------------------
    /// Callback class. Execute method is called from within a vtkCommand::Execute
    /// method when molecule is picked.
    class SelectMoleculeCB : public MoleculeCallback
    {
        /// Reference to MainWindow instance.
        MainWindow* w_;
    public:
        SelectMoleculeCB( MainWindow* w ) : w_( w ) {}
        SelectMoleculeCB() : w_( 0 ) {} ;
        void SetMainWindow( MainWindow* w ) { w_ = w; }
        void Execute( MolekelData::IndexType i, SoPickedPoint* pp, bool multiSelection )
        {
            assert( w_ && "MainWindow is NULL" );
            w_->SelectMolecule( i, pp, multiSelection, true );
        }
    };
//@}
};

#endif /*MAINWINDOW_H_*/
