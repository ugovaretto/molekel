#ifndef MOLECULEVTKCOMMAND_
#define MOLECULEVTKCOMMAND_
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
#include <vtkCommand.h>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkRenderWindowInteractor.h>

#include "MolekelData.h"

/// Interface derived from vtkCommand interface; Execute() method
/// has to be defined in derived classes.
/// Holds references to actor, interactor and molecule.
/// Instances of derived classes are supposed to be passed as observers
/// to MolekelMolecule::AddVtkObserver() e.g. :
/// @code
/// ...
/// MolekelMolecule* mol = molecules_->GetMolecule( i );
/// SelectMoleculeCB* smcb = new SelectMoleculeCB( this );
/// // MoleculeActorPickCommand implements MoleculeVtkCommand interface.
/// typedef MoleculeActorPickCommand MolActorCmd;
/// vtkSmartPointer< MolActorCmd > mc( MolActorCmd::New() );
///	mc->SetMoleculeId( i );
///	mc->SetCallback( smcb );
///	mc->SetRenderWindowInteractor( interactor );
///	mol->AddVtkObserver( vtkCommand::PickEvent, mc.GetPointer() );
/// ...
/// @endcode
class MoleculeVtkCommand : public vtkCommand
{
    /// Reference to actor.
    vtkSmartPointer< vtkActor > actor_;
    /// Reference to interactor.
    vtkSmartPointer< vtkRenderWindowInteractor > in_;
    /// Reference to molecule.
    MolekelData::IndexType moleculeId_;

public:
    /// Constructor.
    MoleculeVtkCommand() : actor_( 0 ), in_( 0 ), moleculeId_( MolekelData::InvalidIndex() ) {}
    /// Set refrence to actor instance.
    void SetActor( vtkActor* a ) { actor_ = a; }
    /// Set reference to molecule instance.
    void SetMoleculeId( MolekelData::IndexType id ) { moleculeId_ = id; }
    /// Returns actor reference.
    vtkActor* GetActor()   { return actor_; }
    /// Returns molecule reference.
    MolekelData::IndexType GetMoleculeId() const { return moleculeId_; }
    /// Returns interactor reference.
    vtkRenderWindowInteractor* GetRenderWindowInteractor() { return in_; }
    /// Set interactor.
    void SetRenderWindowInteractor( vtkRenderWindowInteractor* in ) { in_ = in; }
};


#endif /*MOLECULEVTKCOMMAND_*/
