#ifndef ATOMANIMATION_H_
#define ATOMANIMATION_H_
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

// STD
#include <map>
#include <cassert>
#include <list>
#include <algorithm>


//QT
#include <QMutex>
#include <QMutexLocker>

#include "AbstractAtomAnimator.h"
#include "old/molekeltypes.h"


namespace OpenBabel
{
    class OBMol;
}


//------------------------------------------------------------------------------
/// Animates atom vibration and optionally displays arrows representing the
/// istantaneous velocity; length of the arrows can be fixed or proportional
/// to speed.
/// @todo move option to display velocity vectors into base class since this
/// is a feature that applies to both vibration and trajectory.
class AtomVibrationAnimator : public AbstractAtomAnimator
{
public:
	
	friend class Locker;
	/// Locker class used to lock AtomVibrationAnimator instances.
	class Locker
	{
		/// References to AtomVibrationAnimator mutex.
		mutable QMutex& m_;
		
		/// Not default-constructible.
		Locker();
		
		/// Not assignable.
		Locker& operator =( const Locker& );
		
	public:
		/// Lock upon construction.
		Locker( const AtomVibrationAnimator& aa ) : m_( aa.indicesLock_ )
		{
			m_.lock();
		}
		/// Unlock upon destruction.
		~Locker() { m_.unlock(); }
	};
	
	
	/// Frequency indices.
	typedef std::list< int > ModeIndices; 
	
