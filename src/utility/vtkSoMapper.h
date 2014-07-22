#ifndef VTKSOMAPPER_H_
#define VTKSOMAPPER_H_
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

// Inventor
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/nodes/SoFrustumCamera.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>

// VTK
#include <vtkCamera.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vtkTransform.h>
#include <vtkRenderWindow.h>

// GL
#include <GL/gl.h>

// STD
#include <cassert>
#include <cmath>

#include "Geometry.h"

//------------------------------------------------------------------------------
/// Computes Inventor SbRotation from VTK matrix.
inline SbRotation ComputeSbRotation( const vtkMatrix4x4& vm )
{
    SbRotation r;
    SbMatrix m( vm[ 0 ][ 0 ], vm[ 0 ][ 1 ], vm[ 0 ][ 2 ], 0,
                vm[ 1 ][ 0 ], vm[ 1 ][ 1 ], vm[ 1 ][ 2 ], 0,
                vm[ 2 ][ 0 ], vm[ 2 ][ 1 ], vm[ 2 ][ 2 ], 0,
                0, 0, 0, 1 );

    r.setValue( m );

    return m;
}

//------------------------------------------------------------------------------
/// Convert VTK camera to Inventor perspective camera
inline void VtkToSoCamera( double aspect, vtkCamera* camera, SoFrustumCamera* socam )
{
    socam->ref();
    double vpn[ 3 ]; camera->GetViewPlaneNormal( vpn );
    double vup[ 3 ]; camera->GetViewUp( vup );
    double vright[ 3 ]; vcross( vup, vpn, vright );
    double p[ 3 ]; camera->GetPosition( p );
    double d[ 2 ]; camera->GetClippingRange( d );
    double fd = float( camera->GetDistance() );
    double ha = camera->GetViewAngle();
//    double axis[ 3 ];
    double angle = 0.;

    socam->position = SbVec3f( float( p[ 0 ] ), float( p[ 1 ] ), float( p[ 2 ] ) );
    socam->nearDistance = float( d[ 0 ] );
    socam->farDistance = float( d[ 1 ] );
    //socam->focalDistance = fd;

    //socam->heightAngle = M_PI * float( ha / 180.f );
    socam->orientation.setValue( ComputeSbRotation( *(
        camera->GetViewTransformObject()->GetMatrix() ) )  ) ;

    double tmp = std::tan( M_PI * ( camera->GetViewAngle() ) / ( 2. * 180. ) );
    double width = 0.;
    double height = 0.;
    if ( camera->GetUseHorizontalViewAngle() )
    {
      width = camera->GetClippingRange()[0] * tmp;
      height = camera->GetClippingRange()[0] * tmp / aspect;
    }
    else
    {
      width = camera->GetClippingRange()[0] * tmp * aspect;
      height = camera->GetClippingRange()[0] * tmp;
    }

    double xmin = ( camera->GetWindowCenter()[0] - 1.0 ) * width;
    double xmax = ( camera->GetWindowCenter()[0] + 1.0 ) * width;
    double ymin = ( camera->GetWindowCenter()[1] - 1.0 ) * height;
    double ymax = ( camera->GetWindowCenter()[1] + 1.0 ) * height;	

    socam->left = xmin;
    socam->right = xmax;
    socam->bottom = ymin;
    socam->top = ymax;	

    socam->unref();
}

//------------------------------------------------------------------------------
/// Copies vtkActor's transform into Inventor transform.
inline void CompTransform( vtkActor* anActor, SoTransform* t )
{

  vtkTransform* trans = vtkTransform::New();
  trans->SetMatrix(anActor->vtkProp3D::GetMatrix());
  double* tempd;
  tempd = trans->GetPosition();
  t->translation.setValue( tempd[ 0 ], tempd[ 1 ], tempd[ 2 ] );
  tempd = trans->GetOrientationWXYZ();
  t->rotation.setValue( SbRotation( SbVec3f( tempd[ 1 ], tempd[ 2 ], tempd[ 3 ] ), tempd[0] * M_PI / 180.0 ) );
  tempd = trans->GetScale();
  t->scaleFactor.setValue( float(tempd[ 0 ]), float(tempd[ 1 ]), float(tempd[ 2 ]) );
  trans->Delete();

}

