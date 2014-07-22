#ifndef EVENTLOGGER_H_
#define EVENTLOGGER_H_
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

#include "EventFilter.h"
#include <QTime>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>

/// Event logger class. Instances of this class will log events to a stream-like class.
/// The stream-like class doesn't have to be an actual ostream but must support the
/// '<<' operator and the 'flush()' method. Events are serialized using client specified
/// class instances for field (event data) and record( event) separators.
/// Event records are logged as: <event time> <target object name> <event type> <event specific data>
/// the logged time is not the absolute time but the time difference between the current and previous event.
/// Event specific data is serialized through a switch( event type ) block, to make this class more flexible
/// it could be useful to use client-specified writers stored into a map indexed by the event type.
template < class OStreamT,
		   class FieldSeparatorT = char,
		   class RecordSeparatorT = char >
class EventLogger : public IEventLogger
{
    /// Field separator used to separate data belonging to the same event.
	FieldSeparatorT fs_;
    /// Record separator used to separate events.
	RecordSeparatorT rs_;
    /// Utility data member used to comput the difference between the current and last event time.
	mutable QTime t_;
    /// Last time an event was logged.
	mutable unsigned lastLog_;

    /// Reference to output stream.
	OStreamT* os_;

    /// Serialize string as "<string>".
	void SerializeString( const QString& name )
	{
		const char SD = '"';
		*os_ << SD << name.toStdString() << SD;
	}

public:

    /// Copy constructor.
	EventLogger( const EventLogger& el ) :
		fs_( el.fs_ ), rs_( el.rs_ ),
		lastLog_( 0 ), os_( el.os_ ), lastLog_( 0 )
	{
		t_.start(); // initialize timer at object construction
	}

    /// Construct object from stream and separators.
    EventLogger( OStreamT& os, FieldSeparatorT fs, RecordSeparatorT rs ) :
		fs_( fs ), rs_( rs ), lastLog_( 0 ), os_( &os ) { t_.start(); }

    /// Construct object with separator and no stream.
    EventLogger( FieldSeparatorT fs, RecordSeparatorT rs ) :
		fs_( fs ), rs_( rs ), lastLog_( 0 ), os_( 0 ) { t_.start(); }

    /// Assignment operator.
    EventLogger& operator=( const EventLogger& el )
    {
        this->EventLogger::EventLogger( el );
        return *this;
    }

    /// Set output stream.
	void SetOutputStream( OStreamT& os ) { os_ = &os; }

    /// Log method, this is the method that serializes events to the stream using the
    /// specified field separator to separate a specific event's data and the specified
    /// record separator to separate events.
    void Log( const QObject* obj, const QEvent* event, const QString& name )
	{
		const unsigned t = t_.elapsed();
		const FieldSeparatorT FS = fs_;
		const RecordSeparatorT RS = rs_;

        // Time elapsed from last event.
		*os_ << t - lastLog_ << FS;
		// Target name.
        SerializeString( name );
		int xSize = -1;
		int ySize = -1;
        *os_ << FS;
		// If target is a widget serialize width and height, if not output -1 -1.
        if( qobject_cast< const QWidget* >( obj ) )
		{
			xSize = qobject_cast< const QWidget* >( obj )->width();
			ySize = qobject_cast< const QWidget* >( obj )->height();
		}
		*os_ << xSize << FS << ySize << FS << event->type() << FS;

        // QMouseEvent
		if( event->type() == QEvent::MouseButtonPress   ||
			event->type() == QEvent::MouseButtonRelease ||
			event->type() == QEvent::MouseMove          ||
			event->type() == QEvent::MouseButtonDblClick )
		{
			const QMouseEvent* me = static_cast< const QMouseEvent* >( event );
			*os_ << me->x() << FS << me->y() << FS << me->globalX() << FS << me->globalY()
				 << FS << me->button() << FS << me->buttons() << FS << me->modifiers();
		}
        // QKeyEvent
        else if( event->type() == QEvent::KeyPress ||
				 event->type() == QEvent::KeyRelease )
		{
			const QKeyEvent* ke = static_cast< const QKeyEvent* >( event );
			*os_ << ke->count() << FS << ke->isAutoRepeat() << FS << ke->key() << FS; SerializeString( ke->text() ); *os_ << FS << ke->modifiers()
				  << FS << ke->nativeModifiers() << FS << ke->nativeScanCode()
				  << FS << ke->nativeVirtualKey();

		}
        // QWheelEvent
		else if( event->type() == QEvent::Wheel )
		{
			const QWheelEvent* we = static_cast< const QWheelEvent* >( event );
			*os_ << we->buttons() << FS << we->delta() << FS << we->orientation()
				 << FS << we->x() << FS << we->y() << FS << we->globalX() << FS
				 << we->globalY() << FS << we->modifiers();
		}
        // QFocusEvent
		else if( event->type() == QEvent::FocusIn ||
				 event->type() == QEvent::FocusOut )
		{
			const QFocusEvent* fe = static_cast< const QFocusEvent* >( event );
			*os_ << fe->reason();
		}
        // QHoverEvent
		else if( event->type() == QEvent::HoverEnter ||
				 event->type() == QEvent::HoverLeave ||
				 event->type() == QEvent::HoverMove )
		{
			const QHoverEvent* oe = static_cast< const QHoverEvent* >( event );
			*os_ << oe->oldPos().x() << FS << oe->oldPos().x() << FS
				 << oe->pos().x()    << FS << oe->pos().y();
		}
        // QContextMenuEvent
		else if( event->type() == QEvent::ContextMenu )
		{
			const QContextMenuEvent* me = static_cast< const QContextMenuEvent* >( event );
			*os_ << me->x() << FS << me->y() << FS << me->globalX() << FS << me->globalY() << me->reason();
		}
        // QMoveEvent
		else if ( event->type() == QEvent::Move )
		{
			const QMoveEvent* me = static_cast< const QMoveEvent* >( event );
			*os_ << me->pos().x() << FS  << me->pos().y() << FS <<  me->oldPos().x() << FS <<  me->oldPos().y();
		}
        // QResizeEvent
		else if( event->type() == QEvent::Resize )
		{
			const QResizeEvent* re = static_cast< const QResizeEvent* >( event );
			*os_ << re->size().width() << FS << re->size().height() << FS << re->oldSize().width() << FS << re->oldSize().height();
		}
        // QShortCutEvent
        else if( event->type() == QEvent::Shortcut )
        {
            const QShortcutEvent* se = static_cast< const QShortcutEvent* >( event );
            SerializeString( se->key().toString() );
            *os_ << FS << se->isAmbiguous() << FS << se->shortcutId();
        }

        // rcord separator and flush
		*os_ << RS; os_->flush();

        // record time of current log
		lastLog_ = t;
	}

    /// Empty destructor.
	~EventLogger() {}
};


#endif /*EVENTLOGGER_H_*/

