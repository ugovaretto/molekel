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

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <exception>
#include <cassert>

#include <iostream>

//------------------------------------------------------------------------------
class ShaderParameterException : public std::exception
{
	std::string msg_;
public:
	ShaderParameterException( const std::string& msg ) : msg_( msg ) {}
	~ShaderParameterException() throw() {}
	const char* what() const throw() { return msg_.c_str(); }

};

//-----------------------------------------------------------------------------
void ReportError( const std::string& msg, int lineNum )
{
	std::cerr << '[' << lineNum << "] " << msg << std::endl;
}

//-----------------------------------------------------------------------------
void log( const char* m ) { std::cout << m << std::endl; }

//------------------------------------------------------------------------------
/// Abstract class holding information a parameter name and type.
/// Derived class shall store a parameter value internally and
/// override @codeSetShaderParameterValue( GLuint program )@endcode
/// which will be called from client code to assign values to parameters
/// in the GLSL program.
class ShaderParameter
{
public:
    /// Supported types.
	typedef enum { GLSL_FLOAT, GLSL_VEC2, GLSL_VEC3, GLSL_VEC4, GLSL_BOOL,
				   GLSL_INT, GLSL_INVALID_TYPE } GLSLType;
    /// Constructor.
    ShaderParameter( GLSLType t, const std::string& name ) :
    	type_( t ), name_( name ) {}
    /// Returns parameter name.
    const std::string& GetName() const { return name_; }
    /// Returns parameter type.
    GLSLType GetType() const { return type_; }
	/// Set the parameter value inside a GLSL program.
    /// Derived classes  should throw a ShaderParameterException if errors occur.
    virtual void SetShaderParameterValue( GLuint program ) = 0; // throw( ShaderParameterException )
    /// Returns a copy.
    virtual ShaderParameter* Clone() const = 0;
private:
   /// Parameter type.
   GLSLType type_;
   /// Parameter name;
   std::string name_;
};

//-------------------------------------------------------------------------------
/// int parameter.
class IntShaderParameter : public ShaderParameter
{
    int v_;
public:
    IntShaderParameter( const std::string& name, int v ) :
                          ShaderParameter( GLSL_INT, name ), v_( v ) {}

    virtual void SetShaderParameterValue( GLuint program )
    {
        GLuint loc = glGetUniformLocation( program, GetName().c_str() );
        if( loc < 0 ) throw ShaderParameterException( GetName() + " not found" );
        glUniform1i( loc, v_ );
    }
    virtual IntShaderParameter* Clone() const
    {
        IntShaderParameter* sp = new IntShaderParameter( GetName(), v_ );
        return sp;
    }
    int GetValue() const { return v_; }
    void SetValue( int v ) { v_ = v; }
};

//-------------------------------------------------------------------------------
/// bool parameter.
class BoolShaderParameter : public ShaderParameter
{
    bool v_;
public:
    BoolShaderParameter( const std::string& name, bool v ) :
                          ShaderParameter( GLSL_BOOL, name ), v_( v ) {}

    virtual void SetShaderParameterValue( GLuint program )
    {
        GLuint loc = glGetUniformLocation( program, GetName().c_str() );
        if( loc < 0 ) throw ShaderParameterException( GetName() + " not found" );
        glUniform1i( loc, v_ );
    }
    virtual BoolShaderParameter* Clone() const
    {
        BoolShaderParameter* sp = new BoolShaderParameter( GetName(), v_ );
        return sp;
    }
    bool GetValue() const { return v_; }
    void SetValue( bool v ) { v_ = v; }
};

//-------------------------------------------------------------------------------
/// float parameter.
class FloatShaderParameter : public ShaderParameter
{
	float v_;
public:
    FloatShaderParameter( const std::string& name, float v ) :
    					  ShaderParameter( GLSL_FLOAT, name ), v_( v ) {}

    virtual void SetShaderParameterValue( GLuint program )
	{
		GLuint loc = glGetUniformLocation( program, GetName().c_str() );
        if( loc < 0 ) throw ShaderParameterException( GetName() + " not found" );
        glUniform1f( loc, v_ );
	}
    virtual FloatShaderParameter* Clone() const
    {
        FloatShaderParameter* sp = new FloatShaderParameter( GetName(), v_ );
        return sp;
    }
    float GetValue() const { return v_; }
    void SetValue( float v ) { v_ = v; }
};


