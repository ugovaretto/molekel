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
#include "ObjectName.h"
#include <QObjectList>
#include <QWidgetList>
#include <QApplication>
#include <QObject>
#include <QString>
#include <QList>
#include <QtDebug>
#include <QStringList>
#include <QRegExp>
#include <QWidget>

#include <iostream>
namespace // Private interface
{

    //--------------------------------------------------------------------------
    /// Depth first traversal of hierarchy: at each step an object name is generated
    /// for the current object and compared to the passed name, if the two match a pointer
    /// to the current object is returned.
    /// @param obj search starting point.
    /// @param object name as returned by a call to GetObjectName()
    QObject* FindQObjectByName_( QObject* obj, const QString& name )
    {
        if( !obj ) return 0;
        if( name == GetObjectName( *obj ) && !name.isEmpty() )
        {
            return obj;
        }

        QObjectList children = obj->children();
        QObjectList::const_iterator i = children.begin();
        const QObjectList::const_iterator e = children.end();
        for( ; i != e; ++i )
        {
            QObject* o =  FindQObjectByName_( *i,  name );
            if( o ) return o;
        }
        return 0;
    }

    //--------------------------------------------------------------------------
    /// Returns a name for the passed object:
    /// - if the object has a name it returns the object name
    /// - if the object name is empty it returns the object class name + '_' + nubmer of objects
    ///   of the same class before current object
    QString ObjectName_( const QObject& obj )
    {

        if( !obj.objectName().isEmpty() ) return obj.objectName();
        // if object is a top level widget name = class name + '_' + number of object of the same class before current object
        // if not find his siblings through its parent and  set name =
        //                       class name + '_' + number of objects of the same class before current object
        QObjectList siblings;
        if( obj.parent() ) siblings = obj.parent()->children();
        else
        {
            QWidgetList widgets = QApplication::topLevelWidgets();
            QWidgetList::iterator i = widgets.begin();
            const QWidgetList::const_iterator e = widgets.end();
            for( ; i != e; ++i  )
            {
                siblings.push_back( *i );
            }
        }

        QObjectList::iterator i = siblings.begin();
        const QObjectList::const_iterator e = siblings.end();
        const QString objectType = obj.metaObject()->className();
        int cnt = 0;
        for( ; i != e; ++i )
        {
            if( *i == &obj ) break;
            if( ( *i )->metaObject()->className() == objectType ) ++cnt;
        }

        if( i == e ) std::cerr << "Object not found in siblings!!!" << std::endl;
        return objectType + QString( "_%1" ).arg( cnt );
    }

    /// Returns true if the name is NOT a generated object name.
    bool IsOwnName_( const QString& name )
    {
        return !name.isEmpty() && !name.contains( QRegExp( "_[\\d]+$" ) );
    }
}

// Public interface

//------------------------------------------------------------------------------
QString GetObjectName( QObject& obj )
{
    const char HS = '/';
    // name =
    //  if has parent parent name + '/' + object name
    //  else parent name + '/' + generated name
    QString name = ObjectName_( obj );
    if( obj.parent() )
    {
        const QString parentName = GetObjectName( *( obj.parent() ) );
        const QString child = name.isEmpty() ? "" : HS + name;
        if( !parentName.isEmpty() ) name = parentName + child;
    }
    return name;
}

//-------------------------------------------------------------------------------
QObject* GetObjectByName( const QString& name )
{
    const char HS = '/';
//    const QStringList splitName = name.split( HS, QString::SkipEmptyParts );
//    const QString objectName = *( --splitName.end() );
//    const QString root = *splitName.begin();
//    const bool hasOwnName = IsOwnName_( name );
    QWidgetList widgets = QApplication::topLevelWidgets();
    QWidgetList::iterator i = widgets.begin();
    const QWidgetList::const_iterator e = widgets.end();
    for( ; i != e; ++i  )
    {
        QObject* obj = FindQObjectByName_( *i, name );
        if( obj ) return obj;
    }
    return 0;
}