    /// Constructor.
    AtomVibrationAnimator() : 
                      freq_pos_( 0 ), step_( M_PI * 0.1 ), showArrows_( false ),
                      constantArrowLength_( true ), arrowScaling_( 1.0f )
                      {}
    /// Adds frequency by specifying position of frequency in frequency table
    /// stored inside molecule. @note value of vibration frequency is never used
    /// throughout new and old Molekel code.
    void AddVibrationMode( VibrationList::size_type i ) { indices_.push_back( i ); }
    /// Removes frequency index from frequency index list.
    void RemoveVibrationMode( VibrationList::size_type i )
    {
    	QMutexLocker idxLocker( &indicesLock_ );
    	indices_.erase( std::find( indices_.begin(), indices_.end(), i ) );
    }
    /// Removes all vibration mode indices.
    void RemoveAllVibrationModes()
    {
    	QMutexLocker idxLocker( &indicesLock_ );
    	indices_.clear();
    }
    /// Return step.
    float GetStep() const { return step_; }
    /// Set step.
    void SetStep( float s ) { step_ = s; }
    /// Sets visibility of vibration vectors.
    void SetVibrationVectorsVisibility( bool on ) { showArrows_ = on; }
    /// Returns visibility of vibration vectors.
    bool GetVibrationVectorsVisibility() const { return showArrows_; }
    /// Sets vibration arrow length to constant length: if true length is NOT
    /// proportional to speed, if set to false length IS proportional to speed.
    void SetConstantArrowLength( bool on ) { constantArrowLength_ = on; }
    /// Returns true if length of arrows fixed, false if proportional to speed.
    bool GetConstantArrowLength() const { return constantArrowLength_; }
    /// Sets arrow scaling factor.
    void SetArrowScalingFactor( float sf ) { arrowScaling_ = sf; }
    /// Returns arrow scaling factor.
    float GetArrowScalingFactor() const { return arrowScaling_; }
    /// Returns frequency indices used to compute atom positions.
    const ModeIndices& GetVibrationModeIndices() const { return indices_; }
    
private:
    /// Index of frequency (vibration mode ? ) being animated.
    ModeIndices indices_;
    /// Current value of variable used as argument of sin() funtions.
    float freq_pos_;
    /// Step.
    float step_;
    /// Show arrows if true.
    bool showArrows_;
    /// If true length of arrows is NOT proportional to speed.
    bool constantArrowLength_;
    /// Arrow scaling.
    float arrowScaling_;
    /// Mutex used to lock the indices while iterating or removing elements.
    mutable QMutex indicesLock_;
    /// Computes the factor by which vibration coordindates are multiplied.
    float ComputeFactor( float frpos, float sc_freq_ar ) const
    {
        return sc_freq_ar * sin( frpos ) / 2.5;
    }

    
    /// Returns delta to be added to atom position.
    /// @param coordIndex atom index.
    /// @param dp returned offset.
    void GetDeltaPos( int atomIndex, double dp[ 3 ] )
    {
    	dp[ 0 ] = 0.0; dp[ 1 ] = 0.0; dp[ 2 ] = 0.0;
    	ModeIndices::const_iterator i = indices_.begin();
    	const ModeIndices::const_iterator end = indices_.end();
    	for( ; i != end; ++i )
    	{
    		dp[ 0 ] += GetMolecule()->GetMolekelMolecule()->vibration[ *i ].coord[ atomIndex ].x;
    		dp[ 1 ] += GetMolecule()->GetMolekelMolecule()->vibration[ *i ].coord[ atomIndex ].y;
    		dp[ 2 ] += GetMolecule()->GetMolekelMolecule()->vibration[ *i ].coord[ atomIndex ].z;
    	}
    }	
    
    
    /// Overridden method called by Update().
    /// Computes new atom positions and optionally vectors tangent to atom trajectory
    void UpdateAtomsPosition( bool forward )
    {
    	QMutexLocker idxLocker( &indicesLock_ );
    	
    	if( indices_.empty() ) return;
        MolekelMolecule* mol = GetMolecule();
        const Molecule* mlkmol = mol->GetMolekelMolecule();
        if( !mlkmol ) return;
        
        // this allows to toggle arrows visibility while animation in progress.
        mol->SetVibrationVectorsVisibility( showArrows_ );
        //VibrationList::const_iterator v = mlkmol->vibration.begin();
        //advance( v, index_ );
        //if( v == mlkmol->vibration.end() ) return;

        if( forward ) freq_pos_ += step_;
        else freq_pos_ -= step_;

        const float factor = ComputeFactor( freq_pos_,  mlkmol->sc_freq_ar );
        SbVec3f* coords = mol->GetChemData()->atomCoordinates.startEditing();
        for( int i = 0; i != mol->GetChemData()->atomCoordinates.getNum(); ++i )
        {
        	double dp[ 3 ] = { 0., 0., 0. };
        	GetDeltaPos( i, dp );
            coords[ i ][ 0 ] = GetAtomCoords()[ i ][ 0 ] + factor * dp[ 0 ];
            coords[ i ][ 1 ] = GetAtomCoords()[ i ][ 1 ] + factor * dp[ 1 ];
            coords[ i ][ 2 ] = GetAtomCoords()[ i ][ 2 ] + factor * dp[ 2 ];

            if( UpdateMoleculeData() )
            {
                UpdateMoleculeAtom( i, coords[ i ][ 0 ], coords[ i ][ 1 ], coords[ i ][ 2 ] );
            }

            if( mol->GetVibrationVectorsVisibility() )
            {
                // tangent vector endpoints
                double start[ 3 ], end[ 3 ];
                const float beginFactor = ComputeFactor( freq_pos_ - step_, mlkmol->sc_freq_ar );
                const float endFactor = ComputeFactor( freq_pos_ + step_, mlkmol->sc_freq_ar );
                const float vibrationSpeed = endFactor - beginFactor;
                // tangent vector:
                // <Atom Positions at step = current step + 1> - <Atom Postions at step = current step - 1>
                // the length of this vector is proportional to the atom's speed.
                double tangentVector[ 3 ];
                tangentVector[ 0 ] = vibrationSpeed * dp[ 0 ];
                tangentVector[ 1 ] = vibrationSpeed * dp[ 1 ];
                tangentVector[ 2 ] = vibrationSpeed * dp[ 2 ];
                // start = current atom position
                start[ 0 ] = coords[ i ][ 0 ];
                start[ 1 ] = coords[ i ][ 1 ];
                start[ 2 ] = coords[ i ][ 2 ];
                // end = current atom position + tangent vector
                end[ 0 ] = start[ 0 ] + arrowScaling_ * tangentVector[ 0 ];
                end[ 1 ] = start[ 1 ] + arrowScaling_ * tangentVector[ 1 ];
                end[ 2 ] = start[ 2 ] + arrowScaling_ * tangentVector[ 2 ];
                mol->SetVibrationVector( i, start, end, !constantArrowLength_ );
            }
        }
        mol->GetChemData()->atomCoordinates.finishEditing();
        mol->RecomputeBBox();	
    }
    /// Overridden method: hide arrows and call base class method.
    void Reset()
    {
        AbstractAtomAnimator::Reset();
        GetMolecule()->SetVibrationVectorsVisibility( false );
    }
};

