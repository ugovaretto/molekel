#ifndef GLSLSHADEREXCEPTION_H_
#define GLSLSHADEREXCEPTION_H_
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
#include <exception>
#include <string>

///Exception thrown by shader handling functions.
class GLSLShaderException : public std::exception
{
    std::string msg_;
public:
    GLSLShaderException( const std::string& msg ) : msg_( msg ) {}
    const char* what() const throw() { return msg_.c_str(); }
    ~GLSLShaderException() throw() {}
};


#endif /*GLSLSHADEREXCEPTION_H_*/
