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

#include <ChemKit2/ChemData.h>
#include <ChemKit2/ChemAssociatedData.h>

#include <openbabel/mol.h>

#include "ElementTable.h"

using namespace std;
using namespace OpenBabel;


//------------------------------------------------------------------------------
void OpenBabelToMOIV( OBMol* mol, ChemData* chemdata, ChemAssociatedData* chemassociateddata)
{

    int numAtoms, numBonds;

    numAtoms = mol->NumAtoms();

    chemdata->numberOfAtoms.setValue( numAtoms );

    // For fast filling of the chemData fields, first set the size of the
    // ChemData fields.
    chemdata->atomicNumber.setNum( numAtoms );
    chemdata->atomId.setNum( numAtoms );
    chemdata->atomName.setNum( numAtoms );
    chemdata->atomIndex.setNum( numAtoms );
    chemdata->atomCoordinates.setNum( numAtoms );
    chemassociateddata->asciiData.setNum( numAtoms );
    chemassociateddata->binaryData.setNum( numAtoms );

    // Then "startEditing()" the fields
    short    *atomicNumber    = chemdata->atomicNumber.startEditing();
    int32_t  *atomId          = chemdata->atomId.startEditing();
    SbString *atomName        = chemdata->atomName.startEditing();
    int32_t  *atomIndex       = chemdata->atomIndex.startEditing();
    SbVec3f  *atomCoordinates = chemdata->atomCoordinates.startEditing();
    SbString *asciiData       = chemassociateddata->asciiData.startEditing();


    for( int a = 0; a != numAtoms; ++a )
    {
        OBAtom* atom         = mol->GetAtom( a + 1 );
        atomicNumber[ a ]    = atom->GetAtomicNum();
        atomIndex[ a ]       = atomicNumber[a];
        atomId[ a ]          = a+1;
        atomName[ a ]        = GetElementTable()[ atomicNumber[ a ] ].symbol;
        atomCoordinates[ a ] = SbVec3f( float( atom->GetX() ), float( atom->GetY() ), float( atom->GetZ() ) );
    }

    chemdata->atomicNumber.finishEditing();
    chemdata->atomId.finishEditing();
    chemdata->atomName.finishEditing();
    chemdata->atomIndex.finishEditing();
    chemdata->atomCoordinates.finishEditing();
    chemassociateddata->asciiData.finishEditing();

    chemassociateddata->binaryData.finishEditing();

    numBonds = mol->NumBonds();

    chemdata->numberOfBonds.setValue( numBonds );
    chemdata->bondFrom.setNum( numBonds );
    chemdata->bondTo.setNum( numBonds );
    chemdata->bondType.setNum( numBonds );
    chemdata->bondIndex.setNum( numBonds );

    int32_t  *bondFrom        = chemdata->bondFrom.startEditing();
    int32_t  *bondTo          = chemdata->bondTo.startEditing();
    int32_t  *bondType        = chemdata->bondType.startEditing();
    int32_t  *bondIndex       = chemdata->bondIndex.startEditing();

    for(int i=0;i<numBonds;i++)
    {
        OBBond* bond   = mol->GetBond( i );
        bondFrom [ i ] = bond->GetBeginAtomIdx() - 1;
        bondTo   [ i ] = bond->GetEndAtomIdx() - 1;
        bondIndex[ i ] = i;
        int t = 1;
        if( bond->IsDouble() ) t = 2;
        else if( bond->IsTriple() ) t = 3;
        bondType [i]  = t;
    }

    chemdata->bondFrom.finishEditing();
    chemdata->bondTo.finishEditing();
    chemdata->bondType.finishEditing();
    chemdata->bondIndex.finishEditing();
}

//------------------------------------------------------------------------------
void OpenBabelToChemData( OBMol* mol, ChemData* chemdata )
{

    int numAtoms, numBonds;

    numAtoms = mol->NumAtoms();

    chemdata->numberOfAtoms.setValue( numAtoms );

    // For fast filling of the chemData fields, first set the size of the
    // ChemData fields.
    chemdata->atomicNumber.setNum( numAtoms );
    chemdata->atomId.setNum( numAtoms );
    chemdata->atomName.setNum( numAtoms );
    chemdata->atomIndex.setNum( numAtoms );
    chemdata->atomCoordinates.setNum( numAtoms );

    // Then "startEditing()" the fields
    short    *atomicNumber    = chemdata->atomicNumber.startEditing();
    int32_t  *atomId          = chemdata->atomId.startEditing();
    SbString *atomName        = chemdata->atomName.startEditing();
    int32_t  *atomIndex       = chemdata->atomIndex.startEditing();
    SbVec3f  *atomCoordinates = chemdata->atomCoordinates.startEditing();


    for( int a = 0; a != numAtoms; ++a )
    {
        OBAtom* atom         = mol->GetAtom( a + 1 );
        atomicNumber[ a ]    = atom->GetAtomicNum();
        atomIndex[ a ]       = atomicNumber[a];
        atomId[ a ]          = a+1;
        atomName[ a ]        = GetElementTable()[ atomicNumber[ a ] ].symbol;
        atomCoordinates[ a ] = SbVec3f( float( atom->GetX() ), float( atom->GetY() ), float( atom->GetZ() ) );
    }

    chemdata->atomicNumber.finishEditing();
    chemdata->atomId.finishEditing();
    chemdata->atomName.finishEditing();
    chemdata->atomIndex.finishEditing();
    chemdata->atomCoordinates.finishEditing();

    numBonds = mol->NumBonds();

    chemdata->numberOfBonds.setValue( numBonds );
    chemdata->bondFrom.setNum( numBonds );
    chemdata->bondTo.setNum( numBonds );
    chemdata->bondType.setNum( numBonds );
    chemdata->bondIndex.setNum( numBonds );

    int32_t  *bondFrom        = chemdata->bondFrom.startEditing();
    int32_t  *bondTo          = chemdata->bondTo.startEditing();
    int32_t  *bondType        = chemdata->bondType.startEditing();
    int32_t  *bondIndex       = chemdata->bondIndex.startEditing();

    for(int i=0;i<numBonds;i++)
    {
        OBBond* bond   = mol->GetBond( i );
        bondFrom [ i ] = bond->GetBeginAtomIdx() - 1;
        bondTo   [ i ] = bond->GetEndAtomIdx() - 1;
        bondIndex[ i ] = i;
        int t = 1;
        if( bond->IsDouble() ) t = 2;
        else if( bond->IsTriple() ) t = 3;
        bondType [i]  = t;
    }

    chemdata->bondFrom.finishEditing();
    chemdata->bondTo.finishEditing();
    chemdata->bondType.finishEditing();
    chemdata->bondIndex.finishEditing();
}