//-------------------------------------------------------------------------------
/// vec2 parameter.
class Vec2ShaderParameter : public ShaderParameter
{
	float v_[ 2 ];
public:
    Vec2ShaderParameter( const std::string& name,
    					 float v0, float v1 ) :
    					 ShaderParameter( GLSL_VEC2, name )
    {
    	v_[ 0 ] = v0;
    	v_[ 1 ] = v1;
    }
    virtual void SetShaderParameterValue( GLuint program )
	{
		GLuint loc = glGetUniformLocation( program, GetName().c_str() );
		if( loc < 0 ) throw ShaderParameterException( GetName() + " not found" );
        glUniform2fv( loc, 1, v_ );
	}
    virtual Vec2ShaderParameter* Clone() const
    {
        Vec2ShaderParameter* sp = new Vec2ShaderParameter( GetName(), v_[ 0 ],  v_[ 1 ] );
        return sp;
    }
    const float* GetValue() const { return &v_[ 0 ]; }
    void SetValue( float v0, float v1 ) { v_[ 0 ] = v0; v_[ 1 ] = v1; }
};

//-------------------------------------------------------------------------------
/// vec3 parameter.
class Vec3ShaderParameter : public ShaderParameter
{
	float v_[ 3 ];
public:
    Vec3ShaderParameter( const std::string& name,
    					 float v0, float v1, float v2 ) :
    					 ShaderParameter( GLSL_VEC3, name )
    {
    	v_[ 0 ] = v0;
    	v_[ 1 ] = v1;
    	v_[ 2 ] = v2;
    }

    virtual void SetShaderParameterValue( GLuint program )
	{
		GLuint loc = glGetUniformLocation( program, GetName().c_str() );
		if( loc < 0 ) throw ShaderParameterException( GetName() + " not found" );
        glUniform3fv( loc, 1, v_ );
	}

    virtual Vec3ShaderParameter* Clone() const
    {
        Vec3ShaderParameter* sp = new Vec3ShaderParameter( GetName(), v_[ 0 ],  v_[ 1 ], v_[ 2 ] );
        return sp;
    }

    const float* GetValue() const { return &v_[ 0 ]; }
    void SetValue( float v0, float v1, float v2 ) { v_[ 0 ] = v0; v_[ 1 ] = v1; v_[ 2 ] = v2; }
};

//-------------------------------------------------------------------------------
/// vec4 parameter.
class Vec4ShaderParameter : public ShaderParameter
{
	float v_[ 4 ];
public:
    Vec4ShaderParameter( const std::string& name,
    					 float v0, float v1, float v2, float v3 ) :
    					 ShaderParameter( GLSL_VEC4, name )
    {
    	v_[ 0 ] = v0;
    	v_[ 1 ] = v1;
    	v_[ 2 ] = v2;
    	v_[ 3 ] = v3;
    }
    virtual void SetShaderParameterValue( GLuint program )
	{
		GLuint loc = glGetUniformLocation( program, GetName().c_str() );
		if( loc < 0 ) throw ShaderParameterException( GetName() + " not found" );
        glUniform4fv( loc, 1, v_ );
	}
    virtual Vec4ShaderParameter* Clone() const
    {
        Vec4ShaderParameter* sp = new Vec4ShaderParameter( GetName(), v_[ 0 ],  v_[ 1 ], v_[ 2 ], v_[ 3 ] );
        return sp;
    }
    const float* GetValue() const { return &v_[ 0 ]; }
    void SetValue( float v0, float v1, float v2, float v3 ) { v_[ 0 ] = v0; v_[ 1 ] = v1; v_[ 2 ] = v2; v_[ 3 ] = v3; }
};

//------------------------------------------------------------------------------
/// Class used to store shader parameters into containers.
/// A pointer data member points to instance of a specific ShaderParameter.
/// Each instance of ShaderParameterWrapper owns the wrapped ShaderParameter instance.
class ShaderParameterWrapper
{
private:
    /// Pointer to ShaderParameter instance; memory is managed by
    /// ShaderParameterWrapper.
    ShaderParameter* sp_;
public:
    /// Default constructor; required for storing instances of this class into
    /// standard containers.
	ShaderParameterWrapper() : sp_( 0 ) {}
    /// Constructor accepting a pointer to the ShaderParameter instance to wrap.
    ShaderParameterWrapper( ShaderParameter* sp ) : sp_( sp ) {}
    /// Copy constructor: the wrapped object is cloned and the new copy is
    /// stored in the newly construced ShaderParameterWrapper.
	ShaderParameterWrapper( const ShaderParameterWrapper& pw ) : sp_( 0 )
    {
        operator=( pw );
    }

    /// Destructor: deletes owned instance of ShaderParameter.
    ~ShaderParameterWrapper() { delete sp_; }

    /// Deletes the currently wrapped object and copies the new object to wrap.
    ShaderParameterWrapper& operator=( const ShaderParameterWrapper& pw )
    {
        delete sp_;
        if( pw.sp_) sp_ = pw.sp_->Clone();
        else sp_ = 0;
        return *this;
    }

    /// Proxy method: forwards the call to ShaderParameter::SetShaderParameterValue().
    void SetShaderParameter( GLuint program )
	{
		assert( sp_ );
		sp_->SetShaderParameterValue( program );
	}

