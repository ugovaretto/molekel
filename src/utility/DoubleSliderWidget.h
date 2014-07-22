#ifndef DOUBLESLIDERWIDGET_H_
#define DOUBLESLIDERWIDGET_H_
//
// Copyright (c) 2006, 2007, 2008, 2009 - Ugo Varetto and 
// Swiss National Supercomputing Centre (CSCS)
//
// This source code is free; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This source code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this source code; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
// 

#include <cassert>
#include <stdexcept>
#include <utility>

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QSlider>

/// Slider for editing float values in the range [0,1].
/// The widget displays the current value and supports an optional text label. 
class DoubleSliderWidget : public QWidget
{
	Q_OBJECT
	
public:
	/// Constructor.
	DoubleSliderWidget( const QString& text = "", Qt::Orientation o = Qt::Horizontal, 
			            QWidget* parent = 0 ) :
			            	QWidget( parent ), label_( 0 ), slider_( 0 )
	{
		// slider
		slider_ = new QSlider( o );
		slider_->setRange( 0, 100 );
		slider_->setValue( 0 );
		connect( slider_, SIGNAL( valueChanged( int ) ), this, SLOT( ValueChangedSlot( int ) ) );
		// label displaying current value
		label_ = new QLabel( "0.00" );
				
		// set the coordinates of label and slider in grid depending on orientation
		const std::pair< int, int > sliderCoord( o == Qt::Horizontal ? 0 : 2, o == Qt::Horizontal ? 2 : 0 ); 
		const std::pair< int, int > labelCoord( o == Qt::Horizontal ? 0 : 1, o == Qt::Horizontal ? 1 : 0 );
		
		// add controls to 3x1 or 1x3 grid layout
		QGridLayout* gridLayout = new QGridLayout;
		gridLayout->setMargin( 0 );
		gridLayout->addWidget( new QLabel( text ), 0, 0 );
		gridLayout->addWidget( slider_, sliderCoord.first, sliderCoord.second );
		gridLayout->addWidget( label_, labelCoord.first, labelCoord.second );
		
		setLayout( gridLayout );		
	}

	/// Returns value in range [0,1].
	double GetValue() const
	{
		const double range = slider_->maximum() - slider_->minimum();
		return ( slider_->value() - slider_->minimum() ) / range;
	}

public slots:
	
	/// Changes value. 
	void SetValue( double v )
	{
		if( v < 0. || v > 1.0 ) throw std::domain_error( "Value out of range [0,1]" );
		assert( v < 0. || v > 1.0 ); //in case exceptions are turned off
		const double range = slider_->maximum() - slider_->minimum();
		slider_->setValue( slider_->minimum() + int( v * range + 0.5 ) );
	}

signals:
	
    /// Emitted when value changed.
	void ValueChanged( double );

private slots:
	
	/// Invoked when slider value changed.
	void ValueChangedSlot( int )
	{
		const double v = GetValue();
		QString l;
		if( v == 1.0 ) l = "1.00";
		else if( v == 0.0 ) l = "0.00";
		else l = QString( QString( "%1" ).arg( v, 3, 'g', 2 ) );
		label_->setText( l );
		emit ValueChanged( GetValue() );
	}
	
private:
	/// Label displaying current value.
	QLabel* label_;
	/// Slider.
	QSlider* slider_;
};


#endif /*DOUBLESLIDERWIDGET_H_*/
