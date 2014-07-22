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

#include "QGLSLShaderEditorWidget.h"

#include <cassert>
#include <fstream>
#include <limits>

#include <QGridLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QVariant>
#include <QColorDialog>
#include <QFileDialog>
#include <QPushButton>

#include "../qtfileutils.h"

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::CheckProgram( GLuint p ) const
{
    typedef QGLSLShaderEditorWidget::Exception Ex;
    if( p == 0 ) throw Ex( "Invalid program id (0)" );
    GLint param = 0;
    glGetProgramiv( p, GL_DELETE_STATUS, &param );
    if( param == GL_TRUE ) throw Ex( QString( "Program %1 has been deleted" ).arg( p ) );
    glGetProgramiv( p, GL_ATTACHED_SHADERS, &param );
    if( param <= 0 ) throw Ex( QString( "No shaders attached to program %1" ).arg( p ) );
    glGetProgramiv( p, GL_LINK_STATUS, &param );
    if( param != GL_TRUE ) throw Ex( QString( "Program %1 has not been linked" ).arg( p ) );
}

//------------------------------------------------------------------------------
bool IsColor( const QString& name )
{
    if( name.startsWith( "gl_" ) ) return false;
    if( name.contains( "Color", Qt::CaseInsensitive ) ||
        name.contains( "Colour", Qt::CaseInsensitive ) ) return true;
    return false;
}


//-------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::AddIntUIControl( ShaderParamInfo& si,
                                               int value,
                                               QGridLayout* layout,
                                               int row, int column )
{
    assert( layout );
    static const int MININT = std::numeric_limits< int >::min();
    static const int MAXINT = std::numeric_limits< int >::max();
    QSpinBox* pb = new QSpinBox;
    pb->setRange( MININT, MAXINT );
    pb->setValue( value );
    connect( pb, SIGNAL( valueChanged( int ) ),
             this, SLOT( IntValueChangedSlot( int ) ) );
    pb->setProperty( "Index", row );
    layout->addWidget( pb, row, column );
    si.uiControls.push_back( CW( pb, GL_INT ) );
}

//-------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::AddBoolUIControl( ShaderParamInfo& si,
                                                bool value,
                                                QGridLayout* layout,
                                                int row, int column )
{
    assert( layout );
    QCheckBox* pc = new QCheckBox;
    pc->setCheckState( value ? Qt::Checked : Qt::Unchecked );
    connect( pc, SIGNAL( stateChanged( int ) ),
             this, SLOT( StateChangedSlot( int ) ) );
    pc->setProperty( "Index", row );
    layout->addWidget( pc, row, column );
    si.uiControls.push_back( CW( pc, GL_BOOL ) );
}

//-------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::AddFloatUIControl( ShaderParamInfo& si,
                                                 float value,
                                                 QGridLayout* layout,
                                                 int row, int column )
{

    static const double MINDOUBLE = std::numeric_limits< float >::min();
    static const double MAXDOUBLE = std::numeric_limits< float >::max();
    QDoubleSpinBox* pb = new QDoubleSpinBox;
    pb->setRange( -MAXDOUBLE, MAXDOUBLE );
    pb->setSingleStep( 0.01f );
    pb->setDecimals( 6 );
    pb->setValue( value );
    connect( pb, SIGNAL( valueChanged( double ) ),
             this, SLOT( DoubleValueChangedSlot( double ) ) );
    pb->setProperty( "Index", row );
    layout->addWidget( pb, row, column );
    si.uiControls.push_back( CW( pb, GL_FLOAT ) );
}


