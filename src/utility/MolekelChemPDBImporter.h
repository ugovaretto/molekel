#ifndef MOLEKELCHEMPDBIMPORTER_
#define MOLEKELCHEMPDBIMPORTER_
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

/// @note MolkelPDBImporter is copied from OpenMOIV's ChemPDBImporter,
/// a spatial partitioning scheme will be addeded into openFile to speed
/// up computation of bonds

#include <ChemKit2/util/ChemFileImporter.h>

class ChemData;
class ChemAssociatedData;

class MolekelChemPDBImporter: public ChemFileImporter
{
public:
	MolekelChemPDBImporter();
	~MolekelChemPDBImporter();

	virtual bool openFile(const char* filename, ChemData* chemdata, ChemAssociatedData* chemassociateddata) const;

private:
	void parseATOMLine (char* s, short &atnum, float &x, float &y, float &z, float &therm, char *Label) const;
};

#endif // MOLEKELCHEMPDBIMPORTER_
