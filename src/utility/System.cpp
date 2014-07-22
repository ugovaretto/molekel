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


#ifdef WIN32
  #include <windows.h>
  #include <string>
  #include <process.h>
  #include <sstream>
  #include <vector>
#endif

#include <cstdio>
#include <cstdlib>
#include <fstream>

#include "System.h"


using namespace std;

//-----------------------------------------------------------------------------
/// Returns value of environment variable.
bool SetEnvironmentVariable( const string& name, const string& value )
{
    const string e( name + '=' + value );
    /// @warning on platforms other than windows putenv requires a
    /// non-const char*
#ifdef WIN32
#ifdef _MSC_VER
    const int r = _putenv( e.c_str() );
#else
	const int r = putenv( e.c_str() );
#endif
#else
    const int r = putenv( const_cast< char* >( e.c_str() ) );
#endif
    if( r == -1 ) return false;
    return true;
}

//-----------------------------------------------------------------------------
/// Returns value of environment variable.
string GetEnvironmentVariableValue( const string& variableName )
{
    string v( getenv( variableName.c_str() ) );
    return v;
}

//-----------------------------------------------------------------------------
/// Returns true if file can be accessed in read mode.
bool FileIsReadable( const string& fileName )
{
    try
    {
        ifstream in( fileName.c_str() );
        if( !in ) return false;
        return true;
    }
    catch( const exception& )
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
/// Deletes file: returns true if operation successful, false otherwise.
bool DeleteFile( const string& filePath )
{
    if( remove( filePath.c_str() ) == -1 ) return false;
    return true;
}

//------------------------------------------------------------------------------
/// Function returning a unique file path.
/// @warning this function simply constructs a unique name at a specific point
/// in time; between the time the file name is created and the file is open
/// another process could have already created a file with the same name;
/// prefix is currently not used on APPLE/UNIX.
/// If file name creation fails an empty string is returned.
string GetTemporaryFileName( const string& prefix )
{
#ifdef WIN32
    const int BUFSIZE = MAX_PATH;
    DWORD dwRetVal = 0;
    WORD dwBufSize=BUFSIZE;
    char lpPathBuffer[ BUFSIZE ];
    char szTempName[ BUFSIZE ];
    UINT uRetVal = 0;
    // Get the temp path.
    dwRetVal = GetTempPath( dwBufSize,      // length of the buffer
                            lpPathBuffer ); // buffer for path
    if( dwRetVal > dwBufSize ) return std::string();

    // Create a temporary file.
    uRetVal = GetTempFileName( lpPathBuffer,   // directory for tmp files
                               prefix.c_str(), // temp file name prefix
                               0,              // create unique name
                               szTempName );   // buffer for name

    if( uRetVal == 0 ) return std::string();

    return string( szTempName );
#else
    return string( tmpnam( 0 ) );
#endif
}

//------------------------------------------------------------------------------
/// Function that starts a synchronous process and returns the value returned
/// by the started process, on Windows it takes care of hiding the console
/// window to be used to run non-GUI programs.
int StartSyncProcess( const std::string& commandLine )
{
#ifdef WIN32
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    ULONG rc;
    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_HIDE;
    if( !CreateProcess( NULL, const_cast< char* >( commandLine.c_str() ),
        NULL, NULL, FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &StartupInfo,
        &ProcessInfo ) )
    {
        return GetLastError();
    }

    WaitForSingleObject( ProcessInfo.hProcess, INFINITE );
    if( !GetExitCodeProcess(ProcessInfo.hProcess, &rc ) ) rc = 0;
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);
    return rc;
#else
    return system( commandLine.c_str() );
#endif
}

//------------------------------------------------------------------------------
/// Returns file size.
unsigned long GetFileSize( const char* fname )
{
    std::ifstream f( fname );
    if( !f ) return 0;
    const unsigned long begin = f.tellg();
    f.seekg( 0, ios::end );
    const unsigned long end = f.tellg();
    return  end - begin;
}

//------------------------------------------------------------------------------
/// Returns content of text file into string.
#include <iostream>
std::string ReadTextFile( const char* fname )
{
    std::ifstream in( fname );
    if( !in ) return "";
    std::string str,strTotal;
    std::getline( in, str);
    while( in ) {
        strTotal += '\n';
        strTotal += str;
        std::getline( in, str );
    }
    return strTotal;
}

