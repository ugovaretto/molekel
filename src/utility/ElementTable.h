#ifndef ELEMENT_TABLE_H_
#define ELEMENT_TABLE_H_
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

/// Element table entry.
struct MolekelElement
{
    int atomicNumber;
    const char* symbol;
    double covalentRadius; // used for bond computation
    double bondOrderRadius;
    double vdwRadius; // Van der Waals radius, used for spacefill
    int maxBondValence;
    double mass;
    double electroNegativity;
    double ionizationPotential;
    double electronicAffinity;
    double red, green, blue; // color
    const char* name;
};

/// Returns a reference to the element table.
const MolekelElement* GetElementTable();

/// Returns element table size.
int GetElementTableSize();
#endif //ELEMENT_TABLE_H_