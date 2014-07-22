#ifndef RAII_H_
#define RAII_H_
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


/// Simple class to support RAII techniques: constructor takes reference
/// to resource to acquire, initial value which is assigned to resource
/// right in the constructor and final value which is assigned to resource
/// in the destructor.
/// @todo add code to support thread safety similar to Loki threading
/// code:
/// - add ThreadingModel template parameter and derive ResourceHandler
///   from it
/// - implement set of threading model base classes
///
/// assignment will be performed by first acquiring a mutex or entering
/// a critical section through code like:
/// <code>
/// ...
/// Lock lock( res_ ); // acquire lock on resource.
/// res_ = <initial or final value>; // assign value.
/// ...
/// </code>
/// @todo consider adding a locking policy: currently this class
/// will assign a value in the constructor and a different value
/// in the desctructor without keeping the resource locked;
/// a locking policy would allow to decide if the resource has to
/// be locked between constructor/destructor invocations.
template < class T >
class ResourceHandler
{
    /// Reference to resource to initialize.
    T& res_;
    /// Final value assigned to resource in destructor.
    T finalValue_;
    /// Private constructor (not default constructible ).
    ResourceHandler();
    /// Private copy constructor ( not default copy constructible ).
    ResourceHandler( const ResourceHandler& );
    /// Private assignment operator (non assignable).
    ResourceHandler& operator=( const ResourceHandler& );
public:
    /// Constructor.
    /// @param res reference to resource to initialize
    /// @param initialValue initial value assigned to resource.
    /// @param finalValue value assigned to resource when destructor
    ///        invoked.
    ResourceHandler( T& res, T initialValue, T finalValue )
    : res_( res ), finalValue_( finalValue )
    {
        res_ = initialValue;
    }
    /// Destructor; assigns finalValue_ to resource.
    ~ResourceHandler() { res_ = finalValue_; }
};


#endif /*RAII_H_*/
