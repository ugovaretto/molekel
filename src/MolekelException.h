#ifndef MOLEKEL_EXCEPTION_H_
#define MOLEKEL_EXCEPTION_H_
//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//
// $Author$
// $Date$
// $Revision$
//

#include <exception>
#include <string>

//template < class StandardExceptionT = std::exception >
//class MolekelStdException : public StandardExceptionT
//{
//};

//------------------------------------------------------------------------------
/// Exception class used by every class/function in Molekel application.
class MolekelException : public std::exception
{
    /// Error message.
    const std::string msg_;
public:
    /// Constructor; used to set error message.
    MolekelException( const std::string& msg ) : msg_( msg ) {}
    /// Overridden method.
    const char* what() const throw() { return msg_.c_str(); }
    /// Required nothrow overridden destructor.
    ~MolekelException() throw() {}
};

#endif /*MOLEKEL_EXCEPTION_H_*/
