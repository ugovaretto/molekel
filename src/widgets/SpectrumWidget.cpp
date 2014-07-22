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

// STD
#include <cassert>

// QT
#include <QWidget>
#include <QVBoxLayout>
#include <QPen>
#include <QPrinter>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>
#include <QMessageBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QSettings>
#include <QCoreApplication>

// QWT
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_grid.h>

#include "../MolekelMolecule.h"
#include "../old/molekeltypes.h"
#include "../utility/qtfileutils.h"

#include "SpectrumWidget.h"

//------------------------------------------------------------------------------
SpectrumWidget::SpectrumWidget( MolekelMolecule* m, const QString& outDataDirKey, QWidget* parent ) :
		QWidget( parent ), plot_( 0 ), curve_( 0 ), picker_( 0 ), zoomer_( 0 ),
		grid_( 0 ),	steps_( 600 ), aspectRatio_( lorentzianInterpolator_.GetHeightWidthRatio() ),
		radiationType_( IR ), graphType_( LORENTZIAN ), xAxisOrientationRightLeft_( false ),
		yAxisOrientationTopDown_( false ), graphTypeCBox_( 0 ), radiationTypeCBox_( 0 ),
		stepsSpinBox_( 0 ), aspectRatioSpinBox_( 0 ),
		halfWidth_( 20.0 ), halfWidthSpinBox_( 0 ),
		oneOverHalfWidthSquared_( 1.0 / ( halfWidth_ * halfWidth_ ) ), outDataDirKey_( outDataDirKey )
{ 
	assert( m );

	// IF NO DATA DISPLAY PROPER MESSAGE AND RETURN
	if( !m->GetMolekelMolecule() || 
		m->GetMolekelMolecule()->n_frequencies <= 0 )
	{
		QVBoxLayout* l = new QVBoxLayout;
		l->addWidget( new QLabel( "No frequencies" ) );
		setLayout( l );
		return;
	}
	
	// COPY DATA
	VibrationList::const_iterator i = m->GetMolekelMolecule()->vibration.begin();
	const VibrationList::const_iterator e = m->GetMolekelMolecule()->vibration.end();
	for( ; i != e; ++i )
	{
		frequencies_.push_back( i->frequency );
		irIntensities_.push_back( i->ir_intensity );
		ramanActivities_.push_back( i->raman_activity );
	}
	moleculeName_ = m->GetFileName().c_str();
	format_ = m->GetFormat().c_str();
	
	//2D PLOT 
	plot_ = new QwtPlot; 
	plot_->setCanvasBackground( QColor( Qt::white ) );

	// set labels
	plot_->setTitle( moleculeName_ );
	plot_->setAxisTitle( QwtPlot::yLeft, "Intensity" );
	plot_->setAxisTitle( QwtPlot::xBottom, "Frequency [cm-1]" );
		
	// set grid 
	grid_ = new QwtPlotGrid;
	grid_->enableXMin( true );
	grid_->setMajPen( QPen( Qt::black, 0, Qt::DotLine ) );
	grid_->setMinPen( QPen( Qt::gray, 0 , Qt::DotLine ) );
	grid_->attach( plot_ );
			
	//PICKER
	picker_ = new QwtPlotPicker( QwtPlot::xBottom, QwtPlot::yLeft,
				QwtPicker::PointSelection | QwtPicker::DragSelection, 
				QwtPlotPicker::CrossRubberBand, QwtPicker::AlwaysOn, 
				plot_->canvas());
	picker_->setRubberBandPen( QColor( Qt::gray ) );
	picker_->setRubberBand( QwtPicker::CrossRubberBand );
			
	//ZOOMER
	zoomer_ = new QwtPlotZoomer( QwtPlot::xBottom, QwtPlot::yLeft, plot_->canvas() );
	zoomer_->setEnabled( false );
						
	// ADD CURVE
	curve_ = new QwtPlotCurve( m->GetFileName().c_str() );
	curve_->setRenderHint( QwtPlotItem::RenderAntialiased );		
	curve_->attach( plot_ );
	QPen pen;
	//pen.setWidthF( 1.1f );
	curve_->setPen( pen );
			
	// ADD WIDGETS
	QVBoxLayout* layout = new QVBoxLayout;
	AddTopToolbar( layout );
	layout->addWidget( plot_ );
	AddBottomToolbar( layout );	
	setLayout( layout );
		
	// GENERATE DATA AND PLOT
	GenerateData();
}

//------------------------------------------------------------------------------
SpectrumWidget::~SpectrumWidget()
{
	delete picker_;
	delete zoomer_;
	delete grid_;
}

