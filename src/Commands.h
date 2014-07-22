#ifndef COMMANDS_H_
#define COMMANDS_H_
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

#include <cstdlib>
#include <iostream>
#include <string>
#include <strstream>

#include <QStringList>
#include <QRegExp>

#include "MainWindow.h"
#include "utility/CommandLine.h"

/// Base class for operations specified through commands
/// on the command line.
template < class TableT = Commands >
class AbstractOp
{
    /// Reference to MainWindow.
    MainWindow* pw_;

protected:
    typedef typename TableT::value_type::second_type::const_iterator Iterator;
    /// Execution method called from within operator().
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    /// Dereferenced values are strings.
    /// @return true if successful false otherwise.
    virtual bool Exec( Iterator b, Iterator e ) const = 0;

    /// Return main window.
    MainWindow* GetMainWindow() const { return pw_; }

    /// Print error message.
    void Error( const char* msg ) const { std::cerr << "Molekel Error: " << msg << std::endl; }

    /// Required virtual destructor since other classes will inherit from this.
    virtual ~AbstractOp() {}

public:
    /// Constructor.
    AbstractOp( MainWindow* pw ) : pw_( pw ) {}

    /// Forward calls to virtual method.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    bool operator()( Iterator b, Iterator e ) const
    {
        assert( pw_ );
        return Exec( b, e );
    }
};

/// Load a molecule from file type and path.
class LoadMoleculeOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;
public:
    /// Constructor.
    LoadMoleculeOp( MainWindow* pw ) : AbstractOp< Commands >( pw ) {}

    /// Overridden command execution. First parameter is type, second parameter is file path.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    /// Dereferenced values are strings.
    bool Exec( It b, It e ) const
    {
        if( b == e )
        {
            Error( "Missing file type & path" );
            return false;
        }
        const char* type = b->c_str();
        ++b;
        if( b == e )
        {
            Error( "Missing file path" );
        }
        const char* name = b->c_str();
        GetMainWindow()->LoadMolecule( name, type ); // might throw an exception
        return true;
    }
};

/// Resize window.
class ResizeOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;
public:
    ResizeOp( MainWindow* pw ) : AbstractOp< Commands >( pw ) {}
    /// Overridden command execution. First parameter is width, second parameter is height.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    /// Dereferenced values are strings.
    bool Exec( It b, It e ) const
    {
        if( b == e )
        {
            Error( "No width and height specified" );
            return false;
        }
        const char* width = b->c_str();
        ++b;
        if( b == e )
        {
            Error( "No height specified" );
            return false;
        }
        const char* height = b->c_str();
        MainWindow* w = GetMainWindow();
        assert( w );
        w->resize( QString( width ).toInt(),
                   QString( height ).toInt() );
        return true;
    }
};

/// Move window.
class PositionOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;
public:
    PositionOp( MainWindow* pw ) : AbstractOp< Commands >( pw ) {}
    /// Overridden command execution. First parameter is x, second parameter is y.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    /// Dereferenced values are strings.
    bool Exec( It b, It e ) const
    {
        if( b == e )
        {
            Error( "No x and y specified" );
            return false;
        }
        const char* x = b->c_str();
        ++b;
        if( b == e )
        {
            Error( "No y specified" );
            return false;
        }
        const char* y = b->c_str();
        MainWindow* w = GetMainWindow();
        assert( w );
        w->move( QString( x ).toInt(),
                 QString( y ).toInt() );
        return true;
    }
};


/// Help command. Prints help on standard output.
class HelpOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;
public:
    /// Default constructor.
    HelpOp() : AbstractOp< Commands >( 0 ) {}
    /// Overridden command execution. No parameter required.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    bool Exec( It, It ) const
    {
       std::cout << "Usage: molekel [-load <type> <file path > ] "
       			 << "[-settings [<key 1> <value 1> ... <key n> <value n>]] "
                 << "[-size <width> <height>] "
                 << "[-events <file> <start delay (s)> <min delay (ms)> <time scaling>] "
                 << "[-position <x> <y>] "
                 << "[-exit]"
                 << std::endl;
       return true;
    }
};


