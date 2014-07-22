#ifndef EVENTPOSITIONTIMEWIDGET_H_
#define EVENTPOSITIONTIMEWIDGET_H_
//
// Copyright (c) 2006, 2007, 2008, 2009 - Ugo Varetto
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

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QGroupBox>

#include "EventPlayer.h"

class EventPositionTimeWidget : public QWidget
{

    Q_OBJECT

public:
    EventPositionTimeWidget( EventPlayer* ep, QWidget* parent = 0 ) :
        QWidget( parent ),
        ep_( ep ),
        xoff_( 0 ),
        yoff_( 0 ),
        xscale_( 0 ),
        yscale_( 0 ),
        toff_( 0 ),
        tscale_( 0 ),
        minDelay_( 0 )
    {
        assert( ep_ && "NULL EventPlayer" );

        // Event position
        xoff_ = new QSpinBox;
        xoff_->setRange( -100, 100 );
        xoff_->setValue( ep_->GetEventOffsetX() );
        xoff_->setSingleStep( 1 );
        connect( xoff_, SIGNAL( valueChanged( int ) ), this, SLOT( UpdatePlayerPos() ) );

        yoff_ = new QSpinBox;
        yoff_->setRange( -100, 100 );
        yoff_->setValue( ep_->GetEventOffsetY() );
        yoff_->setSingleStep( 1 );
        connect( yoff_, SIGNAL( valueChanged( int ) ), this, SLOT( UpdatePlayerPos() ) );

        xscale_ = new QDoubleSpinBox;
        xscale_->setRange( .1, 10.0 );
        xscale_->setValue( ep_->GetEventScalingX() );
        xscale_->setSingleStep( 0.1 );
        connect( xscale_, SIGNAL( valueChanged( double ) ), this, SLOT( UpdatePlayerPos() ) );

        yscale_ = new QDoubleSpinBox;
        yscale_->setRange( .1, 10.0 );
        yscale_->setValue( ep_->GetEventScalingY() );
        yscale_->setSingleStep( 0.1 );
        connect( yscale_, SIGNAL( valueChanged( double ) ), this, SLOT( UpdatePlayerPos() ) );

        // Event timing
        toff_ = new QSpinBox;
        toff_->setRange( 0, 10000 );
        toff_->setValue( ep_->GetDelay() );
        toff_->setSingleStep( 10 );
        connect( toff_, SIGNAL( valueChanged( int ) ), this, SLOT( UpdatePlayerTime() ) );

        tscale_ = new QDoubleSpinBox;
        tscale_->setRange( 0., 10.0 );
        tscale_->setValue( ep_->GetTimeScaling() );
        tscale_->setSingleStep( 0.1 );
        connect( tscale_, SIGNAL( valueChanged( double ) ), this, SLOT( UpdatePlayerTime() ) );

        // Event timing
        minDelay_ = new QSpinBox;
        minDelay_->setRange( 0, 10000 );
        minDelay_->setValue( ep_->GetMinDelay() );
        minDelay_->setSingleStep( 10 );
        connect( minDelay_, SIGNAL( valueChanged( int ) ), this, SLOT( UpdatePlayerTime() ) );

        // Event position layout
        QGridLayout* pgl = new QGridLayout;
        // 1st row
        pgl->addWidget( new QLabel( "x scaling:" ), 0, 0 ); pgl->addWidget( xscale_, 0, 1 );
        pgl->addWidget( new QLabel( "y scaling:" ), 0, 2 ); pgl->addWidget( yscale_, 0, 3 );
        // 2nd row
        pgl->addWidget( new QLabel( "x offset:" ), 1, 0 ); pgl->addWidget( xoff_, 1, 1 );
        pgl->addWidget( new QLabel( "y offset:" ), 1, 2 ); pgl->addWidget( yoff_, 1, 3 );

        // Timing layout
        QGridLayout* tgl = new QGridLayout;
        // 1st row
        tgl->addWidget( new QLabel( "time scaling:" ), 0, 0 ); tgl->addWidget( tscale_, 0, 1 );
        // 2nd row
        tgl->addWidget( new QLabel( "time offset (ms):" ), 1, 0 ); tgl->addWidget( toff_, 1, 1 );
        // 3rd row
        tgl->addWidget( new QLabel( "min delay (ms):" ), 2, 0 ); tgl->addWidget( minDelay_, 2, 1 );

        // Event position group
        QGroupBox* pgb = new QGroupBox( "Event Position" );
        pgb->setLayout( pgl );

        // Event timing group
        QGroupBox* tgb = new QGroupBox( "Event Timing" );
        tgb->setLayout( tgl );

        // Add everything to main widget layout
        QVBoxLayout* vbl = new QVBoxLayout;
        vbl->addWidget( pgb );
        vbl->addWidget( tgb );

        setLayout( vbl );
    }

public:

    void SetTimeScaling( double s )
    {
        assert( tscale_ );
        tscale_->setValue( s );
    }


private slots:

    void UpdatePlayerPos()
    {
        ep_->SetEventScaling( xscale_->value(), yscale_->value() );
        ep_->SetEventOffset( xoff_->value(), yoff_->value() );
    }

    void UpdatePlayerTime()
    {
        ep_->SetDelay( toff_->value() );
        ep_->SetTimeScaling( tscale_->value() );
        ep_->SetMinDelay( minDelay_->value() );
    }

private:
    EventPlayer* ep_;
    QSpinBox* xoff_;
    QSpinBox* yoff_;
    QDoubleSpinBox* xscale_;
    QDoubleSpinBox* yscale_;
    QSpinBox* toff_;
    QDoubleSpinBox* tscale_;
    QSpinBox* minDelay_;
};


#endif /*EVENTPOSITIONTIMEWIDGET_H_*/
