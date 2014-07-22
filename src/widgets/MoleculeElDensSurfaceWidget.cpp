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

#include <cassert>
#include <limits>

// QT
#include <QWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QStringList>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QPushButton>
#include <QColorDialog>
#include <QSettings>

#include "../MolekelMolecule.h"
#include "MoleculeElDensSurfaceWidget.h"
#include "../utility/RAII.h"

const QString MoleculeElDensSurfaceWidget::POSITIVE_ORBITAL_COLOR_KEY = 
	"gui/eldens_surface_widget/positive_orbital_color";
const QString MoleculeElDensSurfaceWidget::NEGATIVE_ORBITAL_COLOR_KEY = 
	"gui/eldens_surface_widget/negative_orbital_color";
const QString MoleculeElDensSurfaceWidget::NODAL_SURFACE_COLOR_KEY = 
	"gui/eldens_surface_widget/nodal_surface_color";
const QString MoleculeElDensSurfaceWidget::DENSITY_MATRIX_COLOR_KEY = 
	"gui/eldens_surface_widget/density_matrix_orbital_color";
const QString MoleculeElDensSurfaceWidget::POSITIVE_TRANSPARENCY_KEY = 
	"gui/eldens_surface_widget/positive_transparency";
const QString MoleculeElDensSurfaceWidget::NEGATIVE_TRANSPARENCY_KEY = 
	"gui/eldens_surface_widget/negative_transparency";
const QString MoleculeElDensSurfaceWidget::NODAL_TRANSPARENCY_KEY = 
	"gui/eldens_surface_widget/nodal_transparency";
const QString MoleculeElDensSurfaceWidget::DENSITY_MATRIX_TRANSPARENCY_KEY = 
	"gui/eldens_surface_widget/density_matrix_transparency";

const QString MoleculeElDensSurfaceWidget::ISOVALUE_KEY = 
	"gui/eldens_surface_widget/isovalue";
const QString MoleculeElDensSurfaceWidget::BOTH_SIGNS_KEY = 
	"gui/eldens_surface_widget/bothsigns";
const QString MoleculeElDensSurfaceWidget::NODAL_SURFACE_KEY = 
	"gui/eldens_surface_widget/nodal_surface";
const QString MoleculeElDensSurfaceWidget::STEP_SIZE_KEY = 
	"gui/eldens_surface_widget/stepsize";

#ifdef PERSISTENT_ISOBBOX
const QString MoleculeElDensSurfaceWidget::BBOX_DX_KEY = 
	"gui/eldens_surface_widget/bbox_dx";
const QString MoleculeElDensSurfaceWidget::BBOX_DY_KEY = 
	"gui/eldens_surface_widget/bbox_dy";
const QString MoleculeElDensSurfaceWidget::BBOX_DZ_KEY = 
	"gui/eldens_surface_widget/bbox_dz";
#endif


//------------------------------------------------------------------------------
MoleculeElDensSurfaceWidget::MoleculeElDensSurfaceWidget( QWidget* parent )
    : QWidget( parent ), mol_( 0 ),  table_( 0 ), updatingGUI_( false )
{
	densityMatrixColor_.setRgbF( 0.9, 0.3, 0.9 );
	negativeOrbitalColor_.setRgbF( 1.0, 0.2, 0.2 );
	nodalSurfaceColor_.setRgbF( 0.8, 0.8, 0.8 );
	positiveOrbitalColor_.setRgbF( 0.2, 0.2, 1.0 );
	
	const QSettings s;
	QVariant v = s.value( POSITIVE_ORBITAL_COLOR_KEY, positiveOrbitalColor_ );
	positiveOrbitalColor_ =	 v.value< QColor >();
	v = s.value( NEGATIVE_ORBITAL_COLOR_KEY, negativeOrbitalColor_ );
	negativeOrbitalColor_ =	 v.value< QColor >();
	v = s.value( NODAL_SURFACE_COLOR_KEY, nodalSurfaceColor_ );
	nodalSurfaceColor_ = v.value< QColor >();
	v = s.value( DENSITY_MATRIX_COLOR_KEY, densityMatrixColor_ );
	densityMatrixColor_ =	 v.value< QColor >();
	CreateGUI();
}