//------------------------------------------------------------------------------
/// Animates atom trajectories read from Molecule::dynamics::trajectory.
class AtomTrajectoryAnimator : public AbstractAtomAnimator
{
public:
    enum LoopMode { SWING, REPEAT, ONE_TIME };
    /// Constructor.
    AtomTrajectoryAnimator() : frame_( 0 ),
                               forward_( true ),
                               loopMode_( REPEAT ),
                               increment_( int() ),
                               /// @todo add speed support
                               speed_( float() ),
                               step_( 1 ) // currently same as speed == number of frames per time interval
                               {}
    /// Initialize.
    void Init()
    {
        InitFrames();
        AbstractAtomAnimator::Init();
    }
    /// Reset state.
    void Reset()
    {
        InitFrames();
        AbstractAtomAnimator::Reset();
    }
    //@{ Getters/Setters
    unsigned int GetFrame() const { return frame_; }
    void SetFrame( unsigned int frame )
    {
        frame_ = ComputeFrame( frame );
    }
    void SetForward( bool f ) { forward_ = f; }
    bool GetForward() const { return forward_; }
    void SetSpeed( float s ) { speed_ = s; }
    float GetSpeed() const { return speed_; }
    void SetLoopMode( LoopMode l ) { loopMode_ = l; }
    LoopMode GetLoopMode() const { return loopMode_; }
    int GetStep() const { return step_; }
    void SetStep( int s ) { step_ = s; }
    //@}

private:
    virtual void InitFrames() = 0;
    virtual unsigned int ComputeFrame( unsigned int ) const = 0;
    /// Current frame.
    unsigned int frame_;
    /// If true animate from frame zero to Max frame num - ;
    /// if false animate from Max frame num - 1 to zero.
    bool forward_;
    /// Frame increment.
    int increment_;
    /// Loop mode: SWING --> back and forth, REPEAT --> repeat, ONE_TIME
    LoopMode loopMode_;
    /// Speed.
    float speed_;
    /// Step == speed == number of frames per time interval
    int step_;
protected:
    //@{ Access to increment member from derived classes only.
    int GetIncrement() const { return increment_; }
    void SetIncrement( int i ) { increment_ = i; }\
    //@}
};




//------------------------------------------------------------------------------
/// Animates atom trajectories read from Molecule::dynamics::trajectory.
class AtomPositionAnimator : public AtomTrajectoryAnimator
{

private:

    /// Initialize current frame.
    void InitFrames()
    {
        const Molecule* mlkmol = GetMolecule()->GetMolekelMolecule();
        if( !mlkmol ) return;
        if( GetForward() ) SetFrame( 0 );
        else SetFrame( mlkmol->dynamics.ntotalsteps - 1 );
    }

    /// Return adjusted frame number.
    unsigned int ComputeFrame( unsigned int frame ) const
    {
        const Molecule* mlkmol = GetMolecule()->GetMolekelMolecule();
        if( !mlkmol ) return 0;
        if( !mlkmol->dynamics.trajectory ) return 0;
        return frame % mlkmol->dynamics.ntotalsteps;
    }

