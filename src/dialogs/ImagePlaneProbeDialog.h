#ifndef IMAGEPLANEPROBEDIALOG_H_
#define IMAGEPLANEPROBEDIALOG_H_
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
#include <vtkImagePlaneWidget.h>
#include <vtkImageData.h>

// QT
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QThread>

#include "../MainWindow.h"
#include "../MolekelMolecule.h"
#include "../widgets/ImagePlaneProbeWidget.h"

#include <QMessageBox>

class vtkProp3D;

/// Class used to asynchronously generate orbital grid data and assign
/// the generated data to a vtkImagePlaneWidget.
/// Instances of this class will start a new thread to generate data
/// when pressing on the 'Generate' button and will stop the running
/// thread when pressing on 'Stop'.
/// @note it was not possible to add multithreading functionality inside
/// the molecule class itsels without adding a dependency on Qt.
/// @todo move all the multithreading code inside MolekelMolecule.
class ImagePlaneProbeDialog : public QDialog
{
    Q_OBJECT

private:

    /// Enables/disables buttons; called from slots.
    void UpdateButtons()
    {
        if( thread_->isRunning() ) return;
        if( selectedOrbital_ >= 0 && pw_->GetSelectedDataType() == ImagePlaneProbeWidget::ORBITAL
            || mol_->HasGridData() && pw_->GetSelectedDataType() == ImagePlaneProbeWidget::GRID_DATA
            || mol_->CanComputeElectronDensity() &&
                    pw_->GetSelectedDataType() == ImagePlaneProbeWidget::ELECTRON_DENSITY
            || mol_->CanComputeSpinDensity() &&
                    pw_->GetSelectedDataType() == ImagePlaneProbeWidget::SPIN_DENSITY
            || mol_->CanComputeMEP() &&
                    pw_->GetSelectedDataType() == ImagePlaneProbeWidget::MEP )
        {
             generateButton_->setEnabled( true );
        }
        else generateButton_->setEnabled( false );
    }

private slots:

    /// Called whenever an orbital has been selected.
    void OrbitalSelectedSlot( int orbitalIndex )
    {
        selectedOrbital_ = orbitalIndex;
        UpdateButtons();
    }

    /// Called when Grid Data group is selected.
    void GridDataToggledSlot( bool )
    {
        UpdateButtons();
    }

    /// Called when electron density group is selected.
    void ElectronDensityToggledSlot( bool )
    {
        UpdateButtons();
    }

    /// Called when electron density group is selected.
    void SpinDensityToggledSlot( bool )
    {
        UpdateButtons();
    }

    /// Called when Molecular Electrostatic Potential group is selected.
    void MEPToggledSlot( bool )
    {
        UpdateButtons();
    }

    /// Called when Orbitals group is selected.
    void OrbitalsToggledSlot( bool )
    {
        UpdateButtons();
    }

    /// Called whenever the step used to generate grid data changes.
    void StepChangedSlot( double step )
    {
        step_ = step;
    }

    /// Updates widget ui with value range.
    void UpdateWidgetUI()
    {
        if( DataGenerationStopped() ) return;
        switch( pw_->GetSelectedDataType() )
        {
            case ImagePlaneProbeWidget::ORBITAL:
                    break;
            case ImagePlaneProbeWidget::ELECTRON_DENSITY:
                    pw_->SetElectronDensityRange( minValue_, maxValue_ );
                    break;
            case ImagePlaneProbeWidget::SPIN_DENSITY:
                    pw_->SetSpinDensityRange( minValue_, maxValue_ );
                    break;
            case ImagePlaneProbeWidget::MEP:
                    pw_->SetMEPRange( minValue_, maxValue_ );
                    break;
            case ImagePlaneProbeWidget::GRID_DATA:
                    break;
            default:
                    break;
        }

    }

