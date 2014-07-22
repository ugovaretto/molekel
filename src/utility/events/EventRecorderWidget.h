#ifndef EVENTRECORDERWIDGET_H_
#define EVENTRECORDERWIDGET_H_
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

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QString>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>

#include <fstream>

#include "EventFilter.h"


/// Simple widget to store recorded event to file.
class EventRecorderWidget : public QWidget
{
    Q_OBJECT

private:

    /// Connect top level menus' ActionTriggered signals to event filter's ActionTriggered slot.
    void ConnectTopLevelMenus();

    /// Disconnect top level menus' ActionTriggered signals from event filter's ActionTriggered slot.
    void DisconnectTopLevelMenus();

private slots:

    /// Record.
    void StartRecording();
    /// Stop recording.
    void StopRecording();
    /// Select output file.
    void SelectFile();

    /// Called whenever the "Record Cursor" check box state changed.
    /// Enables/disables recording of mouse move events when no mouse button is down.
    void RecordCursorSlot( int state )
    {
        eventFilter_.SetRecordMouseMove( state == Qt::Checked );
    }

signals:
	
    /// Emitted when recording starts.
    void RecordingStarted();
    
    /// Emitted when recording ends.
    void RecordingStopped();
    
    /// Emitted when status changed. 
    void Status( const QString& );
    
public:
    /// Constructor.
    EventRecorderWidget( QWidget* parent = 0 ) : QWidget( parent ),
#ifdef __APPLE_CC__
    recordActions_( true ), autoConnectMenus_( true )
#else
    recordActions_( false ), autoConnectMenus_( false )
#endif
    {
        record_ = new QPushButton( "Record" );
        connect( record_, SIGNAL( released() ), this, SLOT( StartRecording() ) );

        stop_ = new QPushButton( "Stop" );
        connect( stop_, SIGNAL( released() ), this, SLOT( StopRecording() ) );

        recordCursor_ = new QCheckBox( "Record Cursor" );
        recordCursor_->setCheckState( eventFilter_.GetRecordMouseMove() ? Qt::Checked : Qt::Unchecked );
        connect( recordCursor_, SIGNAL( stateChanged( int ) ), this, SLOT( RecordCursorSlot( int ) ) );

        filePath_ = new QLineEdit;
        select_ = new QPushButton( "Select Output File..." );
        connect( select_, SIGNAL( released() ), this, SLOT( SelectFile() ) );

        QGridLayout* gl = new QGridLayout;

        gl->addWidget( record_, 0, 0 );
        gl->addWidget( stop_, 0, 1 );
        gl->addWidget( recordCursor_, 0, 2 );

        gl->addWidget( filePath_, 1, 1, 1, 2 );
        gl->addWidget( select_, 1, 0 );

        QGroupBox* gb = new QGroupBox( "Record Events" );
        gb->setLayout( gl );

        QVBoxLayout* l = new QVBoxLayout;
        l->addWidget( gb );

        setLayout( l );

        setObjectName( "Event Recorder" );
    }

    /// Set target for which events will be ignored.
    void SetObjectToIgnore( QObject* obj ) { eventFilter_.SetObjectToIgnore( obj ); }

    /// Enable/disable recording of action trigger events.
    void SetRecordActions( bool on ) { recordActions_ = on; }

    /// Return value of record actions flag.
    bool GetRecordActions() const { return recordActions_; }

    /// QSettings key used to load/store output file path.
    void SetSettingsKey( const QString& key ) { settingsKey_ = key; }

    /// Returns QSettings key used to load/store output file path.
    const QString& GetSettingsKey() const { return settingsKey_; }

    /// Enable/disable auto connection of top level menus. If autoconnect
    /// is enabled top level menus' ActionTriggered signals are automatically
    /// connected to event filter's ActionTriggered slot to allow for recording of
    /// action trigger events.
    void SetTopMenuAutoconnect( bool on ) { autoConnectMenus_ = on; }

    /// Returns top level menu autoconnect flag.
    bool GetTopMenuAutoconnect() const { return autoConnectMenus_; }


private:

    /// Updates application settings with current dir path.
    void UpdateSettings() const;

private:
    /// Record button.
    QPushButton* record_;
    /// Stop button.
    QPushButton* stop_;
    /// Select file button.
    QPushButton* select_;
    /// Output file path.
    QLineEdit* filePath_;
    /// Event filter.
    LogEventFilter eventFilter_;
    /// Output stream.
    std::ofstream os_;
    /// If true top level QMenu's actionTrigger(QAction*) signals are connected
    /// to the event filter instance.
    bool recordActions_;
    /// QSettings key: if value not empty the file path is read/stored from/into QSettings
    /// using this data member as the key.
    QString settingsKey_;
    /// Allows users to switch on/off recording of mouse cursor when no mouse button is down.
    QCheckBox* recordCursor_;
    /// Auto connect top level menus.
    bool autoConnectMenus_;
};


#endif /*EVENTRECORDERWIDGET_H_*/
