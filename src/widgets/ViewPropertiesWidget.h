#ifndef VIEWPROPERTIESWIDGET_H_
#define VIEWPROPERTIESWIDGET_H_

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

#include <QWidget>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QSize>

#include "../MainWindow.h"

#include <cassert>


//
//---------------------------------------------
//  -3D View Size---------------------------
// |        _______ __          _______ __  |
// | Width |_______|^v| Height |_______|^v| |
// |                                        |
//  ----------------------------------------
//---------------------------------------------
//
/// Allows the user to change the 3D View properties.
class ViewPropertiesWidget : public QWidget
{
    Q_OBJECT

    /// Reference to main window.
	MainWindow* mw_;

    /// Updates the size of the main window so that that width and height
    /// of the 3d view match the size passed to this method.
    /// @param w requested 3d view width.
    /// @param h requested 3d view height.
	void UpdateSize( int w, int h )
	{
		assert( mw_ );
		const QSize ws = mw_->Get3DViewSize();
		const QSize delta( w - ws.width(), h - ws.height() );
		mw_->resize( mw_->size().width() + delta.width(),
					 mw_->size().height() + delta.height() );
	}

public slots:

    /// Invoked when the horizontal size spin box value changes.
	void VerticalSizeChanged( int h )
	{
		UpdateSize( mw_->Get3DViewSize().width(), h );
	}

     /// Invoked when the vertical size spin box value changes.
	void HorizontalSizeChanged( int w )
	{
		UpdateSize( w, mw_->Get3DViewSize().height() );
	}

public:

    /// Constructor. Builds UI and connects spinbox signals to slots.
	ViewPropertiesWidget( MainWindow* mw, QWidget* parent = 0 ) : mw_( mw ), QWidget( parent )
	{
		QGroupBox* gb = new QGroupBox( "3D View Size" );
		QHBoxLayout* layout = new QHBoxLayout;

        QSpinBox* sb = new QSpinBox;
        sb->setRange( 1, 2048 );
		sb->setValue( mw->Get3DViewSize().width() );
		connect( sb, SIGNAL( valueChanged( int ) ), this, SLOT( HorizontalSizeChanged( int ) ) );
		layout->addWidget( new QLabel( "Width: " ) );
		layout->addWidget( sb );

      	sb = new QSpinBox;
        sb->setRange( 1, 2048 );
        sb->setValue( mw->Get3DViewSize().height() );
		connect( sb, SIGNAL( valueChanged( int ) ), this, SLOT( VerticalSizeChanged( int ) ) );
		layout->addWidget( new QLabel( "Height: " ) );
		layout->addWidget( sb );

		gb->setLayout( layout );

		layout = new QHBoxLayout;
		layout->addWidget( gb );
		setLayout( layout );		
	}

};


#endif /*VIEWPROPERTIESWIDGET_H_*/
