#ifndef EVENTREADER_H_
#define EVENTREADER_H_
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

#include <QList>
#include <QEvent>
#include <string>
#include <QMouseEvent>
#include <QKeyEvent>

#include <iostream>
#include <list>

#include "ObjectName.h"

/// Event info record.
struct EventInfo
{
    /// Time duration = next event execution time - this event execution time.
	unsigned t;
    /// Name of target object.
	QString target;
    /// Event.
	QEvent* event;
	/// Width of target widget or -1 if target not widget.
    int width;
    /// Height of target widget or -1 if target not widget.
	int height;
};


/// Event reader: generates a sequence of EventInfo instances from an input stream containing
/// event information.
template < class IStream >
class EventReader
{
	/// Reference to input stream.
    IStream* is_;

    /// Read string in "<string>" format.
	QString ReadString() const
	{
		char c;
		const char SD = '"';
		std::list< char > l;
		*is_ >> c >> std::noskipws >> c;
		while( *is_ && c != SD )
		{
			l.push_back( c );
			*is_ >> std::noskipws >> c;
		}
		std::string r( l.begin(), l.end() );
		*is_ >> std::skipws;
		return r.c_str();
	}

    /// Stores last read string, currently not used, needed when an event
    /// needs to know about the previous target object.
    mutable QString tmpString_;

public:

    /// Construct instance from input stream reference.
	EventReader( IStream* is ) : is_( is ) {}

    /// Read
	QList< EventInfo > Read()
	{
        // record flag status
		std::ios_base::fmtflags flags = is_->flags();
		*is_ >> std::skipws;
		// event info list to return
        QList< EventInfo > events;
		// read data until the end of stream
        while( *is_ )
		{
			EventInfo ei;
			ei.event = 0; // init event pointer to NULL
			int tp;
			*is_ >>  ei.t; // read time duration of this event
			ei.target = ReadString(); tmpString_ = ei.target; // read target object name
			*is_ >> ei.width >> ei.height; // read width and height of widget (set to -1 if not widget)
			*is_ >> tp;

			switch( tp )
			{
				case QEvent::MouseButtonPress: ei.event = ReadMouseButtonPress();
					break;
				case QEvent::MouseButtonRelease: ei.event = ReadMouseButtonRelease();
					break;
				case QEvent::MouseButtonDblClick: ei.event = ReadMouseButtonDblClick();
					break;
				case QEvent::MouseMove: ei.event = ReadMouseMove();
					break;
				case QEvent::KeyPress: ei.event = ReadKeyPress();
					break;
				case QEvent::KeyRelease: ei.event = ReadKeyRelease();
					break;
				case QEvent::Wheel: ei.event = ReadWheel();
					break;
				case QEvent::FocusIn: ei.event = ReadFocusIn();
					break;
				case QEvent::FocusOut: ei.event = ReadFocusOut();
					break;
				case QEvent::MenubarUpdated: ei.event = ReadMenuBarUpdated();
					break;
				case QEvent::HoverEnter: ei.event = ReadHoverEnter();
					break;
				case QEvent::HoverLeave: ei.event = ReadHoverLeave();
					break;
				case QEvent::HoverMove:  ei.event = ReadHoverMove();
					break;
				case QEvent::Show:  ei.event = new QShowEvent;
					break;
				case QEvent::Hide:  ei.event = new QHideEvent;
					break;
				case QEvent::ContextMenu: ei.event = ReadContextMenu();
					break;
				case QEvent::Move: ei.event = ReadMove();
					break;
				case QEvent::Resize: ei.event = ReadResize();
					break;
                case QEvent::Shortcut: ei.event = ReadShortcut();
                    break;
                //case QEvent::ActionChanged: ei.event = ReadActionChanged();
                //    break;
				default:
					ei.event = new QEvent( QEvent::Type( tp ) );
					break;
			};
			if( ei.event && !ei.target.isEmpty() ) events.push_back( ei );
		}
		is_->flags( flags );
		return events;
	}

    //@{ Event readers; one read method per event type.
	QEvent* ReadMouseButtonPress()
	{

		int x, y, gX, gY;
		int button;
		int buttons;
		int modifiers;
		*is_ >> x >> y >> gX >> gY >> button >> buttons >> modifiers;
		return new QMouseEvent( QEvent::MouseButtonPress, QPoint( x, y ),
								Qt::MouseButton( button ), Qt::MouseButtons( buttons ),
								Qt::KeyboardModifiers( modifiers ) );
	}
	QEvent* ReadMouseButtonRelease()
	{

		int x, y, gX, gY;
		int button;
		int buttons;
		int modifiers;
		*is_ >> x >> y >> gX >> gY >> button >> buttons >> modifiers;
		return new QMouseEvent( QEvent::MouseButtonRelease, QPoint( x, y ),
								Qt::MouseButton( button ), Qt::MouseButtons( buttons ),
								Qt::KeyboardModifiers( modifiers ) );
	}

