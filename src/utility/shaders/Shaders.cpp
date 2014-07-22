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

#include <GL/glew.h>
#include <GL/gl.h>
#include <cstring>
#include <string>
#include "../System.h"
#include "GLSLShaderException.h"

#include <iostream>

using namespace std;


//------------------------------------------------------------------------------
/// Returns OpenGL version as ints and string.
string GetOpenGLVersion( int& maj, int& min )
{
	
    maj = 0;
    min = 0;
    const char* versionString = reinterpret_cast< const char* >( glGetString( GL_VERSION ) );
    if( versionString == 0 || sscanf( versionString, "%d.%d", &maj, &min ) != 2 )
    {
         return string();
    }
    return versionString;
}

//------------------------------------------------------------------------------
/// Returns GLSL version as ints and string.
string GetGLSLVersion( int& maj, int& min )
{
    maj = 0;
    min = 0;
    int glMaj = 0;
    int glMin = 0;
    GetOpenGLVersion( glMaj, glMin );
    if( glMaj == 0 ) return string();

    if( glMaj == 1 )
    {
        const char* extensionString = reinterpret_cast< const char* >( glGetString( GL_EXTENSIONS ) );
        cout << extensionString << endl;
        if( extensionString != 0 &&
            strstr( extensionString, "GL_ARB_shading_language_100" ) != 0 )
        {
            maj = 1;
            min = 0;
			return "GL_ARB_shading_language_100";
        }
    }
    else if( glMaj >= 2 )
    {
        const char* versionString = reinterpret_cast< const char* >( glGetString( GL_SHADING_LANGUAGE_VERSION ) );
        if( versionString == 0 || sscanf( versionString, "%d.%d", &maj, &min ) != 2 )
        {
            return string();
        }
        return versionString;
    }
	return "";
}

//------------------------------------------------------------------------------
/// Returns true if GLSL supported, false otherwise.
bool GLSLShadersSupported()
{
	int maj = 0;
    int min = 0;
    GetOpenGLVersion( maj, min );
    return maj >= 2;
}


//------------------------------------------------------------------------------
/// Validates shader; throws GLSLShaderException derived from std::exception
void ValidateGLSLShader( GLuint s, const std::string& msg )
{
    GLint p = 0;
    glGetShaderiv( s, GL_COMPILE_STATUS, &p );
    if( p == GL_FALSE )
    {
        glGetShaderiv( s, GL_INFO_LOG_LENGTH, &p );
        if( p > 0 )
        {
            char* buffer = new char[ p ];
            GLsizei length;
            glGetShaderInfoLog( s, p, &length, buffer );
            string m( buffer );
            delete [] buffer;
            throw GLSLShaderException( msg + ": " + m );
        }
        else throw GLSLShaderException( msg );
    }
}

//-----------------------------------------------------------------------------
/// Validates shader program; throws GLSLShaderException derived from std::exception
void ValidateGLSLProgram( GLuint p, const std::string& msg )
{
    typedef GLSLShaderException Ex;
    if( p == 0 ) throw Ex( msg + ": " + string( "Invalid program id (0)" ) );
    GLint param = 0;
    glGetProgramiv( p, GL_DELETE_STATUS, &param );
    if( param == GL_TRUE ) throw Ex( msg + ": " + "program has been deleted" );
    glGetProgramiv( p, GL_LINK_STATUS, &param );
    if( param != GL_TRUE )
    {
        GLint l = 0;
        glGetProgramiv( p, GL_INFO_LOG_LENGTH, &l );
        if( l > 0 )
        {
            char* buffer = new char[ p ];
            GLsizei length;
            glGetProgramInfoLog( p, l, &length, buffer );
            string m( buffer );
            delete [] buffer;
            throw Ex( msg + ": " + string( "program has not been linked. " + m ) );
        } else throw Ex( msg + ": " + string( "program has not been linked" ) );
    }
}

//-----------------------------------------------------------------------------
/// Validates program execution. throws GLSLShaderException derived from std::exception
void ValidateGLSLProgramExecution( GLuint p, const std::string& msg )
{
    ValidateGLSLProgram( p, msg );
    glValidateProgram( p );
    GLint param = 0;
    glGetProgramiv( p, GL_VALIDATE_STATUS, &param );
    if( param == GL_FALSE ) throw GLSLShaderException( msg + ": " + string( "invalid program" ) );
}

//-----------------------------------------------------------------------------
/// Deletes shader.
void DeleteGLSLShader( GLuint s )
{
    if( s == 0 ) return;
    GLint p = 0;
    glGetShaderiv( s, GL_DELETE_STATUS, &p );
    if( p == GL_TRUE ) return;
    glDeleteShader( s );
}

//-----------------------------------------------------------------------------
/// Detaches shader.
void DetachGLSLShader( GLuint program, GLuint shader )
{
    if( shader == 0 || program == 0 ) return;
    glDetachShader( program, shader );
}