//------------------------------------------------------------------------------		
void SpectrumWidget::GenerateHistogramData( std::vector< double >& intensities,
											double& xmin, double& xmax,
											double& ymin, double& ymax )
{		
	ymin = std::numeric_limits< double >::max();
	ymax = std::numeric_limits< double >::min();
	// histogram-like diagram: to look like an histogram data have to be laid out
	// like this:
	// [x0, x0, x0, x1, x1, x1, ..., xN, xN, xN]
	// [0,  y0, 0,  0,  y1, 0,  ..., 0,  yN, 0 ]
	xdata_ = std::vector< double >( 3 * intensities.size(), 0.0 );
	ydata_ = xdata_;
	for( int i = 0; i != intensities.size(); ++i )
	{
		xdata_[ 3 * i     ] = frequencies_[ i ];
		xdata_[ 3 * i + 1 ] = frequencies_[ i ];
		xdata_[ 3 * i + 2 ] = frequencies_[ i ];
		const double y = intensities[ i ];
		if( y < ymin ) ymin = y;
		if( y > ymax ) ymax = y;
		ydata_[ 3 * i + 1 ] = y;
	}
	xmin = frequencies_.front();
	xmax = frequencies_.back();
}

//------------------------------------------------------------------------------
void SpectrumWidget::GenerateLorentzianData( std::vector< double >& intensities,
								 			 double& xmin, double& xmax,
								 			 double& ymin, double& ymax )
{
	
	LorentzianInterpolator& l = lorentzianInterpolator_;

	l.Clear();
	
	l.SetHeightWidthRatio( aspectRatio_ );
	for( int i = 0; i != frequencies_.size(); ++i ) l.Add( frequencies_[ i ], intensities[ i ] );
			
	const double dx = ( l.GetMaxX() - l.GetMinX() ) / steps_;
	const double mx = l.GetMinX();
	const int extension = int( 0.1 * steps_ ); // (10%) number of steps past the end of frequency interval
	
	ymin = std::numeric_limits< double >::max();
	ymax = std::numeric_limits< double >::min();

	xdata_.clear();
	ydata_.clear();
	
	for( int d = 0; d != steps_ + extension; ++d )
	{
		xdata_.push_back( mx + d * dx );
		const double y = l.Eval( mx + d * dx );
		if( y < ymin ) ymin = y;
		if( y > ymax ) ymax = y;
		ydata_.push_back( y );			
	}
	
	xmin = l.GetMinX();
	xmax = l.GetMaxX() + dx * extension; 
		
}


//------------------------------------------------------------------------------
void SpectrumWidget::GenerateInterpolatedData( std::vector< double >& intensities,
								 			   double& xmin, double& xmax,
								 			   double& ymin, double& ymax )
{
	
				
	const double dx = ( frequencies_.back() - frequencies_.front() ) / steps_;
	const double mx = frequencies_.front();
	const int extension = int( 0.1 * steps_ ); // (10%) number of steps past the end of frequency interval
	
	ymin = std::numeric_limits< double >::max();
	ymax = std::numeric_limits< double >::min();

	xdata_.clear();
	ydata_.clear();
	
		
	typedef double ( SpectrumWidget::*BasisFunction )( double ) const;
	
	BasisFunction basisFun = graphType_ == GAUSSIAN ? &SpectrumWidget::Gaussian : &SpectrumWidget::Lorentzian;
		
	for( int d = 0; d != steps_ + extension; ++d )
	{
		const double x = mx + d * dx;
		xdata_.push_back( x );
		double y = 0.0;
		
		for( int i = 0; i != intensities.size(); ++i )
		{
			const double offset = ( x - frequencies_[ i ] );
			y +=  intensities[ i ] * ( this->*basisFun )( offset );
		}		
		if( y < ymin ) ymin = y;
		if( y > ymax ) ymax = y;
		ydata_.push_back( y );			
	}
	
	xmin = frequencies_.front();
	xmax = frequencies_.back() + dx * extension; 
		
}

