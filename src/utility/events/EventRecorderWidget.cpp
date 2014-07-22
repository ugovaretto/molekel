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

#include <QCoreApplication>
#include <QEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QMenu>
#include <QSettings>
#include <QDir>

#include "EventRecorderWidget.h"
#include "EventLogger.h"

#include <iostream>

#include "../RAII.h"
#include "../qtfileutils.h"

//------------------------------------------------------------------------------
void EventRecorderWidget::StartRecording()
{

	if( recordActions_ && autoConnectMenus_ )
    {
        // connect all the top level menus' ActionTriggered signal to the event filter
        ConnectTopLevelMenus();
    }

	if( filePath_->text().isEmpty() )
    {
    	DisconnectTopLevelMenus();
    	QMessageBox::critical( this, "I/O Error", "Select output file" + filePath_->text() );
        return;
    }

    UpdateSettings();

    if( !settingsKey_.isEmpty() )
    {
        QSettings settings;
        settings.setValue( settingsKey_, DirPath( filePath_->text() ) );
    }

    const std::string path = filePath_->text().toStdString();
    os_.open( path.c_str(), std::ios_base::out );
    if( !os_.is_open() )
    {
        DisconnectTopLevelMenus();
        QMessageBox::critical( this, "I/O Error", "Cannot write to file" + filePath_->text() );
        return;
    }

    QEvent::Type events[] = {
                            QEvent::MouseButtonPress,
                            QEvent::MouseButtonRelease,
                            QEvent::MouseButtonDblClick,
                            QEvent::MouseMove,
                            QEvent::Wheel,
                            QEvent::Shortcut,
                            QEvent::KeyPress,
                            QEvent::KeyRelease,
                            QEvent::MenubarUpdated,
                            QEvent::ContextMenu,
                            QEvent::Resize,
                            QEvent::Move,
                          /*QEvent::ActionChanged,
                            QEvent::ModifiedChange,
                            QEvent::FocusIn,
                            QEvent::FocusOut,
                            QEvent::Enter,
                            QEvent::Leave,
                            QEvent::EnabledChange,
                            QEvent::HoverEnter,
                            QEvent::HoverLeave,
                            QEvent::HoverMove,
                            QEvent::Show,
                            QEvent::Hide,
                            QEvent::UpdateRequest,
                            QEvent::WindowActivate,
                            QEvent::WindowDeactivate,
                            QEvent::ActivationChange,*/
                          };
    std::set< QEvent::Type > ev( events, events + sizeof( events ) / sizeof( QEvent::Type ) );
    eventFilter_.SetLogger( new EventLogger< std::ostream >( os_, '\t', '\n' ) );
    eventFilter_.SetFilter( ev );
    QCoreApplication::instance()->installEventFilter( &eventFilter_ );
    eventFilter_.SetObjectToIgnore( this );
    eventFilter_.StartRecording();
    emit RecordingStarted();
    emit Status( "Recording Events" );
}

//------------------------------------------------------------------------------
void EventRecorderWidget::StopRecording()
{
    if( !eventFilter_.Recording() ) return;
    os_.flush();
    os_.close();
    eventFilter_.StopRecording();

    // disconnect all the top level menus' ActionTriggered signal from the event filter
    DisconnectTopLevelMenus();
    QCoreApplication::instance()->removeEventFilter( &eventFilter_ );
    emit RecordingStopped();
    emit Status( "Finished Recording Events" );
}

//------------------------------------------------------------------------------
void EventRecorderWidget::SelectFile()
{
    QSettings settings;
    const QString dir = settings.value( settingsKey_, "" ).toString();
    const QString file = GetSaveFileName( this, "Select output file",
                                          dir.isEmpty() ? QCoreApplication::applicationDirPath() + "/../" : dir );
    if( file.isEmpty() ) return;
    filePath_->setText( file );
    UpdateSettings();
}

//------------------------------------------------------------------------------
void EventRecorderWidget::ConnectTopLevelMenus()
{
    foreach( QWidget *widget, QApplication::topLevelWidgets() )
    {
         if( qobject_cast< QMenu* >( widget ) )
         {
            connect( qobject_cast< QMenu* >( widget ), SIGNAL( triggered( QAction* ) ),
                     &eventFilter_, SLOT( ActionTriggered( QAction* ) ) );
         }
     }
}

//------------------------------------------------------------------------------
void EventRecorderWidget::DisconnectTopLevelMenus()
{
    foreach( QWidget *widget, QApplication::topLevelWidgets() )
    {
         if( qobject_cast< QMenu* >( widget ) )
         {
            disconnect( qobject_cast< QMenu* >( widget ), SIGNAL( triggered( QAction* ) ),
                        &eventFilter_, SLOT( ActionTriggered( QAction* ) ) );
         }
    }
}

//------------------------------------------------------------------------------
void EventRecorderWidget::UpdateSettings() const
{
    if( !settingsKey_.isEmpty() )
    {
        QSettings settings;
        settings.setValue( settingsKey_, DirPath( filePath_->text() ) );
    }
}