    /// Update atom positions.
    void UpdateAtomsPosition( bool forward )
    {
        MolekelMolecule* mol = GetMolecule();
        const Molecule* mlkmol = mol->GetMolekelMolecule();
        if( !mlkmol ) return;
        if( !mlkmol->dynamics.trajectory ) return;
        NextFrame( forward );
        Vector* v = mlkmol->dynamics.trajectory[ GetFrame() ];
        SbVec3f* coords = mol->GetChemData()->atomCoordinates.startEditing();
        for( int i = 0; i < mol->GetChemData()->atomCoordinates.getNum(); ++i )
        {
            coords[ i ][ 0 ] = v[ i ].x;
            coords[ i ][ 1 ] = v[ i ].y;
            coords[ i ][ 2 ] = v[ i ].z;
            if( UpdateMoleculeData() )
            {
                UpdateMoleculeAtom( i, v[ i ].x, v[ i ].y, v[ i ].z );
            }
        }
        mol->GetChemData()->atomCoordinates.finishEditing();
        mol->RecomputeBBox();	
    }
    /// Advance to next frame depending on current frame and
    /// preferences (swing, forward, onetime).
    /// @see frame_
    void NextFrame( bool forward )
    {
        MolekelMolecule* mol = GetMolecule();
        const Molecule* mlkmol = mol->GetMolekelMolecule();
        if( !mlkmol ) return;
        if( mlkmol->dynamics.ntotalsteps <= 1 ) return;
        int increment = GetForward() && forward ? 1 : -1;
        if( GetForward() && forward )
        {
            if( GetFrame() == ( mlkmol->dynamics.ntotalsteps - 1 ) )
            {
                if( GetLoopMode() == ONE_TIME ) return;
                if( GetLoopMode() == SWING ) SetIncrement( -1 );
                else
                {
                     SetFrame( 0 );
                     SetIncrement( 1 );
                }
            }
            else if( GetFrame() == 0 )
            {
                SetIncrement( 1 );
            }
            SetFrame( GetFrame() + GetIncrement() );
        }
        else // backward
        {
            if( GetFrame() == ( mlkmol->dynamics.ntotalsteps - 1 ) )
            {
                SetIncrement( -1 );
            }
            else if( GetFrame() == 0 )
            {
                if( GetLoopMode() == ONE_TIME ) return;
                if( GetLoopMode() == SWING ) SetIncrement( 1 );
                else
                {
                     SetFrame( mlkmol->dynamics.ntotalsteps - 1 );
                     SetIncrement( -1 );
                }
            }
            SetFrame( GetFrame() + GetIncrement() );
        }
    }
};

//------------------------------------------------------------------------------
/// Animates atom positions read from frames stored in multi-molecule file formats.
class AtomFramesAnimator : public AtomTrajectoryAnimator
{
private:

    /// Initialize current frame.
    void InitFrames()
    {
        if( GetForward() ) SetFrame( 0 );
        else SetFrame( GetMolecule()->GetNumberOfFrames() - 1 );
        GetMolecule()->SetOBMolToFrame( 0 );
    }

    /// Return adjusted frame number.
    unsigned int ComputeFrame( unsigned int frame ) const
    {
        return frame % GetMolecule()->GetNumberOfFrames();
    }

    /// Update atom positions.
    void UpdateAtomsPosition( bool forward )
    {
        if( GetMolecule()->GetNumberOfFrames() <= 1 ) return;
        NextFrame( forward );
        // each frames contains an OBMol instance: copy the data
        // to OpenMOIV
        OpenBabel::OBMol* obmol = GetMolecule()->GetOBMolAtFrame( GetFrame() );
        assert( obmol && "NULL frame" );
        GetMolecule()->CopyOBMolToChemData( obmol );
        GetMolecule()->SetOBMolToFrame( GetFrame() );
        GetMolecule()->RecomputeBBox();	
    }
    /// Advance to next frame depending on current frame and
    /// preferences (swing, forward, onetime).
    /// @see frame_
    void NextFrame( bool forward )
    {
        if( GetMolecule()->GetNumberOfFrames() <= 1 ) return;
        int increment = GetForward() && forward ? GetStep() : - GetStep();
        if( GetForward() && forward )
        {
            if( ( GetMolecule()->GetNumberOfFrames() - GetFrame() ) <= increment )
            {
                if( GetLoopMode() == ONE_TIME ) return;
                if( GetLoopMode() == SWING ) SetIncrement( -increment );
                else
                {
                     SetFrame( 0 );
                     SetIncrement( increment );
                }
            }
            else if( GetFrame() == 0 )
            {
                SetIncrement( increment );
            }
        }
        else // backward
        {
            if( ( GetMolecule()->GetNumberOfFrames() - GetFrame() ) <= increment )
            {
                SetIncrement( -increment );
            }
            else if( GetFrame() < increment )
            {
                if( GetLoopMode() == ONE_TIME ) return;
                if( GetLoopMode() == SWING ) SetIncrement( increment );
                else
                {
                     SetFrame( GetMolecule()->GetNumberOfFrames() - 1 );
                     SetIncrement( -increment );
                }
            }
        }

        SetFrame( GetFrame() + GetIncrement() );
    }
};




