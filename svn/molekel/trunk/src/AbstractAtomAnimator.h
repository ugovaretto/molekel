#ifndef ABSTRACTATOMANIMATOR_H_
#define ABSTRACTATOMANIMATOR_H_
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

// Inventor
#include <Inventor/fields/SoMFVec3f.h>

// MOIV
#include <ChemKit2/ChemData.h>

// STD
#include <ctime>
#include <vector>

#include "MolekelMolecule.h"
#include "old/molekeltypes.h"
#include <openbabel/mol.h>
#include <openbabel/obiter.h>

/// Abstract base class for atom animators i.e. classes that change
/// the positions of atoms.
class AbstractAtomAnimator : public IMoleculeUpdater
{
public:
    /// Animation mode: vibration, trajectory, mixed = trajectory + animation.
    typedef enum { VIBRATION, TRAJECTORY, MIXED } AnimationMode;
private:
    AnimationMode animationMode_;
    /// Reference to molecule whose atoms are being animated.
    MolekelMolecule* mol_;
    /// Temporary storage space for atom coordinates: atom coordinates are
    /// saved when animation starts, changed during animation and restored
    /// when animation stops.
    SoMFVec3f atomCoordinates_;
    /// Temporary storage space for Molekel Molecule's atoms.
    MolekelAtomList molekelAtoms_;
    /// Temporary storage for OBMol data.
    /// @warning OBMol copy constructor is broken: doesn't copy all the data
    /// i.e. is not a copy constructor, no API doc on how to safely clone a molecule,
    /// no swap method; only safe way is to save all the coordinates into an array.
    typedef std::vector< double > AtomCoordVector;
    AtomCoordVector obCoordinates_;
    /// Enable disable animation: animation does not start/progress if this
    /// variable is false.
    bool enable_;
    /// Timestep in milliseconds.
    unsigned int timestep_;
    /// Last time atom positions were updated.
    unsigned int lastUpdateTime_;
    /// If true both the coordinates used for graphical representation of atoms
    /// and the actual atom position are updated; if false only the graphical
    /// representation is updated.
    bool updateMoleculeData_;
public:
    /// Updates atom positions.
    virtual void UpdateAtomsPosition( bool forward ) = 0;
    /// Constructor.
    AbstractAtomAnimator() : animationMode_( VIBRATION ), mol_( 0 ), enable_( true ),
                             timestep_( 0 ), lastUpdateTime_( 0 ), updateMoleculeData_( true )
                             {}
    /// Returns reference to atom coordinates.
    const SoMFVec3f& GetAtomCoords() const { return atomCoordinates_; }
    /// Sets molecule to animate. @note setting this value while animation is
    /// in progress has undefined behavior.
    void SetMolecule( MolekelMolecule* m ) { mol_ = m; }
    /// Returns reference to molecule.
    MolekelMolecule* GetMolecule() const { return mol_; }
    /// Initialize animation; saves atom positions.
    void Init()
    {
        assert( mol_ && "NULL molecule" );
        atomCoordinates_.copyFrom( mol_->GetChemData()->atomCoordinates );
        if( mol_->GetMolekelMolecule() ) molekelAtoms_ = mol_->GetMolekelMolecule()->Atoms;
        /// @warning OBMol copy constructor is broken: doesn't copy all the data
        /// i.e. is not a copy constructor, no API doc on how to safely clone a molecule, no swap method.
        /// Only safe way is to save all the coordinates into an array
        obCoordinates_.reserve( mol_->GetOpenBabelMolecule()->NumAtoms() * 3 );
        /// @warning cannot use FOR_ATOMS_OF_MOL since there is no namespace prefix
        /// in front of OBMolAtomIter! true of all the macros defined in obiter.h
        for( OpenBabel::OBMolAtomIter a( *mol_->GetOpenBabelMolecule() );
             a; a++ ) /// @warning OBMolAtomIter: only postfix ++ operator implemented!
        {
            obCoordinates_.push_back( a->GetCoordinate()[ 0 ] );
            obCoordinates_.push_back( a->GetCoordinate()[ 1 ] );
            obCoordinates_.push_back( a->GetCoordinate()[ 2 ] );
        }
        lastUpdateTime_ = 0;
    }
    /// Update animation, called to advance to the next frame.
    /// @note note that animation will actually advance to the next frame
    /// if and only if <current time> - <last update time> >= <time step>.
    void Update( bool forward )
    {
        const unsigned int t = ( unsigned int ) ( 1000 * ( double( std::clock() ) /  CLOCKS_PER_SEC ) );
        const unsigned int dt =  t - lastUpdateTime_;

        if( dt < timestep_ ) return;
        lastUpdateTime_ = t;
        UpdateAtomsPosition( forward );
    }
    /// Reset: restores original atom positions.
    void Reset()
    {
        assert( mol_ && "NULL molecule" );
        mol_->GetChemData()->atomCoordinates.copyFrom( atomCoordinates_ );
        if( mol_->GetMolekelMolecule() ) mol_->GetMolekelMolecule()->Atoms = molekelAtoms_;
        AtomCoordVector::size_type i = 0;
        /// @warning cannot use FOR_ATOMS_OF_MOL since there is no namespace prefix
        /// in front of OBMolAtomIter! true of all the macros defined in obiter.h
        for( OpenBabel::OBMolAtomIter a( *mol_->GetOpenBabelMolecule() );
             a; a++ ) /// @warning OBMolAtomIter: only postfix ++ operator implemented!
        {
            a->SetVector( obCoordinates_[ i     ],
                          obCoordinates_[ i + 1 ],
                          obCoordinates_[ i + 2 ] );
            i += 3;
        }
        lastUpdateTime_ = 0;
    }
    //@{ Enable/Disable animator.
    void SetEnable( bool e ) { enable_ = e; }
    bool GetEnable() const { return enable_; }
    //@}
    /// Sets animation mode.
    virtual void SetAnimationMode( AnimationMode m ) { animationMode_ = m; }
    /// Return animation mode.
    AnimationMode GetAnimationMode() const { return animationMode_; }
    /// Sets timestep (milliseconds)
    void SetTimeStep( unsigned int ts ) { timestep_ = ts; }
    /// Returns timestep.
    unsigned int GetTimeStep() const { return timestep_; }
    /// Enable/disable update of actual atom coordinates.
    void UpdateMoleculeData( bool on ) { updateMoleculeData_ = on; }
    /// Returns true if molecule data are being changed by animator; false
    /// if only the data used for graphic rerendering are being modified.
    bool UpdateMoleculeData() const { return updateMoleculeData_; }
    /// Default implementation: simply calls Update();
    virtual void Next() { Update( true ); }
    /// @todo implement following methods:
    virtual void Previous() { Update( false ); }
    ///	virtual void Begin() = 0;
    ///	virtual void End() = 0;

protected:
    /// Updates actual atom position.
    void UpdateMoleculeAtom( int i, double x, double y, double z )
    {
        if( mol_->GetMolekelMolecule() != 0 )
        {
            mol_->GetMolekelMolecule()->Atoms[ i ].coord[ 0 ] = x;
            mol_->GetMolekelMolecule()->Atoms[ i ].coord[ 1 ] = y;
            mol_->GetMolekelMolecule()->Atoms[ i ].coord[ 2 ] = z;
        }
        if( mol_->GetOpenBabelMolecule() != 0 )
        {
            mol_->GetOpenBabelMolecule()->GetAtom( i + 1 )->SetVector( x, y, z );
        }
    }
};




#endif /*ABSTRACTATOMANIMATOR_H_*/
