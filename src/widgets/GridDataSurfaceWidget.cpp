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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QSpinBox>
#include <QColorDialog>
#include <QPushButton>

#include "../MolekelMolecule.h"
#include "GridDataSurfaceWidget.h"
#include "../utility/RAII.h"

//------------------------------------------------------------------------------
GridDataSurfaceWidget::GridDataSurfaceWidget( QWidget* parent )
    : QWidget( parent ), mol_( 0 ),  updatingGUI_( false )
{
    ResourceHandler< bool >( updatingGUI_, true, false );
    QVBoxLayout* mainLayout = new QVBoxLayout;
    QGroupBox* infoBox = new QGroupBox;
    infoBox->setTitle( tr( "Grid Data" ) );
    QGridLayout* infoBoxLayout = new QGridLayout;
    stepSizeLabel_ = new QLabel;
    stepNumLabel_  = new QLabel;
    minMaxLabel_   = new QLabel;
    stepSizeLabel_->setFrameStyle( QFrame::StyledPanel );
    stepNumLabel_->setFrameStyle( QFrame::StyledPanel );
    minMaxLabel_->setFrameStyle( QFrame::StyledPanel );
    QLabel* numSteps = new QLabel( tr( "Number of steps" ) );
    //numSteps->setFrameStyle( QFrame::Box );
    infoBoxLayout->addWidget( numSteps, 1, 0 );
    QLabel* stepSize = new QLabel( tr( "Step size" ) );
    //stepSize->setFrameStyle( QFrame::Box );
    infoBoxLayout->addWidget( stepSize, 0, 0 );
    QLabel* minMax = new QLabel( tr( "Value range" ) );
    //minMax->setFrameStyle( QFrame::Box );
    infoBoxLayout->addWidget( minMax, 2, 0 );
    infoBoxLayout->addWidget( stepSizeLabel_, 0, 1 );
    infoBoxLayout->addWidget( stepNumLabel_, 1, 1 );
    infoBoxLayout->addWidget( minMaxLabel_, 2, 1 );
    infoBox->setLayout( infoBoxLayout );
    mainLayout->addWidget( infoBox );

    QHBoxLayout* valueLayout = new QHBoxLayout;
    valueLayout->addWidget( new QLabel( tr( "Value " ) ) );
    valueSpinBox_ = new QDoubleSpinBox;
    valueSpinBox_->setSingleStep( 0.001 );
    valueSpinBox_->setValue( 0.05 );
    valueSpinBox_->setDecimals( 6 );
    connect( valueSpinBox_, SIGNAL( valueChanged( double ) ),
             this, SLOT( ValueChangedSlot( double ) ) );
    valueLayout->addWidget( valueSpinBox_ );
    mainLayout->addItem( valueLayout );

    // grid list
    QHBoxLayout* gridsLayout = new QHBoxLayout;
    labelsComboBox_ = new QComboBox;
    connect( labelsComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( SurfaceChangedSlot( int ) ) );
    gridsLayout->addWidget( new QLabel( "Grids" ) );
    gridsLayout->addWidget( labelsComboBox_ );
    mainLayout->addItem( gridsLayout );

    // step multiplier
    QHBoxLayout* stepMultiplierLayout = new QHBoxLayout;
    stepMultiplierLayout->addWidget( new QLabel( tr( "Step Multiplier" ) ) );
    stepMulSpinBox_ = new QSpinBox;
    stepMulSpinBox_->setValue( 1 );
    connect( stepMulSpinBox_, SIGNAL( valueChanged( int ) ),
             this, SLOT( StepMultiplierChangedSlot( int ) ) );
    stepMultiplierLayout->addWidget( stepMulSpinBox_ );
    mainLayout->addItem( stepMultiplierLayout );

    // color
    QGridLayout* colorLayout = new QGridLayout;
    //colorLayout->setSpacing( SPACING );
    QPushButton* colorPushButton = new QPushButton( "Color" );
    connect( colorPushButton, SIGNAL( released() ), this, SLOT( SetColorSlot() ) );
    colorLayout->addWidget( colorPushButton, 0, 0 );
    colorLabel_ = new QLabel;
    const int frameStyle = QFrame::Sunken | QFrame::Panel;
    colorLabel_->setFrameStyle( frameStyle );
    colorLayout->addWidget( colorLabel_, 0, 1 );
    mainLayout->addItem( colorLayout );

    // rendering style combo box
    QLabel* renderingStyleLabel = new QLabel( tr( "Rendering Style" ) );
    renderingStyleComboBox_ = new QComboBox;
    //renderingStyleComboBox_->addItem( tr( "Select..." ) );
    renderingStyleComboBox_->addItem( tr( "Solid" ),  int( MolekelMolecule::SOLID ) );
    renderingStyleComboBox_->addItem( tr( "Wireframe" ),  int( MolekelMolecule::WIREFRAME ) );
    renderingStyleComboBox_->addItem( tr( "Points" ),  int( MolekelMolecule::POINTS ) );
    renderingStyleComboBox_->addItem( tr( "Transparent Solid" ),  int( MolekelMolecule::TRANSPARENT_SOLID ) );
    renderingStyleComboBox_->setCurrentIndex( 0 );
    connect( renderingStyleComboBox_, SIGNAL( currentIndexChanged( int ) ),
             this, SLOT( RenderingStyleChangedSlot( int ) ) );
    QHBoxLayout* renderingStyleLayout = new QHBoxLayout;
    renderingStyleLayout->addWidget( renderingStyleLabel );
    renderingStyleLayout->addWidget( renderingStyleComboBox_ );
    mainLayout->addItem( renderingStyleLayout );

    // Smoothing
    QGroupBox* smoothingGroupBox = new QGroupBox;
    smoothingGroupBox->setTitle( "Smoothing" );
    QGridLayout* smoothLayout = new QGridLayout;
    smoothLayout->addWidget( new QLabel( "Iterations" ), 0, 0 );
    iterationsSpinBox_ = new QSpinBox;
    iterationsSpinBox_->setRange( 0, 2000 );
    iterationsSpinBox_->setValue( 0 );
    smoothLayout->addWidget( iterationsSpinBox_, 0, 1 );
    smoothLayout->addWidget( new QLabel( "Relaxation Factor" ), 1, 0 );
    relFactorSpinBox_ = new QDoubleSpinBox;
    relFactorSpinBox_->setRange( 0, 1 );
    relFactorSpinBox_->setValue( 0.01 );
    relFactorSpinBox_->setSingleStep( 0.01 );
    smoothLayout->addWidget( relFactorSpinBox_, 1, 1 );
    smoothingGroupBox->setLayout( smoothLayout );
    mainLayout->addWidget( smoothingGroupBox );

    this->setLayout( mainLayout );

}