    /// Called when data generation thread ends.
    void ThreadFinishedSlot()
    {
        generateButton_->setText( tr( "Generate Data" ) );
        UpdateWidgetUI();
        if( DataGenerationStopped() ) return;
        mw_->DisplayStatusMessage( "Done" );
        switch( pw_->GetSelectedDataType() )
        {
            case ImagePlaneProbeWidget::ORBITAL:
                    {
                        const QString s( tr( "Electron Density, orbital %1" ).arg( selectedOrbital_ ) );
                        mw_->SetProbeWidgetScalarBarTitle( s.toStdString().c_str() );
                    }
                    break;
            case ImagePlaneProbeWidget::ELECTRON_DENSITY:
                    pw_->SetElectronDensityRange( minValue_, maxValue_ );
                    mw_->SetProbeWidgetScalarBarTitle( "Electron Density - Density Matrix" );
                    break;
            case ImagePlaneProbeWidget::SPIN_DENSITY:
                    pw_->SetSpinDensityRange( minValue_, maxValue_ );
                    mw_->SetProbeWidgetScalarBarTitle( "Spin Density" );
                    break;
            case ImagePlaneProbeWidget::MEP:
                    pw_->SetMEPRange( minValue_, maxValue_ );
                    mw_->SetProbeWidgetScalarBarTitle( "Electrostatic Potential" );
                    break;
            case ImagePlaneProbeWidget::GRID_DATA:
                    {
                        const std::string sl = pw_->GetGridLabel();
                        if( sl.size() != 0 )
                        {
                            mw_->SetProbeWidgetScalarBarTitle( ( "Grid Data - " + sl ).c_str() );
                        }
                        else mw_->SetProbeWidgetScalarBarTitle( "Grid Data" );
                    }
                    break;
            default:
                    break;
        }
        mw_->Refresh();

    }
    /// Called when data generation thread starts.
    void ThreadStartedSlot()
    {
        mw_->DisplayStatusMessage( "Generating data..." );
        //generateButton_->setText( "Stop" );

    }
    /// Called when generate button pressed.
    void GenerateSlot()
    {
        if( step_ <= 0. )
        {
            QMessageBox::critical( this, "Invalid Input", "Step must be > 0" );
            return;
        }
        // disable multithreading: issues on Linux
        ThreadStartedSlot();
        thread_->run();
        ThreadFinishedSlot();
        return;
        // plane widget must be disabled
        // if thread is running stop thread and wait for completion.
        if( thread_->isRunning() )
        {
            StopDataGeneration();
            thread_->wait();
            return;
        }
        thread_->start();
       }
    /// Called when close button pressed.
    void CloseSlot()
    {
       done( QDialog::Rejected );
    }

public:
    /// Overridden method: in case thread is running stop thread
    /// and wait for completion.
    void done( int r )
    {
        if( thread_->isRunning() )
        {
            StopDataGeneration();
            thread_->wait();
        }
        QDialog::done( r );
    }

    /// Generate vtkImageData.
    vtkImageData* GenerateData()
    {
        ProgressCallback pcb = MainWindow::ProgressCallback;
        switch( pw_->GetSelectedDataType() )
        {
            case ImagePlaneProbeWidget::ORBITAL:
                return mol_->GenerateMOGridData( selectedOrbital_, step_, pcb, mw_ );
                break;
            case ImagePlaneProbeWidget::ELECTRON_DENSITY:
                return mol_->GenerateElectronDensityData( step_, minValue_, maxValue_, pcb, mw_ );
                break;
            case ImagePlaneProbeWidget::SPIN_DENSITY:
                return mol_->GenerateSpinDensityData( step_, minValue_, maxValue_, pcb, mw_ );
                break;
            case ImagePlaneProbeWidget::MEP:
                return mol_->GenerateMEPData( step_, minValue_, maxValue_, pcb, mw_ );
                break;
            case ImagePlaneProbeWidget::GRID_DATA:
                return mol_->GridDataToVtkImageData( pw_->GetGridLabel(), 1, pcb, mw_ );
                break;
            default:
                break;
        }
        return 0;
    }

    /// Was thread stopped ?
    bool DataGenerationStopped()
    {
        switch( pw_->GetSelectedDataType() )
        {
            case ImagePlaneProbeWidget::ORBITAL:
                return mol_->MOGridDataGenerationStopped();
                break;
            case ImagePlaneProbeWidget::ELECTRON_DENSITY:
                return mol_->ElectronDensityDataGenerationStopped();
                break;
            case ImagePlaneProbeWidget::SPIN_DENSITY:
                return mol_->SpinDensityDataGenerationStopped();
                break;
            case ImagePlaneProbeWidget::MEP:
                return mol_->MEPDataGenerationStopped();
                break;
            case ImagePlaneProbeWidget::GRID_DATA:
                return false;
                break;
            default:
                break;
        }

        return false;
    }

    /// Stop data generation thread.
    void StopDataGeneration()
    {
        switch( pw_->GetSelectedDataType() )
        {
            case ImagePlaneProbeWidget::ORBITAL:
                mol_->StopMOGridDataGeneration();
                break;
            case ImagePlaneProbeWidget::ELECTRON_DENSITY:
                mol_->StopElectronDensityDataGeneration();
                break;
            case ImagePlaneProbeWidget::SPIN_DENSITY:
                mol_->StopSpinDensityDataGeneration();
                break;
            case ImagePlaneProbeWidget::MEP:
                mol_->StopMEPDataGeneration();
                break;
            case ImagePlaneProbeWidget::GRID_DATA:
                break;
            default:
                break;
        }
    }

    /// Sets the scalar value range.
    void SetDataRange( const double range[ 2 ] )
    {
        dataRange_[ 0 ] = range[ 0 ];
        dataRange_[ 1 ] = range[ 1 ];
    }

