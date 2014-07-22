#ifndef EVENTPLAYER_H_
#define EVENTPLAYER_H_
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

#include <QEvent>
#include <QList>
#include <QMutex>
#include <QWaitCondition>
#include <QApplication>
#include <QWidget>
#include <QDebug>
#include <QCoreApplication>
#include <set>
#include <QTimer>

#include <QMenuBar>

#include "ObjectName.h"
#include "EventReader.h"

/// Event player: sends events asynchronously to an application. In case of action trigger events QAction::triggered
/// is called instead of postEvent. The events are deleted after being consumed by the application and it's therefore
/// impossible to play the events more than once.
/// All events are posted to QCoreApplication::instance().
/// Play, Pause, Continue are supported.
/// Events are intended to be played only once after having been copied into a player's instance.
/// All events will be destroyed when the player is deleted.
/// On Mac OS do not use a combination of menu entries and toolbar buttons: either perform all the actions through
/// the main menu or through toolbar buttons. The "Action Trigger" check box has to be checked iff menus were used to record
/// events, unchecked otherwise.
class EventPlayer : public QObject
{

	Q_OBJECT;

	/// Event id associated with action triggered event, has to be set in constructor,
	/// and match the id set in the EventFilter instance used to serialize action triggered events.
	const QEvent::Type ActionTriggered;

    /// Event list.
	QList< EventInfo > events_;
	/// Event type filter; if filter not empty only events in the filter list will be sent.
    std::set< QEvent::Type > filter_;
	/// Event iterator pointing to next event to be sent.
    QList< EventInfo >::iterator eventIterator_;
    /// End of event list.
	QList< EventInfo >::const_iterator eventEnd_;
	/// Timer used to schedule the dispatch of the next event.
    QTimer timer_;
    /// If true the cursor is moved in case of QMouseEvent.
	bool manageCursor_;
    /// True if player is in pause mode.
	bool paused_;
	/// Constant offset added to the (scaled) time interval between next and current event.
    unsigned delay_;
    /// Factor by which the time interval between next and current event is multiplied.
    double timeScaling_;
    /// If true a signal is emitted after each event has been posted.
    bool emitPostedSignal_;
    /// Time to wait for missing object: in case an object is not found wait up to
    /// waitTime_ milliseconds and try again.
    unsigned waitTime_;
    /// Maximum number of times PlayNextEvent() will wait in case event target is NULL.
    /// If retries_ is negative the method will execute until the target is available.
    int retries_;
    /// Number of retries; decremented within PlayNextEvent at each retry if retries_ > 0.
    mutable int retry_;
	/// Action pointer pointing to the action to be triggered asynchronously through the
	/// PlayAction() method.
	QAction* scheduledAction_;
	/// If true actions are triggered.
	bool triggerActions_;

    /// X coordinate offset.
    int xoff_;
    /// Y coordinate offset.
    int yoff_;
    /// X coordinate scaling factor.
    double xscale_;
    /// Y coordinate scaling factor.
    double yscale_;

    /// Minimum delay in milliseconds allowed between subsequent events.
    unsigned minDelay_;

    /// Reset event iterators to begin and one past the end of the event sequence.
	void ResetIterators()
	{
		eventIterator_ = events_.begin();
		eventEnd_ = events_.end();
	}

	/// Dispatch event to target object. Takes care of dispatching the event pointed to by
	/// eventIterator_ or triggering and action if target is of tipe QAction and triggerActions_ is
	/// set to true. Note that this method will also replace mouse events with events having the
	/// correct relative cursor coordinates computed from:
	/// - original widget size
	/// - current widget size
	/// - original relative cursor coordinates
	void DispatchEvent( QObject* target );

	/// Sort events moving action triggering events at the right position and removing
	/// multiple consecutive action trigger events.
	void SortActionTriggerEvents();

    /// Destroy events; called only from destructor and SetEvents() method to ensure that all the events
    /// left in the event queue are properly deleted; note that events are deleted as they are
    /// consumed but in case events are set and never or partially played they have to be
    /// explicitly deleted.
    void DestroyEvents();

public:
    /// Construct player from event list and event filter.
	EventPlayer( const QList< EventInfo >& events,
				 const std::set< QEvent::Type >& filter = std::set< QEvent::Type >() )
				 : ActionTriggered( QEvent::MaxUser ), events_( events ), filter_( filter ), manageCursor_( false ), paused_( false ),
				   delay_( 0 ), timeScaling_( 1.0 ), emitPostedSignal_( false ), waitTime_( 0 ),
                   retries_( 0 ), retry_( retries_ ), scheduledAction_( 0 ), xoff_( 0 ), yoff_( 0 ), xscale_( 1. ), yscale_( 1. ),
                   minDelay_( 0 ),
#ifdef __APPLE_CC__
                   triggerActions_( true )
#else
				   triggerActions_( false )
#endif
	{ }

    /// Default constructor.
    EventPlayer() : manageCursor_( false ), paused_( false ), delay_( 0 ), timeScaling_( 1.0 ),
                    emitPostedSignal_( false ), waitTime_( 0 ), retries_( 0 ), retry_( retries_ ),
                    ActionTriggered( QEvent::MaxUser ), scheduledAction_( 0 ), xoff_( 0 ), yoff_( 0 ), xscale_( 1. ), yscale_( 1. ),
                    minDelay_( 0 ),
#ifdef __APPLE_CC__
                   triggerActions_( true )
#else
				   triggerActions_( false )
#endif
    {}