//------------------------------------------------------------------------------
double GridDataSurfaceWidget::GetValue() const
{
    return valueSpinBox_->value();
}


//------------------------------------------------------------------------------
int GridDataSurfaceWidget::GetStepMultiplier() const
{
    return stepMulSpinBox_->value();
}

//------------------------------------------------------------------------------
MolekelMolecule::RenderingStyle GridDataSurfaceWidget::GetRenderingStyle() const
{

    return MolekelMolecule::RenderingStyle (
            renderingStyleComboBox_->itemData(
             renderingStyleComboBox_->currentIndex() ).toInt() );
}

//------------------------------------------------------------------------------
void GridDataSurfaceWidget::UpdateGUI()
{
    setEnabled( false );
    if( mol_ == 0 || !mol_->HasGridData() ) return;
    ResourceHandler< bool >( updatingGUI_, true, false );

    typedef MolekelMolecule::SurfaceLabels Labels;
    Labels labels = mol_->GetGridLabels();
    labelsComboBox_->setEnabled( labels.size() != 0 );
    Labels::const_iterator i = labels.begin();
    const Labels::const_iterator end = labels.end();
    for( i; i != end; ++i ) labelsComboBox_->addItem( i->c_str() );

    const double minValue = mol_->GetGridDataMin( labels.size() ? *( labels.begin() ) : "" );
    const double maxValue = mol_->GetGridDataMax( labels.size() ? *( labels.begin() ) : "" );
    double stepSize[ 3 ];
    mol_->GetGridDataStepSize( stepSize );
    int numSteps[ 3 ];
    mol_->GetGridDataNumSteps( numSteps );

    valueSpinBox_->setRange( minValue, maxValue );
    valueSpinBox_->setSingleStep( ( maxValue - minValue ) / 50 );
    valueSpinBox_->setValue( ( minValue + maxValue ) / 2 );

    stepMulSpinBox_->setRange( 1, std::min( std::min( numSteps[ 0 ], numSteps[ 1 ] ),
                                                std::min( numSteps[ 0 ], numSteps[ 2 ] ) ) );
    stepMulSpinBox_->setValue( 1 );


    stepSizeLabel_->setText( tr( "dx: %1  dy: %2  dz: %3" ).arg( stepSize[ 0 ] )
                                                                      .arg( stepSize[ 1 ] )
                                                                      .arg( stepSize[ 2 ] ) );
    stepNumLabel_->setText( tr( "x: %1  y: %2  z: %3" ).arg( numSteps[ 0 ] )
                                                                      .arg( numSteps[ 1 ] )
                                                                      .arg( numSteps[ 2 ] ) );
    minMaxLabel_->setText( tr( "(%1, %2)" ).arg( minValue ).arg( maxValue ) );

    // color
    float r = 0.8f;
    float g = 0.9f;
    float b = 0.9f;
    mol_->GetGridDataSurfaceColor( r, g, b, labels.size() ? *( labels.begin() ) : "" );
    SetColor( r, g, b );

    setEnabled( true );
}