//------------------------------------------------------------------------------
/// Class holding containing a list of AbstractAtomAnimator objects only one
/// of which is active at any given time; animators are stored into a map and
/// owned by instance of this class.
template < class KeyType >
class SwitchAtomAnimator : public AbstractAtomAnimator
{
private:
    /// Animator map; KeyType is user defined.
    typedef std::map< KeyType, AbstractAtomAnimator* > ModeAnimatorMap;
    ModeAnimatorMap modeAnimatorMap_;
    /// Key of currently active animator.
    KeyType current_;
    /// Updates atom positions called from Update() method.
    void UpdateAtomsPosition( bool forward )
    {
        assert( modeAnimatorMap_.find( current_ ) != modeAnimatorMap_.end() );
        modeAnimatorMap_[ current_ ]->UpdateAtomsPosition( forward );
    }
public:
    /// Overridden init method: calls init on every animator in map.
    void Init()
    {
        assert( modeAnimatorMap_.find( current_ ) != modeAnimatorMap_.end() );
        modeAnimatorMap_[ current_ ]->Init();
    }
    /// Overridden reset method: calls reset on every animator in map.
    void Reset()
    {
        assert( modeAnimatorMap_.find( current_ ) != modeAnimatorMap_.end() );
        modeAnimatorMap_[ current_ ]->Reset();
    }
    /// Overridden set method: calls set method on every animator in map.
    void SetMolecule( MolekelMolecule* m )
    {
        AbstractAtomAnimator::SetMolecule( m );
        for( typename ModeAnimatorMap::iterator i = modeAnimatorMap_.begin();i != modeAnimatorMap_.end();++i )
        {
            i->second->SetMolecule( m );
        }
    }
    /// Add animator into database.
    void SetAnimator( AbstractAtomAnimator* aa, KeyType k )
    {
        if( modeAnimatorMap_.find( k ) != modeAnimatorMap_.end() )
        {
            delete modeAnimatorMap_[ k ];
        }
        modeAnimatorMap_[ k ] = aa;
    }
    /// Return animator from key.
    AbstractAtomAnimator* GetAnimator( KeyType k ) { return modeAnimatorMap_[ k ]; }
    /// Get active animator.
    AbstractAtomAnimator* GetCurrentAnimator() { return modeAnimatorMap_[ current_ ]; }
    /// Destructor: destroy objects in database.
    ~SwitchAtomAnimator()
    {
        for( typename ModeAnimatorMap::iterator i = modeAnimatorMap_.begin();i != modeAnimatorMap_.end();++i )
        {
            delete i->second;
        }
    }
    /// Set animation mode.
    void SetAnimationMode( AbstractAtomAnimator::AnimationMode am )
    {
        AbstractAtomAnimator::SetAnimationMode( am );
        SetCurrent( am ); // works if a conversion exist between
                          // AbstractAtomAnimator::AnimationMode an KeyType
    }
    /// Set active animator.
    void SetCurrent( KeyType k )
    {
        current_ = k;
        AbstractAtomAnimator::SetAnimationMode( k ); // works if a conversion exist between
                                                     // AbstractAtomAnimator::AnimationMode an KeyType
    }
    /// Returns key of active animator.
    KeyType GetCurrent() const { return current_; }
};


#endif /*ATOMANIMATION_H_*/
