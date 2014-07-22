#ifndef UNIFORMGRID_H_
#define UNIFORMGRID_H_
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

#include <vector>
#include <cassert>
#include <utility>

#include <cstdio>

/// Uniform grid.
/// Objects are stored into grid cells: it is possible to iterate over
/// a sequence of objects lying in the cells intersected by sphere
/// specified with a center point and a radius.
template < class ObjectT, class NumT = float > class UniformGrid
{
private:

    /// Non copiable.
    UniformGrid( const UniformGrid& );
    /// Non assignable.
    UniformGrid& operator=( const UniformGrid& );

public:
    //--------------------------------------------------------------------------
    /// Grid cell: stores a sequence of objects and gives access to elements in
    /// the sequence through iterators.
    template < class T > class Element
    {
        std::vector< T > objects_;
     public:
        void Add( const T& obj ) { objects_.push_back( obj ); }
        void Clear() { objects_.swap( std::vector< T >() ); }
        typedef typename std::vector< T >::iterator Iterator;
        typedef typename std::vector< T >::const_iterator ConstIterator;
        Iterator Begin() { return objects_.begin(); }
        Iterator End() { return objects_.end(); }
        ConstIterator Begin() const { return objects_.begin(); }
        ConstIterator End() const { return objects_.end(); }
    };

    //--------------------------------------------------------------------------
    /// Constructor.
    /// @param cellSise size of cell edge length
    /// @param minX x coordinate of back bottom left corner of bounding box
    /// @param minY y coordinate of back bottom left corner of bounding box
    /// @param minZ z coordinate of back bottom left corner of bounding box
    /// @param maxX x coordinate of front top left corner of bounding box
    /// @param maxY y coordinate of front top left corner of bounding box
    /// @param maxZ z coordinate of front top left corner of bounding box
    UniformGrid( NumT cellSize,
                 NumT minX, NumT minY, NumT minZ,
                 NumT maxX, NumT maxY, NumT maxZ ) :
                 cellSize_( cellSize ),
                 minX_( minX ), minY_( minY ), minZ_( minZ ),
                 maxX_( maxX ), maxY_( maxY ), maxZ_( maxZ ),
                 dx_( 0. ), dy_( 0. ), dz_( 0. ),
                 nx_( 0 ), ny_( 0 ), nz_( 0 )
    {
        assert( cellSize_ > 0. );
        assert( minX_ < maxX_ );
        assert( minY_ < maxY_ );
        assert( minZ_ < maxZ_ );
        dx_ = maxX_ - minX_;
        dy_ = maxY_ - minY_;
        dz_ = maxZ_ - minZ_;
        nx_ = int( dx_ / cellSize_ + NumT( 0.5f ) );
        ny_ = int( dy_ / cellSize_ + NumT( 0.5f ) );
        nz_ = int( dz_ / cellSize_ + NumT( 0.5f ) );
        numCells_ = nx_ * ny_ * nz_;
        grid_ = new Element< ObjectT >[ numCells_ ];
        // option: allocate ( nx + 2 ) * ( ny + 2 ) * ( nz + 2 ) cells;
        // this will allow to simplify the search code since in current
        // case code has to check for boundary conditions; the problem
        // is that by doing so the allocated memory increases dramatically
        // e.g. 10 x 10 x 10 = 1000; 12 x 12 x 12 = 1720!
    }

    /// Destructor: releases memory allocated for grid cells.
    ~UniformGrid() { delete [] grid_; }
    /// Add object into grid.
    bool Add( const ObjectT& obj, NumT x, NumT y, NumT z )
    {
        const int i = IndexFrom3DPoint( x, y, z );
        if( i < 0 ) return false;
        grid_[ i ].Add( obj );
        return true;
    }
    /// Return constant reference to cell given grid cell coordinates.
    const Element< ObjectT >& CellAt( int i, int j, int k ) const
    {
        const int idx = Index( i, j, k );
        assert( idx >= 0 && idx < numCells_ );
        return grid_[ idx ];
    }
    /// Return reference to cell given grid cell coordinates.
    Element< ObjectT >& CellAt( int i, int j, int k )
    {
        const int idx = Index( i, j, k );
        assert( idx >= 0 );
        return grid_[ idx ];
    }

    //--------------------------------------------------------------------------
    /// Proximity itrator: iterates over objects stored in specific cell and
    /// neighboring cells.
    template < class T > class ProximityIterator
    {
        mutable int i_; ///< current i
        mutable int j_; ///< current j
        mutable int k_; ///< current k
        int startI_; ///< start i index value
        int startJ_; ///< start j index value
        int startK_; ///< start k index value
        int endI_; ///< one past last i index value
        int endJ_; ///< one past last j index value
        int endK_; ///< one past last k index value
        UniformGrid< T, NumT >* grid_; ///< reference to grid
        /// Current iterator into object sequence.
        mutable typename Element< T >::Iterator ei_;
        /// One past the end of the object sequence.
        mutable typename Element< T >::Iterator end_;
        /// Set interators to start and end of object sequence.
        void InitIterators()
        {
            ei_  = grid_->CellAt( i_, j_, k_ ).Begin();
            end_ = grid_->CellAt( i_, j_, k_ ).End();
        }
    public:
        /// Constructor.
        /// @param g reference to grid.
        /// @param i i initial value
        /// @param j j initial value
        /// @param k k initial value
        /// @param endI one past final i value
        /// @param endJ one past final j value
        /// @param endk one past final k value
        /// @param end iterator pointing to one past the last
        /// object of the last sequence visited
        ProximityIterator( UniformGrid< T, NumT >* g,
                           int i, int j, int k,
                           int endI, int endJ, int endK,
                           typename Element< T >::Iterator end  )
            : grid_( g ), i_( i ), j_( j ), k_( k ),
              startI_( i ), startJ_( j ), startK_( k ),
              endI_( endI ), endJ_( endJ ), endK_( endK ),
              ei_( end ), end_( end )
        {
            assert( grid_ );
            if( startI_ != endI_ && startJ_ != endJ_ && startK_ != endK_ )
            {
                InitIterators();
                if( Empty( *this ) ) operator++();
            }
        }
        /// Returns true if current sequence is empty.
        friend bool Empty( const ProximityIterator< T >& pi )
        {
            return pi.ei_ == pi.end_;
        }

        /// Returns value of one past last i index.
        friend int GetEndI( const ProximityIterator< T >& pi ) { return pi.endI_; }

        /// Returns value of one past last j index.
        friend int GetEndJ( const ProximityIterator< T >& pi ) { return pi.endJ_; }

        /// Returns value of one past last k index.
        friend int GetEndK( const ProximityIterator< T >& pi ) { return pi.endK_; }

        /// Checks for equality of two proximity iterators.
        bool operator==( const ProximityIterator< T >& pi ) const
        {
            // minimal check, no check on grid
            return pi.i_ == i_ && pi.j_ == j_ && pi.k_ == k_ && ei_ == pi.ei_;
        }
        /// Checks for inequality of two proximity iterators.
        bool operator !=( const ProximityIterator< T >& pi ) const
        {
            return !operator==( pi );
        }
        /// Increment operator. Returns a reference to an iterator pointing to
        /// an object inside a grid cell within the range specified in the
        /// constructor.
        ProximityIterator< T >& operator++()
        {

            // equivalent to
            // for( k...
            //   for( j...
            //     for( i...
            //       for( object_iterator = objects_in_cell.begin...
            //

            // if object iterator not at end of sequence increment and check:
            // if not at end return this, if not call operator++() again
            if( ei_ != end_ )
            {
                ++ei_;
                if( ei_ == end_ ) return operator++();
                return *this;
            }
            // increment x index; if at the end of x range reset and
            // increment y index; if at the end of y range reset and
            // increment z index
            ++i_;
            if( i_ == endI_ )
            {
                i_ = startI_;
                ++j_;
                if( j_ == endJ_ )
                {
                    j_ = startJ_;
                    ++k_;
                }
            }
            // if z index at end of sequence set all indices at the end
            // of their range; this is required for further comparison to
            // iterator returned by UniformGrid::End( UniformGrid::ProximityIterator )
            if( k_ == endK_ )
            {
                 i_ = endI_;
                 j_ = endJ_;
                 ei_ = grid_->CellAt( endI_ - 1, endJ_ -1, endK_ -1 ).End();
                 end_ = ei_;
                 return *this;
            }
            // if we reach this point it means we are within the index ranges: set
            // start and end iterators to beginning of object sequence at grid cell
            // {i_, j_, k_}
            InitIterators();
            // if object sequence at current grid cell is empty advance to next non empty
            // cell or end
            if( Empty( *this) ) operator++();
            return *this;
        }
        /// Dereference operator.
        T& operator*() { return *ei_; }
        /// Member access operator.
        T* operator->() { return &( *ei_ ); }
        /// Const dereference operator.
        const T& operator*() const { return *ei_; }
        /// Const member access operator.
        const T* operator->() const { return &(*ei_); }
    };
    //--------------------------------------------------------------------------
    /// Returns proximity iterator that will iterate over the objects in the cell
    /// containing the specified point and all the neighboring cells;
    /// @warning radius is currently not used.
    ProximityIterator< ObjectT > Begin( NumT x, NumT y, NumT z, NumT radius )
    {
        std::pair< int, int > xe;
        std::pair< int, int > ye;
        std::pair< int, int > ze;
        const bool noerror = GetCellExtent( x, y, z, radius, xe, ye, ze );
        assert( noerror && "Point out of bounds" );
        // point to one past last element
        ++xe.second;
        ++ye.second;
        ++ze.second;
        return ProximityIterator< ObjectT >( this,
                                             xe.first, ye.first, ze.first,
                                             xe.second, ye.second, ze.second,
                                             CellAt( xe.second - 1,
                                                     ye.second - 1,
                                                     ze.second - 1 ).End() );
    }
    /// Returns end iterator that points to one past the last object in the
    /// sequence iterated over by the iterator passed to this method.
    ProximityIterator< ObjectT > End( const ProximityIterator< ObjectT >& begin )
    {
        return ProximityIterator< ObjectT >( this,
                                             GetEndI( begin ), GetEndJ( begin ), GetEndK( begin ),
                                             GetEndI( begin ), GetEndJ( begin ), GetEndK( begin ),
                                             CellAt( GetEndI( begin ) - 1,
                                                     GetEndJ( begin ) - 1,
                                                     GetEndK( begin ) - 1 ).End() );
    }
    /// Returns the number of cells along the x axis.
    int GetNx() const { return nx_; }
    /// Returns the number of cells along the y axis.
    int GetNy() const { return ny_; }
    /// Returns the number of cells along the z axis.
    int GetNz() const { return nz_; }

private:
    /// Compute 1D index into element array given 3D index.
    int Index( int i, int j, int k ) const
    {
        return i + nx_ *( j +  ny_ * k );
    }
    /// Compute 3D indices of cell containing specified point.
    bool IndicesFrom3DPoint( NumT x, NumT y, NumT z,
                             int& i, int& j, int& k ) const
    {
        if( x < minX_ || x > maxX_ ||
            y < minY_ || y > maxY_ ||
            z < minZ_ || z > maxZ_ )
        {
            return false; // outside bounds
        }

        i = int( nx_ * ( x - minX_ ) / dx_ );
        j = int( ny_ * ( y - minY_ ) / dy_ );
        k = int( nz_ * ( z - minZ_ ) / dz_ );

        return true;

    }
    /// Compute 1D index into Element array given 3D point.
    int IndexFrom3DPoint( NumT x, NumT y, NumT z )
    {
        int i = -1;
        int j = -1;
        int k = -1;
        if( !IndicesFrom3DPoint( x, y, z, i, j, k ) ) return -1;
        return Index( i, j, k );
    }

    /// Return min and max indices of cells intersecting specified sphere.
    bool GetCellExtent( NumT x, NumT y, NumT z, NumT radius, // radius is not currently used: simply returs all neighbors
                        std::pair< int, int >& xExtent,
                        std::pair< int, int >& yExtent,
                        std::pair< int, int >& zExtent ) const

    {
        int i = -1;
        int j = -1;
        int k = -1;
        //Box b( x, y, z, radius ); // not quite yet
        // option pre-store nx_ - 1, ny_ - 1, nz_ -1 values
        if( !IndicesFrom3DPoint( x, y, z, i, j, k ) ) return false;
        xExtent.first  = std::max( i - 1, 0 );
        xExtent.second = std::min( i + 1, nx_ - 1 );
        yExtent.first  = std::max( j - 1, 0 );
        yExtent.second = std::min( j + 1, ny_ - 1 );
        zExtent.first  = std::max( k - 1, 0 );
        zExtent.second = std::min( k + 1, nz_ - 1 );
        return true;
    }

private:

    NumT cellSize_; ///< cell edge length
    NumT minX_; ///< min x value
    NumT minY_; ///< min y value
    NumT minZ_; ///< min z value
    NumT maxX_; ///< max x value
    NumT maxY_; ///< max y value
    NumT maxZ_; ///< maz z value
    NumT dx_; ///< length of bounding box along x axis
    NumT dy_; ///< length of bounding box along y axis
    NumT dz_; ///< length of bounding box along z axis
    int nx_; ///< number of cells along x axis
    int ny_; ///< number of cells along y axis
    int nz_; ///< number of cells along z axis
    int numCells_; ///< total number of cells
    /// 3D grid stored as a 1D array; memory is allocated in constructor
    /// and deallocated in destructor.
    Element< ObjectT >* grid_;
};


#endif /*UNIFORMGRID_H_*/
