#ifndef VTKOPENGLGLYPHMAPPER_H_
#define VTKOPENGLGLYPHMAPPER_H_
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

#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkOpenGLPolyDataMapper.h>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

#include "Geometry.h"


//----------------------------------------------------------------------
/// Data type to hold values for Position, Orientation and Scaling.
struct POS
{
    ///  x position
    double x;
    /// y position
    double y;
    /// z position
    double z;
    /// Rotation about x axis in degrees.
    double rotx;
    /// Rotation about y axis in degrees.
    double roty;
    /// Scaling.
    double scale;
};

//----------------------------------------------------------------------
/// Compute position, orientation and scaling given a vector specified
/// with two end points.
/// @param p1 first (start) point
/// @param p2 second (end) point
/// @param centerDistance distance from p1 along (p2 - p1) axis.
/// @returns position, scaling and orientation: orientation is returned
/// as two angles:
/// - alpha: rotation about x axis
/// - beta:  rotation about y axis
inline POS ComputePOS( const double* p1, const double* p2, double centerDistance = 0 )
{
    POS pos;
    double l = vdist(p1, p2);
    double v[ 3 ];
    vsub(p2, p1, v);

    double alpha = (180 / M_PI) * std::acos(v[1]/l);
    if(v[2] < 0) alpha = -alpha;
    double beta = 0.0;

    if( !( std::abs(v[2]) <= std::numeric_limits<double>::epsilon() ) ) beta = (180 / M_PI) * std::atan(v[0]/v[2]);
    else if(v[0] < 0.) beta = -90.0;
    else              beta = 90.0;

    vnorm( v );
    pos.x = p1[ 0 ] + centerDistance * l * v[ 0 ];
    pos.y = p1[ 1 ] + centerDistance * l * v[ 1 ];
    pos.z = p1[ 2 ] + centerDistance * l * v[ 2 ];
    pos.rotx = alpha;
    pos.roty = beta;
    pos.scale = l;
    return pos;
}

//----------------------------------------------------------------------
/// Convert vtkTransform to OpenGL matrix.
inline void CopyTransformToGLMatrix( vtkTransform* t, double* g )
{
    vtkMatrix4x4* m = t->GetMatrix();
    for( int i = 0; i < 4; ++i )
    {
        for( int j = 0; j < 4; ++j )
        {
            g[ i + ( j << 2 ) ] = m->GetElement( i, j );
        }
    }
}

//----------------------------------------------------------------------
/// Compute transform from orientation vector.
/// @returns transform to align the y axis with the p1 - p2 axis.
/// @see ComputePOS
inline vtkTransform* ComputeTransform( const double* p1,
                                       const double *p2,
                                       double centerDistance = 0.,
                                       bool scale = true )
{
    POS p = ComputePOS( p1, p2, centerDistance );
    vtkTransform* t = vtkTransform::New();
    t->Identity();
    t->Translate( p.x, p.y, p.z );
    t->RotateY( p.roty );
    t->RotateX( p.rotx );
    if( scale ) t->Scale( 1, p.scale, 1 );
    return t;
}

//----------------------------------------------------------------------
/// Compute transform from position.
inline vtkTransform* ComputeTransform( const double* p )
{
    vtkTransform* t = vtkTransform::New();
    t->Identity();
    t->Translate( p );
    return t;
}

//----------------------------------------------------------------------
/// Data type used to store OpenGL matrix.
struct GLMatrix {
    double m[ 16 ];
    GLMatrix()
    {
        m[ 0 ] = 1.; m[ 4 ] = 0.; m[ 8 ]  = 0.; m[ 12 ] = 0.;
        m[ 1 ] = 0.; m[ 5 ] = 1.; m[ 9 ]  = 0.; m[ 13 ] = 0.;
        m[ 2 ] = 0.; m[ 6 ] = 0.; m[ 10 ] = 1.; m[ 14 ] = 0.;
        m[ 3 ] = 0.; m[ 7 ] = 0.; m[ 11 ] = 0.; m[ 15 ] = 1.;
    }
};

