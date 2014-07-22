#ifndef VTKGLSLSHADERACTOR_H_
#define VTKGLSLSHADERACTOR_H_
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

#include <GL/gl.h>
#include <vtkOpenGLActor.h>

/// Adds support for GLSL shaders to vtkOpenGLActor.
/// Works if EXT_fragment_lighting and EXT_vertex_shader available.
class vtkGLSLShaderActor : public vtkOpenGLActor
{
private:
    bool useShaderProgram_;
	GLuint shaderProgram_;
	mutable GLint activeProgram_;
	vtkGLSLShaderActor() :	useShaderProgram_( true ),
							shaderProgram_( 0 ),
							activeProgram_( 0 ) {}
	/// Saves currently active shader program.
	void PushProgram();
	/// Restores active shader program.
	void PopProgram();	
public:
	/// Constructor.
    static  vtkGLSLShaderActor* New();

    /// Overridden method.
    void Render( vtkRenderer* ren, vtkMapper* a );

    /// Returns shader program id.
    GLuint GetShaderProgramId() const { return shaderProgram_; }

    /// Sets shader program id.
    void SetShaderProgramId( GLuint id );

    /// Returns true if shaders enabled, false otherwise.
    bool GetShaderProgramEnabled() const { return useShaderProgram_; }

    /// Enable/disable shader program.
    void SetShaderProgramEnabled( bool enabled ) { useShaderProgram_ = enabled; }
};

#endif /*VTKGLSLSHADERACTOR_H_*/
