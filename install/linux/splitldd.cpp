// Program that extracts the full path of a dynamic library in ldd output
// format  and copies the library into a user defined directory.
// Input is read from standard input.
// Sample usage - copies all the libraries found in a specified path:
//   ldd dist/bin/Molekel | grep /usr/local | ./sp dist/lib


#include <iostream>
#include <string>
#include <cstdlib>

using namespace std;

int main( int argc, char** argv )
{
  if( argc < 2 )
  {
    cout << "usage: " << argv[ 0 ] << " output directory" << endl;
    return 0;
  }
  string buf;
  while( getline( cin, buf ) )
  {
     string::size_type begin = buf.find( "=> " );
     begin += 2;
     string::size_type end = buf.find( " (0" );
     string c( buf, begin, end - begin );
     cout << c << endl;
     string command = "cp " + c + string( " " ) + argv[ 1 ];
     system( command.c_str() );
  }

  return 0;
}
