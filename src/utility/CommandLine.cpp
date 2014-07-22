//
// Copyright (c) 2006, 2007, 2008, 2009 - Ugo Varetto and 
// Swiss National Supercomputing Centre (CSCS)
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

#include "CommandLine.h"

//------------------------------------------------------------------------------
std::map< std::string, std::list< std::string > >
ParseCommandLine( int argc,
                  char** argv,
                  const std::string& paramPrefix )
{
    assert( argc > 0 );
    assert( argv );
    assert( paramPrefix.size() );
    char** b = argv;
    ++b; // skip program name
    char** e = argv + argc; // point to one past end of char* array
    return ParseArguments( b, e, paramPrefix ); // call actual parse function
}
