#ifndef TIMER_H_
#define TIMER_H_
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
#include <ctime>

//------------------------------------------------------------------------------
/// Timer class that records current time when created and invokes function (object)
/// when destroyed.
/// The client supplied function is invoked in the class destructor and therefore it
/// should not throw an exception.
template < class ExpiredFunT >
class Timer
{
	/// Start time. Recorded at creation.
    const std::clock_t start_;
	/// Function invoked when Timer object destroyed.
    ExpiredFunT expiredFun_;

public:
	/// Constructor. Records current time.
	/// @param f timeout function; invoked when object destroyed. The difference
	/// between time at invokation and start time is passes as a parameter.
    Timer( ExpiredFunT f = ExpiredFunT() ) : start_( std::clock() ), expiredFun_( f ) {}
    ~Timer()
    {
        expiredFun_( double( std::clock() - start_ ) / CLOCKS_PER_SEC );
    }
};

//------------------------------------------------------------------------------
/// Stopwatch implementation.
class StopWatch
{
	/// Start time.
    double start_;
	/// Stop time.
    double stop_;
public:
	/// Default constructor; initializes start and stop time to same value.
    StopWatch() : start_( 0. ), stop_( 0. ) {}
	/// Record start time.
    void Start() { start_ = std::clock(); }
    /// Record stop time.
	void Stop()  { stop_ = std::clock();  }
    /// Return stop - start time difference.
	double GetElapsedTime()
    { return double( stop_ - start_ ) / CLOCKS_PER_SEC; }
    /// Return current time - start time difference.
	double GetCurrentElapsedTime()
    { return double( std::clock() - start_ ) / CLOCKS_PER_SEC; }
};
//------------------------------------------------------------------------------
#endif /*TIMER_H_*/
