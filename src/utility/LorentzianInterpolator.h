#ifndef LORENTZIANINTERPOLATOR_H_
#define LORENTZIANINTERPOLATOR_H_
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

#include <limits>
#include "Lorentzian.h"

///1D interpolator using Lorentzian functions as basis functions
///and coordinates of functions peaks as control points.
///All the basis functions have the same height / half-width-at-half-height ratio.
class LorentzianInterpolator
{
public:
	/// Default constructor.
	LorentzianInterpolator() : heightWidthRatio_( 10.0 ),
		xmin_( std::numeric_limits< double >::max() ),
		xmax_( std::numeric_limits< double >::min() ),
		peak_min_( std::numeric_limits< double >::max() ),
		peak_max_( std::numeric_limits< double >::min() )
	{}
	
	/// Add control point.
	/// @param center center of Lorentzian basis function.
	/// @param peak max value of Lorentzian basis function (at center).
	void Add( double center, double peak )
	{
		if( center < xmin_ ) xmin_ = center;
		if( center > xmax_ ) xmax_ = center;
		if( peak < peak_min_ ) peak_min_ = peak;
		if( peak > peak_max_ ) peak_max_ = peak;
		
		Lorentzian l( center, peak );
		l.SetHeightHalfWidthRatio( heightWidthRatio_ );
		functions_.push_back( l );
	}
	
	/// Clear basis set. 
	void Clear()
	{
		xmin_ = std::numeric_limits< double >::max();
		xmax_ = std::numeric_limits< double >::min();
		peak_min_ = std::numeric_limits< double >::max();
		peak_max_ = std::numeric_limits< double >::min();
		functions_.clear();
	}
	
	/// Sets control points from forward iterators.
	/// @param xbegin iterator pointing at first x coordinate.
	/// @param xend iterator pointing at one past last x coordinate.
	/// @param peak_begin iterator pointing at first y coordinate.
	template < class FwdIterT >
	void SetData( FwdIterT xbegin, FwdIterT xend, FwdIterT peak_begin )
	{
		Clear();		
		for( ; xbegin != xend; ++xbegin, ++peak_begin ) Add( *xbegin, *peak_begin );
			
	}
	
	/// Returns height / half-width-at-half-height ratio common to all basis functions.
	double GetHeightWidthRatio() const { return heightWidthRatio_; }
	/// Sets height / half-width-at-half-height ratio.
	void SetHeightWidthRatio( double r )
	{
		assert( r != 0.0 && "Height width ratio is zero" );
		if( r == 0.0 ) throw std::domain_error( "Height width ratio is zero" );
		heightWidthRatio_ = r;
		LorentzianArray::iterator i = functions_.begin();
		const LorentzianArray::const_iterator e = functions_.end();
		for( ; i != e; ++i ) i->SetHeightHalfWidthRatio( heightWidthRatio_ );
	}
	
	/// Evaluates value at specific coordinate as sum of values returned by
	/// each basis function.
	double Eval( double x ) const
	{
		LorentzianArray::const_iterator i = functions_.begin();
		const LorentzianArray::const_iterator e = functions_.end();
		double r = 0.0;
		for( ; i != e; ++i ) r += i->Eval( x );
		return r;
	}
	
	/// Simply forwards call to Eval( x ).
	double operator=( double x ) const
	{
		return Eval( x );
	}
	
	/// Returns min x-coordinate value.
	double GetMinX() const { return xmin_; }
	/// Returns max x-coordinate value.
	double GetMaxX() const { return xmax_; }
	
	/// Returns min peak value.
	double GetMinPeak() const { return peak_min_; }
	/// Returns max peak value. @note This is not the actual global maximum
	/// for the interpolated functions since values are obtained by summing together
	/// several basis functions. 
	double GetMaxPeak() const { return peak_max_; }
	
private:
	/// Type for Array of Lorentzian-like basis functions.
	typedef std::vector< Lorentzian > LorentzianArray;
	/// Array of Lorentzian-like basis functions.
	LorentzianArray functions_;
	/// Height over half-width-at-half-height.
	double heightWidthRatio_;
	/// Min x.
	double xmin_;
	/// Max x.
	double xmax_;
	/// Min peak.
	double peak_min_;
	/// Max peak.
	double peak_max_;
	
};



#endif /*LORENTZIANINTERPOLATOR_H_*/
