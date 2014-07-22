#ifndef SYSTEM_H_
#define SYSTEM_H_
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


#include <string>

/// Returns value of environment variable.
bool SetEnvironmentVariable( const std::string& name, const std::string& value );

/// Returns value of environment variable.
std::string GetEnvironmentVariableValue( const std::string& variableName );

/// Deletes file: returns true if operation successful, false otherwise.
bool DeleteFile( const std::string& filePath );

/// Returns true if file can be accessed in read mode.
bool FileIsReadable( const std::string& fileName );

/// Function returning a unique file path.
/// @warning this function simply constructs a unique name at a specific point
/// in time; between the time the file name is created and the file is open
/// another process could have already created a file with the same name;
/// prefix is currently not used on APPLE/UNIX.
/// If file name creation fails an empty string is returned.
std::string GetTemporaryFileName( const std::string& prefix = std::string() );

/// Function that starts a synchronous process and returns the value returned
/// by the started process.
int StartSyncProcess( const std::string& commandLine );

/// Returns file size.
unsigned long GetFileSize( const char* fname );

/// Returns content of text file into string.
std::string ReadTextFile( const char* fname );


#endif /*SYSTEM_H_*/
