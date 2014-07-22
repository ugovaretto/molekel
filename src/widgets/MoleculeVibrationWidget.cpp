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
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QCheckBox>

#include "../MolekelMolecule.h"
#include "../old/molekeltypes.h"
#include "../AtomAnimation.h"
#include "../utility/RAII.h"

#include "MoleculeVibrationWidget.h"

//------------------------------------------------------------------------------
MoleculeVibrationWidget::MoleculeVibrationWidget( QWidget* parent )
    : QWidget( parent ), mol_( 0 ), animator_( 0 ), scalingSpinBox_( 0 ),
      arrowScalingSpinBox_( 0 ), table_( 0 ), updatingGUI_( false )
{
    CreateGUI();
}


//------------------------------------------------------------------------------
void MoleculeVibrationWidget::CreateGUI()
{
    /// Layouts
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    // Scaling layout
    QHBoxLayout* scalingLayout = new QHBoxLayout;
    // Show arrows checkbox layout
    QHBoxLayout* showArrowsLayout = new QHBoxLayout;
    // Const arrow length checkbox
    QHBoxLayout* constArrowLengthLayout = new QHBoxLayout;
    /// Controls
    // Scaling label
    QLabel* scalingLabel = new QLabel( tr( "Scaling factor" ) );
    // Scaling spinbox
    /// @todo review min, max, step default values
    scalingSpinBox_ = new QDoubleSpinBox();
    scalingSpinBox_->setDecimals( 1 );
    scalingSpinBox_->setSingleStep( 0.1 );
    scalingSpinBox_->setValue( 2.5 );
    connect( scalingSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( DoubleValueChangedSlot( double ) ) );
    connect( scalingSpinBox_, SIGNAL( valueChanged( const QString& ) ),
             this, SLOT( StringValueChangedSlot( const QString& ) ) );
    scalingSpinBox_->setEnabled( false );

    // Show arrows checkbox
    showArrowsCheckBox_ = new QCheckBox( "Show arrows" );
    connect( showArrowsCheckBox_, SIGNAL( stateChanged( int ) ),
             this, SLOT( ShowArrowsSlot( int ) ) );
    showArrowsCheckBox_->setEnabled( false );

    // Constant arrow length check box
    constArrowLengthCheckBox_ = new QCheckBox( "Constant arrow length" );
    connect( constArrowLengthCheckBox_, SIGNAL( stateChanged( int ) ),
             this, SLOT( ConstArrowLengthSlot( int ) ) );
    constArrowLengthCheckBox_->setEnabled( false );

    // Arrows scaling factor TBD
    QHBoxLayout* scaleArrowsLayout = new QHBoxLayout;
    scaleArrowsLayout->addWidget( new QLabel( "Arrow scaling" ) );
    arrowScalingSpinBox_ = new QDoubleSpinBox;
    arrowScalingSpinBox_->setValue( 1.0 );
    arrowScalingSpinBox_->setSingleStep( 0.2 );
    arrowScalingSpinBox_->setEnabled( false );
    connect( arrowScalingSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ArrowScalingChangedSlot( double ) ) );
    scaleArrowsLayout->addWidget( arrowScalingSpinBox_ );

    // Table
    table_ = new QTableWidget();
    table_->setColumnCount( 6 ); // checkbox, frequency, type, IR, Mass, Raman
    QStringList labels;
    labels << tr( "Selected" ) << tr( "Frequency" ) << tr( "Type" )
    	   << tr( "IR" )       << tr( "Red. Mass" ) << tr( "Raman" );
    table_->setHorizontalHeaderLabels( labels );
    table_->setEditTriggers( QAbstractItemView::NoEditTriggers );
    table_->setAlternatingRowColors( true );
    connect( table_, SIGNAL( itemChanged( QTableWidgetItem* ) ),
                this, SLOT( TableItemChangedSlot( QTableWidgetItem* ) ) );
    table_->verticalHeader()->hide();
    table_->setEnabled( false );

    /// Fill layouts
    scalingLayout->addWidget( scalingLabel );
    scalingLayout->addWidget( scalingSpinBox_ );
    showArrowsLayout->addWidget( showArrowsCheckBox_ );
    constArrowLengthLayout->addWidget( constArrowLengthCheckBox_ );
    mainLayout->addItem( scalingLayout );
    mainLayout->addItem( showArrowsLayout );
    mainLayout->addItem( constArrowLengthLayout );
    mainLayout->addItem( scaleArrowsLayout );
    mainLayout->addWidget( table_ );

    /// Assign layout to this widget
    this->setLayout( mainLayout );
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::UpdateGUI()
{
    table_->setEnabled( false );
    scalingSpinBox_->setEnabled( false );
    arrowScalingSpinBox_->setEnabled( false );
    if( mol_ == 0 || animator_ == 0 || !mol_->HasVibrationData() ) return;
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    // update scaling factor
    scalingSpinBox_->setValue( mol_->GetMolekelMolecule()->sc_freq_ar );
    scalingSpinBox_->setEnabled( true );
    // update show arrows checkbox
    showArrowsCheckBox_->setCheckState(
        mol_->GetVibrationVectorsVisibility() ? Qt::Checked : Qt::Unchecked
        );
    showArrowsCheckBox_->setEnabled( true );
    arrowScalingSpinBox_->setValue( animator_->GetArrowScalingFactor() );
    arrowScalingSpinBox_->setEnabled( showArrowsCheckBox_->checkState() == Qt::Checked
                                      && !animator_->GetConstantArrowLength()  );
    constArrowLengthCheckBox_->setCheckState(
        animator_->GetConstantArrowLength() ? Qt::Checked : Qt::Unchecked
        );
    constArrowLengthCheckBox_->setEnabled( showArrowsCheckBox_->checkState() == Qt::Checked );
    // update frequencies
    const int freqs = mol_->GetMolekelMolecule()->vibration.size();
    table_->setRowCount( freqs );
    int rc = 0;
    
    for( VibrationList::const_iterator i = mol_->GetMolekelMolecule()->vibration.begin();
         i != mol_->GetMolekelMolecule()->vibration.end();
         ++rc, ++i )
    {
    	// chekboxes
    	QTableWidgetItem *selectedItem = new QTableWidgetItem;
    	selectedItem->setCheckState( Qt::Unchecked );
    	selectedItem->setData( Qt::UserRole, rc );
    	
    	// freq
        QTableWidgetItem *freqItem = new QTableWidgetItem;
        freqItem->setData( Qt::UserRole, rc );
        freqItem->setText( QString( "%1" ).arg( i->frequency ) );
        
        // type
        QTableWidgetItem *typeItem = new QTableWidgetItem;
        typeItem->setText( QString( i->type ) );
        typeItem->setData( Qt::UserRole, rc );
        
        // IR intensity
        QTableWidgetItem *irItem = new QTableWidgetItem;
        irItem->setText( QString( "%1" ).arg( i->ir_intensity ) );
        irItem->setData( Qt::UserRole, rc );
        
        // reduced mass
        QTableWidgetItem *rmassItem = new QTableWidgetItem;
        rmassItem->setText( QString( "%1" ).arg( i->reduced_mass ) );
        rmassItem->setData( Qt::UserRole, rc );
        
        // Raman activities
        QTableWidgetItem *ramanItem = new QTableWidgetItem;
        ramanItem->setText( QString( "%1" ).arg( i->raman_activity ) );
        ramanItem->setData( Qt::UserRole, rc );
        
        table_->setItem( rc, 0, selectedItem );
        table_->setItem( rc, 1, freqItem );
        table_->setItem( rc, 2, typeItem );
        table_->setItem( rc, 3, irItem );
        table_->setItem( rc, 4, rmassItem );
        table_->setItem( rc, 5, ramanItem );
    }

    if( rc > 0 )
    {
    	
    	AtomVibrationAnimator::Locker l( *animator_ );
    	typedef AtomVibrationAnimator::ModeIndices IDS;
    	table_->setEnabled( true );
        const IDS& indices = animator_->GetVibrationModeIndices();
        for( IDS::const_iterator i = indices.begin(); i != indices.end(); ++i )
        {
        	QTableWidgetItem *item = table_->item( *i, 0 );
        	item->setCheckState( Qt::Checked );
        }  
    }
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::DoubleValueChangedSlot( double v )
{
    if( updatingGUI_ ) return;
    assert( mol_ && "NULL molecule" );

    // update scaling factor
    mol_->GetMolekelMolecule()->sc_freq_ar =  v;
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::StringValueChangedSlot( const QString& s )
{
    if( updatingGUI_ ) return;
    assert( mol_ && "NULL molecule" );
    // update scaling factor
    mol_->GetMolekelMolecule()->sc_freq_ar =  s.toFloat();
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::ShowArrowsSlot( int state  )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    animator_->SetVibrationVectorsVisibility( state == Qt::Checked );
    constArrowLengthCheckBox_->setEnabled( state == Qt::Checked );
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::ConstArrowLengthSlot( int state  )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL animator" );
    animator_->SetConstantArrowLength( state == Qt::Checked );
    arrowScalingSpinBox_->setEnabled( state == Qt::Unchecked );
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::ArrowScalingChangedSlot( double v )
{
    if( updatingGUI_ ) return;
    assert( animator_ && "NULL molecule" );

    // update arrow scaling factor
    animator_->SetArrowScalingFactor( v );
}

//------------------------------------------------------------------------------
void MoleculeVibrationWidget::TableItemChangedSlot( QTableWidgetItem* i  )
{
    if( updatingGUI_  ) return;
    assert( i );
    const int freqIndex = i->data( Qt::UserRole ).toInt();
    if( i->checkState() == Qt::Unchecked ) // checked to unchecked
    {
        animator_->RemoveVibrationMode( freqIndex );
    }
    else // unchecked to checked
    {
       
    	animator_->AddVibrationMode( freqIndex );
    }    
}