    /// Proxy method: forwards the call to ShaderParameter::GetName().
	const std::string& GetName() const
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		return sp_->GetName();
	}

    /// Proxy method: forwards the call to ShaderParameter::GetType().
	ShaderParameter::GLSLType GetType() const
	{
		if( !sp_ ) return ShaderParameter::GLSL_INVALID_TYPE;
		return sp_->GetType();
	}

    //@{ Get methods: each method returns the value of the requested type
    ///  returned from ShaderParameter::GetValue(); in case the requested
    ///  value type doesn't match the ShaderParameter type an exception
    ///  is thrown.
    int GetInt() const
    {
        if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
        if( GetType() != ShaderParameter::GLSL_INT )
        {
            throw ShaderParameterException( "int type mismatch" );
        }
        return dynamic_cast< IntShaderParameter* >( sp_ )->GetValue();
    }

	bool GetBool() const
    {
        if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
        if( GetType() != ShaderParameter::GLSL_BOOL )
        {
            throw ShaderParameterException( "bool type mismatch" );
        }
        return dynamic_cast< BoolShaderParameter* >( sp_ )->GetValue();
    }

    float GetFloat() const
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_FLOAT )
		{
			throw ShaderParameterException( "float type mismatch" );
		}
		return dynamic_cast< FloatShaderParameter* >( sp_ )->GetValue();
	}

	const float* GetVec2() const
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_VEC2 )
		{
			throw ShaderParameterException( "vec2 type mismatch" );
		}
		return dynamic_cast< Vec2ShaderParameter* >( sp_ )->GetValue();
	}

	const float* GetVec3() const
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_VEC3 )
		{
			throw ShaderParameterException( "vec3 type mismatch" );
		}
		return dynamic_cast< Vec3ShaderParameter* >( sp_ )->GetValue();
	}

	const float* GetVec4() const
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_VEC4 )
		{
			throw ShaderParameterException( "vec4 type mismatch" );
		}
		return dynamic_cast< Vec3ShaderParameter* >( sp_ )->GetValue();
	}
    //@}

    //@{ Set methods: each method sets the value of ShaderParameter to the
    ///  passed value(s).
    ///  In case the value tyep doesn't match the ShaderParameter type
    ///  an exception is thrown.
    void SetInt( int v )
    {
        if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
        if( GetType() != ShaderParameter::GLSL_INT )
        {
            throw ShaderParameterException( "int type mismatch" );
        }
        return dynamic_cast< IntShaderParameter* >( sp_ )->SetValue( v );
    }

    void SetBool( bool v )
    {
        if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
        if( GetType() != ShaderParameter::GLSL_BOOL )
        {
            throw ShaderParameterException( "bool type mismatch" );
        }
        return dynamic_cast< BoolShaderParameter* >( sp_ )->SetValue( v );
    }

	void SetFloat( float f )
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_FLOAT )
		{
			throw ShaderParameterException( "float type mismatch" );
		}
		return dynamic_cast< FloatShaderParameter* >( sp_ )->SetValue( f );
	}

	void SetVec2( float v0, float v1 )
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_VEC2 )
		{
			throw ShaderParameterException( "vec2 type mismatch" );
		}
		return dynamic_cast< Vec2ShaderParameter* >( sp_ )->SetValue( v0, v1 );
	}

	void SetVec3( float v0, float v1, float v2 )
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_VEC3 )
		{
			throw ShaderParameterException( "vec3 type mismatch" );
		}
		return dynamic_cast< Vec3ShaderParameter* >( sp_ )->SetValue( v0, v1, v2 );
	}

	void SetVec4( float v0, float v1, float v2, float v3 )
	{
		if( !sp_ ) throw ShaderParameterException( "NULL ShaderParameter" );
		if( GetType() != ShaderParameter::GLSL_VEC4 )
		{
			throw ShaderParameterException( "vec4 type mismatch" );
		}
		return dynamic_cast< Vec4ShaderParameter* >( sp_ )->SetValue( v0, v1, v2, v3 );
	}
    // @}
};

typedef std::vector< ShaderParameterWrapper > ShaderParameters;
//-----------------------------------------------------------------------------
void SetShaderParameters( GLuint program, ShaderParameters& params )
{
    glUseProgram( program );
	ShaderParameters::iterator i = params.begin();
	for( ; i != params.end(); ++i )
	{
		i->SetShaderParameter( program );
	}
    glUseProgram( 0 );
}


typedef std::vector< std::string > Tokens;