    /// Delete all events.
    ~EventPlayer() { DestroyEvents(); }

    /// Number of times player will try to retrieve event target. Set retries to a negative
    /// number to have the player wait indefinitely for the event target to become available.
    void SetRetries( int r ) { retries_ = r; retry_ = retries_; }
    /// Return retries.
    int GetRetries() const { return retries_; }

    /// Returns wait time between retries.
    unsigned GetWaitTime() const { return waitTime_; }
    /// Set wait time between two subsequent retries.
    void SetWaitTime( unsigned wt ) { waitTime_ = wt; }

    /// Returs a reference to the event list.
	const QList< EventInfo >&  GetEvents() const { return events_; }
    /// Sets event list.
	void SetEvents( const QList< EventInfo >& events )
    {
        DestroyEvents();
        events_ = events;
    }

    /// Make player move mouse cursor when posting mouse events.
	void ManageCursorOn() { manageCursor_ = true; }
	/// Disable cursor movement.
    void ManageCursorOff() { manageCursor_ = false; }
	/// Returns true if cursor is moved by player, false otherwise.
    bool CursorManaged() const { return manageCursor_; }

    /// Sets time interval scaling factor.
	void SetTimeScaling( double ts ) { timeScaling_ = ts; }
	/// Returns time interval scaling factor.
    double GetTimeScaling() const { return timeScaling_; };

    /// Sets time delay.
	void SetDelay( unsigned d ) { delay_ = d; }
	/// Returns time delay.
    unsigned GetDelay() const { return delay_; };

    /// Sets minimum delay allowed in milliseconds between subsequent events.
    void SetMinDelay( unsigned md ) { minDelay_ = md; }
    /// Returns minimum delay between subsequent events.
    unsigned GetMinDelay() const { return minDelay_; }

    /// Sets default (x,y) offset for events.
    void SetEventOffset( int xoff, int yoff ) { xoff_ = xoff; yoff_ = yoff; }
    /// Returns x event offset.
    int GetEventOffsetX() const { return xoff_; }
    /// Returns y event offset.
    int GetEventOffsetY() const { return yoff_; }

    /// Sets default scaling factor for x and y event coordinates.
    void SetEventScaling( double xs, double ys ) { xscale_ = xs; yscale_ = ys; }
    /// Returns x coordinate scaling factor.
    double GetEventScalingX() const { return xscale_; }
    /// Returns x coordinate scaling factor.
    double GetEventScalingY() const { return yscale_; }

    /// Enable signal emission after posting event.
	void EmitPostedSignalOn() { emitPostedSignal_ = true; }
	/// Disable signal emission after posting event.
    void EmitPostedSignalOff() { emitPostedSignal_ = false; }
	/// Returns true if signal emitted after posting event, false otherwise.
    bool PostedSignalEmitted() const { return emitPostedSignal_; }

	/// Trigger actions.
	void TriggerActionsOn() { triggerActions_ = true; }
	/// Do not trigger actions.
	void TriggerActionsOff() { triggerActions_ = false; }
	/// Return status of triggerActions_ flag.
	bool TriggerActions() const { return triggerActions_; }


    /// Utility method which dumps events to an output stream.
	void DumpEvents( std::ostream& os )
	{
		QList< EventInfo >::const_iterator i = events_.begin();
		for( ; i != events_.end(); ++i ) if( i->event ) os << i->target.toStdString() << ' ' << i->event->type() << std::endl;
	}

signals:
    /// Emitted when playing finished.
	void Finished();
    /// Emitted when player paused.
    void Paused();
    /// Emitted when playing restarted after having been paused.
    void Continued();
    /// Emitted after event posted.
	void EventPosted( EventInfo ei );

private slots:
    /// Post next event to application through QCoreApplication::postEvent() function.
    /// This method recursively calls itself using a timer to delay the call according to client specified parameters
    /// and data stored in the event information the event iterator points to.
    //  This method posts the event pointed to by the event iterator then increments the event iterator;
    /// if the incremented iterator is not equal to the end iterator a one time timer is initialized with:
    /// - timeout = dt * timeScaling_ + delay; where dt is the time interval stored in current the EventInfo instance.
    /// - slot = this->PlayNextEvent()
	void PlayNextEvent();

    /// In case a QAction::trigger() method has to be invoked this slot is connected to a single shot timer
    /// started within the DispatchEvent method. When this method is actually called it simply calls
    /// scheduledAction_->trigger(). @note the timer can be connected directly to the QAction::trigger() slot.
	void PlayAction();

public slots:
	/// Pause player.
    void Pause() { paused_ = true; emit Paused(); }
	/// Continue.
    void Continue() { if( !paused_ ) return; paused_ = false; emit Continued(); PlayNextEvent(); }
	/// Reset iterators and asynchronously post events to application through QCoreApplication::postEvent() function.
    void AsynchPlay()
	{
		if( triggerActions_ ) SortActionTriggerEvents();
		paused_ = false;
		ResetIterators();
		PlayNextEvent();
	}
};



#endif /*EVENTPLAYER_H_*/

