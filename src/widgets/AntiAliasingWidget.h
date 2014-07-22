#ifndef ANTIALIASINGWIDGET_H_
#define ANTIALIASINGWIDGET_H_
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

#include "../MainWindow.h"

#include <cassert>

//
//---------------------------------------------------------
//  -Anti-aliasing---------------------------------------
// |          _______ __                                 |
// | Samples |_______|^v|                                |
// |                                                     |
//  -----------------------------------------------------
//---------------------------------------------------------
//
/// Editing of anti-aliasing parameters.
class AntiAliasingWidget : public QWidget
{
    Q_OBJECT
    
public slots:

    /// Number of samples.
	void SamplesChanged( int f )
	{
		mw_->SetAAFrames( f );
		label_->setText( f ? on_ : off_ );
		if( realTimeUpdate_ ) mw_->Refresh();
	}

public:

    /// Constructor. Builds UI and connects spinbox signals to slots.
	AntiAliasingWidget( MainWindow* mw, QWidget* parent = 0 ) : 
		QWidget( parent ), mw_( mw ), off_( "Samples (anti-aliasing off): " ),
		on_( "Samples: " ), label_( new QLabel( off_ ) ), realTimeUpdate_( false )
	{
		QGroupBox* gb = new QGroupBox( "Anti Aliasing" );
		QHBoxLayout* layout = new QHBoxLayout;

        QSpinBox* sb = new QSpinBox;
        sb->setRange( 0, 64 );
        layout->addWidget( label_ );
        connect( sb, SIGNAL( valueChanged( int ) ), this, SLOT( SamplesChanged( int ) ) );
        sb->setValue( mw_->GetAAFrames() );
		layout->addWidget( sb );

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
  
	/// Off
	const QString off_;
	/// On
    const QString on_;
  
    /// Label.
    QLabel* label_;
    /// Enable/disable real-time update.
    bool realTimeUpdate_;

};


#endif /*ANTIALIASINGWIDGET_H_*/
