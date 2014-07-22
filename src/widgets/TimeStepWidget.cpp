//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008 Swiss National Supercomputing Centre (CSCS)
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
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

#include "TimeStepWidget.h"
#include "../MainWindow.h"
#include "../utility/RAII.h"

class QSpinBox;
class MainWindow;
class QString;


//------------------------------------------------------------------------------
TimeStepWidget::TimeStepWidget( MainWindow* mw, QWidget* parent )
    : QWidget( parent ), mw_( mw ), timestepSpinBox_( 0 ), updatingGUI_( false )
{
    assert( mw && "NULL MainWindow" );
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    QHBoxLayout* mainLayout = new QHBoxLayout;
    QLabel* timestepLabel = new QLabel( tr( "Time Step" ) + " (ms)" );
    timestepSpinBox_ = new QSpinBox;
    timestepSpinBox_->setRange( TIMESTEP_MIN, TIMESTEP_MAX );
    timestepSpinBox_->setSingleStep( TIMESTEP_SINGLE_STEP );
    timestepSpinBox_->setValue( mw->GetTimeStep() );
    connect( timestepSpinBox_, SIGNAL( valueChanged( int ) ),
             this, SLOT( TimeStepIntValueChanged( int ) ) );
    connect( timestepSpinBox_, SIGNAL( valueChanged( const QString& ) ),
             this, SLOT( TimeStepStringValueChanged( const QString& ) ) );
    mainLayout->addWidget( timestepLabel );
    mainLayout->addWidget( timestepSpinBox_ );
    this->setLayout( mainLayout );
}

//------------------------------------------------------------------------------
void TimeStepWidget::TimeStepIntValueChanged( int v )
{
    if( updatingGUI_ ) return;
    assert( mw_ && "NULL MainWindow" );
    mw_->SetTimeStep( v );
}

//------------------------------------------------------------------------------
void TimeStepWidget::TimeStepStringValueChanged( const QString& s )
{
    if( updatingGUI_ ) return;
    assert( mw_ && "NULL MainWindow" );
    mw_->SetTimeStep( s.toInt() );
}