//------------------------------------------------------------------------------
/// vtkMapper specialization for rendering OpenInventor SoNodes.
/// @warning currently works only with perspective camera.
/// @todo add support for orthographic camera.
/// The root node of the scenegraph to render is set in the @code SetRoot() @endcode
/// method.
/// Rendering is implemented in the overridden @code Draw() @endcode.
/// A new bounding box is computed each time a new root node is set.
/// This class is supposed to be used by specifying a geometric object (e.g. sphere)
/// as its input; the object's bounding box will have to match the mapper's
/// bounding box for picking to work properly.
///
/// e.g.:
///
/// @code
/// // create vtkSoMapper instance, use sphere as input, all events
/// vtkSoMapper* m = vtkSoMapper::New();
/// m->SetImmediateModeRendering( true ); // <- won't work properly without this
///
/// // set reference to Inventor node
/// m->SetRoot( root );
/// // create sphere and set sphere center to Inventor Node's bounding box
/// // center and sphere radius to max bounding box edge
/// vtkSphereSource* ss = vtkSphereSource::New();
/// ss->SetCenter( .5 * ( m->GetBounds()[ 0 ] +  m->GetBounds()[ 1 ] ),
///                .5 * ( m->GetBounds()[ 2 ] +  m->GetBounds()[ 3 ] ),
///                .5 * ( m->GetBounds()[ 4 ] +  m->GetBounds()[ 5 ] ) );
/// const double sx = abs( .5* ( m->GetBounds()[ 0 ] -  m->GetBounds()[ 1 ] ) );
///	const double sy = abs( .5* ( m->GetBounds()[ 2 ] -  m->GetBounds()[ 3 ] ) );
///	const double sz = abs( .5* ( m->GetBounds()[ 4 ] -  m->GetBounds()[ 5 ] ) );
/// double s = sx;
/// if( sy > s ) s = sy;
/// if( sz > s ) s = sz;
/// ss->SetRadius( s );
/// // set sphere as input connection of vtkSoMapper instance
/// m->SetInputConnection( ss->GetOutputPort() );
/// // IMPORTANT: set actor referencing the mapper
/// m->SetActor( actor );
/// m->Update();
/// @endcode
class vtkSoMapper : public vtkOpenGLPolyDataMapper
{
private:

    SbViewportRegion v_;
    SoGLRenderAction renderAction_;
    SoSeparator* pRoot_;
    SoFrustumCamera* pcam_;
    vtkActor* actor_;

    /// Private constructor accessble only from New() method.
    vtkSoMapper() : pRoot_( 0 ),
                    renderAction_( ( SbViewportRegion() ) ),
                    pcam_( new SoFrustumCamera ),
                    actor_( 0 )
    {
            pcam_->ref();
    }

public:
    /// Get actor referencing this mapper.
    vtkActor* GetActor() { return actor_; }

    /// Set actor referencing this mapper;
    void SetActor( vtkActor* a ) { actor_ = a; }

    /// Get Inventor camera.
    SoFrustumCamera* GetCamera() { return pcam_; }

    /// Get mapper's scengraph root; this is not the Inventor's
    /// scenegraph root.
    SoSeparator* GetRoot() { return pRoot_; }

    /// Get Inventor's scnegraph root.
    SoNode* GetModelRoot() { return pRoot_->getChild( 2 ); }

    /// Get viewport.
    const SbViewportRegion& GetViewportRegion() const { return v_; }

    /// constructor
    static  vtkSoMapper* New()
    {
        return new vtkSoMapper;
    }