//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::AddRow( QGridLayout* layout,
                                      GLenum type,
                                      const QString& name,
                                      int index )
{
    /// @todo fix ranges; find a way to load range info from e.g. comments
    /// in source files
    const bool isColor = IsColor( name );
    if( isColor )
    {
        QPushButton* pb = new QPushButton( name );
        pb->setProperty( "Index", index );
        connect( pb, SIGNAL( released() ), this, SLOT( ColorSlot() ) );
        layout->addWidget( pb, index, 0 );
    }
    else layout->addWidget( new QLabel( name ), index, 0 );
    GLint location = glGetUniformLocation( shaderProgram_, name.toAscii().constData() );
    if( location < 0 ) return;
    ShaderParamInfo si;
    si.type = type;
    si.name = name;
    const int cnt = index;
    switch( type )
    {
        case GL_INT:
        {
            GLint v[ 1 ];
            glGetUniformiv( shaderProgram_, location, v );
            AddIntUIControl( si, v[ 0 ], layout, cnt, 1 );
        }
        break;
        case GL_BOOL:
        {
            GLint v[ 1 ];
            glGetUniformiv( shaderProgram_, location, v );
            AddBoolUIControl( si, v[ 0 ], layout, cnt, 1 );
        }
        break;
        case GL_FLOAT:
        {
            GLfloat v[ 1 ];
            glGetUniformfv( shaderProgram_, location, v );
		    AddFloatUIControl( si, v[ 0 ], layout, cnt, 1 );
        }
        break;
        case GL_FLOAT_VEC2:
        {
            GLfloat v[ 2 ];
            glGetUniformfv( shaderProgram_, location, v );
            AddFloatUIControl( si, v[ 0 ], layout, cnt, 1 );
            AddFloatUIControl( si, v[ 1 ], layout, cnt, 2 );
        }
        break;
        case GL_FLOAT_VEC3:
        {
	        GLfloat v[ 3 ];
            glGetUniformfv( shaderProgram_, location, v );
            AddFloatUIControl( si, v[ 0 ], layout, cnt, 1 );
            AddFloatUIControl( si, v[ 1 ], layout, cnt, 2 );
            AddFloatUIControl( si, v[ 2 ], layout, cnt, 3 );
        }
        break;
        case GL_FLOAT_VEC4:
        {
            GLfloat v[ 4 ];
            glGetUniformfv( shaderProgram_, location, v );
            AddFloatUIControl( si, v[ 0 ], layout, cnt, 1 );
            AddFloatUIControl( si, v[ 1 ], layout, cnt, 2 );
            AddFloatUIControl( si, v[ 2 ], layout, cnt, 3 );
            AddFloatUIControl( si, v[ 3 ], layout, cnt, 4 );
        }
        break;
        default: throw Exception( QString( "Invalid parameter type %1" ).arg( type ) );
        break;
    }
     shaderValues_.push_back( si );
}

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::SetGLSLProgram( unsigned int  p )
{
    CheckProgram( p ); // throws Exception

    shaderProgram_ = p;

    // enable program
    glUseProgram( shaderProgram_ );

    // get number of parameters
    GLint numParameters = 0;
    glGetProgramiv( shaderProgram_, GL_ACTIVE_UNIFORMS, &numParameters );
    if( numParameters == 0 ) return;
    // get max length of parameter name
    GLint maxParamSize = 0;
    glGetProgramiv( shaderProgram_, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxParamSize );
    // allocate buffer that will hold the parameter name
    char* buffer = new char[ maxParamSize + 1 ];
    buffer[ 0 ] = '\0';
    int cnt = 0;
    shaderValues_.clear();
    shaderValues_.reserve( numParameters );
    QGridLayout* layout = new QGridLayout;
    // for each parameter query value
    for( int i = 0; i != numParameters; ++i )
    {
        // GL return values
        GLsizei length = 0;
        GLint size = 0;
        GLenum type;

        // get value
        glGetActiveUniform( shaderProgram_,
                            i,
                            maxParamSize,
                            &length,
                            &size,
                            &type,
                            buffer );
        if( length == 0 ) continue;

        // if not float or vec2/3/4 continue
        if( type != GL_FLOAT && type != GL_FLOAT_VEC2 &&
            type != GL_FLOAT_VEC3 && type != GL_FLOAT_VEC4
            && type != GL_BOOL && type != GL_INT ) continue;
        // add row with name | val0 spin box | val1 spin box ...
        // adding to each spin box the parameter name and type
        // connect to DoubleValueChangedSlot
        AddRow( layout, type, buffer, cnt );
        ++cnt;
    }
    delete [] buffer;
    setLayout( layout );
}

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::DoubleValueChangedSlot( double )
{
    // get sender
    QObject* s = sender();
    assert( s );
    // get index
    const int index = s->property( "Index" ).toInt();
    // get values and set shader value
    const ShaderParamInfo& sp = shaderValues_[ index ];
    glUseProgram( shaderProgram_ );
    GLint loc = glGetUniformLocation( shaderProgram_, sp.name.toAscii().constData() );
    switch( sp.type )
    {
        case GL_FLOAT:
        {
            const float v = float( sp.uiControls[ 0 ].GetFloatValue() );
            glUniform1f( loc, v );
        }
        break;
        case GL_FLOAT_VEC2:
        {
            const float v0 = float( sp.uiControls[ 0 ].GetFloatValue() );
            const float v1 = float( sp.uiControls[ 1 ].GetFloatValue() );
            glUniform2f( loc, v0, v1 );
        }
        break;
        case GL_FLOAT_VEC3:
        {
            const float v0 = float( sp.uiControls[ 0 ].GetFloatValue() );
            const float v1 = float( sp.uiControls[ 1 ].GetFloatValue() );
            const float v2 = float( sp.uiControls[ 2 ].GetFloatValue() );
            glUniform3f( loc, v0, v1, v2 );
        }
        break;
        case GL_FLOAT_VEC4:
        {
            const float v0 = float( sp.uiControls[ 0 ].GetFloatValue() );
            const float v1 = float( sp.uiControls[ 1 ].GetFloatValue() );
            const float v2 = float( sp.uiControls[ 2 ].GetFloatValue() );
            const float v3 = float( sp.uiControls[ 3 ].GetFloatValue() );
            glUniform4f( loc, v0, v1, v2, v3 );
        }
        break;
        default: break;
    }
    emit ValueChanged();
}


