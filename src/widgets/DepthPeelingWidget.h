#ifndef DEPTHPEELINGWIDGET_H_
#define DEPTHPEELINGWIDGET_H_
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

#include <QWidget>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QDoubleSpinBox>

#include "../MainWindow.h"

#include <cassert>


//
//---------------------------------------------------------
//  -[ ] Depth peeling-----------------------------------
// |            _______ __                   _______ __  |
// | Max peels |_______|^v| Occlusion ratio |_______|^v| |
// |                                                     |
//  -----------------------------------------------------
//---------------------------------------------------------
//
/// Editing of depth peeling parameters.
class DepthPeelingWidget : public QWidget
{
    Q_OBJECT
  
public slots:

    /// Change the max number of peels.
	void MaxPeelsChanged( int p )
	{
		mw_->SetMaxNumberOfPeels( p );
		if( realTimeUpdate_ ) mw_->Refresh();
	}

    /// Change occlusion ratio.
	void OcclusionRatioChanged( double r )
	{
		mw_->SetDPOcclusionRatio( r );
		if( realTimeUpdate_ ) mw_->Refresh();
	}
	
	/// Enable/disable depth peeling.
	void GroupBoxToggled( bool on )
	{
		mw_->SetDepthPeelingEnabled( on );
		if( realTimeUpdate_ ) mw_->Refresh();
	}

public:

    /// Constructor. Builds UI and connects spinbox signals to slots.
	DepthPeelingWidget( MainWindow* mw, QWidget* parent = 0 ) :
		QWidget( parent ), mw_( mw ), realTimeUpdate_( false )
	{
		QGroupBox* gb = new QGroupBox( "Depth peeling" );
		gb->setCheckable( true );
		gb->setChecked( mw_->DepthPeelingSupported() && mw_->GetDepthPeelingEnabled() );
		gb->setEnabled( mw_->DepthPeelingSupported() );
		connect( gb, SIGNAL( toggled( bool ) ), this, SLOT( GroupBoxToggled( bool ) ) );
		QHBoxLayout* layout = new QHBoxLayout;

        QSpinBox* sb = new QSpinBox;
        sb->setRange( 0, 1000 );
		sb->setValue( mw_->GetMaxNumberOfPeels() );
		connect( sb, SIGNAL( valueChanged( int ) ), this, SLOT( MaxPeelsChanged( int ) ) );
		layout->addWidget( new QLabel( "Max peels: " ) );
		layout->addWidget( sb );

      	QDoubleSpinBox* sb2 = new QDoubleSpinBox;
        sb2->setRange( 0.0, 1.0 );
        sb2->setSingleStep( 0.05 );
        sb2->setValue( mw_->GetDPOcclusionRatio() );
		connect( sb2, SIGNAL( valueChanged( double ) ), this, SLOT( OcclusionRatioChanged( double ) ) );
		layout->addWidget( new QLabel( "Occlusion ratio: " ) );
		layout->addWidget( sb2 );

		gb->setLayout( layout );

		layout = new QHBoxLayout;
		layout->addWidget( gb );
		setLayout( layout );		
	}
	
	//@{ Enable/disable real-time update.
	void SetRealTimeUpdate( bool on ) { realTimeUpdate_ = on; }
	bool GetRealTimeUpdate() const { return realTimeUpdate_; }
	//@}
	
private:

	/// Reference to main window.
	MainWindow* mw_;
	
	/// Enable/disable real-time update.
	bool realTimeUpdate_;

};


#endif /*DEPTHPEELINGWIDGET_H_*/