/// Exit command. Exits from application.
class ExitOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;
public:
    /// Default constructor.
    ExitOp() : AbstractOp< Commands >( 0 ) {}
    /// Overridden command execution. No parameter required.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    bool Exec( It, It ) const
    {
       std::exit( 0 );
       return true; // we never get here
    }
};


/// Settings command. Sets QSettings string values.
/// Settings are set through a list of key-value pairs.
/// If no key-value pairs are specified the current settings are printed
/// to standard output.
class SettingsOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;

    /// If true new entries are created if not present; if false
    /// an error is reported if settings entry not found
    bool create_;
    
    /// Returns true if string represents a float.
    bool IsFloat( const char* f ) const
    {
         return QString( f ).contains( QRegExp( "^[-+]?(([0-9]*\\.[0-9]+([eE][-+]?[0-9]+)?)|([0-9]+\\.))$" ) );
    }

    /// Returns true if string represent a signed integer.
    bool IsInt( const char* f ) const
    {
        return QString( f ).contains( QRegExp( "^[-+]?[0-9]+$" ) );
    }

public:
    /// Default constructor.
    SettingsOp( bool create) : AbstractOp< Commands >( 0 ), create_( create ) {}
    /// Overridden command execution. No parameter required.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    bool Exec( It b, It e ) const
    {
        QSettings settings;
        if( b == e )
        {
            QStringList keys = settings.allKeys();
            keys.sort();
            QStringList::const_iterator i = keys.begin();
            const QStringList::const_iterator e = keys.end();
            for( ; i != e; ++i )
            {
                const QString value = settings.value( *i ).toString();
                std::cout << i->toStdString() << " = " << value.toStdString() << std::endl;
            }
            return true;
        }

        while( b != e )
        {
            if( !settings.contains( b->c_str() ) && !create_ )
            {
                Error( ( *b + " not found in settings" ).c_str() );
                return false;
            }
            const char* key = b->c_str();
            ++b;
            if( b == e )
            {
                Error( ( "No value for key " + *b ).c_str() );
                return false;
            }
            const char* value = b->c_str();
            if( IsFloat( value ) )
            {
                settings.setValue( key, QString( value ).toDouble() );
            }
            else if( IsInt( value ) )
            {
                settings.setValue( key, QString( value ).toInt() );
            }
            else
            {
                settings.setValue( key, value );
            }
            ++b;
        }

       return true;
    }
};

/// Load and playback events.
class PlayEventsOp : public AbstractOp< Commands >
{
    typedef AbstractOp< Commands >::Iterator It;
public:
    /// Constructor.
    PlayEventsOp( MainWindow* pw ) : AbstractOp< Commands >( pw ) {}

    /// Overridden command execution.
    /// - First parameter is file path
    /// - second parameter is initial delay in seconds
    /// - third parameter is time scaling.
    /// @param b start iterator: points to the first parameter passed to command.
    /// @param e end iterator: points to one past the last parameter passed to command.
    /// Dereferenced values are strings.
    bool Exec( It b, It e ) const
    {

        if( b == e )
        {
            Error( "No file path, initial delay and time scaling" );
            return false;
        }
        const char* filePath = b->c_str();
        
        ++b;
        if( b == e )
        {
            Error( "No initial delay" );
            return false;
        }
        std::istringstream is( *b );
        unsigned initialDelay = 0;
        is >> initialDelay;
        
        ++b;
        if( b == e )
        {
            Error( "No min delay" );
            return false;
        }
        std::istringstream is2( *b );
        int minDelay = 0;
        is2 >> minDelay;
        
        ++b;
        if( b == e )
        {
            Error( "No time scaling" );
            return false;
        }
        double timeScaling = 0.;
        std::istringstream is3( *b );
        is3 >> timeScaling;
        
        // replay events; convert initial delay from seconds to milliseconds
        GetMainWindow()->PlayEvents( filePath, 1000 * initialDelay, minDelay, timeScaling );
        return true;
    }
};


/// Command name => operation map.
typedef std::map< std::string, const AbstractOp< Commands >* > Operations;

#endif /*COMMANDS_H_*/