//------------------------------------------------------------------------------
/// Creates vertex shader from source code.
/// @throws GLSLShaderException.
GLuint CreateGLSLVertexShader( const char* text )
{
    if( text == 0 ) throw GLSLShaderException( "NULL source code for vertex shader" );
    GLuint v = glCreateShader( GL_VERTEX_SHADER );
    if( v == 0 ) throw GLSLShaderException( "Cannot create vertex shader" );
    glShaderSource( v, 1, &text, 0 );
    glCompileShader( v );
    ValidateGLSLShader( v, "Vertex shader error" );
    return v;
}

//------------------------------------------------------------------------------
/// Creates fragment shader from source code.
/// @throws GLSLShaderException.
GLuint CreateGLSLFragmentShader( const char* text )
{
    if( text == 0 ) throw GLSLShaderException( "NULL source code for fragment shader" );
    GLuint f = glCreateShader( GL_FRAGMENT_SHADER );
    if( f == 0 ) throw GLSLShaderException( "Cannot create fragmentx shader" );
    glShaderSource( f, 1, &text, 0 );
    glCompileShader( f );
    ValidateGLSLShader( f, "Fragment shader error" );
    return f;
}

//------------------------------------------------------------------------------
/// Creates vertex shader from file.
/// @throws GLSLShaderException.
GLuint CreateGLSLVertexShaderFromFile( const char* fileName )
{
    if( fileName == 0 ) throw GLSLShaderException( "NULL vertex shader file name" );

    string vs( ReadTextFile( fileName ) );
    if( vs.size() == 0 )
    {
        throw GLSLShaderException( string( "Error reading vertex shader file " ) + fileName );
    }
    GLuint v = CreateGLSLVertexShader( vs.c_str() );
    return v;
}

//------------------------------------------------------------------------------
/// Creates fragment shader from file.
/// @throws GLSLShaderException.
GLuint CreateGLSLFragmentShaderFromFile( const char* fileName )

{
    if( fileName == 0 ) throw GLSLShaderException( "NULL fragment shader file name" );

    string fs( ReadTextFile( fileName ) );
    if( fs.size() == 0 )
    {
        throw GLSLShaderException( string( "Error reading fragment shader file " ) + fileName );
    }
    GLuint f = CreateGLSLFragmentShader( fs.c_str() );
    return f;
}


//------------------------------------------------------------------------------
/// Re-link and validate program.
/// @throws GLSLShaderException.
void RebuildGLSLProgram( GLuint program )
{
    glLinkProgram( program );
    ValidateGLSLProgramExecution( program, "Error validating program"  );
}

//------------------------------------------------------------------------------
/// Replace shader.
/// @throws GLSLShaderException.
void ReplaceGLSLShader( GLuint program, GLuint oldShader, GLuint newShader, bool rebuildProgram )
{
    DetachGLSLShader( program, oldShader );
    DeleteGLSLShader( oldShader );
    glAttachShader( program, newShader );
    if( rebuildProgram ) RebuildGLSLProgram( program );
}

//------------------------------------------------------------------------------
/// Creates program from vertex and fragment shaders read from files. Returns
/// the value of the created shader handlers and the handler of the created program.
/// @throws GLSLShaderException
GLuint CreateGLSLProgramFromFiles( const char* vert,
                                   const char* frag,
                                   GLuint& v,
                                   GLuint& f )
{
    v = 0;
    f = 0;

    if( vert == 0 && frag == 0 )
    {
        throw GLSLShaderException( "Both vertex and fragment shader file names are empty" );
    }

    if( vert ) v = CreateGLSLVertexShaderFromFile( vert );
    if( frag ) f = CreateGLSLFragmentShaderFromFile( frag );

    GLuint p = glCreateProgram();
    if( p == 0 )
    {
        DeleteGLSLShader( v );
        DeleteGLSLShader( f );
        throw GLSLShaderException( "Cannot create shader program" );
    }

    if( v ) glAttachShader( p, v );
    if( f ) glAttachShader( p, f );

    glLinkProgram( p );
    ValidateGLSLProgram( p, "Errror linking program" );
    return p;
}

//------------------------------------------------------------------------------
/// Creates program from vertex and fragment shaders read from files. Returns
/// the value of the created shader handlers and the handler of the created program.
/// @throws GLSLShaderException
GLuint CreateGLSLProgramFromFiles( const char* vert, const char* frag )
{

    GLuint v = 0;
    GLuint f = 0;

    return CreateGLSLProgramFromFiles( vert, frag, v, f );
}

//------------------------------------------------------------------------------
/// Removes shader program and shaders.
void DeleteGLSLProgramAndShaders( GLuint program,
                                  GLuint vertexShader,
                                  GLuint fragmentShader )
{
    DetachGLSLShader( program, vertexShader );
    DeleteGLSLShader( vertexShader );
    DetachGLSLShader( program, fragmentShader );
    DeleteGLSLShader( fragmentShader );
    glDeleteProgram( program );
}