//------------------------------------------------------------------------------
void SpectrumWidget::GenerateData()
{
	double xmin = 0.0;
	double xmax = 0.0;
	double ymin = 0.0;
	double ymax = 0.0;
	
	std::vector< double >* intensities = 0;
	if( radiationType_ == IR )
	{
		intensities = &irIntensities_;
		plot_->setTitle( moleculeName_ + " - Infrared spectrum" );
	}
	else if( radiationType_ == RAMAN )
	{
		intensities = &ramanActivities_;
		plot_->setTitle( moleculeName_ + " - Raman activities" );
	}
	
	assert( intensities );
	
	GenerateInterpolatedData( *intensities, xmin, xmax, ymin, ymax );
	
	if( yAxisOrientationTopDown_   ) std::swap( ymin, ymax );
	if( xAxisOrientationRightLeft_ ) std::swap( xmin, xmax );
	plot_->setAxisScale( QwtPlot::yLeft, ymin, ymax );
	plot_->setAxisScale( QwtPlot::xBottom, xmin, xmax );
	
	// copy data into curve
	curve_->setData( &( *xdata_.begin() ),  &( *ydata_.begin() ), xdata_.size() );
			
	// draw
	plot_->replot();
}

//------------------------------------------------------------------------------
void SpectrumWidget::SaveSlot()
{
	QSettings settings;
	QString dir = settings.value( outDataDirKey_,
	                              QCoreApplication::applicationDirPath() ).toString();
	const QString fileName = GetSaveFileName( this, "Save to PS or PDF", dir );
	if( fileName.isEmpty() ) return;
	
	if( !fileName.endsWith( ".ps", Qt::CaseInsensitive )
		&& !fileName.endsWith( ".pdf", Qt::CaseInsensitive ) )
	{
		QMessageBox::information( this,
				"Save diagram", "Use extension to set file format to PostScript (.ps) or PDF (.pdf)" ); 
		return;
	}
	settings.setValue( outDataDirKey_, DirPath( fileName ) );
	QPrinter p;
	p.setOutputFileName( fileName );
	p.setCreator( "Molekel" );
	plot_->print( p ); 
}

//------------------------------------------------------------------------------
void SpectrumWidget::EnableZoomModeSlot( int state )
{
	const bool on = ( state == Qt::Checked );
	zoomer_->setEnabled( on );
	picker_->setEnabled( !on );
}  

//------------------------------------------------------------------------------
void SpectrumWidget::ShowGridSlot( int state )
{
	const bool on = ( state == Qt::Checked );
	grid_->enableX( on );
	grid_->enableY( on );
	plot_->replot();
}

//------------------------------------------------------------------------------
void SpectrumWidget::AddTopToolbar( QLayout* layout )
{
	QToolBar* toolBar = new QToolBar;
	toolBar->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
	
	// SAVE
	QToolButton* saveButton = new QToolButton;
	saveButton->setText( "Save" );
	saveButton->setToolButtonStyle( Qt::ToolButtonTextOnly );
	connect( saveButton, SIGNAL( released() ), this, SLOT( SaveSlot() ) );
	toolBar->addWidget( saveButton );
	
	toolBar->addSeparator();
	
	// GRAPH TYPE
	toolBar->addWidget( new QLabel( " Graph type: " ) );
	graphTypeCBox_ = new QComboBox;
	graphTypeCBox_->addItem( "Lorentzian", LORENTZIAN );
	graphTypeCBox_->addItem( "Gaussian", GAUSSIAN );
	//probably not needed: similar effect can be achieved by setting half width to 0.5
	//graphTypeCBox_->addItem( "Histogram", HISTOGRAM );
	connect( graphTypeCBox_, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( GraphTypeSlot( int ) ) );
	toolBar->addWidget( graphTypeCBox_ );
		
	toolBar->addSeparator();
	
	// RADIATION TYPE
	toolBar->addWidget( new QLabel( " Radiation type: " ) );
	radiationTypeCBox_ = new QComboBox;
	radiationTypeCBox_->addItem( "Infrared", IR );
	radiationTypeCBox_->addItem( "Raman", RAMAN );
	connect( radiationTypeCBox_, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( RadiationTypeSlot( int ) ) );
	toolBar->addWidget( radiationTypeCBox_ );
	
	toolBar->addSeparator();
	
	// Lorentzian aspect ratio: Height-at-peak / Half-width-at-half-height
	//	toolBar->addWidget( new QLabel( " Lorentzian aspect ratio: " ) );
	//	aspectRatioSpinBox_ = new QDoubleSpinBox;
	//	aspectRatioSpinBox_->setRange( 1.0, 50. );
	//	aspectRatioSpinBox_->setSingleStep( .5 );
	//	aspectRatioSpinBox_->setValue( lorentzianInterpolator_.GetHeightWidthRatio() );
	//	connect( aspectRatioSpinBox_, SIGNAL( valueChanged( double ) ),
	//			this, SLOT( SetHWRatioSlot( double ) ) );
	//	toolBar->addWidget( aspectRatioSpinBox_ );
	
	// Lorentzian half height coordinate
	toolBar->addWidget( new QLabel( " Half width: " ) );
	halfWidthSpinBox_ = new QDoubleSpinBox;
	halfWidthSpinBox_->setRange( 0.5, 50. );
	halfWidthSpinBox_->setSingleStep( .5 );
	halfWidthSpinBox_->setValue( halfWidth_ );
	connect( halfWidthSpinBox_, SIGNAL( valueChanged( double ) ),
			this, SLOT( SetHalfWidthSlot( double ) ) );
	toolBar->addWidget( halfWidthSpinBox_ );
	
	layout->addWidget( toolBar );
}