    /// Set Open Inventor root.
    /// This method creates a scenegraph with one camera and one
    /// transform node; camera and transform are copied from
    /// VTK's camera and actor transform each time the object is
    /// rendered.
    /// root
    ///   |
    ///    -- camera
    ///   |
    ///    -- actor transform
    ///   |
    ///    -- Inventor scenegraph
    void SetRoot( SoSeparator* pRoot )
    {

        if( !pRoot ) return;
        pRoot->ref();
        if( pRoot_ ) pRoot_->unref();
        pRoot_ = new SoSeparator;
        pRoot_->ref();
        pRoot_->addChild( pcam_ );
        pRoot_->addChild( new SoTransform );
        pRoot_->addChild( pRoot );
        ComputeBBox();
        pRoot->unref();
    }

    /// Computes the bounding box.
    void ComputeBBox()
    {
        assert( pRoot_ && "NULL root node" );
        SbViewportRegion v;
        SoGetBoundingBoxAction bba ( v );
        bba.apply( GetModelRoot() );
         SbBox3f b = bba.getBoundingBox();
        float mx, my, mz, Mx, My, Mz;
        b.getBounds( mx, my, mz, Mx, My, Mz );
        Bounds[ 0 ] = mx; Bounds[ 1 ] = Mx;
        Bounds[ 2 ] = my; Bounds[ 3 ] = My;
        Bounds[ 4 ] = mz; Bounds[ 5 ] = Mz;
    }

    /// Overridden Draw method. This method renders the openinventor
    /// scenegraph using VTK camera and actor transform.
    /// Flow of operation:
    /// - convert vtkActor transform and copy it to Inventor scenegraph
    /// - save GL matrices
    /// - set Inventor camera to match VTK camera
    /// - render Inventor scenegraph
    /// - reset GL matrices
    int Draw( vtkRenderer* ren, vtkActor* a )
    {
        assert( actor_ && "vtkSoMapper::actor_ is NULL" );
        assert( pcam_ &&  "vtkSoMapper::pcam_ is NULL"  );
        assert( pRoot_ && "vtkSoMapper::pRoot_ is NULL" );
        assert( ren && "vtkSoMapper::Draw() - renderer parameter is NULL" );
        assert( a && "vtkSoMapper::Draw() - actor parameter is NULL" );
        CompTransform( actor_, ( SoTransform* ) pRoot_->getChild( 1 ) );
        
        GLint mm;
        glGetIntegerv( GL_MATRIX_MODE, &mm );

        // save state
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();

        if( !pRoot_ ) return 0;
        const double* vp = ren->GetViewport();
        vtkCamera* camera = ren->GetActiveCamera();
        v_.setWindowSize( ren->GetRenderWindow()->GetSize()[ 0 ],
                          ren->GetRenderWindow()->GetSize()[ 1 ] );
        v_.setViewport( float( vp[ 0 ] ), float( vp[ 1 ] ), float( vp[ 2 ] - vp[ 0 ] ), float( vp[ 3 ] - vp[ 1 ] ) );
        renderAction_.setViewportRegion( v_ );
        VtkToSoCamera( ren->GetRenderWindow()->GetSize()[ 0 ] / double( ren->GetRenderWindow()->GetSize()[ 1 ] ),
			 camera, pcam_ );
        //if saveGLAttributes_
        //glPushAttrib( GL_ALL_ATTRIB_BITS );
        renderAction_.apply( pRoot_ );
        //if saveGLAttributes_ 
        //glPopAttrib();	

        // reset state
        glGetIntegerv( GL_MATRIX_MODE, &mm );
        glMatrixMode( GL_PROJECTION );
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
        glPopMatrix();
        glMatrixMode( ( GLenum ) mm );
        return 0;
    }

    /// Destructor
    void Delete()
    {
        pcam_->unref();
        pRoot_->unref();
        vtkMapper::Delete();
    }

    /// Returns Inventor scenegraph's bounds
    double* GetBounds()
    {
        return Bounds;
    }

};

#endif /*VTKSOMAPPER_H_*/
