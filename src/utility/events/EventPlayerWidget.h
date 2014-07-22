#ifndef EVENTPLAYERWIDGET_H_
#define EVENTPLAYERWIDGET_H_
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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>

#include "EventPlayer.h"
#include "EventPositionTimeWidget.h"

#include "../DialogWidget.h"

/// Simple widget to set input file and playback events.
class EventPlayerWidget : public QWidget
{
    Q_OBJECT

private slots:

    /// Stop player.
    void Stop();

    /// Select input file
    void SelectFile();

    /// Invoked when "Move Cursor" checkbox state changed. Forward call to
    /// EventPlayer::ManageCursorOn/Off() method to enable/disable
    void ManageCursorSlot( int state )
    {
        if( state == Qt::Checked ) eventPlayer_.ManageCursorOn();
        else eventPlayer_.ManageCursorOff();
    }

    /// Invoked when "Trigger Action" checkbox state changed. Forward call to
    /// EventPlayer::TriggerActionsOn/Off() method to enable/disable
    void TriggerActionsSlot( int state )
    {
        if( state == Qt::Checked ) eventPlayer_.TriggerActionsOn();
        else eventPlayer_.TriggerActionsOff();
    }

    /// Invoked when 'Advanced...' button pressed.
    void Advanced();

public slots:

    /// Start playing events.
    void Play();

public:
    /// Constructor.
    EventPlayerWidget( QWidget* parent = 0 ) : QWidget( parent ),
    advancedDlg_( new EventPositionTimeWidget( &eventPlayer_ ), 0, Qt::Tool )
    {
        play_ = new QPushButton( "Play" );
        connect( play_, SIGNAL( released() ), this, SLOT( Play() ) );

        stop_ = new QPushButton( "Stop" );
        connect( stop_, SIGNAL( released() ), this, SLOT( Stop() ) );

        manageCursor_ = new QCheckBox( "Move Cursor" );
        connect( manageCursor_, SIGNAL( stateChanged( int ) ), this, SLOT( ManageCursorSlot( int ) ) );

        triggerActions_ = new QCheckBox( "Trigger Actions" );
        if( eventPlayer_.TriggerActions() ) triggerActions_->setCheckState( Qt::Checked );
        connect( triggerActions_, SIGNAL( stateChanged( int ) ), this, SLOT( TriggerActionsSlot( int ) ) );

        filePath_ = new QLineEdit;
        select_ = new QPushButton( "Select Input File..." );
        connect( select_, SIGNAL( released() ), this, SLOT( SelectFile() ) );

        QPushButton* advanced = new QPushButton( "Advanced..." );
        connect( advanced, SIGNAL( released() ), this, SLOT( Advanced() ) );

        QGridLayout* gl = new QGridLayout;

        gl->addWidget( play_, 0, 0 );
        gl->addWidget( stop_, 0, 1 );
        gl->addWidget( manageCursor_, 0, 2 );
        gl->addWidget( triggerActions_, 0, 3 );
        gl->addWidget( advanced, 0, 4 );

        gl->addWidget( select_, 1, 0 );
        gl->addWidget( filePath_, 1, 1, 1, 4 );

        QGroupBox* gb = new QGroupBox( "Playback Events" );
        gb->setLayout( gl );

        QVBoxLayout* vl = new QVBoxLayout;
        vl->addWidget( gb );

        setLayout( vl );

        setObjectName( "Event Player" );

        advancedDlg_.setWindowTitle( "GUI Events Playback - Advanced Settings" );
    }

    /// QSettings key used to read/store input file path.
    void SetSettingsKey( const QString& key ) { settingsKey_ = key; }

    /// @return QSettings key used to read/store input file path.
    const QString& GetSettingsKey() const { return settingsKey_; }

    /// Set file path.
    void SetFilePath( const QString& fp ) { filePath_->setText( fp ); }

    /// Set time scaling.
    void SetTimeScaling( double s )
    {
        EventPositionTimeWidget* w =
            qobject_cast< EventPositionTimeWidget* >( advancedDlg_.GetWidget() );
        assert( w );
        w->SetTimeScaling( s );
    }
    
    /// Set min delay between subsequent events.
    void SetMinDelay( int md )
    {
    	eventPlayer_.SetMinDelay( md );
    }

private:

    /// Updates application settings with current dir path.
    void UpdateSettings() const;

private:
    /// Play button.
    QPushButton* play_;
    /// Stop button.
    QPushButton* stop_;
    /// Select file button.
    QPushButton* select_;
    /// File path edit box.
    QLineEdit* filePath_;
    /// Event player instance.
    EventPlayer eventPlayer_;
    /// QSettings key: if value not empty the file path is read/stored from/into QSettings
    /// using this data member as the key.
    QString settingsKey_;
    /// Check/uncheck to enable/disable mouse cursor handling.
    QCheckBox* manageCursor_;
    /// Enable/Disable action trigger.
    QCheckBox* triggerActions_;
    /// Simple wrapper for position/timing widget.
    DialogWidget advancedDlg_;

};


#endif /*EVENTPLAYERWIDGET_H_*/
