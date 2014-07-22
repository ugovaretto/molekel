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

// QT
#include <QWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QSpinBox>

#include "../MolekelMolecule.h"
#include "../old/molekeltypes.h"
#include "../AtomAnimation.h"
#include "../utility/RAII.h"

#include "MoleculeTrajectoryWidget.h"

//------------------------------------------------------------------------------
MoleculeTrajectoryWidget::MoleculeTrajectoryWidget( QWidget* parent )
    : QWidget( parent ), mol_( 0 ), animator_( 0 ),
      directionComboBox_( 0 ), loopModeComboBox_( 0 ),
      updatingGUI_( false )
{
    CreateGUI();
}


//------------------------------------------------------------------------------
/// @todo add option to create horizontal layout for inclusion into
/// an horizontal dockwidget or toolbar.
void MoleculeTrajectoryWidget::CreateGUI()
{
    /// Layout
    // Main layout
    QGridLayout* mainLayout = new QGridLayout();


    QLabel* directionLabel = new QLabel( tr( "Direction" ) );
    directionComboBox_ = new QComboBox;
    directionComboBox_->setEditable( false );
    directionComboBox_->addItem( tr( "Forward" ), true );
    directionComboBox_->addItem( tr( "Backward" ), false );
    connect( directionComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( DirectionIndexChangedSlot( int ) ) );

    QLabel* loopModeLabel = new QLabel( tr( "Loop Mode" ) );
    loopModeComboBox_ = new QComboBox;
    loopModeComboBox_->setEditable( false );
    loopModeComboBox_->addItem( tr( "Swing" ), AtomTrajectoryAnimator::SWING );
    loopModeComboBox_->addItem( tr( "Repeat" ), AtomTrajectoryAnimator::REPEAT );
    loopModeComboBox_->addItem( tr( "One Time" ), AtomTrajectoryAnimator::ONE_TIME );
    connect( loopModeComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( LoopModeIndexChangedSlot( int ) ) );

    QLabel* stepLabel = new QLabel( tr( "Step" ) );
    stepSpinBox_ = new QSpinBox;
    stepSpinBox_->setMinimum( 1 );
    stepSpinBox_->setValue( 1 );
    connect( stepSpinBox_, SIGNAL( valueChanged( int ) ),
             this, SLOT( StepChangedSlot( int ) ) );


    mainLayout->addWidget( directionLabel, 0, 0 );
    mainLayout->addWidget( directionComboBox_, 0, 1 );
    mainLayout->addWidget( loopModeLabel, 1, 0 );
    mainLayout->addWidget( loopModeComboBox_, 1, 1 );
    mainLayout->addWidget( stepLabel, 2, 0 );
    mainLayout->addWidget( stepSpinBox_, 2, 1 );

    /// Assign layout to this widget
    this->setLayout( mainLayout );

    this->setEnabled( false );
}

//------------------------------------------------------------------------------
void MoleculeTrajectoryWidget::UpdateGUI()
{
    this->setEnabled( false );
    if( mol_ == 0 || animator_ == 0 ) return;
    if( mol_->GetNumberOfFrames() <= 1 && mol_->GetMolekelMolecule() == 0 ) return;
    this->setEnabled( true );
    ResourceHandler< bool > rh( updatingGUI_, true, false );

    if( animator_->GetForward() ) directionComboBox_->setCurrentIndex( DIRECTION_FORWARD_INDEX );
    else directionComboBox_->setCurrentIndex( DIRECTION_BACKWARD_INDEX );

    stepSpinBox_->setMaximum( mol_->GetNumberOfFrames() );
    stepSpinBox_->setValue( animator_->GetStep() );

    switch( animator_->GetLoopMode() )
    {
        case AtomTrajectoryAnimator::ONE_TIME:
            loopModeComboBox_->setCurrentIndex( LOOPMODE_ONE_TIME_INDEX );
            break;
        case AtomTrajectoryAnimator::SWING:
            loopModeComboBox_->setCurrentIndex( LOOPMODE_SWING_INDEX );
            break;
        case AtomTrajectoryAnimator::REPEAT:
            loopModeComboBox_->setCurrentIndex( LOOPMODE_REPEAT_INDEX );
            break;
        default:
            assert( false && "UNKNOWN LOOP MODE" );
    }

}


//------------------------------------------------------------------------------
void MoleculeTrajectoryWidget::DirectionIndexChangedSlot( int i )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    if( i == DIRECTION_FORWARD_INDEX ) animator_->SetForward( true );
    else animator_->SetForward( false );
}

//------------------------------------------------------------------------------
void MoleculeTrajectoryWidget::LoopModeIndexChangedSlot( int i )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    switch( i )
    {
        case LOOPMODE_SWING_INDEX:
            animator_->SetLoopMode( AtomTrajectoryAnimator::SWING );
            break;
        case LOOPMODE_REPEAT_INDEX:
            animator_->SetLoopMode( AtomTrajectoryAnimator::REPEAT );
            break;
        case LOOPMODE_ONE_TIME_INDEX:
            animator_->SetLoopMode( AtomTrajectoryAnimator::ONE_TIME );
            break;
        default:
            assert( false && "INVALID INDEX FOR LOOP MODE" );
            break;
    }
}

//------------------------------------------------------------------------------
void MoleculeTrajectoryWidget::StepChangedSlot( int i )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    animator_->SetStep( i );
}