//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::StateChangedSlot( int )
{
    // get sender
    QObject* s = sender();
    assert( s );
    // get index
    const int index = s->property( "Index" ).toInt();
    // get values and set shader value
    const ShaderParamInfo& sp = shaderValues_[ index ];
    glUseProgram( shaderProgram_ );
    GLint loc = glGetUniformLocation( shaderProgram_, sp.name.toAscii().constData() );
    switch( sp.type )
    {
        case GL_BOOL:
        {
            const bool v = sp.uiControls[ 0 ].GetBooleanValue();
            glUniform1i( loc, v );
        }
        break;
        case GL_BOOL_VEC2:
        {
            // TBD
        }
        break;
        case GL_BOOL_VEC3:
        {
            // TBD
        }
        break;
        case GL_BOOL_VEC4:
        {
            // TBD
        }
        break;
        default: break;
    }
    emit ValueChanged();
}

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::IntValueChangedSlot( int )
{
    // get sender
    QObject* s = sender();
    assert( s );
    // get index
    const int index = s->property( "Index" ).toInt();
    // get values and set shader value
    const ShaderParamInfo& sp = shaderValues_[ index ];
    glUseProgram( shaderProgram_ );
    GLint loc = glGetUniformLocation( shaderProgram_, sp.name.toAscii().constData() );
    switch( sp.type )
    {
        case GL_INT:
        {
            const int i = sp.uiControls[ 0 ].GetIntValue();
            glUniform1i( loc, i );
        }
        break;
        case GL_INT_VEC2:
        {
            // TBD
        }
        break;
        case GL_INT_VEC3:
        {
            // TBD
        }
        break;
        case GL_INT_VEC4:
        {
            // TBD
        }
        break;
        default: break;
    }
    emit ValueChanged();
}