//----------------------------------------------------------------------
/// OpenGL Glyph mapper: renders the polygonal input data multiple times
/// at different positions and orientations.
class vtkOpenGLGlyphMapper : public vtkOpenGLPolyDataMapper
{
public:
    typedef std::vector< GLMatrix > TransformPtrList;
    typedef TransformPtrList::iterator TransformPtrListIterator;
private:
    /// Transform list.
    TransformPtrList transforms_;
    /// Constructor.
    vtkOpenGLGlyphMapper() : vtkOpenGLPolyDataMapper(), initBounds_( true ) {}
    /// Assignment disabled.
    vtkOpenGLGlyphMapper& operator=( const vtkOpenGLGlyphMapper& );
    /// Set to true when bounds have to be recomputed.
    bool initBounds_;
    /// Recompute bounds.
    void UpdateBounds( vtkTransform* t )
    {

        double b[ 6 ];
        this->GetInput()->GetBounds( b );
        double bmin[ 3 ];
        double bmax[ 3 ];
        bmin[ 0 ] = b[ 0 ];
        bmin[ 1 ] = b[ 2 ];
        bmin[ 2 ] = b[ 4 ];
        bmax[ 0 ] = b[ 1 ];
        bmax[ 1 ] = b[ 3 ];
        bmax[ 2 ] = b[ 5 ];
        double tbmin[ 3 ];
        double tbmax[ 3 ];
        t->TransformPoint( bmin, tbmin );
        t->TransformPoint( bmax, tbmax );
        if( tbmin[ 0 ] > tbmax[ 0 ] ) std::swap( tbmin[ 0 ], tbmax[ 0 ] );
        if( tbmin[ 1 ] > tbmax[ 1 ] ) std::swap( tbmin[ 1 ], tbmax[ 1 ] );
        if( tbmin[ 2 ] > tbmax[ 2 ] ) std::swap( tbmin[ 2 ], tbmax[ 2 ] );

        if( initBounds_ )
        {
            this->Bounds[ 0 ] = tbmin[ 0 ];
            this->Bounds[ 1 ] = tbmax[ 0 ];
            this->Bounds[ 2 ] = tbmin[ 1 ];
            this->Bounds[ 3 ] = tbmax[ 1 ];
            this->Bounds[ 4 ] = tbmin[ 2 ];
            this->Bounds[ 5 ] = tbmax[ 2 ];
            initBounds_ = false;
        }
        else
        {
            if( tbmin[ 0 ] < this->Bounds[ 0 ] ) this->Bounds[ 0 ] = tbmin[ 0 ];
            if( tbmax[ 0 ] > this->Bounds[ 1 ] ) this->Bounds[ 1 ] = tbmax[ 0 ];
            if( tbmin[ 1 ] < this->Bounds[ 2 ] ) this->Bounds[ 2 ] = tbmin[ 1 ];
            if( tbmax[ 1 ] > this->Bounds[ 3 ] ) this->Bounds[ 3 ] = tbmax[ 1 ];
            if( tbmin[ 2 ] < this->Bounds[ 4 ] ) this->Bounds[ 4 ] = tbmin[ 2 ];
            if( tbmax[ 2 ] > this->Bounds[ 5 ] ) this->Bounds[ 5 ] = tbmax[ 2 ];
        }

    }
public:

    /// Constructor
    static  vtkOpenGLGlyphMapper* New()
    {
        return new  vtkOpenGLGlyphMapper;
    }

    /// Overridden Draw method
    /// @note: this code is executed only once to generate the
    /// display list in RenderPiece method;
    virtual int Draw( vtkRenderer* ren, vtkActor* a )
    {

        TransformPtrListIterator i = transforms_.begin();
        TransformPtrListIterator e = transforms_.end();
        for( ; i != e; ++i )
        {
            ::glPushMatrix(); // <-- can be removed specifying starting point and deltas
            ::glMultMatrixd( i->m );
            vtkOpenGLPolyDataMapper::Draw( ren, a );
            ::glPopMatrix(); // <-- can be removed specifying starting point and deltas
        }

        return 0;
    }
    /// Add transform: transform is computed from orientation.
    void Add( double* p1, double* p2 )
    {
        vtkTransform* t = ComputeTransform( p1, p2 );
        UpdateBounds( t );
        GLMatrix m;
        CopyTransformToGLMatrix( t, m.m );
        transforms_.push_back( m );
    }
    /// Recompute specific transform from orientation.
    void Set( int i, const double* p1, const double* p2, double centerDistance = 0., bool scale = false )
    {
        assert( i >= 0 && i < transforms_.size() );
        vtkTransform* t = ComputeTransform( p1, p2, centerDistance, scale );
        //UpdateBounds( t );
        GLMatrix m;
        CopyTransformToGLMatrix( t, transforms_[ i ].m );
    }
    /// Add translation transform.
    void Add( double* p1 )
    {
        vtkTransform* t = ComputeTransform( p1 );
        UpdateBounds( t );
        GLMatrix m;
        CopyTransformToGLMatrix( t, m.m );
        transforms_.push_back( m );
    }
    /// Allocate space for transforms.
    void Allocate( int s )
    {
        transforms_.resize( s );
    }
    /// Returns bounds.
    double* GetBounds()
    {
        return this->Bounds;
    }
};

#endif /*VTKOPENGLGLYPHMAPPER_H_*/
