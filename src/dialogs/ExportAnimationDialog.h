#ifndef EXPORTANIMATIONDIALOG_H_
#define EXPORTANIMATIONDIALOG_H_
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

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QGridLayout>

/// @warning cannot define Q_OBJECT classes as local classes since they
/// define a static method.
/// @warning cannot define more than own Q_OBJECT class per translation unit

/// Dialog to retrieve FPS and total animation time.
/// @todo change layout to QGridLayout.
class ExportAnimationDialog : public QDialog
{
    Q_OBJECT
 public:
    /// Constructor.
    ExportAnimationDialog(  QWidget* parent, int fps, int totalTime ) : QDialog( parent ),
                                                bboxScalingFactor_( 1.1 ),
                                                sesProbeRadius_( 1.4 ),
                                                sesDensity_( 10. ),
                                                step_( 0.2 ),
                                                bothSigns_( false ),
                                                nodalSurface_( false ),
                                                mapMepOnOrbitals_( false ),
                                                mapMepOnSES_( false ),
                                                mapMepOnElDensSurface_( false ),
                                                totalTime_( totalTime ),
                                                fps_( fps ),
                                                saveFrames_( true )
    {
        // main layout
        QVBoxLayout* mainLayout = new QVBoxLayout;

        QGroupBox* group = new QGroupBox;

        QGridLayout* groupLayout = new QGridLayout;

        // time
        groupLayout->addWidget( new QLabel( tr( "Total time (s) " ) ), 0, 0 );
        timeSpinBox_ = new QSpinBox;
        timeSpinBox_->setMinimum( 0 );
        timeSpinBox_->setMaximum( 10000 );
        timeSpinBox_->setSingleStep( 1 );
        timeSpinBox_->setValue( totalTime_ );
        connect( timeSpinBox_, SIGNAL( valueChanged( int ) ),
                 this, SLOT( TimeChanged( int ) ) );
        groupLayout->addWidget( timeSpinBox_, 0, 1 );

        // fps
        groupLayout->addWidget( new QLabel( tr( "Frames per second" ) ), 1, 0 );
        fpsSpinBox_ = new QDoubleSpinBox;
        fpsSpinBox_->setMinimum( 0 );
        fpsSpinBox_->setSingleStep( 0.2 );
        fpsSpinBox_->setValue( fps_ );
        connect( fpsSpinBox_, SIGNAL( valueChanged( double ) ),
                 this, SLOT( FPSChanged( double ) ) );
        groupLayout->addWidget( fpsSpinBox_, 1, 1 );

        // frames/avi
        groupLayout->addWidget( new QLabel( "Output type " ), 2, 0 );
        QComboBox* typeComboBox = new QComboBox;
        typeComboBox->addItem( tr( "Individual frames" ), true );
        typeComboBox->setCurrentIndex( 0 );
#ifdef WIN32 // enable AVI output when compiling on windows
        typeComboBox->addItem( tr( "AVI" ), false );
#endif
        typeComboBox->setEditable( false );
        connect( typeComboBox, SIGNAL( currentIndexChanged( int ) ),
                 this, SLOT( OutputTypeChangedSlot( int ) ) );
        groupLayout->addWidget( typeComboBox, 2, 1 );

        group->setLayout( groupLayout );
        mainLayout->addWidget( group );

        // surfaces
        AddSurfaceAnimationUI( mainLayout );

        // ok, cancel
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        QPushButton* okButton = new QPushButton( tr( "Ok" ) );
        connect( okButton, SIGNAL( released() ), this, SLOT( accept() ) );
        buttonLayout->addWidget( okButton );
        QPushButton* cancelButton = new QPushButton( tr( "Cancel" ) );
        connect( cancelButton, SIGNAL( released() ), this, SLOT( reject() ) );
        buttonLayout->addWidget( cancelButton );
        mainLayout->addItem( buttonLayout );

        this->setLayout( mainLayout );

    }
    /// Returns total time.
    int GetTotalTime() const { return totalTime_; }
    /// Returns frames per second.
    double GetFPS() const { return fps_; }
    /// Returns true if option to save individual frames selected.
    /// @todo implement with combobox.
    bool SaveFrames() const { return saveFrames_; }
    /// Set total time: initialize to MAX( num frames ) x fps
    void SetTotalTime( int t ) { totalTime_ = t; timeSpinBox_->setValue( totalTime_ ); }
    /// Set frames per second.
    void SetFPS( int fps ) { fps_ = fps; fpsSpinBox_->setValue( fps_ ); }

private:
    void AddSurfaceAnimationUI( QLayout* layout )
    {
        QGroupBox* surfaceGroup = new QGroupBox( "Surface Animation" );
        QVBoxLayout* mainLayout = new QVBoxLayout;

        // SES probe radius
        QHBoxLayout* sesRadiusLayout = new QHBoxLayout;
        sesRadiusLayout->addWidget( new QLabel( "(MSMS) SES Probe Radius  " ) );
        QDoubleSpinBox* probeRadiusSpinBox = new QDoubleSpinBox;
        probeRadiusSpinBox->setDecimals( 5 );
        probeRadiusSpinBox->setMinimum( 0. );
        probeRadiusSpinBox->setValue( 1.4 );
        probeRadiusSpinBox->setSingleStep( 0.1 );
        connect( probeRadiusSpinBox, SIGNAL( valueChanged( double ) ),
                 this, SLOT( SesProbeRadiusSlot( double ) ) );
        sesRadiusLayout->addWidget( probeRadiusSpinBox );
        mainLayout->addItem( sesRadiusLayout );

        // SES density
        QHBoxLayout* sesDensityLayout = new QHBoxLayout;
        sesDensityLayout->addWidget( new QLabel( "(MSMS) SES Density" ) );
        QDoubleSpinBox* densitySpinBox = new QDoubleSpinBox;
        densitySpinBox->setMinimum( 0.5 );
        densitySpinBox->setValue( 10. );
        densitySpinBox->setSingleStep( 0.5 );
        densitySpinBox->setMaximum( 1000 );
        connect( densitySpinBox, SIGNAL( valueChanged( double ) ),
                 this, SLOT( SesDensitySlot( double ) ) );
        sesDensityLayout->addWidget( densitySpinBox );
        mainLayout->addItem( sesDensityLayout );

        // Bounding box
        QHBoxLayout* bboxLayout = new QHBoxLayout;
        bboxLayout->addWidget( new QLabel( "Bounding Box Scaling Factor  " ) );
        QDoubleSpinBox* bboxSpinBox = new QDoubleSpinBox;
        bboxSpinBox->setMinimum( 1. );
        bboxSpinBox->setValue( 1.1 );
        bboxSpinBox->setSingleStep( 0.5 );
        bboxSpinBox->setMaximum( 100 );
        connect( bboxSpinBox, SIGNAL( valueChanged( double ) ),
                 this, SLOT( BBoxFactorSlot( double ) ) );
        bboxLayout->addWidget( bboxSpinBox );
        mainLayout->addItem( bboxLayout );

        // Step (for iso-surface generation)
        QHBoxLayout* stepLayout = new QHBoxLayout;
        stepLayout->addWidget( new QLabel( "Iso-surface Step" ) );
        QDoubleSpinBox* stepSpinBox = new QDoubleSpinBox;
        stepSpinBox->setValue( .2 );
        stepSpinBox->setDecimals( 4 );
        stepSpinBox->setSingleStep( 0.05 );
        connect( stepSpinBox, SIGNAL( valueChanged( double ) ),
                 this, SLOT( StepSlot( double ) ) );
        stepLayout->addWidget( stepSpinBox );
        mainLayout->addItem( stepLayout );

        // Both signs for orbitals
        QCheckBox* bothSignsCheckBox = new QCheckBox( "Use both signs (orbitals)" );
        mainLayout->addWidget( bothSignsCheckBox );
        connect( bothSignsCheckBox, SIGNAL( stateChanged( int ) ),
                 this, SLOT( BothSignsSlot( int ) ) );

        // Nodal surface
        QCheckBox* nodalSurfaceCheckBox = new QCheckBox( "Nodal Surface (orbitals)" );
        mainLayout->addWidget( nodalSurfaceCheckBox );
        connect( nodalSurfaceCheckBox, SIGNAL( stateChanged( int ) ),
                 this, SLOT( NodalSurfaceSlot( int ) ) );

        // MEP on orbitals
        QCheckBox* mepOrbitalCheckBox = new QCheckBox( "Map MEP on orbitals" );
        mainLayout->addWidget( mepOrbitalCheckBox );
        connect( mepOrbitalCheckBox, SIGNAL( stateChanged( int ) ),
                 this, SLOT( MepOrbitalSlot( int ) ) );

        // MEP on SES
        QCheckBox* mepSESCheckBox = new QCheckBox( "Map MEP on SES" );
        mainLayout->addWidget( mepSESCheckBox );
        connect( mepSESCheckBox, SIGNAL( stateChanged( int ) ),
                 this, SLOT( MepSESSlot( int ) ) );

        // MEP on electron density surface
        QCheckBox* mepElDensCheckBox = new QCheckBox( "Map MEP on Electron Density Surface" );
        mainLayout->addWidget( mepElDensCheckBox );
        connect( mepElDensCheckBox, SIGNAL( stateChanged( int ) ),
                 this, SLOT( MepElDensSlot( int ) ) );

        surfaceGroup->setLayout( mainLayout );
        layout->addWidget( surfaceGroup );
    }

private slots:

