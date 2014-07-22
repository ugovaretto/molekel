#ifndef GEOMETRY_H_
#define GEOMETRY_H_
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

#include <cmath>

//----------------------------------------------------------------------
/// Subtract two 3D vectors.
template < class T >
inline void vsub( const T* v1, const T* v2, T* d )
{
    d[ 0 ] = v1[ 0 ] - v2[ 0 ];
    d[ 1 ] = v1[ 1 ] - v2[ 1 ];
    d[ 2 ] = v1[ 2 ] - v2[ 2 ];
}


//----------------------------------------------------------------------
/// Compute distance between two 3D points.
template < class T >
inline T vdist( const T* p1, const T* p2 )
{
    T x = p1[ 0 ] - p2[ 0 ];
    T y = p1[ 1 ] - p2[ 1 ];
    T z = p1[ 2 ] - p2[ 2 ];
    return std::sqrt( x*x + y*y + z*z );
}

//----------------------------------------------------------------------
/// Normalize 3D vector.
template < class T >
inline void vnorm( T* v )
{
    T n = std::sqrt( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
    if( n == 0.0 ) return;
    n = 1./n;
    v[ 0 ] *= n;
    v[ 1 ] *= n;
    v[ 2 ] *= n;
}

//----------------------------------------------------------------------
/// Inner product.
template < class T >
inline T vdot( const T* v1, const T* v2, T* d )
{
    return  v1[ 0 ] * v2[ 0 ] + v1[ 1 ] * v2[ 1 ] + v1[ 2 ] * v2[ 2 ];
}

//----------------------------------------------------------------------
/// Copy vector.
template < class T >
inline void vcopy( const T* src, T* dest )
{
    dest[ 0 ] = src[ 0 ];
    dest[ 1 ] = src[ 1 ];
    dest[ 2 ] = src[ 2 ];
}

//----------------------------------------------------------------------
/// Returns the angle between two vectors in radians.
template < class T >
inline T vangle( const T* v1, const T* v2, T* d )
{
    T n1[ 3 ];
    T n2[ 3 ];
    vcopy( v1, n1 );
    vcopy( v2, n2 );
    vnorm( n1 );
    vnorm( n2 );
    return std::acos( vdot( n1, n2 ) );
}

//------------------------------------------------------------------------------
/// Cross product.
template < class T >
inline void vcross( const T* v1, const T* v2, T* v )
{
    v[ 0 ] = v1[ 1 ] * v2[ 2 ] - v1[ 2 ] * v2[ 1 ];
    v[ 1 ] = - v1[ 0 ] * v2[ 2 ] + v2[ 0 ] * v2[ 2 ];
    v[ 2 ] = v1[ 0 ] * v2[ 1 ] - v1[ 2 ] * v2[ 0 ];
}


#endif /*GEOMETRY_H_*/
