#ifndef SELECTION_H_
#define SELECTION_H_
//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//
// $Author$
// $Date$
// $Revision$
//

#include "MolekelData.h"
#include <deque>
#include <list>
#include <algorithm>

///Container storing a sequence of selection information object.
///This class implements a circular buffer whose size is set at construction time. Objects
///are inserted at the end of the buffer and removed from the front.
///The last N selections, where N is the buffer size, are stored into a circular buffer;
///the entire selection list is stored into a history data member.
class SelectionList
{
public:
    ///Holds selection data: type and handler of selected object.
    class SelectionInfo
    {
    public:
        ///Type of selected object.
        typedef enum { NULL_TYPE, ATOM, BOND, RESIDUE } Type;
        ///Molecule handler.
        typedef MolekelData::IndexType MoleculeId;
    public:
        ///Default constructor.
        SelectionInfo() : moleculeId_( MolekelData::InvalidIndex() ),
                          type_( NULL_TYPE ),
                          id_( -1 )
        {}
        ///Constructor.
        ///@param molid molecule handler.
        ///@param t selection type (atom, bond or residue).
        ///@param i index of selected object.
        SelectionInfo( MoleculeId molid, Type t, int i )
        : moleculeId_( molid ), type_( t ), id_( i )
        {}
        ///Returns molecule handler.
        MoleculeId GetMoleculeId() const { return moleculeId_; }
        ///Returns type of selected object.
        Type GetType() const { return type_; }
        ///Returns index of selected object.
        int GetId() const { return id_; }
        ///Equality.
        bool operator==( const SelectionInfo& si ) const
        {
            return moleculeId_ == si.moleculeId_ &&
                   type_       == si.type_       &&
                   id_         == si.id_;

        }
     private:
        ///Molecule handler.
        MoleculeId moleculeId_;
        ///Type of selected object.
        Type type_;
        ///Index of selected object.
        int id_;
    };
public:
    ///Const iterator type.
    typedef std::deque< SelectionInfo >::const_iterator ConstIterator;
    ///Iterator type.
    typedef std::deque< SelectionInfo >::size_type IndexType;
    ///Constructor.
    SelectionList( int size = 4 ) : size_( size ) {}
    ///Adds selection into buffer.
    void Add( const SelectionInfo& si )
    {
        if( std::find( list_.begin(), list_.end(), si ) != list_.end() ) return;
        if( list_.size() == size_ ) list_.pop_front();
        list_.push_back( si );
        history_.push_back( si );
    }
    ///Adds selection into buffer if selection not present; removes selection otherwise.
    ///@return true if selection object added, false if removed.
    bool AddRemove( const SelectionInfo& si )
    {
        std::list< SelectionInfo >::iterator li = std::find( history_.begin(), history_.end(), si );
        std::deque< SelectionInfo >::iterator i = std::find( list_.begin(), list_.end(), si );
        if( li != history_.end() )
        {
            history_.erase( li );
        }
        if( i == list_.end() && li != history_.end() ) return false;
        if( i != list_.end() )
        {
            list_.erase( i );
            return false;
        }
        if( list_.size() == size_ ) list_.pop_front();
        list_.push_back( si );
        history_.push_back( si );
        return true;
    }
    ///Clear lists: removes all selection objects.
    void Clear() { list_.clear(); history_.clear(); }
    ///Returns number of selections in list.
    int GetSize() const { return list_.size(); }
    ///Returns selection.
    const SelectionInfo& GetSelectionInfo( IndexType i ) const { return list_[ i ]; }
    ///Returns const iterator to beginning of selection list.
    ConstIterator Begin() const { return list_.begin(); }
    ///Returns const iterator to end of selection list.
    ConstIterator End() const { return list_.end(); }
    ///Random access operator to element at index i.
    const SelectionInfo& operator[]( IndexType i ) const { return GetSelectionInfo( i ); }
private:
    ///Sequence container holding SelectionInfo objects.
    std::deque< SelectionInfo > list_;
    ///Size of circular buffer.
    int size_;
    ///History. Used to keep track of all the selection added into the list.
    std::list< SelectionInfo > history_;
};




#endif /*SELECTION_H_*/
