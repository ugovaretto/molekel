#ifndef LORENTZIAN_H_
#define LORENTZIAN_H_
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

#include <stdexcept>
#include <cassert>
#include <cmath>


/// Lorenztian-like function. Useful for approximating radiation spectra.
/// L( x ) = peak / ( 1 + c * ( x - center )^2 ).
/// - Max( L( x ) ) = peak at x = center.
/// - Half height is reached at x = +/- sqrt( 1/c ).
/// It is possible to set the ratio height over half-width-at-half-height
/// and have the constant multiplier automatically computed.
class Lorentzian
{
public:
	/// Default constructor.
	Lorentzian() : center_( 0.0 ), peak_( 1.0 ), halfHeight_( 1.0 ), c_( 1.0 ) {}
	/// Constructor.
	/// @param center coordinate at which L( center ) = peak
	/// @param peak max value of function
	Lorentzian( double center, double peak ) :
		center_( center ), peak_( peak ), halfHeight_( 1.0 ), c_( 1.0 ) {}
	/// Constructor.
	/// @param center coordinate at which L( center ) = peak
	/// @param peak max value of function
	/// @param halfHeight coordinate at which L( halfHeight ) = 0.5 * MAX
	Lorentzian( double center, double peak, double halfHeight ) :
		center_( center ), peak_( peak ), halfHeight_( halfHeight ), c_( 0.0 )
		
	{
		assert( halfHeight != 0.0 && "Zero half-height coordinate" );
		if( halfHeight == 0.0 ) throw std::domain_error( "Half-height coordinate is zero" );
		c_ = halfHeight * halfHeight;
		c_ = 1. / c_;
	}
	
	/// Returns center.
	double GetCenter() const { return center_; }
	/// Sets center coordinate.
	void SetCenter( double center ) { center_ = center; }
	
	/// Returns max function value.
	double GetPeak() const { return peak_; }
	/// Sets max function value. 
	void SetPeak( double peak ) { peak_ = peak; }
	
	/// Returns coordinate x at which L( x ) = 0.5 * peak. 
	double GetHalfHeight() const { return halfHeight_; }
	/// Sets coordinate x at which L( x ) = 0.5 * peak. 
	void SetHalfHeight( double hh )
	{
		assert( hh != 0.0 && "Zero half-height coordinate" );
		if( hh == 0.0 ) throw std::domain_error( "Zero half-height coordinate" );
		halfHeight_ = hh;
		c_ = 1.0 / halfHeight_ * halfHeight_;
	}
	
	/// Returns multiplier of ( x - center )^2 term.
	double GetC() const { return c_; }
	
	/// Sets multiplier of( x - center )^2 term.
	void SetC( double c )
	{
		assert( c != 0.0 );
		if( c == 0.0 ) throw std::domain_error( "Zero multiplier" );
		c_ = c;
		halfHeight_ = 1. / std::sqrt( c );
	}

	/// Sets height over half-width-at-half-height ratio. 
	void SetHeightHalfWidthRatio( double r )
	{
		if( peak_ == 0.0 ) return;		
		assert( r != 0.0 && "Height to half-width ratio is zero" );
		if( r == 0.0 ) std::domain_error( "Height to half-width ratio is zero" );
		c_ = std::abs( r / peak_ );
		c_ *= c_;
		halfHeight_ = std::abs( peak_ / r );
	}
	
	/// Evaluates function at specific coordinate.
	double Eval( double x ) const
	{
		x -= center_;
		return peak_ / ( 1.0 + c_ * x * x );
	}
	
	/// Forward calls to Evaluate(x).
	double operator=( double x ) const
	{
		return Eval( x );
	}
	
private:
	/// Center coordinate.
	double center_;
	/// Max function value.
	double peak_;
	/// Coordinate x at which L( x ) = 0.5 * peak. 
	double halfHeight_;
	/// Multiplier of ( x - center )^2 term.
	double c_;
};



#endif /*LORENTZIAN_H_*/