#include "../utility/FoldableWidget.h"
//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::CreateGUI()
{
	const QSettings s;
    /// Layouts
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout();

    /// Density
    QHBoxLayout* densityMatrixLayout = new QHBoxLayout;
    densityMatrixCheckBox_ = new QCheckBox( tr( "Density Matrix" ) );
    densityMatrixCheckBox_->setChecked( false );
    densityMatrixLayout->addWidget( densityMatrixCheckBox_ );
    connect( densityMatrixCheckBox_, SIGNAL( toggled( bool ) ),
             this, SLOT( DensityMatrixToggledSlot( bool ) ) );
    mainLayout->addItem( densityMatrixLayout );
    /// MEP
    QHBoxLayout* mepLayout = new QHBoxLayout;
    QCheckBox* mepCheckBox = new QCheckBox( tr( "Map Molecular Electrostatic Potential" ) );
    mepCheckBox->setChecked( false );
    mepLayout->addWidget( mepCheckBox );
    connect( mepCheckBox, SIGNAL( toggled( bool ) ),
             this, SLOT( MEPMappingToggledSlot( bool ) ) );
    mainLayout->addItem( mepLayout );
    /// Orbitals
    // Table
    table_ = new QTableWidget();
    table_->setColumnCount( 4 );
    QStringList labels;
    labels << tr( "Generated" ) << tr( "Eigenvalue" ) << tr( "Occupation" ) << tr( "Type" );
    table_->setHorizontalHeaderLabels( labels );
    table_->setEditTriggers( QAbstractItemView::NoEditTriggers );
    table_->setAlternatingRowColors( true );
    connect( table_, SIGNAL( cellClicked( int, int ) ),
             this, SLOT( TableCellClickedSlot( int, int ) ) );
    table_->setSelectionBehavior( QAbstractItemView::SelectRows );
    table_->setEnabled( false );

    // Value & step
    QLabel* valueLabel = new QLabel( tr( "Isosurface Value" ) );
    valueSpinBox_ = new QDoubleSpinBox;
    const double value = s.value( ISOVALUE_KEY, 0.05 ).toDouble();
    valueSpinBox_->setValue( value );
    valueSpinBox_->setSingleStep( 0.01 );
    valueSpinBox_->setDecimals( 4 );
    connect( valueSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ComputeSteps() ) );
    QHBoxLayout* valueLayout = new QHBoxLayout;
    valueLayout->setSpacing( 40 );
    valueLayout->addWidget( valueLabel );
    valueLayout->addWidget( valueSpinBox_ );

    // Check boxes for sign and nodal surface
    signCheckBox_ = new QCheckBox( "Use both signs" );
    nodalSurfaceCheckBox_ = new QCheckBox( "Generate Nodal Surface" );
    Qt::CheckState checked = Qt::CheckState( s.value( BOTH_SIGNS_KEY, Qt::Unchecked ).toInt() );
    signCheckBox_->setCheckState( checked );
    checked = Qt::CheckState( s.value( NODAL_SURFACE_KEY, Qt::Unchecked ).toInt() );
    nodalSurfaceCheckBox_->setCheckState( checked );
    QHBoxLayout* checkBoxesLayout = new QHBoxLayout;
    checkBoxesLayout->setSpacing( 40 );
    checkBoxesLayout->addWidget( signCheckBox_ );
    checkBoxesLayout->addWidget( nodalSurfaceCheckBox_ );
    connect( signCheckBox_, SIGNAL( stateChanged( int ) ),
             this, SLOT( BothSignsSlot( int ) ) );
    connect( nodalSurfaceCheckBox_, SIGNAL( stateChanged( int ) ),
             this, SLOT( NodalSurfaceSlot( int ) ) );

    // Combo box for rendering style and transparency
    QLabel* renderingStyleLabel = new QLabel( tr( "Rendering Style" ) );
    renderingStyleComboBox_ = new QComboBox;
    renderingStyleComboBox_->addItem( tr( "Solid" ),  int( MolekelMolecule::SOLID ) );
    renderingStyleComboBox_->addItem( tr( "Wireframe" ),  int( MolekelMolecule::WIREFRAME ) );
    renderingStyleComboBox_->addItem( tr( "Points" ),  int( MolekelMolecule::POINTS ) );
    renderingStyleComboBox_->setCurrentIndex( 0 );
    connect( renderingStyleComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( RenderingStyleChangedSlot( int ) ) );
    QBoxLayout* rsLayout = new QHBoxLayout;
    rsLayout->addWidget( renderingStyleLabel );
    rsLayout->addWidget( renderingStyleComboBox_ );
    
   
    // Transparency.
    QGridLayout* tLayout = new QGridLayout;
    // density matrix
    dmTransparencyWidget_ = new QDoubleSpinBox;
    dmTransparencyWidget_->setRange( 0, 1 );
    dmTransparencyWidget_->setSingleStep( 0.05 );
    double transparency = s.value( DENSITY_MATRIX_TRANSPARENCY_KEY, 0.0 ).toDouble();
    dmTransparencyWidget_->setValue( transparency );
    connect( dmTransparencyWidget_, SIGNAL( valueChanged( double ) ), this, SLOT( DMTransparencyChangedSlot( double ) ) );
    tLayout->addWidget( new QLabel( "Density Matrix" ), 0, 0 );
    tLayout->addWidget( dmTransparencyWidget_, 0, 1 );
    // negative
    negTransparencyWidget_ = new QDoubleSpinBox;
    negTransparencyWidget_->setRange( 0, 1 );
    negTransparencyWidget_->setSingleStep( 0.05 );   
    transparency = s.value( NEGATIVE_TRANSPARENCY_KEY, 0.0 ).toDouble();
    negTransparencyWidget_->setValue( transparency );
    connect( negTransparencyWidget_, SIGNAL( valueChanged( double ) ), this, SLOT( NegTransparencyChangedSlot( double ) ) );
    tLayout->addWidget( new QLabel( "Negative" ), 1, 0 );
    tLayout->addWidget( negTransparencyWidget_, 1, 1 );
    // nodal
    nodTransparencyWidget_ = new QDoubleSpinBox;
    nodTransparencyWidget_->setRange( 0, 1 );
    nodTransparencyWidget_->setSingleStep( 0.05 );    
    transparency = s.value( NODAL_TRANSPARENCY_KEY, 0.0 ).toDouble();
    nodTransparencyWidget_->setValue( transparency );
    connect( nodTransparencyWidget_, SIGNAL( valueChanged( double ) ), this, SLOT( NodalTransparencyChangedSlot( double ) ) );
    tLayout->addWidget( new QLabel( "Nodal" ), 2, 0 );
    tLayout->addWidget( nodTransparencyWidget_, 2, 1 );
    // positive
    posTransparencyWidget_ = new QDoubleSpinBox;
    posTransparencyWidget_->setRange( 0, 1 );
    posTransparencyWidget_->setSingleStep( 0.05 );    
    transparency = s.value( POSITIVE_TRANSPARENCY_KEY, 0.0 ).toDouble();
    posTransparencyWidget_->setValue( transparency );
    connect( posTransparencyWidget_, SIGNAL( valueChanged( double ) ), this, SLOT( PosTransparencyChangedSlot( double ) ) );
    tLayout->addWidget( new QLabel( "Positive" ), 3, 0 );
    tLayout->addWidget( posTransparencyWidget_, 3, 1 );
            
    // Color buttons
    QBoxLayout* cLayout = new QVBoxLayout;
    cLayout->setSpacing( 3 );
     QPushButton* pb = new QPushButton( "Density Matrix" );
    connect( pb, SIGNAL( released() ), this, SLOT( DensityMatrixColorSlot() ) );
    cLayout->addWidget( pb );
    pb = new QPushButton( "Negative" );
    connect( pb, SIGNAL( released() ), this, SLOT( NegativeOrbitalColorSlot() ) );
    cLayout->addWidget( pb );
    pb = new QPushButton( "Nodal" );
    connect( pb, SIGNAL( released() ), this, SLOT( NodalSurfaceColorSlot() ) );
    cLayout->addWidget( pb );
    pb = new QPushButton( "Positive" );
    connect( pb, SIGNAL( released() ), this, SLOT( PositiveOrbitalColorSlot() ) );
    cLayout->addWidget( pb );
    
    FoldableWidget* tGroup = new FoldableWidget( tLayout, "Transparency" );
	    
 	FoldableWidget* cGroup = new FoldableWidget( cLayout, "Color" );
        
    QBoxLayout* renderingStyleLayout = new QVBoxLayout;
    renderingStyleLayout->setSpacing( 3 );
    renderingStyleLayout->addItem( rsLayout );
    QBoxLayout* tcLayout = new QHBoxLayout;
    tcLayout->addWidget( tGroup );
    tcLayout->addWidget( cGroup );
    renderingStyleLayout->addItem( tcLayout );
        
    // Step size
    QLabel* stepSizeLabel = new QLabel( tr( "Step Size" ) );
    stepSizeSpinBox_ = new QDoubleSpinBox;
    const double sstep = s.value( STEP_SIZE_KEY, 0.25 ).toDouble();
    stepSizeSpinBox_->setSingleStep( 0.025 );
    stepSizeSpinBox_->setValue( sstep );
    connect( stepSizeSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ComputeSteps() ) );
    QHBoxLayout* stepSizeLayout = new QHBoxLayout;
    stepSizeLayout->setSpacing( 50 );
    stepSizeLayout->addWidget( stepSizeLabel );
    stepSizeLayout->addWidget( stepSizeSpinBox_ );

    // Bounding box
    QGroupBox* bboxFrame = new QGroupBox;
    bboxFrame->setTitle( tr( "Bounding Box" ) );
    QGridLayout* bboxLayout = new QGridLayout;
    //bboxLayout->setSpacing( 30 );

    // all parameters will be re-recomputed from actual molecule
    // bounding box data

    const double MAXVALUE = 100.0;

    // center x
    bboxLayout->addWidget( new QLabel( tr( "x:" ) ), 0, 0 );
    xSpinBox_ = new QDoubleSpinBox;
    xSpinBox_->setSingleStep( 0.1 );
    xSpinBox_->setMinimum( -MAXVALUE );
    xSpinBox_->setMaximum( MAXVALUE );
    xSpinBox_->setDecimals( 2 );
    bboxLayout->addWidget( xSpinBox_, 0, 1 );
    connect( xSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( MoveCenterSlot() ) );
    // center y
    bboxLayout->addWidget( new QLabel( tr( "y:" ) ), 0, 2 );
    ySpinBox_ = new QDoubleSpinBox;
    ySpinBox_->setSingleStep( 0.1 );
    ySpinBox_->setMinimum( -MAXVALUE );
    ySpinBox_->setMaximum( MAXVALUE );
    ySpinBox_->setDecimals( 2 );
    bboxLayout->addWidget( ySpinBox_, 0, 3 );
    connect( ySpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( MoveCenterSlot() ) );
    // center z
    bboxLayout->addWidget( new QLabel( tr( "z:" ) ), 0, 4 );
    zSpinBox_ = new QDoubleSpinBox;
    zSpinBox_->setSingleStep( 0.1 );
    zSpinBox_->setMinimum( -MAXVALUE );
    zSpinBox_->setMaximum( MAXVALUE );
    zSpinBox_->setDecimals( 2 );
    bboxLayout->addWidget( zSpinBox_, 0, 5 );
    connect( zSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( MoveCenterSlot() ) );


    // dx
    bboxLayout->addWidget( new QLabel( tr( "dx:" ) ), 1, 0 );
    dxSpinBox_ = new QDoubleSpinBox;
    dxSpinBox_->setSingleStep( 0.1 );
    bboxLayout->addWidget( dxSpinBox_, 1, 1 );
    connect( dxSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ComputeSteps() ) );
    // dy
    bboxLayout->addWidget( new QLabel( tr( "dy:" ) ), 1, 2 );
    dySpinBox_ = new QDoubleSpinBox;
    dySpinBox_->setSingleStep( 0.1 );
    bboxLayout->addWidget( dySpinBox_, 1, 3 );
    connect( dySpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ComputeSteps() ) );
    // dz
    bboxLayout->addWidget( new QLabel( tr( "dz:" ) ), 1, 4 );
    dzSpinBox_ = new QDoubleSpinBox;
    dzSpinBox_->setSingleStep( 0.1 );
    bboxLayout->addWidget( dzSpinBox_, 1, 5 );
    connect( dzSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ComputeSteps() ) );
    bboxFrame->setLayout( bboxLayout );

    // Steps
    QGroupBox* stepsFrame = new QGroupBox;
    stepsFrame->setTitle( tr( "Steps" ) );
    QGridLayout* stepsLayout = new QGridLayout;
    //stepsLayout->setSpacing( 20 );
    // nx
    stepsLayout->addWidget( new QLabel( tr( "Nx" ) ), 0, 0 );
    nxLineEdit_ = new QLabel;
    nxLineEdit_->setFrameStyle( QFrame::StyledPanel );
    //nxLineEdit_->setEnabled( false );
    stepsLayout->addWidget( nxLineEdit_, 1, 0 );
    // ny
    stepsLayout->addWidget( new QLabel( tr( "Ny" ) ), 0, 1 );
    nyLineEdit_ = new QLabel;
    nyLineEdit_->setFrameStyle( QFrame::StyledPanel );
    //nyLineEdit_->setEnabled( false );
    stepsLayout->addWidget( nyLineEdit_, 1, 1 );
    // nz
    stepsLayout->addWidget( new QLabel( tr( "Nz" ) ), 0, 2 );
    nzLineEdit_ = new QLabel;
    nzLineEdit_->setFrameStyle( QFrame::StyledPanel );
    //nzLineEdit_->setEnabled( false );
    stepsLayout->addWidget( nzLineEdit_, 1, 2 );

    stepsFrame->setLayout( stepsLayout );

    // Bounding box + steps
    QHBoxLayout* bboxStepsLayout = new QHBoxLayout;
    bboxStepsLayout->setSpacing( 8 );
    bboxStepsLayout->addWidget( bboxFrame );
    bboxStepsLayout->addWidget( stepsFrame );

    /// Fill layout
    mainLayout->addWidget( table_ );
    mainLayout->addItem( valueLayout );
    mainLayout->addItem( checkBoxesLayout );
    mainLayout->addItem( stepSizeLayout );
    mainLayout->addItem( bboxStepsLayout );
    mainLayout->addItem( renderingStyleLayout );

    /// Assign layout to this widget
    this->setLayout( mainLayout );
}

