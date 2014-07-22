#ifndef OBJECT_NAME_H_
#define OBJECT_NAME_H_
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

class QObject;
class QString;

/// Returns a unique identifier for the given object.
/// The returned name encodes hierarchy information using '/' to separate parent from child.
QString GetObjectName( QObject& Object );

/// Given a unique identifier returned by GetObjectName(), returns the corresponding object, or NULL.
QObject* GetObjectByName( const QString& Name );

#endif // #define OBJECT_NAME_H_
