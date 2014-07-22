#ifndef MOLECULEACTORPICKCOMMAND_H_
#define MOLECULEACTORPICKCOMMAND_H_
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

// VTK
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderWindowInteractor.h>

// Inventor
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/actions/SoHandleEventAction.h>
// OpenMOIV
#include <ChemKit2/ChemDetail.h>

// STD
#include <cassert>

#include "utility/vtkSoMapper.h"
#include "MolekelData.h"
#include "MoleculeVtkCommand.h"
#include "MoleculeCallback.h"

//------------------------------------------------------------------------------
/// Command invoked when a pick event is received.
/// VTK pick event is translated into a SoRayPickAction to determine which
/// molecule's feature (atom, bond, residue...) has been picked.
/// In order to highlight the picked feature through Open Inventor the event
/// is also translated into a sequence of mouse button up/down events which
/// will trigger highlighting.
/// Instances of this class have to be constructed through the New() factory
/// method as with any other vtkObject derived class.
/// @note it is cleaner to use to use ChemSelection::select( ChemPath* ) method;
/// in order to do that a DisplayChemPath instance has to be created manually and
/// passed to the select() method - investigate.
/// @warning the callback object passed to an instance of this class is destroyed
//  in the Delete() method.
class MoleculeActorPickCommand : public MoleculeVtkCommand
{
    /// Constructor.
    MoleculeActorPickCommand() : pickAction_( SbViewportRegion() ),
                                 cback_( 0 ),
                                 allowActorOnlyPick_( true ),
                                 pickAtoms_( false )
                                 {}

    /// Pick action used by Execute method to determine which object has been
    /// picked.
    SoRayPickAction pickAction_;
    /// Callback object to be invoked upon picking. @warning the callback object
    /// is owned by instances of MoleculeActorPickCommand.
    MoleculeCallback* cback_;
    /// Allow picking of bounding box. If this parameter is true the callback object
    /// will be called even no atom/bond/residue is picked. If this parameter id false
    /// the callback object will be invoked only if an atom/bond/residue has been picked.
    bool allowActorOnlyPick_;
    /// Enable/Disable atom/bond/residue selection. If this parameter is set to true
    /// the SoPickedPoint parameter passed to cback_->Execute() method will contain
    /// the picked point. If this parameter is set to false the SoPickedPoint parameter
    /// passed to cback_->Execute() will be set to NULL.
    bool pickAtoms_;
public:
    /// Sets callback object.
    void SetCallback( MoleculeCallback* cback ) { cback_ = cback; }
    /// Factory method to create an instance of this class.
    static MoleculeActorPickCommand *New() { return new MoleculeActorPickCommand(); }
    /// Enables/disables picking of molecule's bounding box.
    /// @see allowActorOnlyPick_.
    void SetAllowActorOnlyPick( bool ap ) { allowActorOnlyPick_ = ap; }
    /// Returns value of allowActorOnlyPick_ member.
    /// @see allowActorOnlyPick_.
    bool GetAllowActorOnlyPick() const { return allowActorOnlyPick_; }
    /// Enables/disables picking of individual atoms/bonds/residues.
    /// @see pickAtoms_
    void SetPickAtoms( bool pa ) { pickAtoms_ = pa; }
    /// Returns value of pickAtoms_ data member.
    bool GetPickAtoms() const { return pickAtoms_; }
    /// Overridden vtkCommand::Execute() method;
    /// invokes callback's Execute method if molecule picked.
    /// @code MoleculeVtkCommand::Execute( MolekelData::IndexType, const SoPickedPoint* ) @endcode
    /// is called when molecule picked; the SoPickedPoint parameter is NULL in case
    /// no atom/bond/residue is picked or picking of atoms/bonds/residues is disabled.
    /// @see SetAllowActorOnlyPick
    /// @see SetPickAtoms
    virtual void Execute( vtkObject *obj, unsigned long id, void *data )
    {
        assert( cback_ && "Callback is NULL" );
         // @warning enable exceptions and RTTI to have dynamic_cast throw a
         // bad_cast exception
        vtkSoMapper* m = dynamic_cast< vtkSoMapper* >( GetActor()->GetMapper() );
        // just in case exceptions are not turned on
        assert( m != 0 && "dynamic_cast< vtkSoMapper* >( GetActor()->GetMapper() )" );

        assert( GetMoleculeId() != MolekelData::InvalidIndex() && "Invalid molecule id" );
        assert( GetRenderWindowInteractor() != 0 && "NULL render window interactor" );
        pickAction_.setViewportRegion( m->GetViewportRegion() );
        int* ep = GetRenderWindowInteractor()->GetEventPosition();
        const int x = ep[ 0 ];
        const int y = ep[ 1 ];
        pickAction_.setPickAll( true );
        pickAction_.setPoint( SbVec2s( short( x ), short( y ) ) );
        pickAction_.setRadius( 2 );
        pickAction_.apply( m->GetRoot() );
        SoPickedPoint* pp = pickAction_.getPickedPoint();
		 if( pp )
		 {
			 //always select molecule
			 cback_->Execute( GetMoleculeId(), 0, false );
			 //then select atoms if required
			 if( pickAtoms_ )
			 {
				SoMouseButtonEvent me;
				vtkRenderWindowInteractor* wi = GetRenderWindowInteractor();
				const bool multipleSelection = wi->GetShiftKey() != 0;
				me.setShiftDown( multipleSelection );
				me.setState( SoButtonEvent::DOWN );
				me.setPosition( SbVec2s( short( x ), short( y ) ) );
				me.setButton( SoMouseButtonEvent::BUTTON1 );
				SoSeparator* root = ( SoSeparator* ) m->GetRoot()->getChild( 2 );
				ChemSelection* sel = ( ChemSelection* ) root->getChild( 0 );
				SoHandleEventAction eva( m->GetViewportRegion() );
				eva.setEvent( &me );
				eva.apply( m->GetRoot() );
				me.setState( SoButtonEvent::UP );
				me.setPosition( SbVec2s( short( x ), short( y ) ) );
				me.setButton( SoMouseButtonEvent::BUTTON1 );
				eva.apply( m->GetRoot() );
				cback_->Execute( GetMoleculeId(), pp, multipleSelection );
			 }
			 else cback_->Execute( GetMoleculeId(), 0, false );
		 }
         else if( allowActorOnlyPick_ ) cback_->Execute( GetMoleculeId(), 0, false );
    }

    /// Overridden delete method.
    /// @warning callback is OWNED by instances of this class.
    virtual void Delete()
    {
        delete cback_;
        MoleculeVtkCommand::Delete();
    }

};
//------------------------------------------------------------------------------
#endif /*MOLECULEACTORPICKCOMMAND_H_*/