//------------------------------------------------------------------------------
QGLSLShaderEditorWidget::~QGLSLShaderEditorWidget()
{
    glUseProgram( 0 );
}

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::ColorSlot()
{
    float r, g, b;
    const int MAX_COMPONENT_VALUE = 255;
    QObject* s = sender();
    assert( s );
    // get index
    const int index = s->property( "Index" ).toInt();
    ShaderParamInfo& sp = shaderValues_[ index ];
    r = float( sp.uiControls[ 0 ].GetFloatValue() );
    g = float( sp.uiControls[ 1 ].GetFloatValue() );
    b = float( sp.uiControls[ 2 ].GetFloatValue() );
    QColor color = QColorDialog::getColor( QColor( int( r * MAX_COMPONENT_VALUE ),
                                                   int( g * MAX_COMPONENT_VALUE ),
                                                   int( b * MAX_COMPONENT_VALUE ) ),
                                           this );
    if ( color.isValid() )
    {

        sp.uiControls[ 0 ].SetFloatValue( float( color.red() ) / MAX_COMPONENT_VALUE );
        sp.uiControls[ 1 ].SetFloatValue( float( color.green() ) / MAX_COMPONENT_VALUE );
        sp.uiControls[ 2 ].SetFloatValue( float( color.blue() ) / MAX_COMPONENT_VALUE );
    }
}

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::SaveParametersToFile( const char* fileName )
{
    std::ofstream os( fileName );
    for( ShaderValues::iterator i = shaderValues_.begin();
         i != shaderValues_.end();
         ++i )
    {
        const ShaderParamInfo& sp = *i;
        switch( sp.type )
        {
            case GL_INT:
            {
                const int v = sp.uiControls[ 0 ].GetIntValue();
                os << "int " << sp.name.toStdString() << '\t' << v << '\n';
            }
            break;
            case GL_BOOL:
            {
                const bool v = sp.uiControls[ 0 ].GetBooleanValue();
                os << "bool " << sp.name.toStdString() << '\t' << v << '\n';
            }
            break;
            case GL_FLOAT:
            {
                const float v = float( sp.uiControls[ 0 ].GetFloatValue() );
                os << "float " << sp.name.toStdString() << '\t' << v << '\n';
            }
            break;
            case GL_FLOAT_VEC2:
            {
                const float v0 = float( sp.uiControls[ 0 ].GetFloatValue() );
                const float v1 = float( sp.uiControls[ 1 ].GetFloatValue() );
                os << "vec2 " << sp.name.toStdString() << '\t' << v0 << ' ' << v1 << '\n';
            }
            break;
            case GL_FLOAT_VEC3:
            {
                const float v0 = float( sp.uiControls[ 0 ].GetFloatValue() );
                const float v1 = float( sp.uiControls[ 1 ].GetFloatValue() );
                const float v2 = float( sp.uiControls[ 2 ].GetFloatValue() );
                os << "vec3 " << sp.name.toStdString() << '\t' << v0 << ' ' << v1 << ' ' << v2 << '\n';
            }
            break;
            case GL_FLOAT_VEC4:
            {
                const float v0 = float( sp.uiControls[ 0 ].GetFloatValue() );
                const float v1 = float( sp.uiControls[ 1 ].GetFloatValue() );
                const float v2 = float( sp.uiControls[ 2 ].GetFloatValue() );
                const float v3 = float( sp.uiControls[ 3 ].GetFloatValue() );
                os << "vec4 " << sp.name.toStdString() << '\t' << v0 << ' ' << v1 << ' ' << v2 << ' ' << v3 << '\n';
            }
            break;
            default: break;
        }
    }
}

//------------------------------------------------------------------------------
void QGLSLShaderEditorWidget::SaveParameters( const QString& dir )
{
    QString f = GetSaveFileName( this, "Save shader parameters", dir );
    if( f.size() == 0 ) return;
    SaveParametersToFile( f.toAscii().constData() );
}
