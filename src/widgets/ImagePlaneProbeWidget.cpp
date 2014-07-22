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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QStringList>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QComboBox>

// STD
#include <cassert>


#include "../MolekelMolecule.h"
#include "ImagePlaneProbeWidget.h"
#include "../utility/RAII.h"

/// Index of "Show" item in visibility combo box.
static const int SHOW_PLANE_INDEX = 0;
/// Index of "Hide" item in visibility combo box.
static const int HIDE_PLANE_INDEX = 1;

//------------------------------------------------------------------------------
ImagePlaneProbeWidget::ImagePlaneProbeWidget( MolekelMolecule* mol,
                                              QWidget* parent )
    : QWidget( parent ),  mol_( 0 ), table_( 0 ),
      stepSizeSpinBox_( 0 ), updatingGUI_( false ), selectedOrbital_( -1 )
{
    CreateGUI();
}


//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::CreateGUI()
{
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout();

    /// Grid Data
    gridDataGroup_ = new QGroupBox( tr( "Grid Data" ) );
    gridDataGroup_->setCheckable( true );
    gridDataGroup_->setChecked( false );
    connect( gridDataGroup_, SIGNAL( toggled( bool ) ),
             this, SLOT( GridDataGroupToggledSlot( bool ) ) );

    QVBoxLayout* gridDataLayout = new QVBoxLayout;
    QHBoxLayout* rangeLayout = new QHBoxLayout;
    rangeLayout->addWidget( new QLabel( tr( "Value range:" ) ) );
    gridDataRangeLabel_ = new QLabel( tr( "(0,0)" ) );
    rangeLayout->addWidget( gridDataRangeLabel_ );
    gridDataLayout->addItem( rangeLayout );
    gridsComboBox_ = new QComboBox;
    connect( gridsComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( GridDataChangedSlot( int ) ) );
    gridDataLayout->addWidget( gridsComboBox_ );
    gridDataGroup_->setLayout( gridDataLayout );

    /// Electron density (from density matrix)
    eldensGroup_ = new QGroupBox( tr( "Electron Density (Density Matrix)" ) );
    eldensGroup_->setCheckable( true );
    eldensGroup_->setChecked( false );
    connect( eldensGroup_, SIGNAL( toggled( bool ) ),
             this, SLOT( ElectronDensityGroupToggledSlot( bool ) ) );

    QHBoxLayout* eldensLayout = new QHBoxLayout;
    eldensLayout->addWidget( new QLabel( tr( "Value range:" ) ) );
    eldensRangeLabel_ = new QLabel( tr( "(0,0)" ) );
    eldensLayout->addWidget( eldensRangeLabel_ );
    eldensGroup_->setLayout( eldensLayout );

    /// Spin density
    spindensGroup_ = new QGroupBox( tr( "Spin Density" ) );
    spindensGroup_->setCheckable( true );
    spindensGroup_->setChecked( false );
    connect( spindensGroup_, SIGNAL( toggled( bool ) ),
             this, SLOT( SpinDensityGroupToggledSlot( bool ) ) );

    QHBoxLayout* spindensLayout = new QHBoxLayout;
    spindensLayout->addWidget( new QLabel( tr( "Value range:" ) ) );
    spindensRangeLabel_ = new QLabel( tr( "(0,0)" ) );
    spindensLayout->addWidget( spindensRangeLabel_ );
    spindensGroup_->setLayout( spindensLayout );

    /// MEP
    mepGroup_ = new QGroupBox( tr( "Molecular Electrostatic Potential" ) );
    mepGroup_->setCheckable( true );
    mepGroup_->setChecked( false );
    connect( mepGroup_, SIGNAL( toggled( bool ) ),
             this, SLOT( MEPGroupToggledSlot( bool ) ) );

    QHBoxLayout* mepLayout = new QHBoxLayout;
    mepLayout->addWidget( new QLabel( tr( "Value range:" ) ) );
    mepRangeLabel_ = new QLabel( tr( "(0,0)" ) );
    mepLayout->addWidget( mepRangeLabel_ );
    mepGroup_->setLayout( mepLayout );

    /// Orbitals
    orbitalsGroup_ = new QGroupBox( tr( "Orbitals" ) );
    QVBoxLayout* orbitalsLayout = new QVBoxLayout;
    orbitalsGroup_->setCheckable( true );
    orbitalsGroup_->setChecked( false );
    connect( orbitalsGroup_, SIGNAL( toggled( bool ) ),
             this, SLOT( OrbitalsGroupToggledSlot( bool ) ) );

    // Table
    table_ = new QTableWidget();
    table_->setColumnCount( 4 );
    QStringList labels;
    labels << tr( "Selected" ) << tr( "Eigenvalue" ) << tr( "Occupation" ) << tr( "Type" );
    table_->setHorizontalHeaderLabels( labels );
    table_->setEditTriggers( QAbstractItemView::NoEditTriggers );
    table_->setAlternatingRowColors( true );
    connect( table_, SIGNAL( itemChanged( QTableWidgetItem* ) ),
             this, SLOT( TableItemChangedSlot( QTableWidgetItem* ) ) );
    table_->setSelectionBehavior( QAbstractItemView::SelectRows );

    // Step size
    QLabel* stepSizeLabel = new QLabel( tr( "Step Size" ) );
    stepSizeSpinBox_ = new QDoubleSpinBox;
    stepSizeSpinBox_->setSingleStep( 0.025 );
    stepSizeSpinBox_->setValue( 0.25 );
    QHBoxLayout* stepSizeLayout = new QHBoxLayout;
    stepSizeLayout->addWidget( stepSizeLabel );
    stepSizeLayout->addWidget( stepSizeSpinBox_ );
    stepSizeSpinBox_->setEnabled( false );
    connect( stepSizeSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( StepChangedSlot( double ) ) );

    // Fill layout
    orbitalsLayout->addWidget( table_ );
    orbitalsGroup_->setLayout( orbitalsLayout );

    /// Add to main layout
//    mainLayout->addItem( visLayout );
    mainLayout->addWidget( gridDataGroup_ );
    mainLayout->addWidget( eldensGroup_ );
    mainLayout->addWidget( spindensGroup_ );
    mainLayout->addWidget( mepGroup_ );
    mainLayout->addWidget( orbitalsGroup_ );
    mainLayout->addItem( stepSizeLayout );

    /// Assign layout to this widget
    this->setLayout( mainLayout );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::UpdateGUI()
{
    if( mol_ == 0 )  return;

    updatingGUI_ = true;

    /// Grid data
    if( mol_->HasGridData() )
    {
        typedef MolekelMolecule::SurfaceLabels Labels;
        Labels labels = mol_->GetGridLabels();
        gridsComboBox_->setEnabled( labels.size() != 0 );
        Labels::const_iterator i = labels.begin();
        const Labels::const_iterator end = labels.end();
        for( i; i != end; ++i ) gridsComboBox_->addItem( i->c_str() );
        const double minValue = mol_->GetGridDataMin( labels.size() ? *( labels.begin() ) : "" );
        const double maxValue = mol_->GetGridDataMax( labels.size() ? *( labels.begin() ) : "" );
        gridDataRangeLabel_->setText( QString( "(%1, %2)" ).arg( minValue ).arg( maxValue ) );
    }
    else gridDataGroup_->setEnabled( false );

    /// Electron density
    if( mol_->CanComputeElectronDensity() ) eldensGroup_->setEnabled( true );
    else eldensGroup_->setEnabled( false );

    /// Spin density
    if( mol_->CanComputeSpinDensity() ) spindensGroup_->setEnabled( true );
    else spindensGroup_->setEnabled( false );

    /// MEP
    if( mol_->CanComputeMEP() ) mepGroup_->setEnabled( true );
    else mepGroup_->setEnabled( false );

    /// Orbitals
    orbitalsGroup_->setEnabled( false );
    if( mol_->GetNumberOfOrbitals() != 0 )
    {
        // update orbitals
        const int orbitals = mol_->GetNumberOfOrbitals();
        table_->setRowCount( orbitals );
        QStringList verticalHeaders;
        for( int i = 0;	 i != orbitals; ++i )
        {
            QTableWidgetItem *eigenValueItem = new QTableWidgetItem;
            eigenValueItem->setFlags(  Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            eigenValueItem->setText( QString( "%1" ).arg( mol_->GetOrbitalEigenValue( i  ) ) );
            QTableWidgetItem *occupationItem = new QTableWidgetItem;
            occupationItem->setFlags(  Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            occupationItem->setText( QString( "%1" ).arg( mol_->GetOrbitalOccupation( i ) ) );
            QTableWidgetItem *typeItem = new QTableWidgetItem;
            typeItem->setText( QString( "%1" ).arg( mol_->GetOrbitalType( i ) ) );
            QTableWidgetItem *selectedItem = new QTableWidgetItem;
            selectedItem->setCheckState( Qt::Unchecked );
            selectedItem->setData( Qt::UserRole, i );
            table_->setItem( i, 0, selectedItem );
            table_->setItem( i, 1, eigenValueItem );
            table_->setItem( i, 2, occupationItem );
            table_->setItem( i, 3, typeItem );

            if( mol_->IsAlphaOrbital( i ) )
            {
                verticalHeaders << QString( "%1 (%2)" ).arg( i ).arg( tr( "alpha" ) );
            }
            else if( mol_->IsBetaOrbital( i ) )
            {
                verticalHeaders << QString( "%1 (%2)" ).arg( i ).arg( tr( "beta" ) );
            }
        }
        table_->setVerticalHeaderLabels( verticalHeaders );
        table_->resizeColumnsToContents();
        /// @todo do we need the following to make sure everything is unselected ?
        /// table_->setRangeSelected( QTableWidgetSelectionRange( 0, 0, table_->rowCount() - 1, 3 ), false );
    }
    stepSizeSpinBox_->setValue( 0.25 );
    updatingGUI_ = false;
    if( mol_->GetNumberOfOrbitals() != 0 )  orbitalsGroup_->setEnabled( true );
    // signals are not emitted since slot are not active during UI update;
    // emit signals with default value to make sure all observers are
    // properly initialized
    // toggle group checkboxes to disable group content
    orbitalsGroup_->setChecked( true );
    orbitalsGroup_->setChecked( false );

    emit OrbitalSelected( -1 );
    emit StepChanged( stepSizeSpinBox_->value() );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::TableItemChangedSlot( QTableWidgetItem* i  )
{
    if( updatingGUI_  ) return;
    assert( i );
    const int orbitalIndex = i->data( Qt::UserRole ).toInt();
    if( i->checkState() == Qt::Unchecked ) // checked to unchecked
    {
        selectedOrbital_ = -1;
    }
    else if( i->checkState() == Qt::Checked ) // unchecked to checked
    {
        if( selectedOrbital_ >= 0 ) // if another orbital selected --> unselect
        {
            QTableWidgetItem* si = table_->item( selectedOrbital_, 0 );
            assert( si );
            ResourceHandler< bool > rh( updatingGUI_, true, false );
            si->setCheckState( Qt::Unchecked );
        }
        selectedOrbital_ = orbitalIndex;
    }
    emit OrbitalSelected( selectedOrbital_ );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::StepChangedSlot( double s  )
{
    if( updatingGUI_ || s <= 0. ) return;
    emit StepChanged( s );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::UnselectAll()
{
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    gridDataGroup_->setChecked( false );
    orbitalsGroup_->setChecked( false );
    eldensGroup_->setChecked( false );
    spindensGroup_->setChecked( false );
    mepGroup_->setChecked( false );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::GridDataGroupToggledSlot( bool v )
{
    if( updatingGUI_ ) return;
    UnselectAll();
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    gridDataGroup_->setChecked( v );
    stepSizeSpinBox_->setEnabled( false );
    emit GridDataToggled( v );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::OrbitalsGroupToggledSlot( bool v )
{
    if( updatingGUI_ ) return;
    UnselectAll();
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    orbitalsGroup_->setChecked( v );
    stepSizeSpinBox_->setEnabled( true );
    emit OrbitalsToggled( v );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::ElectronDensityGroupToggledSlot( bool v )
{
    if( updatingGUI_ ) return;
    UnselectAll();
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    eldensGroup_->setChecked( v );
    stepSizeSpinBox_->setEnabled( true );
    emit ElectronDensityToggled( v );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::SpinDensityGroupToggledSlot( bool v )
{
    if( updatingGUI_ ) return;
    UnselectAll();
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    spindensGroup_->setChecked( v );
    stepSizeSpinBox_->setEnabled( true );
    emit SpinDensityToggled( v );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::MEPGroupToggledSlot( bool v )
{
    if( updatingGUI_ ) return;
    UnselectAll();
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    mepGroup_->setChecked( v );
    stepSizeSpinBox_->setEnabled( true );
    emit MEPToggled( v );
}

//------------------------------------------------------------------------------
ImagePlaneProbeWidget::DataType ImagePlaneProbeWidget::GetSelectedDataType() const
{
    if( orbitalsGroup_->isChecked() ) return ORBITAL;
    else if( gridDataGroup_->isChecked() ) return GRID_DATA;
    else if( eldensGroup_->isChecked() ) return ELECTRON_DENSITY;
    else if( spindensGroup_->isChecked() ) return SPIN_DENSITY;
    else if( mepGroup_->isChecked() ) return MEP;
    return NONE;
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::SetElectronDensityRange( double minVal, double maxVal )
{
    eldensRangeLabel_->setText( QString( "(%1, %2)" ).arg( minVal ).arg( maxVal ) );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::SetSpinDensityRange( double minVal, double maxVal )
{
    spindensRangeLabel_->setText( QString( "(%1, %2)" ).arg( minVal ).arg( maxVal ) );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::SetMEPRange( double minVal, double maxVal )
{
    mepRangeLabel_->setText( QString( "(%1, %2)" ).arg( minVal ).arg( maxVal ) );
}

//------------------------------------------------------------------------------
void ImagePlaneProbeWidget::GridDataChangedSlot( int i )
{
    if( updatingGUI_ ) return;
    const std::string label = gridsComboBox_->count() ? gridsComboBox_->currentText().toStdString() : "";
    const double minValue = mol_->GetGridDataMin( label );
    const double maxValue = mol_->GetGridDataMax( label );
    gridDataRangeLabel_->setText( tr( "(%1, %2)" ).arg( minValue ).arg( maxValue ) );
}

//-------------------------------------------------------------------------------
std::string ImagePlaneProbeWidget::GetGridLabel() const
{
    return gridsComboBox_->count() ? gridsComboBox_->currentText().toStdString() : "";
}