	QEvent* ReadMouseButtonDblClick()
	{

		int x, y, gX, gY;
		int button;
		int buttons;
		int modifiers;
		*is_ >> x >> y >> gX >> gY >> button >> buttons >> modifiers;
		return new QMouseEvent( QEvent::MouseButtonDblClick, QPoint( x, y ),
								Qt::MouseButton( button ), Qt::MouseButtons( buttons ),
								Qt::KeyboardModifiers( modifiers ) );
	}

	QEvent* ReadMouseMove()
	{

		int x, y, gX, gY;
		int button;
		int buttons;
		int modifiers;
		*is_ >> x >> y >> gX >> gY >> button >> buttons >> modifiers;
		return new QMouseEvent( QEvent::MouseMove, QPoint( x, y ),
								Qt::MouseButton( button ), Qt::MouseButtons( buttons ),
								Qt::KeyboardModifiers( modifiers ) );
	}

	QEvent* ReadKeyPress()
	{
		//Type type, int key, Qt::KeyboardModifiers modifiers, const QString & text = QString(), bool autorep = false, ushort count = 1 )
		int count;
		bool autoRepeat;
		int key;
		int modifiers;
		int nativeModifiers;
		int nativeScanCode;
		int nativeVirtualKey;
		QString text;
		*is_ >> count >> autoRepeat >> key; text = ReadString(); *is_ >> modifiers >> nativeModifiers >> nativeScanCode >> nativeVirtualKey;

		return new QKeyEvent( QEvent::KeyPress, key, Qt::KeyboardModifiers( modifiers ), text );
	}

	QEvent* ReadKeyRelease()
	{
		//Type type, int key, Qt::KeyboardModifiers modifiers, const QString & text = QString(), bool autorep = false, ushort count = 1 )
		int count;
		bool autoRepeat;
		int key;
		int modifiers;
		int nativeModifiers;
		int nativeScanCode;
		int nativeVirtualKey;
		QString text;
		*is_ >> count >> autoRepeat >> key; text = ReadString(); *is_ >> modifiers >> nativeModifiers >> nativeScanCode >> nativeVirtualKey;

		return new QKeyEvent( QEvent::KeyRelease, key, Qt::KeyboardModifiers( modifiers ), text );
	}

	QEvent* ReadWheel()
	{
		int x, y, gX, gY;
		int delta;
		int buttons;
		int modifiers;
		int orientation;

		*is_ >>  buttons >> delta >> orientation >> x >> y >> gX >> gY >> modifiers;

		delta /= 2;

		return new QWheelEvent( QPoint( x, y ),
					delta,
					Qt::MouseButtons( buttons ),
					Qt::KeyboardModifiers( modifiers ),
					Qt::Orientation( orientation ) );
	}

	QEvent* ReadFocusIn()
	{
		int reason;
		*is_ >> reason;
		return new QFocusEvent( QEvent::FocusIn, Qt::FocusReason( reason ) );
	}

	QEvent* ReadFocusOut()
	{
		int reason;
		*is_ >> reason;
		return new QFocusEvent( QEvent::FocusOut, Qt::FocusReason( reason ) );
	}

	QEvent* ReadMenuBarUpdated()
	{
		return new QEvent( QEvent::MenubarUpdated );
	}

	QEvent* ReadHoverEnter()
	{
		int ox, oy, x, y;
		*is_ >> ox >> oy >> x >> y;
		return new QHoverEvent( QEvent::HoverEnter, QPoint( x, y ), QPoint( ox, oy ) );
	}

	QEvent* ReadHoverLeave()
	{
		int ox, oy, x, y;
		*is_ >> ox >> oy >> x >> y;
		return new QHoverEvent( QEvent::HoverLeave, QPoint( x, y ), QPoint( ox, oy ) );
	}

	QEvent* ReadHoverMove()
	{
		int ox, oy, x, y;
		*is_ >> ox >> oy >> x >> y;
		return new QHoverEvent( QEvent::HoverMove, QPoint( x, y ), QPoint( ox, oy ) );
	}

	QEvent* ReadContextMenu()
	{
		int x, y, gX, gY, reason;
		*is_ >> x >> y >> gX  >> gY >> reason;
		return new QContextMenuEvent( QContextMenuEvent::Reason( reason ), QPoint( x, y ) );
	}

	QEvent* ReadMove()
	{
		int x, y, ox, oy;
		*is_ >> x >> y >> ox >> oy;
	    return new QMoveEvent( QPoint( x, y ), QPoint( ox, oy ) );
	}

	QEvent* ReadResize()
	{
		int w, h, ow, oh;
		*is_ >> w >> h >> ow >> oh;
		return new QResizeEvent( QSize( w, h ), QSize( ow, oh ) );
	}

    QEvent* ReadShortcut()
    {
        QString s = ReadString();
        bool amb;
        int id;
        *is_ >> amb >> id;
        return new QShortcutEvent( s, amb );
    }

    QEvent* ReadActionChanged()
    {
        QAction* a = static_cast< QAction* >( GetObjectByName( tmpString_ ) );
        if( a ) return new QActionEvent( QEvent::ActionChanged, a, 0 );
        return 0;

    }
    //@}

};

#endif /*EVENTREADER_H_*/

