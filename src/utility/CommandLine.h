#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_
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
#include <list>
#include <map>
#include <cassert>
#include <algorithm>
#include <stdexcept>

//------------------------------------------------------------------------------
/// Function returning a map whose keys are command line switch parameters
/// and whose values are lists or parameter values.
/// @warning The argc and argv parameters passed to this function must be
/// the same parameters received on the command line i.e. the argv[ 0 ] must
/// point to the executable name.
/// @param argc number of argument on the command line including the the executable path
/// @param argv arguments on the command line including the executable path
/// @param paramPrefix string representing the parameter's name prefix e.g. "--"
/// Usage:
/// <code>
///int main( int argc, char** argv )
///{
///    typedef std::map< std::string, std::list< std::string > > ParamMap;
///    ParamMap params = ParseCommandLine( argc, argv );
///    ParamMap::iterator i = params.begin();
///    ParamMap::iterator e = params.end();
///    for( ; i != e; ++i )
///    {
///        std::cout << i->first << ": ";
///        std::copy( i->second.begin(), i->second.end(),
///                   std::ostream_iterator< std::string >( std::cout, " " ) );
///        std::cout << std::endl;
///    }
///    return 0;
///}
/// </code>
///
std::map< std::string, std::list< std::string > >
ParseCommandLine( int argc,
                  char** argv,
                  const std::string& paramPrefix = "-" );

//------------------------------------------------------------------------------
/// Iterates over a sequence of strings and returns a map whose keys are parameter names
/// and values are lists of parameter values.
/// @param begin forward iterator identifying the beginning of the sequence
/// @param end forward iterator identifying the element one past the end of sequence
/// @paramPrefix parameter prefix
template < class ForwardIteratorT >
std::map< std::string, std::list< std::string > >
ParseArguments( ForwardIteratorT begin, ForwardIteratorT end,
                const std::string& paramPrefix = "-" )
{
    std::list< std::string > valueList;
    std::map< std::string, std::list< std::string > > params;
    std::string lastParam;
    while( begin != end )
    {
        // if *b is parameter name add all previously read parameters into map
        // and reset list; if not add value to list
        const std::string s( *begin );
        if( s.find( paramPrefix ) == 0 )
        {
            const std::string paramName( *begin + paramPrefix.size() );
            if( lastParam.size() == 0 )
            {
                lastParam = paramName;
            }
            else
            {
                params[ lastParam ] = valueList;
                valueList.clear();
                lastParam = paramName;
            }
        }
        else
        {
            valueList.push_back( *begin );
        }
        ++begin;
    }
    params[ lastParam ] = valueList;
    return params;
}





/// Command name => command parameters map.
typedef std::map< std::string, std::list< std::string > > Commands;

/// Invokes a functor given a command name. Begin and end of command parameter
/// sequence are passed to invoked functor.
template < class MapT >
class InvokeFunctor
{
    /// Reference to name => functor map.
    const MapT* table_;
public:
    /// Default constructor.
    InvokeFunctor() : table_( 0 ) {}
    /// Constructor accepting a pointer to a map type.
    InvokeFunctor( const MapT* table ) : table_( table ) {}
    void operator()( const Commands::value_type& p ) const
    {
        assert( table_ );
        if( table_->find( p.first ) == table_->end() ) return;
        if( !( *( table_->find( p.first ) )->second )( p.second.begin(), p.second.end() ) )
        {
            throw std::runtime_error( "Commmand \"" + p.first + "\" ERROR" );
        }
    }
};

/// Iterates through command => parameter map and invokes functors
/// stored in command => functor map.
template < class TableT >
void ExecuteCommands( const Commands& cmds, const TableT& table )
{
    std::for_each( cmds.begin(), cmds.end(), InvokeFunctor< TableT >( &table ) );
}

#endif /*COMMANDLINE_H_*/