//------------------------------------------------------------------------------
bool MoleculeElDensSurfaceWidget::ValidateData() const
{
    /// @todo fill status message with info on error condition
    if( stepSizeSpinBox_->value() <= 0. ) return false;
    const double dx = dxSpinBox_->value();
    const double dy = dySpinBox_->value();
    const double dz = dzSpinBox_->value();
    const double step = stepSizeSpinBox_->value();
    if( dx <= 0. ) return false;
    if( dy <= 0. ) return false;
    if( dz <= 0. ) return false;
    if( step <= 0. ) return false;
    if( step >= dx || step >= dy || step >= dz ) return false;
    return true;
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::ComputeSteps()
{
    if( !ValidateData() ) return;
    int steps[ 3 ]; // nx, ny, nz
    double bboxSize[ 3 ]; // dx, dy, dz
    bboxSize[ 0 ] = dxSpinBox_->value();
    bboxSize[ 1 ] = dySpinBox_->value();
    bboxSize[ 2 ] = dzSpinBox_->value();
    steps[ 0 ] = int( bboxSize[ 0 ] / stepSizeSpinBox_->value() + .5 );
    steps[ 1 ] = int( bboxSize[ 1 ] / stepSizeSpinBox_->value() + .5 );
    steps[ 2 ] = int( bboxSize[ 2 ] / stepSizeSpinBox_->value() + .5 );
    nxLineEdit_->setText( QString( "%1" ).arg( steps[ 0 ] ) );
    nyLineEdit_->setText( QString( "%1" ).arg( steps[ 1 ] ) );
    nzLineEdit_->setText( QString( "%1" ).arg( steps[ 2 ] ) );
    //Emit signal only in case of user input
    QSettings s;
    s.setValue( STEP_SIZE_KEY, stepSizeSpinBox_->value() );
    s.setValue( ISOVALUE_KEY, valueSpinBox_->value() );
#ifdef PERSISTENT_ISOBBOX
    s.setValue( BBOX_DX_KEY, dxSpinBox_->value() );
    s.setValue( BBOX_DY_KEY, dySpinBox_->value() );
    s.setValue( BBOX_DZ_KEY, dzSpinBox_->value() );
#endif
    if( mol_ != 0 )
    {
        mol_->SetIsoBoundingBoxSize( dxSpinBox_->value(),
                                     dySpinBox_->value(),
                                     dzSpinBox_->value() );
    }
    if( !updatingGUI_ ) emit ValuesChanged();
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::MoveCenterSlot()
{
    mol_->SetIsoBoundingBoxCenter( xSpinBox_->value(),
                                   ySpinBox_->value(),
                                   zSpinBox_->value() );
    emit ValuesChanged();
}

//------------------------------------------------------------------------------
bool MoleculeElDensSurfaceWidget::GetData( double& v,
                                           double bboxSize[ 3 ],
                                           int steps[ 3 ],
                                           bool& bothSigns,
                                           bool& nodalSurface,
                                           MolekelMolecule::RenderingStyle& rs,
                                           double& dmTransparency,
                                           double& negTransparency,
                                           double& nodalTransparency,
                                           double& posTransparency ) const
{

    if( !ValidateData() )
    {
        // Shall we handle the error condition here or inside the enclosing widget ?
        QMessageBox::critical( 0, "Error", "Invalid Data", "Close" );
        /// @todo fill status message with info on error condition and make this
        /// information accessible from outside.
        return false;
    }
    v  = valueSpinBox_->value();
    bboxSize[ 0 ] = dxSpinBox_->value();
    bboxSize[ 1 ] = dySpinBox_->value();
    bboxSize[ 2 ] = dzSpinBox_->value();
    steps[ 0 ] = nxLineEdit_->text().toInt();
    steps[ 1 ] = nyLineEdit_->text().toInt();
    steps[ 2 ] = nzLineEdit_->text().toInt();
    if( signCheckBox_->checkState() == Qt::Checked ) bothSigns = true;
    else bothSigns = false;
    if( nodalSurfaceCheckBox_->checkState() == Qt::Checked ) nodalSurface = true;
    else nodalSurface = false;

    rs = MolekelMolecule::RenderingStyle (
            renderingStyleComboBox_->itemData(
             renderingStyleComboBox_->currentIndex() ).toInt() );

    dmTransparency 	  = dmTransparencyWidget_->value();
    negTransparency   = negTransparencyWidget_->value();
    nodalTransparency = nodTransparencyWidget_->value();
    posTransparency    = posTransparencyWidget_->value();
    
    return true;
}

//------------------------------------------------------------------------------
bool MoleculeElDensSurfaceWidget::GetSteps( int steps[ 3 ] ) const
{

    if( !ValidateData() )
    {
        // Shall we handle the error condition here or inside the enclosing widget ?
        QMessageBox::critical( 0, "Error", "Invalid Data", "Close" );
        /// @todo fill status message with info on error condition and make this
        /// information accessible from outside.
        return false;
    }
    steps[ 0 ] = nxLineEdit_->text().toInt();
    steps[ 1 ] = nyLineEdit_->text().toInt();
    steps[ 2 ] = nzLineEdit_->text().toInt();
    return true;
}

//------------------------------------------------------------------------------
bool MoleculeElDensSurfaceWidget::GetBBoxSize( double bboxSize[ 3 ] ) const
{

    if( !ValidateData() )
    {
        // Shall we handle the error condition here or inside the enclosing widget ?
        QMessageBox::critical( 0, "Error", "Invalid Data", "Close" );
        /// @todo fill status message with info on error condition and make this
        /// information accessible from outside.
        return false;
    }
    bboxSize[ 0 ] = dxSpinBox_->value();
    bboxSize[ 1 ] = dySpinBox_->value();
    bboxSize[ 2 ] = dzSpinBox_->value();
    return true;
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::UpdateGUI( bool updateTable, bool updateBBox )
{
    table_->setEnabled( false );
    if( mol_ == 0 ) return;
    densityMatrixCheckBox_->setEnabled( mol_->CanComputeElectronDensity() );
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    if( updateTable && mol_->GetNumberOfOrbitals() )
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
            QTableWidgetItem *generatedItem = new QTableWidgetItem;
            generatedItem->setFlags(  Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            // Removed check box, not sure it's a good idea, user
            // will want to check the checkbox and then click on generate
            // to have multiple orbital surfaces generated at once.
            if( mol_->HasOrbitalSurface( i ) )
            {

                //generatedItem->setCheckState( Qt::Checked );
                generatedItem->setText( tr( "Yes" ) );
            }
            else
            {
                //generatedItem->setCheckState( Qt::Unchecked );
                generatedItem->setText( tr( "No" ) );
            }
            table_->setItem( i, 0, generatedItem );
            table_->setItem( i, 1, eigenValueItem );
            table_->setItem( i, 2, occupationItem );
            table_->setItem( i, 3, typeItem );

            //TEMPORARY DISABLED
            //if( mol_->IsAlphaOrbital( i ) )
            //{
            //    verticalHeaders << QString( "%1 (%2)" ).arg( i + 1 ).arg( tr( "alpha" ) );
            //}
            //else if( mol_->IsBetaOrbital( i ) )
            //{
            //    verticalHeaders << QString( "%1 (%2)" ).arg( i + 1 ).arg( tr( "beta" ) );
            //}
        }
        table_->setVerticalHeaderLabels( verticalHeaders );
        table_->resizeColumnsToContents();
        /// @todo do we need the following to make sure everything is unselected ?
        /// table_->setRangeSelected( QTableWidgetSelectionRange( 0, 0, table_->rowCount() - 1, 3 ), false );
    }
    if( updateBBox )
    {
        double dx = 0.;
        double dy = 0.;
        double dz = 0.;
        mol_->GetIsoBoundingBoxSize( dx, dy, dz );
#ifdef PERSISTENT_ISOBBOX
        QSettings s;
        if( s.contains( BBOX_DX_KEY ) ) dx = s.value( BBOX_DX_KEY ).toDouble();
        if( s.contains( BBOX_DY_KEY ) ) dy = s.value( BBOX_DY_KEY ).toDouble(); 
        if( s.contains( BBOX_DZ_KEY ) ) dz = s.value( BBOX_DZ_KEY ).toDouble();
#endif 
        dxSpinBox_->setValue( dx );
        dySpinBox_->setValue( dy );
        dzSpinBox_->setValue( dz );
        ComputeSteps();
    }
    
    double cx = 0.;
    double cy = 0.;
    double cz = 0.;
    double dx = 0.;
    double dy = 0.;
    double dz = 0.;
    mol_->GetIsoBoundingBoxSize( dx, dy, dz );
    mol_->GetIsoBoundingBoxCenter( cx, cy, cz );
    xSpinBox_->setValue( cx );
    ySpinBox_->setValue( cy );
    zSpinBox_->setValue( cz );
    xSpinBox_->setSingleStep( 0.05 * dx );
    ySpinBox_->setSingleStep( 0.05 * dy );
    zSpinBox_->setSingleStep( 0.05 * dz );
    xSpinBox_->setMinimum( cx - dx );
    xSpinBox_->setMaximum( cx + dx );
    ySpinBox_->setMinimum( cy - dy );
    ySpinBox_->setMaximum( cy + dy );
    zSpinBox_->setMinimum( cz - dz );
    zSpinBox_->setMaximum( cz + dz );
    dxSpinBox_->setSingleStep( 0.05 * dx );
    dySpinBox_->setSingleStep( 0.05 * dy );
    dzSpinBox_->setSingleStep( 0.05 * dz );
    table_->setEnabled( true );

}


//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::TableCellClickedSlot( int row, int column  )
{
    if( updatingGUI_  || row < 0 ) return;
    emit OrbitalSelected( row );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::RenderingStyleChangedSlot( int index  )
{
    if( updatingGUI_  ) return;
    emit RenderingStyleChanged
        (
            MolekelMolecule::RenderingStyle
            (
                renderingStyleComboBox_->itemData
                (
                    renderingStyleComboBox_->currentIndex()
                 ).toInt()
             )
        );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::DensityMatrixToggledSlot( bool  v )
{
    if( updatingGUI_ ) return;
    // enable/disable controls depending on value of checkbox
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    table_->setEnabled( !v && mol_->GetNumberOfOrbitals() );
    signCheckBox_->setEnabled( !v );
    nodalSurfaceCheckBox_->setEnabled( !v );
    emit DensityMatrixToggled( v );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::MEPMappingToggledSlot( bool  v )
{
    emit MEPMappingToggled( v );
}


//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::SetRenderingStyle( MolekelMolecule::RenderingStyle rs )
{
	ResourceHandler< bool > rh( updatingGUI_, true, false );
    const int i = renderingStyleComboBox_->findData( int( rs ) );
    if( i < 0 ) return;
    renderingStyleComboBox_->setCurrentIndex( i );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::DMTransparencyChangedSlot( double t )
{
	if( updatingGUI_ ) return;
	QSettings s;
	s.setValue( DENSITY_MATRIX_TRANSPARENCY_KEY, t  );
	emit DMTransparencyChanged( t );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::NegTransparencyChangedSlot( double t )
{
	if( updatingGUI_ ) return;
	QSettings s;
	s.setValue( NEGATIVE_TRANSPARENCY_KEY, t  );
	emit NegTransparencyChanged( t );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::NodalTransparencyChangedSlot( double t )
{
	if( updatingGUI_ ) return;
	QSettings s;
	s.setValue( NODAL_TRANSPARENCY_KEY, t  );
	emit NodalTransparencyChanged( t );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::PosTransparencyChangedSlot( double t )
{
	if( updatingGUI_ ) return;
	QSettings s;
	s.setValue( POSITIVE_TRANSPARENCY_KEY, t  );
	emit PosTransparencyChanged( t );
}


//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::SetDMTransparency( double a )
{
	ResourceHandler< bool > rh( updatingGUI_, true, false );
	assert( dmTransparencyWidget_ );
	dmTransparencyWidget_->setValue( a );
	QSettings s;
	s.setValue( DENSITY_MATRIX_TRANSPARENCY_KEY, a  );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::SetNegTransparency( double a )
{
	ResourceHandler< bool > rh( updatingGUI_, true, false );
	assert( negTransparencyWidget_ );
	negTransparencyWidget_->setValue( a );
	QSettings s;
	s.setValue( NEGATIVE_TRANSPARENCY_KEY, a  );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::SetNodalTransparency( double a )
{
	ResourceHandler< bool > rh( updatingGUI_, true, false );
	assert( nodTransparencyWidget_ );
	nodTransparencyWidget_->setValue( a );
	QSettings s;
	s.setValue( NODAL_TRANSPARENCY_KEY, a  );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::SetPosTransparency( double a )
{
	ResourceHandler< bool > rh( updatingGUI_, true, false );
	assert( posTransparencyWidget_ );
	posTransparencyWidget_->setValue( a );
	QSettings s;
	s.setValue( POSITIVE_TRANSPARENCY_KEY, a  );
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::DensityMatrixColorSlot()
{
	QColor c = QColorDialog::getColor( densityMatrixColor_ );
	if( c.isValid() )
	{
		densityMatrixColor_ = c;
		QSettings c;
		c.setValue( DENSITY_MATRIX_COLOR_KEY, densityMatrixColor_ );
		emit DensityMatrixColorChanged( densityMatrixColor_ );
	}
}
//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::NegativeOrbitalColorSlot()
{
	QColor c = QColorDialog::getColor( negativeOrbitalColor_ );
	if( c.isValid() )
	{
		negativeOrbitalColor_ = c;
		QSettings c;
		c.setValue( NEGATIVE_ORBITAL_COLOR_KEY, negativeOrbitalColor_ );
		emit NegativeOrbitalColorChanged( negativeOrbitalColor_ );
	}
}
//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::NodalSurfaceColorSlot()
{
	QColor c = QColorDialog::getColor( nodalSurfaceColor_ );
	if( c.isValid() )
	{
		nodalSurfaceColor_ = c;
		QSettings c;
		c.setValue( NODAL_SURFACE_COLOR_KEY, nodalSurfaceColor_ );
		emit NodalSurfaceColorChanged( nodalSurfaceColor_ );
	}
}
//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::PositiveOrbitalColorSlot()
{
	QColor c = QColorDialog::getColor( positiveOrbitalColor_ );
	if( c.isValid() )
	{
		positiveOrbitalColor_ = c;
		QSettings c;
		c.setValue( POSITIVE_ORBITAL_COLOR_KEY, positiveOrbitalColor_ );
		emit PositiveOrbitalColorChanged( positiveOrbitalColor_ );
	}
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::BothSignsSlot( int state )
{
	QSettings s;
	s.setValue( BOTH_SIGNS_KEY, state );
	emit ValuesChanged();
}

//------------------------------------------------------------------------------
void MoleculeElDensSurfaceWidget::NodalSurfaceSlot( int state )
{
	QSettings s;
	s.setValue( NODAL_SURFACE_KEY, state );
	emit ValuesChanged();
}



