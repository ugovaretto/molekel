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
#include <GL/glew.h>
#include "vtkGLSLShaderActor.h"

//------------------------------------------------------------------------------
vtkGLSLShaderActor* vtkGLSLShaderActor::New()
{
    return new vtkGLSLShaderActor;
}

//------------------------------------------------------------------------------
 void vtkGLSLShaderActor::Render( vtkRenderer* ren, vtkMapper* m )
 {

	if( shaderProgram_ != 0 && useShaderProgram_ )
	{
		PushProgram();
		glUseProgram( shaderProgram_ );
		vtkOpenGLActor::Render( ren, m );
	    PopProgram();
	}
	else vtkOpenGLActor::Render( ren, m );
 }

//------------------------------------------------------------------------------
void vtkGLSLShaderActor::SetShaderProgramId( GLuint id )
{
    if( shaderProgram_ != 0 ) glDeleteProgram( shaderProgram_ );
    shaderProgram_ = id;
}

//------------------------------------------------------------------------------
void vtkGLSLShaderActor::PushProgram()
{
	glGetIntegerv( GL_CURRENT_PROGRAM, &activeProgram_ );
}

//------------------------------------------------------------------------------
void vtkGLSLShaderActor::PopProgram()
{
	glUseProgram( activeProgram_ );
}