    /// Returns the vtkProp3D associated with the data if any.
    vtkProp3D* GetProp3D( ) const
    {
        return mol_->GetBBoxActor();
    }

public:
    /// Constructor.
    ImagePlaneProbeDialog( vtkImagePlaneWidget* w,
                           MolekelMolecule* mol,
                           MainWindow* mw )
                          : QDialog( mw ),
                              pw_( 0 ), vtkImagePlaneWidget_( w ), mw_( mw ), mol_( mol ),
                            enabled_( false ), step_( 0.25 ), selectedOrbital_( -1 ),
                            generateButton_( 0 ), closeButton_( 0 ), thread_( 0 )
    {
        dataRange_[ 0 ] = 0.;
        dataRange_[ 1 ] = 1.;
        assert( mol && "NULL Molecule" );
        assert( mw &&  "NULL Main Window" );
        pw_ = new ImagePlaneProbeWidget( mol_ );
        pw_->SetMolecule( mol_ );
        connect( pw_, SIGNAL( OrbitalSelected( int ) ),
                 this, SLOT( OrbitalSelectedSlot( int ) ) );
        connect( pw_, SIGNAL( StepChanged( double ) ),
                 this, SLOT( StepChangedSlot( double ) ) );
        connect( pw_, SIGNAL( OrbitalsToggled( bool ) ),
                 this, SLOT( OrbitalsToggledSlot( bool ) ) );
        connect( pw_, SIGNAL( GridDataToggled( bool ) ),
                 this, SLOT( GridDataToggledSlot( bool ) ) );
        connect( pw_, SIGNAL( ElectronDensityToggled( bool ) ),
                 this, SLOT( ElectronDensityToggledSlot( bool ) ) );
        connect( pw_, SIGNAL( SpinDensityToggled( bool ) ),
                 this, SLOT( SpinDensityToggledSlot( bool ) ) );
        connect( pw_, SIGNAL( MEPToggled( bool ) ),
                 this, SLOT( MEPToggledSlot( bool ) ) );

        QVBoxLayout* mainLayout = new QVBoxLayout;
        mainLayout->addWidget( pw_ );
        // Separator
        QFrame* line = new QFrame;
        line->setFrameShape( QFrame::HLine );
        mainLayout->addWidget( line );
        // Ok, Apply, Cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        generateButton_ = new QPushButton( tr( "Generate Data" ) );
        generateButton_->setEnabled( false );
        connect( generateButton_, SIGNAL( released() ), this, SLOT( GenerateSlot() ) );
        buttonLayout->addWidget( generateButton_ );
        closeButton_ = new QPushButton( tr( "Close" ) );
        connect( closeButton_, SIGNAL( released() ), this, SLOT( CloseSlot() ) );
        buttonLayout->addWidget( closeButton_ );
        mainLayout->addItem( buttonLayout );
        this->setLayout( mainLayout );
        thread_ = new DataGenerationThread( this, *vtkImagePlaneWidget_ );
        connect( thread_, SIGNAL( finished() ), this, SLOT( ThreadFinishedSlot() ) );
        connect( thread_, SIGNAL( started() ), this, SLOT( ThreadStartedSlot() ) );
    }
    /// Destructor: delete thread.
    ~ImagePlaneProbeDialog() { delete thread_; }



private:

    /// ImagePlaneProbeWidget instance owned by this dialog.
    ImagePlaneProbeWidget* pw_;
    /// Reference to vtkImagePlaneWidget
    vtkImagePlaneWidget* vtkImagePlaneWidget_;
    /// Refernce to main window.
    MainWindow* mw_;
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Enable/disable.
    bool enabled_;
    /// Step for orbital grid generation.
    double step_;
    /// Currently selected orbital.
    int selectedOrbital_;
    /// Generates orbital grid data asynchronously.
    QPushButton* generateButton_;
    /// Closes window ans stops thread if running.
    QPushButton* closeButton_;
    // @{ Value range
    double minValue_;
    double maxValue_;
    // @}
    /// Scalar data range [min, max].
    double dataRange_[ 2 ];
    /// Thread to generate data: calls proper MolekelMolecule
    /// methods to generate orbital grid data.
    class DataGenerationThread : public QThread
       {
           vtkImagePlaneWidget& pw_;
    public:
        DataGenerationThread( QObject* parent,
                                vtkImagePlaneWidget& pw )
                                : QThread( parent ), pw_( pw ) {}
           void run()
           {
                ImagePlaneProbeDialog* pd = dynamic_cast< ImagePlaneProbeDialog* >( parent() );
                assert( pd );
                vtkSmartPointer< vtkImageData > data = pd->GenerateData();
                if( !pd->DataGenerationStopped() )
                {
                    assert( data && "NULL vtkImageData" );
                    pw_.SetInput( data );
                    const double* range = data->GetScalarRange();
                    assert( range && "NULL data range" );
                    pw_.SetProp3D( pd->GetProp3D() );
                    pw_.SetWindowLevel( range[ 1 ] - range[ 0 ], .5 * ( range[ 0 ] + range[ 1 ] ) );
                    pd->SetDataRange( range );
                }
        }
    };
    DataGenerationThread* thread_;
};


#endif /*IMAGEPLANEPROBEDIALOG_H_*/
