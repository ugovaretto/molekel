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

#include <algorithm>

#include "EventPlayer.h"

#include <QMouseEvent>
#include <QMenuBar>

#include <iostream>

//------------------------------------------------------------------------------
void EventPlayer::PlayAction()
{
    scheduledAction_->trigger();
}

//------------------------------------------------------------------------------
void EventPlayer::DispatchEvent( QObject* target )
{
    // if mouse event convert relative cursor coordinates to current widget coordinates
    // ( current x or y coord ) =
    //  ( ( original coord ) * ( current widget width or height ) ) / ( original widget width or length )
	if( eventIterator_->event->type() == QEvent::MouseMove ||
		eventIterator_->event->type() == QEvent::MouseButtonPress ||
		eventIterator_->event->type() == QEvent::MouseButtonRelease )
	{
        // mouse event
        QMouseEvent* me = static_cast< QMouseEvent* >( eventIterator_->event );
        // if target is a QWidget and original sizes are greater than zero replace
        // QMouseEvent with new QMouseEvent
        if( qobject_cast< QWidget* >( target ) &&
            eventIterator_->width > 0 &&
            eventIterator_->height > 0 )
        {
            QWidget* w = qobject_cast< QWidget* >( target );
            QPoint p( ( w->size().width() * me->x() ) / eventIterator_->width,
			          ( w->size().height() * me->y() ) / eventIterator_->height );
            if( !qobject_cast< QMenuBar* >( target ) )
            {
                p.setX( int( p.x() * xscale_ ) + xoff_ );
                p.setY( int( p.y() * yscale_ ) + yoff_ );
            }
            else
            {
                p.setX( int( me->x() * xscale_ ) + xoff_ );
                p.setY( int( me->y() * yscale_ ) + yoff_ );
            }
            QMouseEvent* ev = new QMouseEvent( eventIterator_->event->type(), p,
                                               me->button(), me->buttons(), me->modifiers() );
	        delete eventIterator_->event;
	        eventIterator_->event = ev;
            me = ev;
        }

        // if cursor is managed by player
        if( eventIterator_->event->type() == QEvent::MouseMove && manageCursor_ )
        {
            QWidget* widget = qobject_cast< QWidget* >( target );
            if( widget )
            {
                widget->blockSignals( true );
                QCursor::setPos( widget->mapToGlobal( me->pos() ) );
                widget->blockSignals( false );
            }
        }
	}

    // trigger action
   	if( eventIterator_->event->type() == ActionTriggered && triggerActions_ )
	{
        // check that target is a QAction
		if( qobject_cast< QAction* >( target ) )
		{
            // delete event
            delete eventIterator_->event;
            // set action to trigger
			scheduledAction_ = qobject_cast< QAction* >( target );
			// schedule single shot timer to fire after 10 ms
            QTimer::singleShot( 10, this, SLOT( PlayAction( ) ) );
		}
	}
    // post event
	else
	{
        if( eventIterator_->event->type() != ActionTriggered )
		{
            QCoreApplication::postEvent( target, eventIterator_->event );
        }
	}
    eventIterator_->event = 0; // remove event reference, regular QEvents will be deleted by the application,
                               // action triggered events are deleted in this method.

    // when this method returns either an event has been placed in the event queue or
    // an action trigger invocation has been scheduled
}


//------------------------------------------------------------------------------
void EventPlayer::PlayNextEvent()
{
	if( eventIterator_ == eventEnd_ || paused_ ) return;

    // get target object
    QObject* w  = GetObjectByName( eventIterator_->target );

    // if object not found retry
    if( !w  && retry_ != 0 )
    {
        if( retry_ > 0 ) --retry_;
        QTimer::singleShot( waitTime_, this, SLOT( PlayNextEvent() ) );
        return;
    }

    retry_ = retries_;

    // dispatch event to target
	if( eventIterator_->event != 0 &&  w && eventIterator_->t > 0 ) // use threshold
	{
      	DispatchEvent( w );
		if( emitPostedSignal_ ) emit EventPosted( *eventIterator_ );
	}

    // move to next event
	++eventIterator_;

    // if done emit termination signal and return
    if( eventIterator_ == eventEnd_ )
	{
		emit Finished();
		return;
	}

	// delayed invocation of current method
    QTimer::singleShot( int( std::max( eventIterator_->t, minDelay_ ) * timeScaling_ ) + delay_, this, SLOT( PlayNextEvent() ) );
}

//------------------------------------------------------------------------------
void EventPlayer::SortActionTriggerEvents()
{
	// remove action trigger events with zero time and move action trigger events
    // back just before first event sent to last widget:
    // i.e. FROM:
    // 10 "MainWindow" 1024 768 5 ...
    // 10 "MainWindow" 1024 768 5 ...
    // 10 "MainWindow/Open File Dialog" ...
    // <Open File Dialog events>
    // 10000 "MainWindow/QAction_0" -1 -1 65535  <--
    // ...
    // TO:
    // 10 "MainWindow" 1024 768 5 ...
    // 10 "MainWindow" 1024 768 5 ...
    // 10000 "MainWindow/QAction_0" -1 -1 65535 <--
    // 10 "MainWindow/Open File Dialog" ...
    // <Open File Dialog events>
    // ...
    // This is required becaue the triggered() signal causing a modal dialog to
    // open is sent to the LogEventFilter instance after the modal dialog is closed.
    // In case the application quits while playing events with trigger actions enabled
	// try increasing the min delay to e.g. 100 ms.
	for( int i = 0; i != events_.size(); ++i )
	{
    	// set min delay for events
    	events_[ i ].t = std::max( events_[ i ].t, minDelay_ );
		if( i != 0 && events_[ i ].event->type() == ActionTriggered )
		{
		
			
			const QString wname = events_[ i - 1 ].target;
			if( !wname.contains( "/" )  ) continue;
			const int d = wname.indexOf( "/" );
			const int end = wname.indexOf( "/", d + 1 );
			const QString name = end < 0 ? wname : wname.left( end );
			
			//search backward for last occurrence of this target in event list
			for( int w = i - 1; w >= 0; --w )
			{
				if( events_[ w ].target == name )
				{
					//if previous element is a child of 'name' widget continue searching
					if( w != 0 && events_[ w - 1 ].target.contains( name ) ) continue;
					events_.move( i, w );
					break;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
void EventPlayer::DestroyEvents()
{
    while( eventIterator_ != eventEnd_ )
    {
        delete eventIterator_->event;
        ++eventIterator_;
    }
    events_.clear();
    ResetIterators();
}

