#ifndef DIALOGWIDGET_H_
#define DIALOGWIDGET_H_
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

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QCoreApplication>

/// Utility class that adds a widget and a close button to a standard dialog.
class DialogWidget : public QDialog
{
	QWidget* w_;
public:
    DialogWidget( QWidget* w, QWidget* parent = 0, Qt::WindowFlags f = 0  ) : QDialog( parent, f ), w_( w )
    {
        QVBoxLayout* vl = new QVBoxLayout;
        vl->addWidget( w );
        if( f != Qt::Tool )
        {
        	QPushButton* close = new QPushButton( "Close" );
       	 	connect( close, SIGNAL( released() ), this, SLOT( accept() ) );
        	vl->addWidget( close );
        }
        setLayout( vl );
        QApplication* app = qobject_cast< QApplication* >( QCoreApplication::instance() );
        if( app ) setWindowIcon( app->windowIcon() );
    }

    QWidget* GetWidget() { return w_; }

    const QWidget* GetWidget() const { return w_; }

};
#endif /*DIALOGWIDGET_H_*/