    /// SES probe radius.
    void SesProbeRadiusSlot( double v ) { sesProbeRadius_ = v; }

    /// SES density.
    void SesDensitySlot( double v ) { sesDensity_= v; }

    /// Bounding box scaling factor.
    void BBoxFactorSlot( double v ) { bboxScalingFactor_  = v; }

    /// Step for isosurface generation.
    void StepSlot( double v ) { step_ = v; }

    /// Both signs (orbitals).
    void BothSignsSlot( int state ) { bothSigns_ = ( state == Qt::Checked ); }

    /// Nodal surface (orbitals).
    void NodalSurfaceSlot( int state ) { nodalSurface_ = ( state == Qt::Checked ); }

    /// MEP on orbitals.
    void MepOrbitalSlot( int state ) { mapMepOnOrbitals_ = ( state == Qt::Checked ); }

    /// MEP on SES.
    void MepSESSlot( int state ) { mapMepOnSES_ = ( state == Qt::Checked ); }

    /// MEP on el. dens. surface.
    void MepElDensSlot( int state ) { mapMepOnElDensSurface_ = ( state == Qt::Checked ); }

    /// Called when total time value changed.
    void TimeChanged( int v ) { totalTime_ = v; }
    /// Called when frames per second value changed.
    void FPSChanged( double v ) { fps_ = v; }
    /// Called when combo box index changes.
    void OutputTypeChangedSlot( int v )
    {
        saveFrames_ = v == SAVE_FRAMES_INDEX;
    }

public:
    /// Returns SES probe radius.
    double GetSESProbeRadius() const { return sesProbeRadius_; }
    /// Returns SES density.
    double GetSESDensity() const { return sesDensity_; }
    /// Returns Bounding Box scaling factor.
    double GetBBoxScalingFactor() const { return bboxScalingFactor_; }
    /// Returns step used for iso-surface generation.
    double GetStep() const { return step_; }
    /// Returns true if both iso-value signs are used to compute orbitals;
    /// false otherwise.
    bool UseBothSigns() const { return bothSigns_; }
    /// Returns true if nodal surface is generated when computing orbitals;
    /// false otherwise.
    bool GenerateNodalSurface() const { return nodalSurface_; }
    /// Returns true if orbital surfaces are color-coded with electrostatic
    /// potential; false otherwise.
    bool MapMEPOnOrbitals() const { return mapMepOnOrbitals_; }
    /// Returns true if Solvent Excluded Surface is color-coded with electrostatic
    /// potential; false otherwise.
    bool MapMEPOnSES() const { return mapMepOnSES_; }
    /// Returns true if electron density surface generated from density matrix
    /// is color-coded with electrostatic potential; false otherwise.
    bool MapMEPOnElDensSurface() const { return mapMepOnElDensSurface_; }

private:
    /// Total time.
    QSpinBox* timeSpinBox_;
    /// FPS.
    QDoubleSpinBox* fpsSpinBox_;

    /// SES probe radius.
    double sesProbeRadius_;
    /// SES density.
    double sesDensity_;
    /// Bounding box scaling factor.
    double bboxScalingFactor_;
    /// Step for iso-surface generation.
    double step_;
    /// Both signs (orbitals).
    bool bothSigns_;
    /// Nodal surface (orbitals).
    bool nodalSurface_;
    /// MEP on orbitals.
    bool mapMepOnOrbitals_;
    /// MEP on SES.
    bool mapMepOnSES_;
    /// MEP on el. dens. surface.
    bool mapMepOnElDensSurface_;
    /// Total animation time in seconds.
    int totalTime_;
    /// Frames per second.
    double fps_;
    /// Save individual frames/AVI
    bool saveFrames_;
    /// Index of 'Save individual frames' in combo box.
    static const int SAVE_FRAMES_INDEX = 0;
};

#endif /*EXPORTANIMATIONDIALOG_H_*/