//------------------------------------------------------------------------------
void SpectrumWidget::AddBottomToolbar( QLayout* layout )
{	
	QToolBar* toolBar = new QToolBar;
	toolBar->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
	// NUMBER OF STEPS
	toolBar->addWidget( new QLabel( " Steps: " ) );
	stepsSpinBox_ = new QSpinBox;
	stepsSpinBox_->setRange( 2, 10000 );
	stepsSpinBox_->setSingleStep( 10 );
	stepsSpinBox_->setValue( steps_ );
	connect( stepsSpinBox_, SIGNAL( valueChanged( int ) ), this, SLOT( SetStepsSlot( int ) ) );
	toolBar->addWidget( stepsSpinBox_ );
	
	toolBar->addSeparator();
			
	// Y-AXIS ORIENTATION
	QCheckBox* yOrientationCB = new QCheckBox( "Top-down" );
	connect( yOrientationCB, SIGNAL( stateChanged( int ) ),
			this, SLOT( SetYAxisTopDownSlot( int ) ) );
	toolBar->addWidget( yOrientationCB );
	
	toolBar->addSeparator();
	
	// GRID ON/OFF
	QCheckBox* gridCB = new QCheckBox( "Grid" );
	gridCB->setCheckState( Qt::Checked ) ;
	connect( gridCB, SIGNAL( stateChanged( int ) ),
			 this, SLOT( ShowGridSlot( int ) ) );
	toolBar->addWidget( gridCB );
	
	// ZOOM ON/OFF
//	QCheckBox* zoomCB = new QCheckBox( "Zoom" );
//	connect( zoomCB, SIGNAL( stateChanged( int ) ),
//			 this, SLOT( EnableZoomModeSlot( int ) ) );
//	// no zoom for now
//	toolBar->addWidget( zoomCB );
		
	layout->addWidget( toolBar );
}

//------------------------------------------------------------------------------
void SpectrumWidget::GraphTypeSlot( int comboBoxIndex )
{
	assert( graphTypeCBox_ );
	graphType_ = GraphType( graphTypeCBox_->itemData( comboBoxIndex ).toInt() );
	if( graphType_ == HISTOGRAM )
	{
		stepsSpinBox_->setEnabled( false );
		if( aspectRatioSpinBox_ ) aspectRatioSpinBox_->setEnabled( false );
		if( halfWidthSpinBox_ ) halfWidthSpinBox_->setEnabled( false );
	}
	else
	{
		stepsSpinBox_->setEnabled( true );
		if( aspectRatioSpinBox_ ) aspectRatioSpinBox_->setEnabled( true );
		if( halfWidthSpinBox_ ) halfWidthSpinBox_->setEnabled( true );
	}
	GenerateData();	
}

//------------------------------------------------------------------------------
void SpectrumWidget::RadiationTypeSlot( int comboBoxIndex )
{
	assert( radiationTypeCBox_ );
	radiationType_ = RadiationType( radiationTypeCBox_->itemData( comboBoxIndex ).toInt() );
	GenerateData();	
}

//------------------------------------------------------------------------------
void SpectrumWidget::SetStepsSlot( int steps )
{
	steps_ = steps;
	GenerateData();
} 

//------------------------------------------------------------------------------
void SpectrumWidget::SetXAxisRightLeftSlot( int state )
{
	xAxisOrientationRightLeft_ = ( state == Qt::Checked );
	GenerateData();
}

//------------------------------------------------------------------------------
void SpectrumWidget::SetYAxisTopDownSlot( int state )
{
	yAxisOrientationTopDown_ = ( state == Qt::Checked );
	GenerateData();
}

//------------------------------------------------------------------------------
void SpectrumWidget::SetHWRatioSlot( double r )
{
	aspectRatio_ = r;
	GenerateData();
}

//------------------------------------------------------------------------------
void SpectrumWidget::SetHalfWidthSlot( double hw )
{
	halfWidth_ = hw;
	oneOverHalfWidthSquared_ = 1.0 / ( halfWidth_ * halfWidth_ );
	GenerateData();
}

