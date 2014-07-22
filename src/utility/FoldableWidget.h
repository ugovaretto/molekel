#ifndef FOLDABLE_WIDGET_H_
#define FOLDABLE_WIDGET_H_
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

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>

/// Simple foldable widget. Client code passes a pointer to a layout which
/// will be embedded inside a QGroupBox to be shown/hidden by clicking on
/// a +/- button placed at the left of the title label.
class FoldableWidget : public QWidget
{
	Q_OBJECT
   
public:
	/// Constructor.
	/// @param lo QLayout containing the widgets to be shown/hidden
	/// @param name title
	/// @param parent parent widget
	explicit FoldableWidget( QLayout* lo,
		                     const QString& name = "",
		                     QWidget* parent = 0 ) : 
							 QWidget( parent ), group_( 0 ), b_( 0 )
	{
		b_ = new QPushButton( "+" );
		b_->setFixedSize( 18, 18 );
		connect( b_, SIGNAL( clicked() ), this, SLOT( Clicked() ) );

		QLabel* l = new QLabel( " " + name );

		QVBoxLayout* vl = new QVBoxLayout;
		QHBoxLayout* hl = new QHBoxLayout;
		hl->addWidget( b_ );
		hl->addWidget( l );
		vl->addItem( hl );

		group_ = new QGroupBox;
		group_->setLayout( lo );
		group_->hide();
		vl->addWidget( group_ );
		setLayout( vl );
	}
  
public slots:
	/// Invoked when button clicked.
    void Clicked()
	{
		if( group_->isHidden() )
		{
			group_->show();
			b_->setText( "-" );
		}
		else
		{
			group_->hide();
			b_->setText( "+" );
		}
	}

private:
	/// Group box containing the content to be folded/unfolded.
	QGroupBox* group_;
	/// Triggers folding un-folding of content.
	QPushButton* b_;
};
#endif // FOLDABLE_WIDGET_H_
