#ifndef OBGRIDDATA_H_
#define OBGRIDDATA_H_
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


#include <openbabel/obmolecformat.h>
#include <openbabel/generic.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <cassert>
#include <string>

/// Class to store values for generic (non axis aligned) grids like
/// those read from Gaussian cube files.
class OBGridData : public OpenBabel::OBGenericData
{
public:
    /// Constructor assigns the values of type and attr protected data
    /// This values will be accessed through the GetDataType, HasData methods.
    OBGridData() : OpenBabel::OBGenericData()
    {
        _type = OpenBabel::OBGenericDataType::CustomData0;
        _attr = "GridData";
        min_ = std::numeric_limits< double >::max();
        max_ = std::numeric_limits< double >::min();
    }
    /// Units.
    typedef enum { BOHR, ANGSTROM, OTHER } Unit;
    /// Returns the three axes parallel to the grid edges the
    /// length of the returned vector is the step along that
    /// direction.
    void GetAxes( double x[ 3 ], double y[ 3 ], double z[ 3 ] ) const
    {
        x[ 0 ] = xAxis_[ 0 ]; x[ 1 ] = xAxis_[ 1 ]; x[ 2 ] = xAxis_[ 2 ];
        y[ 0 ] = yAxis_[ 0 ]; y[ 1 ] = yAxis_[ 1 ]; y[ 2 ] = yAxis_[ 2 ];
        z[ 0 ] = zAxis_[ 0 ]; z[ 1 ] = zAxis_[ 1 ]; z[ 2 ] = zAxis_[ 2 ];
    }
    /// Return number of points along the three axes paralled to the
    /// grid edges.
    void GetNumberOfPoints( int& nx, int& ny, int& nz ) const
    {
        nx = nx_;
        ny = ny_;
        nz = nz_;
    }

    /// Return number of points along the three axes paralled to the
    /// grid edges.
    void GetNumberOfSteps( int steps[ 3 ] ) const
    {
        steps[ 0 ] = nx_ - 1;
        steps[ 1 ] = ny_ - 1;
        steps[ 2 ] = nz_ - 1;
    }

    /// Return grid values as an array of doubles.
    const std::vector< double >& GetValues() const
    {
        return values_;
    }
    /// Returns point at position i, j, k in the grid.
    double GetValue( int i, int j, int k ) const
    {
        const int idx = ComputeIndex( i, j, k );
        return values_[ idx ];
    }

    /// Returns unit.
    Unit GetUnit() const { return unit_; }

    /// Returns min value.
    double GetMinValue() const
    {
        return min_;
    }

    /// Returns max value.
    double GetMaxValue() const
    {
        return max_;
    }

    /// Returns origin.
    //const double* GetOrigin() const { return &origin_[ 0 ]; }

    /// Returns origin.
    void GetOrigin( double o[ 3 ] ) const
    {
        o[ 0 ] = origin_[ 0 ];
        o[ 1 ] = origin_[ 1 ];
        o[ 2 ] = origin_[ 2 ];
    }

    /// Reserve data in value vector.
    void Reserve( int size ) { values_.reserve( size ); }

    /// Append value to value vector.
    void AddValue( double v )
    {
        values_.push_back( v );
        if( v < min_ ) min_ = v;
        if( v > max_ ) max_ = v;
    }

    /// Set number of points along the three axes.
    void SetNumberOfPoints( int nx, int ny, int nz )
    {
        nx_ = nx;
        ny_ = ny;
        nz_ = nz;
    }

    /// Set grid axes.
    void SetAxes( double x[ 3 ], double y[ 3 ], double z[ 3 ] )
    {
        xAxis_[ 0 ] = x[ 0 ]; xAxis_[ 1 ] = x[ 1 ]; xAxis_[ 2 ] = x[ 2 ];
        yAxis_[ 0 ] = y[ 0 ]; yAxis_[ 1 ] = y[ 1 ]; yAxis_[ 2 ] = y[ 2 ];
        zAxis_[ 0 ] = z[ 0 ]; zAxis_[ 1 ] = z[ 1 ]; zAxis_[ 2 ] = z[ 2 ];
    }

    /// Set value vector.
    void SetValues( const std::vector< double >& v )
    {
        values_ = v;
        min_ = *std::min_element( values_.begin(), values_.end() );
        max_ = *std::max_element( values_.begin(), values_.end() );
    }

    /// Set unit.
    void SetUnit( Unit u ) { unit_ = u; }

    /// Set origin.
    void SetOrigin( double o[ 3 ] )
    {
        origin_[ 0 ] = o[ 0 ];
        origin_[ 1 ] = o[ 1 ];
        origin_[ 2 ] = o[ 2 ];
    }

    /// Get label
    const std::string& GetLabel() const { return label_; }

    /// Set label
    void SetLabel( const std::string& label ) { label_ = label; }

private:
    ///Label used to identify grid in multi-grid files e.g. t41
    std::string label_;
    // @{ Axes
    double xAxis_[ 3 ];
    double yAxis_[ 3 ];
    double zAxis_[ 3 ];
    // @}
    //@{ Steps along axes
    int nx_;
    int ny_;
    int nz_;
    // @}
    /// Grid values.
    std::vector< double > values_;
    /// Unit of length.
    Unit unit_;
    /// Origin.
    double origin_[ 3 ];
    /// Min value.
    double min_;
    /// Max value.
    double max_;
    /// Return vector index given i, j, k grid coordinates.
    int ComputeIndex( int i, int j, int k ) const
    {
        assert( i >= 0 && i < nx_ &&
                j >= 0 && j < ny_ &&
                k >= 0 && k < nz_ &&
                "Grid index out of bounds" );
        return  k + nz_ *( j +  ny_ * i );
    }
};


#endif /*OBGRIDDATA_H_*/
