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
#ifdef _MSC_VER
#include <openbabel/mol.h>
namespace OpenBabel
{
  //! Global OBElementTable for element properties
  OBElementTable   etab;
  //! Global OBTypeTable for translating between different atom types
  //! (e.g., Sybyl <-> MM2)
  OBTypeTable      ttab;
  //! Global OBIsotopeTable for isotope properties
  OBIsotopeTable   isotab;
  //! Global OBAromaticTyper for detecting aromatic atoms and bonds
  OBAromaticTyper  aromtyper;
  //! Global OBAtomTyper for marking internal valence, hybridization,
  //!  and atom types (for internal and external use)
  OBAtomTyper      atomtyper;
  //! Global OBChainsParser for detecting macromolecular chains and residues
  OBChainsParser   chainsparser;
  //! Global OBMessageHandler error handler
  OBMessageHandler obErrorLog;
  //! Global OBResidueData biomolecule residue database
  OBResidueData    resdat;
}
#endif