//------------------------------------------------------------------------------
std::string GridDataSurfaceWidget::GetSurfaceLabel() const
{
    if( labelsComboBox_->count() == 0 ) return "";
    return labelsComboBox_->currentText().toStdString();
}


//------------------------------------------------------------------------------
void GridDataSurfaceWidget::RenderingStyleChangedSlot( int index  )
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
void GridDataSurfaceWidget::SurfaceChangedSlot( int index  )
{
    if( updatingGUI_ ) return;

    const std::string label = labelsComboBox_->count() ? labelsComboBox_->currentText().toStdString() : "";
    const double minValue = mol_->GetGridDataMin( label );
    const double maxValue = mol_->GetGridDataMax( label );
    valueSpinBox_->setRange( minValue, maxValue );

    minMaxLabel_->setText( tr( "(%1, %2)" ).arg( minValue ).arg( maxValue ) );

    SetRenderingStyle( mol_->GetGridDataSurfaceRenderingStyle( label ) );

    // color
    float r = 0.8f;
    float g = 0.9f;
    float b = 0.9f;
    mol_->GetGridDataSurfaceColor( r, g, b, label );
    SetColor( r, g, b );

    emit SurfaceChanged( labelsComboBox_->currentText().toStdString() );
}


//------------------------------------------------------------------------------
void GridDataSurfaceWidget::ValueChangedSlot( double v  )
{
    if( updatingGUI_  ) return;
    emit ValueChanged( v );
}

//------------------------------------------------------------------------------
void GridDataSurfaceWidget::StepMultiplierChangedSlot( int v  )
{
    if( updatingGUI_  ) return;
    emit StepMultiplierChanged( v );
}

//------------------------------------------------------------------------------
void GridDataSurfaceWidget::SetRenderingStyle( MolekelMolecule::RenderingStyle rs )
{
    const int i = renderingStyleComboBox_->findData( int( rs ) );
    if( i < 0 ) return;
    renderingStyleComboBox_->setCurrentIndex( i );
}

//------------------------------------------------------------------------------
int GridDataSurfaceWidget::GetIterations() const
{
   return iterationsSpinBox_->value();
}

//------------------------------------------------------------------------------
double GridDataSurfaceWidget::GetRelaxationFactor() const
{
   return relFactorSpinBox_->value();
}

//------------------------------------------------------------------------------
void GridDataSurfaceWidget::SetColorSlot()
{
//    float r, g, b;
    const int MAX_COMPONENT_VALUE = 255;
    const std::string label = labelsComboBox_->count() ? labelsComboBox_->currentText().toStdString() : "";
    QColor color = QColorDialog::getColor( color_, this );
    if ( color.isValid() )
    {
       SetColor( color );
       if( mol_->HasGridDataSurface( label ) )
       {
            mol_->SetGridDataSurfaceColor( float( color.red() )   / MAX_COMPONENT_VALUE,
                                           float( color.green() ) / MAX_COMPONENT_VALUE,
                                           float( color.blue() )  / MAX_COMPONENT_VALUE,
                                           label );
       }
    }
}

//------------------------------------------------------------------------------
void GridDataSurfaceWidget::SetColor( const QColor& color )
{
     colorLabel_->setText( color.name() );
     colorLabel_->setPalette( QPalette( color ) );
     colorLabel_->setAutoFillBackground( true );
     color_ = color;
}


//------------------------------------------------------------------------------
void GridDataSurfaceWidget::SetColor( float r, float g, float b )
{
     const int MAX_COMPONENT_VALUE = 255;
     SetColor( QColor( int( r * MAX_COMPONENT_VALUE ),
                       int( g * MAX_COMPONENT_VALUE ),
                       int( b * MAX_COMPONENT_VALUE ) ) );
}

//------------------------------------------------------------------------------
QColor GridDataSurfaceWidget::GetColor() const
{
     return color_;
}

//------------------------------------------------------------------------------
void GridDataSurfaceWidget::GetColor( float& r, float& g, float& b ) const
{
    const int MAX_COMPONENT_VALUE = 255;
    r = float( color_.red() )   / MAX_COMPONENT_VALUE;
    g = float( color_.green() ) / MAX_COMPONENT_VALUE;
    b = float( color_.blue() )  / MAX_COMPONENT_VALUE;
}
