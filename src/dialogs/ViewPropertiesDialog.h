#ifndef VIEWPROPERTIESDIALOG_H_
#define VIEWPROPERTIESDIALOG_H_
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

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QCheckBox>

#include "../widgets/ViewPropertiesWidget.h"
#include "../widgets/DepthPeelingWidget.h"
#include "../widgets/AntiAliasingWidget.h"

class MainWindow;

//
//--------------------------
//  ----------------------
// |                      |
// | ViewPropertiesWidget |
// |                      |
// | DepthPeelingWidget   |
// |                      |
// | AntiAliasingWidget   |
// |                      |
// | [ ] Real-time update |
//  ----------------------
//  __________
// |__Close___|
//
//--------------------------
//
/// Simple container for ViewPropertiesWidget, DepthPeelingWidget
/// and AntiAliasingWidget.
class ViewPropertiesDialog : public QDialog
{
	Q_OBJECT
public:
	
    /// Constructor. Adds ViewPropertiesWidget instance and push button to dialog.
	ViewPropertiesDialog( MainWindow* mw, QWidget* parent = 0 ) : 
		QDialog( parent ), depthPeelingWidget_( 0 ), antiAliasingWidget_( 0 )
	{
		QVBoxLayout* vb = new QVBoxLayout;
		vb->addWidget( new ViewPropertiesWidget( mw ) );
		depthPeelingWidget_ = new DepthPeelingWidget( mw ); 
		vb->addWidget( depthPeelingWidget_ );
		antiAliasingWidget_ = new AntiAliasingWidget( mw );
		vb->addWidget( antiAliasingWidget_ );
		QCheckBox* realTimeCB = new QCheckBox( "Real-time update" );
		connect( realTimeCB, SIGNAL( stateChanged( int ) ),
				this, SLOT( RealTimeUpdateSlot( int ) ) );
		vb->addWidget( realTimeCB );
		QPushButton* pb = new QPushButton( "Close" );
		connect( pb, SIGNAL( released() ), this, SLOT( accept() ) );
		vb->addWidget( pb );
		setLayout( vb );		
	}
private slots:
	
	/// Invoked when real-time checkbox state changed.
	/// Enables/disables real-time update by setting child widgets' flags.
	void RealTimeUpdateSlot( int state )
	{
		const bool on = ( state == Qt::Checked );
		depthPeelingWidget_->SetRealTimeUpdate( on );
		antiAliasingWidget_->SetRealTimeUpdate( on );		
	}

private:
	/// Depth peeling.
	DepthPeelingWidget* depthPeelingWidget_;
	/// Anti-aliasing.
	AntiAliasingWidget* antiAliasingWidget_;
};




#endif /*VIEWPROPERTIESDIALOG_H_*/