//------------------------------------------------------------------------------
ShaderParameter::GLSLType ShaderParameterType( const std::string& t )
{
    if( t == "int" ) return ShaderParameter::GLSL_INT;
    if( t == "bool" ) return ShaderParameter::GLSL_BOOL;
	if( t == "float" ) return ShaderParameter::GLSL_FLOAT;
	if( t == "vec2" ) return ShaderParameter::GLSL_VEC2;
	if( t == "vec3" ) return ShaderParameter::GLSL_VEC3;
	if( t == "vec4" ) return ShaderParameter::GLSL_VEC4;
	return ShaderParameter::GLSL_INVALID_TYPE;

}

//------------------------------------------------------------------------------
ShaderParameterWrapper BuildShaderParameter( const std::string& paramType,
									  		 const std::string& paramName,
									  		 std::istream& values )
{
	switch( ShaderParameterType( paramType ) )
	{
        case ShaderParameter::GLSL_INT:
        {
            int v;
            values >> v;
            return ShaderParameterWrapper(
                new IntShaderParameter( paramName, v ) );
        }
        break;
        case ShaderParameter::GLSL_BOOL:
        {
            bool v;
            values >> v;
            return ShaderParameterWrapper(
                new BoolShaderParameter( paramName, v ) );
        }
        break;
		case ShaderParameter::GLSL_FLOAT:
		{
			float v;
			values >> v;
			return ShaderParameterWrapper(
				new FloatShaderParameter( paramName, v ) );
		}
		break;
		case ShaderParameter::GLSL_VEC2:
		{
			float v0;
			float v1;
			values >> v0 >> v1;
			return ShaderParameterWrapper(
				new Vec2ShaderParameter( paramName, v0, v1 ) );
		}
		break;
		case ShaderParameter::GLSL_VEC3:
		{
			float v0;
			float v1;
			float v2;
			values >> v0 >> v1 >> v2;
			return ShaderParameterWrapper(
				new Vec3ShaderParameter( paramName, v0, v1, v2 ) );
		}
		break;
		case ShaderParameter::GLSL_VEC4:
		{
			float v0;
			float v1;
			float v2;
			float v3;
			values >> v0 >> v1 >> v2 >> v3;
            return ShaderParameterWrapper(
				new Vec4ShaderParameter( paramName, v0, v1, v2, v3 ) );
		}
		break;
		default: break;
	}

	throw ShaderParameterException( "Invalid parameter type " + paramType
									+ " for parameter " + paramName );
}

//------------------------------------------------------------------------------
ShaderParameters ReadShaderParameters( std::istream& in )
{
	ShaderParameters params;
	static const char COMMENT_CHAR = '#';
	static const std::string ASSIGNMENT_OPERATOR = "=";
	int lineCount = 0;
	std::string lineBuffer;
	while( std::getline( in, lineBuffer ) )
	{
		++lineCount;
		if( lineBuffer.size() == 0 ) continue;
		std::istringstream is( lineBuffer );
		std::string firstToken;
		is >> firstToken; // type
		if( firstToken[ 0 ] == COMMENT_CHAR ) continue;
		if( ShaderParameterType( firstToken ) == ShaderParameter::GLSL_INVALID_TYPE )
		{
			 std::ostringstream os;
			 os << lineCount;
			 throw ShaderParameterException( "Invalid type at line " + os.str() );
		}
		std::string secondToken;
		is >> secondToken; // identifier name
		if( secondToken.size() == 0 )
		{
			 std::ostringstream os;
			 os << lineCount;
			 throw ShaderParameterException( "Missing Identifier at line " + os.str() );
			 break;
		}
        params.push_back( BuildShaderParameter( firstToken, secondToken, is ) );
	}

	return params;
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC INTERFACE - CATCH std::exception and print string returned by what()
////////////////////////////////////////////////////////////////////////////////
//
// FILE FORMAT (# --> comment)
// <type> <parameter names > <parameter values>
//
//EXAMPLE:
//#GLSL OrangeBook - CH 18 - Gooch
//vec3 LightPosition 0 10 4
//vec3 SurfaceColor 0.2 0.2 0.2
//vec3 WarmColor 0.6 0.3 0.3
//vec3 CoolColor 0 0 0.6
//float DiffuseWarm 0.45
//float DiffuseCool 1
//
// Only  float, vec2, vec3, vec4, bool, int supported

//------------------------------------------------------------------------------
/// Sets parameter values from configuration file.
void SetShaderParametersFromFile( GLuint program, const char* fileName )
{
	std::ifstream in( fileName );
	if( !in ) throw ShaderParameterException( std::string( "Cannot open file " ) + fileName );
	ShaderParameters params = ReadShaderParameters( in );
	SetShaderParameters( program, params );
}

//------------------------------------------------------------------------------
/// Sets parameter values from string.
void SetShaderParametersFromBuffer( GLuint program, const std::string& buffer )
{
	std::istringstream in( buffer );
	ShaderParameters params = ReadShaderParameters( in );
	SetShaderParameters( program, params );
}
