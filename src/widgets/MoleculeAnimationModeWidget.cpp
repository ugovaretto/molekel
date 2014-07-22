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
#include <QSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QMessageBox>

#include "../MolekelMolecule.h"
#include "../old/molekeltypes.h"
#include "../AbstractAtomAnimator.h"
#include "../utility/RAII.h"
#include "../MainWindow.h"

#include "MoleculeAnimationModeWidget.h"

//------------------------------------------------------------------------------
MoleculeAnimationModeWidget::MoleculeAnimationModeWidget( MainWindow* mw, QWidget* parent )
    : QWidget( parent ), timeSpinBox_( 0 ), animationEnableComboBox_( 0 ),
      animationModeComboBox_( 0 ), animator_( 0 ), updatingGUI_( false ), mw_( mw )
{
    CreateGUI();
}


//------------------------------------------------------------------------------
/// @todo add option to create horizontal layout for inclusion into
/// an horizontal dockwidget or toolbar.
void MoleculeAnimationModeWidget::CreateGUI()
{
    /// Layout
    // Main layout
    QGridLayout* mainLayout = new QGridLayout();

    /// Widgets
    QLabel* animationEnableLabel = new QLabel( tr( "Animation" ) );
    animationEnableComboBox_ = new QComboBox;
    animationEnableComboBox_->setEditable( false );
    animationEnableComboBox_->addItem( tr( "On" ), true );
    animationEnableComboBox_->addItem( tr( "Off" ), false );
    connect( animationEnableComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( AnimationEnableIndexChangedSlot( int ) ) );

    QLabel* animationModeLabel = new QLabel( tr( "Animation Mode" ) );
    animationModeComboBox_ = new QComboBox;
    animationModeComboBox_->setEditable( false );
    animationModeComboBox_->addItem( tr( "Vibration" ), AbstractAtomAnimator::VIBRATION );
    animationModeComboBox_->addItem( tr( "Trajectory" ), AbstractAtomAnimator::TRAJECTORY );
    connect( animationModeComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT(  AnimationModeIndexChangedSlot( int ) ) );

    QLabel* timeLabel = new QLabel( tr( "Time step (ms)" ) );
    timeSpinBox_ = new QSpinBox;
    timeSpinBox_->setSingleStep( 10 );
    timeSpinBox_->setRange( 0, 2000 );
    connect( timeSpinBox_, SIGNAL( valueChanged( int ) ),
             this, SLOT( TimeIntValueChangedSlot( int ) ) );
    connect( timeSpinBox_, SIGNAL( valueChanged( const QString& ) ),
             this, SLOT( TimeStringValueChangedSlot( const QString& ) ) );

    mainLayout->addWidget( animationEnableLabel, 0, 0 );
    mainLayout->addWidget( animationEnableComboBox_, 0, 1 );
    mainLayout->addWidget( animationModeLabel, 1, 0 );
    mainLayout->addWidget( animationModeComboBox_, 1, 1 );
    mainLayout->addWidget( timeLabel, 2, 0 );
    mainLayout->addWidget( timeSpinBox_, 2, 1 );

    /// Assign layout to this widget
    this->setLayout( mainLayout );

    this->setEnabled( false );
}

//------------------------------------------------------------------------------
void MoleculeAnimationModeWidget::UpdateGUI()
{
    this->setEnabled( false );
    if( animator_ == 0 ) return;
    if( animator_->GetMolecule()->GetNumberOfFrames() <= 1 &&
        animator_->GetMolecule()->GetMolekelMolecule() == 0 ) return;
    this->setEnabled( true );
    ResourceHandler< bool > rh( updatingGUI_, true, false );

    if( animator_->GetEnable() ) animationEnableComboBox_->setCurrentIndex( ANIMATION_ON_INDEX );
    else animationEnableComboBox_->setCurrentIndex( ANIMATION_OFF_INDEX );

    switch( animator_->GetAnimationMode() )
    {
        case AbstractAtomAnimator::VIBRATION:
            animationModeComboBox_->setCurrentIndex( VIBRATION_INDEX );
            break;
        case AbstractAtomAnimator::TRAJECTORY:
            animationModeComboBox_->setCurrentIndex( TRAJECTORY_INDEX );
            break;
        default:
            assert( false && "UNKNOWN ANIMATION MODE" );
            break;
    }
    timeSpinBox_->setValue( animator_->GetTimeStep() );
    animationModeComboBox_->setEnabled( !mw_->AnimationStarted() );
   
}

//------------------------------------------------------------------------------
void MoleculeAnimationModeWidget::AnimationEnableIndexChangedSlot( int i )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    if( i == ANIMATION_ON_INDEX ) animator_->SetEnable( true );
    else animator_->SetEnable( false );
}

//------------------------------------------------------------------------------
void MoleculeAnimationModeWidget::AnimationModeIndexChangedSlot( int i )
{
    if( updatingGUI_ ) return;
    assert( mw_ );
  
    assert( animator_ && "NULL animator" );
    switch( i )
    {
        case VIBRATION_INDEX:
            animator_->SetAnimationMode( AbstractAtomAnimator::VIBRATION );
            break;
        case TRAJECTORY_INDEX:
            animator_->SetAnimationMode( AbstractAtomAnimator::TRAJECTORY );
            break;
        default:
            assert( false && "INVALID INDEX FOR ANIMATION MODE" );
            break;
    }
}

//------------------------------------------------------------------------------
void MoleculeAnimationModeWidget::TimeIntValueChangedSlot( int v )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    animator_->SetTimeStep( v );
}

//------------------------------------------------------------------------------
void MoleculeAnimationModeWidget::TimeStringValueChangedSlot( const QString& s )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    animator_->SetTimeStep( s.toInt() );
}
