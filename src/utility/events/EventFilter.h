#ifndef EVENTFILTER_H_
#define EVENTFILTER_H_
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

#include <QObject>
#include <QEvent>
#include <QMutex>
#include <set>
#include <QWidget>
#include <QAction>
#include <QMouseEvent>

#include "ObjectName.h"

#include <iostream>

/// Event logger interface.
class IEventLogger
{
public:
    /// Log event.
    /// @param target event receiver
    /// @event event
    /// @name name identifying target object
	virtual void Log( const QObject* target, const QEvent* event, const QString& name ) = 0;
	/// Destructor.
    virtual ~IEventLogger() {}
};

/// Log event filter. Overrides the eventFilter() method to log the received events.
/// Instances of this class contain and own an IEventLogger implementations and will forward
/// filtered events to the IEventLogger::Log() method.
/// It is also possible to log triggered() signals as events by connecting triggered() signals
/// to LogEventFilter::ActionTriggered() method; in this case a custom 'ActionTrigger' event
/// is created and logged; the ID associated with the ActionTrigger event can be assigned in
/// the instance constructor, default value is QEvent::MaxUser.
/// It is not possible with the current Qt architecture to record/playback events
/// sent to system dialogs, non-native dialogs (e.g. QFileDialog) should be used to make
/// the record/playback system work properly.
/// Note that tools like KD executor do fail to record events sent to system dialog as well, here is
/// an excerpt from the KD Executor manual:
/// ...
///10.1.
/// KD Executor Fails to Record Print Dialogs?
/// On Windows and MacOS X, the print dialog is a system dialog. This has the consequence
/// that no Qt-level events go to this dialog. Since KD Executor is recording
/// events at the Qt level, it is simply impossible for KD Executor to record anything
/// for the print dialog.
///
/// On Mac OS when using a file dialog some times double click and selection events are not recorded;
/// to avoid this simply select a file and the double click or select a file twice and click on open.
class LogEventFilter : public QObject
{
	Q_OBJECT

	/// Event id identifying an action trigger event.
	const QEvent::Type ActionTrigger;

    /// Owned instance of event logger; destroyed in destructor.
	IEventLogger* logger_;
    /// Event filter; the event will be logged only if the event type is found in this
    /// list or if the filter is emtpy.
	std::set< QEvent::Type > filter_;
    /// True if event filter is active.
	bool record_;
    /// Reference to object for which events are not logged: an event is not logged if
    /// the ignored pointer points to the target or any of its ancestors.
	QObject* ignore_;
    ///Enable/disable logging of mouse move events when no mouse button pressed.
    bool recordMouseMove_;


public slots:

	/// Logs action trigger event. This slot has to be connected to action trigger signals. Typical
	/// usage is to connect the top level menu widget triggered() signals to to this method to make
	/// the record/playback framework work properly on Mac OS where no support for event filtering is
	/// offered for top level menus; note however that using this method in conjunction with menu
	/// and toolbars events will result in both user events and actions to be recorder which will
	/// ultimately result in duplicated actions being triggered during playback.
	void ActionTriggered( QAction* a )
	{
		QEvent e( ActionTrigger );
		logger_->Log( a, &e, GetObjectName( *a ) );
	}

protected:
    /// Overridden method: checks if event type is in filter and invokes logger method
    /// to log the event.
 	bool eventFilter( QObject *obj, QEvent *event )
 	{
        // check if recording and object not NULL
		if( record_ && obj != 0 )
		{
            // check if mouse move events have to be recorded
            if( !recordMouseMove_                   &&
                event->type() == QEvent::MouseMove &&
                !static_cast< QMouseEvent* >( event )->buttons() ) return QObject::eventFilter( obj, event );

            // log event if filter is empty or event type in filtered events list
            if( filter_.empty() || filter_.find( event->type() ) != filter_.end() )
		    {
                // if an object to ignore is set the events will be logged iff
                // the event target and all its ancestors are different from the object to ignore
				if( ignore_ != 0 )
				{
					if( obj == ignore_ ) return QObject::eventFilter( obj, event );
					QObject* parent = obj->parent();
					while( parent )
					{
						if( parent == ignore_ ) return QObject::eventFilter( obj, event );
						parent = parent->parent();
					}
					logger_->Log( obj, event, GetObjectName( *obj ) ); // log event
				}
				else logger_->Log( obj, event, GetObjectName( *obj ) ); // log event
 			}

		}
        // call parent class' implementation
		return QObject::eventFilter( obj, event );
 	}
public:
    /// Sets the event filter.
	void SetFilter( const std::set< QEvent::Type >& filter ) { filter_ = filter; }
	/// Returnd the current event filter.
    std::set< QEvent::Type > GetFilter() const { return filter_; }
	/// Constructor.
    /// @param logger pointer to IEventLogger implementation; the instance is owned by
    /// this class and destroyed in this class' destructor.
    /// @param filer event filter containing the list of event types that will be logged.
    /// @param paren passed to QObject constructor.
    LogEventFilter( IEventLogger* logger,
					const std::set< QEvent::Type >& filter = std::set< QEvent::Type >(),
					QEvent::Type actionTriggerId = QEvent::MaxUser,
					QObject* parent = 0 ) : QObject( parent ), ActionTrigger( actionTriggerId ),
					logger_( logger ), filter_( filter ),
					record_( false ), ignore_( 0 ), recordMouseMove_( true )
	{}
    LogEventFilter( const std::set< QEvent::Type >& filter = std::set< QEvent::Type >(),
    				QEvent::Type actionTriggerId = QEvent::MaxUser,
                    QObject* parent = 0 ) : QObject( parent ), ActionTrigger( actionTriggerId ),
                    logger_( 0 ), filter_( filter ),
                    record_( false ), ignore_( 0 ), recordMouseMove_( true )
    {}
    /// Destructor deletes the logger instance.
	~LogEventFilter() { delete logger_; }
    /// Set new logger; current logger instance is deleted.
 	void SetLogger( IEventLogger* el ) { delete logger_; logger_ = el; }
 	/// Start recording.
    void StartRecording() { record_ = true;  }
 	/// Stop recording.
    void StopRecording() { record_ = false; }
 	/// Returns true if recording in progress, false otherwise.
    bool Recording() const { return record_; }
    /// Set object to ignore.
    /// @param obj reference to object to ignore or NULL. Events sent to this object
    /// or any of its ancestors will not be logged.
	void SetObjectToIgnore( QObject* obj ) { ignore_ = obj; }
	/// Returns object for which events sent to it or any of its ancestors are not logged.
    QObject* ObjectToIgnore() { return ignore_; }
    /// Returns value of action trigger event id.
    QEvent::Type ActionTriggerId() const { return ActionTrigger; }
    /// Enables/disables logging of mouse move events when no mouse button pressed.
    void SetRecordMouseMove( bool on ) { recordMouseMove_ = on; }
    /// Returns true if mouse move events are logged even if no mouse button is pressed,
    /// false otherwise.
    bool GetRecordMouseMove() const { return recordMouseMove_; }
};



#endif /*EVENTFILTER_H_*/

