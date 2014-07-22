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
/// a spatial partitioning scheme will be addeded into openFile to speed up
/// computation of bonds.
/// @warning ChemPDBImporter does not read CONECT fields and therefore it
/// always requires computation of bonds.


#include <ChemKit2/ChemData.h>
#include <ChemKit2/ChemAssociatedData.h>
#include <ChemKit2/SbResidue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <Inventor/SbPList.h>
#include <limits>
#include <queue>
#include <vector>
#include <algorithm>
#include <functional>


///@note the following include is required to access valence information which OpenMOIV doesn't have
#include "ElementTable.h"

#include "MolekelChemPDBImporter.h"
#include "UniformGrid.h"

using namespace std;

//------------------------------------------------------------------------------
MolekelChemPDBImporter::MolekelChemPDBImporter()
{
}

//------------------------------------------------------------------------------
MolekelChemPDBImporter::~MolekelChemPDBImporter()
{
}

//------------------------------------------------------------------------------
namespace
{
    /// Atom information stored in grid structure.
    struct AtomInfo
    {
        int atomIndex; ///< Atom index.
        float x, y, z; ///< Atom position
        /// Constructor.
        AtomInfo( int i, float cx, float cy, float cz )
        : atomIndex( i ), x( cx ), y( cy ), z( cz )
        {}
        /// Default constructor.
        AtomInfo() : atomIndex( -1 ), x( 0 ), y( 0 ), z( 0 ) {}
    };
}

//------------------------------------------------------------------------------
bool MolekelChemPDBImporter::openFile ( const char* filename, ChemData* chemdata,
                                        ChemAssociatedData* chemassociateddata ) const
{
    int i,j;
    char s[255];
    char label[80];
    int32_t numAtoms, numBonds;

    SbStringList resList;
    SbStringList asciiInfo;

    FILE *fp = fopen(filename,"r");

    if (fp==0) return false;

    /// Retrieve number of atoms
    numAtoms=0;
    while (!feof(fp))
    {
	   fgets(s,255,fp);
	   if (strlen(s)==0) continue;
       if (strncmp(s,"END",3)==0) break;
	   if (strncmp(s,"ATOM",4)==0 || strncmp(s,"HETATM",6)==0) numAtoms++;
	   if (strncmp(s,"HELIX",5)==0 || strncmp(s,"TURN",4)==0 || strncmp(s,"SHEET",5)==0)
	   {
		  resList.append(new SbString(s));
		  continue;
	   }
    }

    // --> change initial bond space allocation
    //  numBonds=numAtoms*4;
    numBonds=(numAtoms+1)*4;
    // <-- change initial bond space allocation
    /// Rewind
    rewind(fp);

    chemdata->numberOfAtoms.setValue(numAtoms);

    // For fast filling of the chemData fields, first set the size of the
    // ChemData fields.
    chemdata->atomicNumber.setNum(numAtoms);
    chemdata->atomId.setNum(numAtoms);
    chemdata->atomName.setNum(numAtoms);
    chemdata->atomIndex.setNum(numAtoms);
    chemdata->atomCoordinates.setNum(numAtoms);
    chemassociateddata->asciiData.setNum(numAtoms);
    chemassociateddata->binaryData.setNum(numAtoms);

    // Then "startEditing()" the fields
    short    *atomicNumber    = chemdata->atomicNumber.startEditing();
    int32_t  *atomId          = chemdata->atomId.startEditing();
    SbString *atomName        = chemdata->atomName.startEditing();
    int32_t  *atomIndex       = chemdata->atomIndex.startEditing();
    SbVec3f  *atomCoordinates = chemdata->atomCoordinates.startEditing();
    SbString *asciiData       = chemassociateddata->asciiData.startEditing();

    // allocate memory for atoms positions and initialize pointers to beginning
    // of coordinate vectors
    float *x  = new float [numAtoms];
    float *y  = new float [numAtoms];
    float *z  = new float [numAtoms];
    float *t  = new float [numAtoms];
    short *at = new short [numAtoms];
    float *xptr  = &x[0];
    float *yptr  = &y[0];
    float *zptr  = &z[0];
    float *tptr  = &t[0];
    short *atptr = &at[0];


    int numsec = 0;

    i = 0;

    float minX = numeric_limits< float >::max();
    float maxX = numeric_limits< float >::min();
    float minY = numeric_limits< float >::max();
    float maxY = numeric_limits< float >::min();
    float minZ = numeric_limits< float >::max();
    float maxZ = numeric_limits< float >::min();
    while (!feof(fp))
    {
        fgets(s,255,fp);

        if (strlen(s)==0) continue;
        if (strncmp(s,"END",3)==0) break;
        if (strncmp(s,"ATOM",4)!=0 && strncmp(s,"HETATM",6)!=0) continue;

        parseATOMLine(s, (*atptr), (*xptr), (*yptr), (*zptr), (*tptr), label);

        asciiInfo.append(new SbString(label));

        atomicNumber[i]    = (*atptr);
        atomIndex[i]       = (*atptr);
        atomId[i]          = i+1;
        atomName[i]        = theElements[(*atptr)];
        atomCoordinates[i] = SbVec3f((*xptr), (*yptr), (*zptr));
        asciiData [i]      = SbString (label);
        if( *xptr < minX ) minX = *xptr;
        if( *xptr > maxX ) maxX = *xptr;
        if( *yptr < minY ) minY = *yptr;
        if( *yptr > maxY ) maxY = *yptr;
        if( *zptr < minZ ) minZ = *zptr;
        if( *zptr > maxZ ) maxZ = *zptr;
        xptr++;
        yptr++;
        zptr++;
        tptr++;
        atptr++;
        i++;
    }
    fclose(fp);
    chemdata->atomicNumber.finishEditing();
    chemdata->atomId.finishEditing();
    chemdata->atomName.finishEditing();
    chemdata->atomIndex.finishEditing();
    chemdata->atomCoordinates.finishEditing();
    chemassociateddata->asciiData.finishEditing();
    // slightly increase size of bounding box
    minX -= 3;
    minY -= 3;
    minZ -= 3;
    maxX += 3;
    maxY += 3;
    maxZ += 3;
    // grid that will hold information about atom positions
    UniformGrid< AtomInfo > grid( 3.f, minX, minY, minZ, maxX, maxY, maxZ );
    typedef UniformGrid< AtomInfo >::ProximityIterator< AtomInfo > GridIterator;
    // add atom information in grid
    for( int a = 0; a != numAtoms; ++a )
    {
        grid.Add( AtomInfo( a, x[ a ], y[ a ], z[ a ] ), x[ a ], y[ a ], z[ a ] );
    }

    // Thermic info must be scaled in order to put then as uint_8
    float thermax=-1000.0f;
    float thermin= 1000.0f;

    tptr = &t[0];
    for (i=0;i<numAtoms;i++)
    {
        if ((*tptr)>thermax) thermax=(*tptr);
        if ((*tptr)<thermin) thermin=(*tptr);
	    tptr++;
    }

    // UV Iterate through atoms: add each atom id and position into
    // regular grid

    float thercoef = 0;
    if (thermax-thermin != 0.0f) thercoef=255.0f/(thermax-thermin);

    uint8_t  *binaryData = chemassociateddata->binaryData.startEditing();
    tptr = &t[0];
    for (i=0;i<numAtoms;i++)
    {
        binaryData[i]    = (uint8_t) (((*tptr)-thermin)*thercoef);
	    tptr++;
    }
    chemassociateddata->binaryData.finishEditing();

    // Start processing bonds
    std::vector< int > bf; bf.reserve( 2 * numAtoms );
    std::vector< int > bt; bt.reserve( 2 * numAtoms );
    std::vector< int > bi; bi.reserve( 2 * numAtoms );
    std::vector< int > by; by.reserve( 2 * numAtoms );

    std::vector< int > atomBonds( numAtoms );
    std::fill( atomBonds.begin(), atomBonds.end(), 0 );

    i=0;
    float Xdif, Ydif, Zdif, dist, cutoff;
    for( int k = 0; k < numAtoms; ++k )
    {
        // UV
        // Use uniform grid to iterate on atoms which lie in the same
        // bucket as atom 'k' or in the neighboring buckets, this means
        // iterating through 27 buckets with each bucket containing only
        // a few atoms: if every bucket contains e.g. 5 atoms at most
        // 135 distance computations will be performed.
        // The cost of building the grid is one allocation for the entire
        // grid and one constant time insertion per atom.
        // The cost of searching in the grid given a point is:
        // - one constant time operation to select the search volume
        // - one linear time operation: 27 *  <number of atoms per cell>

        typedef const MolekelElement* ElementTable;
        ElementTable elements = GetElementTable();

        // iterate through elements in grid cells and put atoms in priority queue
        // ordered by increasing distance
        GridIterator gridIterator = grid.Begin( x[ k ], y[ k ], z[ k ], 0 );
        GridIterator gridEndIterator = grid.End( gridIterator );
        std::priority_queue< std::pair< double, int >,
                             std::vector< std::pair< double, int > >,
                             std::greater< std::pair< double, int > > > tmpBonds;
        for( ; gridIterator != gridEndIterator; ++gridIterator )
        {
            if( gridIterator->atomIndex == k ) continue;
            Xdif = x[k] - gridIterator->x; if( ( fabs( Xdif ) > AXIAL_CUTOFF ) ) continue;
            Ydif = y[k] - gridIterator->y; if( ( fabs( Ydif ) > AXIAL_CUTOFF ) ) continue;
            Zdif = z[k] - gridIterator->z; if( ( fabs( Zdif ) > AXIAL_CUTOFF ) ) continue;

            dist = sqrtf( ( Xdif ) * ( Xdif ) +
                          ( Ydif ) * ( Ydif ) +
                          ( Zdif ) * ( Zdif ) );


            cutoff = 1.2f * (covRadius[at[ gridIterator->atomIndex ]]+covRadius[at[k]]);
            if( dist < cutoff)
            {
                tmpBonds.push( std::make_pair( dist, gridIterator->atomIndex ) );
            }
        }

        // iterated through priority queue and add bonds if valence and angle constraints
        // are met
        while( tmpBonds.size() )
        {
            std::pair< double, int > p = tmpBonds.top();
            tmpBonds.pop();

            // limit the minimum angle between bonds:
            // 1) check distance between closest atom (p) and next closest atom (p2)
            // 2) if distance is less than distance between current atom (k) and next closest atom (p2)
            //    remove next closest atom (p2) from queue
            if( tmpBonds.size() )
            {
                const std::pair< double, int > p2 = tmpBonds.top();
                // compute squared distance between two closest atoms
                const double xd = x[ p2.second ] - x[ p.second ];
                const double yd = y[ p2.second ] - y[ p.second ];
                const double zd = z[ p2.second ] - z[ p.second ];
                const double d = xd * xd + yd * yd + zd * zd;
//debug statements
//                printf( "k: %d(%s)\t p1: %d(%s)\t p2: %d(%s)\t d1: %f\t d2: %f\t d12: %f\t\n",
//                         k, elements[ atomicNumber[ k ] ].symbol,
//                         p.second, elements[ atomicNumber[ p.second ] ].symbol,
//                         p2.second, elements[ atomicNumber[ p2.second ] ].symbol,
//                         p.first, p2.first, sqrt( d ) );fflush( stdout );
                // compare distance with distance between next closest atom and current atom,
                if( d < p2.first * p2.first ) tmpBonds.pop();
            }

            if( atomBonds[ k ] >= int( elements[ atomicNumber[ k ] ].maxBondValence ) ) break;

            if( atomBonds[ p.second ] >=
                    int( elements[ atomicNumber[ p.second ] ].maxBondValence ) ) continue;

            // discard if index < current atom index: it means this bond has already been
            // added into bond list
            if( p.second < k ) continue;

            // add bond to from and to lists
            bf.push_back( k );
            bt.push_back( p.second );
            // increase number of bonds for atoms
            ++atomBonds[ k ];
            ++atomBonds[ p.second ];
            // add bond index to index array
            bi.push_back( i );
            /// @todo add code to compute bond multiplicity
            // set bond multiplicity
            by.push_back( 1 );
            ++i;
        }
    }
    delete [] x;
    delete [] y;
    delete [] z;
    delete [] at;

    numBonds = i;

    chemdata->numberOfBonds.setValue(numBonds);
    chemdata->bondFrom.setNum(numBonds);
    chemdata->bondTo.setNum(numBonds);
    chemdata->bondType.setNum(numBonds);
    chemdata->bondIndex.setNum(numBonds);

    int32_t  *bondFrom        = chemdata->bondFrom.startEditing();
    int32_t  *bondTo          = chemdata->bondTo.startEditing();
    int32_t  *bondType        = chemdata->bondType.startEditing();
    int32_t  *bondIndex       = chemdata->bondIndex.startEditing();

    for (i=0;i<numBonds;i++)
    {
        bondFrom [i]  = bf[i];
        bondTo   [i]  = bt[i];
        bondIndex[i]  = bi[i];
        bondType [i]  = by[i];
    }

    chemdata->bondFrom.finishEditing();
    chemdata->bondTo.finishEditing();
    chemdata->bondType.finishEditing();
    chemdata->bondIndex.finishEditing();

  // UV XXX fix indentation from here
  // parse residues
  SbString line,resname,atomname,chain;
  int id=-1,numberId=0;
  for (i=0;i<numAtoms;i++)
  {
	  line = *asciiInfo[i];
	  resname = line.getSubString(8,10);
	  chain = line.getSubString(11,12);
	  atomname = line.getSubString(4,7);
	  numberId = atoi(line.getSubString(line.getLength()-3).getString());

	  if (id>=0)//avoid silly searching, check old resId
	  {
		  if (!(chemdata->residues[id].getNumber()==numberId &&
			  chemdata->residues[id].getChain()==chain.getString()[0]))
			id=-1;
	  }

	  if (id<0)
	  {
		  for (j=0;j<chemdata->residues.getNum();j++)
		  {
			  if (chemdata->residues[j].getNumber()==numberId &&
				  chemdata->residues[j].getChain()==chain.getString()[0])
			  {
				  id=j;
				  break;
			  }
		  }
	  }

	  if (id==-1) //not found
	  {
		  SbResidue myResidue;
		  myResidue.setName(resname);
		  myResidue.setChain(chain.getString()[0]);
		  myResidue.setNumber(numberId);

		  short j;
		  for (j=0;j<_RESNUM;j++)
		  {
			  if (strncmp(stdName3[j],resname.getString(),3)==0)
			  {
				  myResidue.setIndex(j);
				  break;
			  }
		  }
		  chemdata->residues.set1Value(chemdata->residues.getNum(),myResidue);
		  id=chemdata->residues.getNum()-1;
	  }
		// store indexes of CA and O atoms (C3' and C4' in DNA)
	  if (atomname ==" CA "     ||
		atomname ==" C3*"     ||
		atomname =="CA  "     ||    // mol2
		atomname =="C3* "           // mol2
								   )
		{
			chemdata->residues[id].addAtomIndex(i,SbResidue::RESIDUE_CONTROL_POINT_CA);
		}
		else
		if (atomname ==" O  "     ||
		atomname ==" C4*"     ||
		atomname =="O   "     ||   // mol2
		atomname =="C4* "          // mol2
								   )
		{
			chemdata->residues[id].addAtomIndex(i,SbResidue::RESIDUE_CONTROL_POINT_O);
		}
		else
			chemdata->residues[id].addAtomIndex(i);

  }
  chemdata->numberOfResidues.setValue(chemdata->residues.getNum());

  struct residueRange
  {
	  char ichain;
	  int itype;
	  int ibegin;
	  int iend;
  };

  residueRange* ranges = 0;
  if (resList.getLength()>0)
	ranges = new residueRange[resList.getLength()];

  // assign types
  for (i=0;i<resList.getLength();i++)
  {
	  line = *resList[i];
	  if (strncmp(line.getString(),"HELIX",5)==0)
	  {
		ranges[i].ichain = line.getString()[19];
		ranges[i].itype = SbResidue::RESIDUE_TYPE_HELIX;
		ranges[i].ibegin = atoi(line.getSubString(21,24).getString());
		ranges[i].iend   = atoi(line.getSubString(33,36).getString());
	  }
	  else if (strncmp(line.getString(),"SHEET",5)==0)
	  {
		ranges[i].ichain = line.getString()[21];
		ranges[i].itype = SbResidue::RESIDUE_TYPE_SHEET;
		ranges[i].ibegin = atoi(line.getSubString(22,25).getString());
		ranges[i].iend   = atoi(line.getSubString(33,36).getString());
	  }
	  else if (strncmp(line.getString(),"TURN",4)==0)
	  {
		ranges[i].ichain = line.getString()[19];
		ranges[i].itype = SbResidue::RESIDUE_TYPE_TURN;
		ranges[i].ibegin = atoi(line.getSubString(15,18).getString());
		ranges[i].iend   = atoi(line.getSubString(26,29).getString());
	  }
  }

  for (i=0;i<chemdata->numberOfResidues.getValue();i++)
  {
	  for (j=0;j<resList.getLength();j++)
	  {
		    if (chemdata->residues[i].getChain() == ranges[j].ichain &&
				chemdata->residues[i].getNumber()>= ranges[j].ibegin &&
				chemdata->residues[i].getNumber()<  ranges[j].iend)
			{
				chemdata->residues[i].residueType.setValue(ranges[j].itype);
				break;
			}

	  }
  }

  if (ranges!=0)
  {
	  delete[] ranges;
	  ranges=0;
  }

  // fill residue names table
  chemdata->residueName.setNum(_RESNUM);
  for (i=0;i<_RESNUM;i++)
	  chemdata->residueName.set1Value(i,stdName3[i]);

  //chemdata->residueName.setValues(0,_RESNUM,(const SbString*)stdName3);crash?

  // fill indices
  chemdata->residueColorIndex.setNum(_RESNUM);
  for (i=0;i<_RESNUM;i++)
	  chemdata->residueColorIndex.set1Value(i,i);

  return true;
}

//------------------------------------------------------------------------------
//! Parses a single line of the PDB file
/*! Parses a PDB line which contains keywords ATOM
    or HETATM. Looks for the atom number, the atom
    cartesian coordinates and the AA label in the
    case of a protein with alfa carbons. Also will
    label the WAT or HOH types. It could be enlarged
    to recognize other residue types.
*/
void MolekelChemPDBImporter::parseATOMLine (char* s, short &atnum, float &x,
                                            float &y, float &z, float &therm,
                                            char* Label) const
{
  char  line[200];
  char  AALabel[20];
  char  atom[3];
  char  t[10];
  char  *sp;
  char  l12,l13;
  int   ilen;

  strncpy (line, s, 199);

  if ((strlen(line)) > 78) {
     if(line[76]==' ') {
        atom[0] = line[77];
        atom[1] = '\0';
     }
     else {
        atom[0] = line[76];
        if(line[76] == 'H' || line[76] == 'D') {
          atom[1]='\0';               // Hidrogen or deuterium are not right justified
        }
        else {
          atom[1] = line[77];
          atom[2] = '\0';
        }
     }

     for (ilen = 1; ilen < 104; ilen++) {
        if (!strcmp (atom, theElements[ilen] )) { atnum = ilen; break;}
     }
     if(!strcmp (atom, "D")) {
        atnum = 1;
        ilen  = 1;
     } //It's a Deuterium

  }
  else {
     ilen = 104;
  } // If length of line is less than 78 looks for atom type in position 13-14

  if (ilen == 104) {
     l12 = line[12];
     l13 = line[13];
     if(l12==' ') {
        atom[0] = l13;
        atom[1] = '\0';
     }
     else {
        atom[0] = l12;
             if(l12 == 'H' || l12 == 'D') {
               atom[1]='\0';
        }
        else if (l12 == '1' || l12 == '2' || l12 == '3') {
               atom[0] = l13;
               atom[1] = '\0';
        }
        else {
               atom[1] = l13;
               atom[2] = '\0';
        }
     }
     for (ilen = 1; ilen < 104; ilen++) {
        if (!strcmp (atom, theElements[ilen] )) {
           atnum = ilen;
           break;
        }
     }

     if(!strcmp (atom, "D")) {
        atnum = 1;
        ilen = 1;
     } // It's a Deuterium

     if (ilen==104) {
        atnum = 6;
     }  // Default Value is Carbon
  }

  // atom number in column 8-11
  AALabel[0] = line[7];
  AALabel[1] = line[8];
  AALabel[2] = line[9];
  AALabel[3] = line[10];

  // atom name in column 13-15
  AALabel[4] = line[12];
  AALabel[5] = line[13];
  AALabel[6] = line[14];
  AALabel[7] = line[15];

  // res name in column 18-20
  AALabel[8] = line[17];
  AALabel[9] = line[18];
  AALabel[10] = line[19];

  // chain in column 22
  AALabel[11] = line[21];

  // res number in column 24-26
  AALabel[12] = line[23];
  AALabel[13] = line[24];
  AALabel[14] = line[25];

  // end
  AALabel[15] = '\0';

  strcpy(Label,AALabel);

  // Gets the coordinates
  sp=line+30; strncpy( t, sp, 9 ); t[8]='\0';
  x = float( atof(t) );
  sp=line+38; strncpy( t, sp, 9 ); t[8]='\0';
  y = float( atof(t) );
  sp=line+46; strncpy( t, sp, 9 ); t[8]='\0';
  z = float( atof(t) );

  // Gets the Thermal factor, here some programs can put chemiometric properties
  // This is stored in positions 61-66 as a float 6.2 (PDB Contents Guide v.2.1)
  if (strlen(line)>65) {
     sp=line+60; strncpy (t, sp, 6); t[6]='\0';
     therm=float(atof(t));
  }
  else {
     therm=0.0f;
  }
}

//====================================================================================
//====================================================================================
//HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
//------------------------------------------------------------------------------------
// Had to add definition code for SbResidue and MFResidue here, if
// not we get an error at link time.
//====================================================================================
//====================================================================================
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// SbResidue class
//
//////////////////////////////////////////////////////////////////////////////

SbResidue::SbResidue()
{
    index = -1;
    number = -1;
    chain = 0;
    name = "";
    residueType.setValue(SbResidue::RESIDUE_TYPE_DEFAULT);
    atomIndices.setNum(2);
    atomIndices.set1Value(RESIDUE_CONTROL_POINT_CA,-1);
    atomIndices.set1Value(RESIDUE_CONTROL_POINT_O,-1);
}

SbResidue::~SbResidue()
{
    atomIndices.deleteValues(0,-1);
}

SbResidue::SbResidue(const SbResidue &source)
{
    setValue(source);
}

void SbResidue::setValue(const SbResidue &source)
{
    number = source.number;
    name = source.name;
    index = source.index;
    chain = source.chain;
    residueType.setValue(source.residueType.getValue());
    atomIndices.copyFrom(source.getAtomIndices());
}

void SbResidue::addAtomIndex(const int32_t index, int cp/* = RESIDUE_CONTROL_POINT_NONE*/)
{
    if (cp == RESIDUE_CONTROL_POINT_NONE)
        atomIndices.set1Value(atomIndices.getNum(),index);
    else
        atomIndices.set1Value(cp,index);
}

bool SbResidue::removeAtomIndex(const int32_t index)
{
    SoMFInt32 newAtomIndices;
    int i;
    bool bFound = false;
    for (i=0; i<atomIndices.getNum(); i++)
    {
        if (atomIndices[i] == index)
        {
            bFound = true;
            if (i == RESIDUE_CONTROL_POINT_CA || i == RESIDUE_CONTROL_POINT_O)
                newAtomIndices.set1Value(i,-1);
        }
        else
            newAtomIndices.set1Value(newAtomIndices.getNum(),atomIndices[i]);
    }

    if (bFound)
    {
        atomIndices.copyFrom(newAtomIndices);
    }

    return bFound;
}

SbString SbResidue::getName() const
{
    return name;
}

void SbResidue::setName(const SbString &newName)
{
    name = newName;
}

short SbResidue::getNumber() const
{
    return number;
}

void SbResidue::setChain(const unsigned char newChain)
{
    chain = newChain;
}

unsigned char SbResidue::getChain() const
{
    return chain;
}

void SbResidue::setNumber(const short newNumber)
{
    number = newNumber;
}

short SbResidue::getIndex() const
{
    return index;
}

void SbResidue::setIndex(const short newIndex)
{
    index = newIndex;
}

const SoMFInt32& SbResidue::getAtomIndices() const
{
    return atomIndices;
}

bool SbResidue::checkControlPoints() const
{
    return (atomIndices[0] != -1 && atomIndices[1] != -1);
}

int SbResidue::search(const char *atomName, const ChemData *chemData) const
{
    for (int i=0; i<atomIndices.getNum(); i++)
    {
        const char *string = chemData->getAtomName(atomIndices[i]).getString();

        if (strcmp(atomName,string) == 0)
        {
            return atomIndices[i];
        }
    }

    return -1;
}

SbResidue & SbResidue::operator =(const SbResidue &source)
{
    setValue(source);
    return *this;
}

  // Equality comparison operator
int operator ==(const SbResidue &a, const SbResidue &b)
{
    if (a.number != b.number ||  (a.index != b.index) || (a.name!= b.name) || (a.chain!= b.chain))
        return 0;
    const int anum = a.getAtomIndices().getNum();
    const int bnum = b.getAtomIndices().getNum();

    if ( anum != bnum) return 0;

    for (int i=0; i<anum; i++)
    {
        if (a[i] != b[i])
            return 0;
    }

    return 1;
}

int operator !=(const SbResidue &a, const SbResidue &b)
{
    return !(a==b);
}

bool SbResidue::computeCenterOfMass(const ChemData* chemData, SbVec3f &cm) const
{
    bool anyPoint = false;
    int i,nPoints;
    nPoints = 0;
    cm.setValue(0.0f,0.0f,0.0f);
    for (i=0;i<atomIndices.getNum(); i++)
    {
        if (atomIndices[i]  == -1) continue;
        anyPoint = true;
        nPoints++;
        cm += chemData->getAtomCoordinates(atomIndices[i]);
    }

    if (nPoints > 1) cm /= (float)nPoints;

    return anyPoint;
}

//////////////////////////////////////////////////////////////////////////////
//
// SFResidue class
//
//////////////////////////////////////////////////////////////////////////////

SO_SFIELD_SOURCE(SFResidue, SbResidue, const SbResidue &);

SbBool sfresidue_read_value(SoInput * in, SbResidue & val)
{
  return false;
}

SbBool SFResidue::readValue(SoInput * in)
{
    return false;
}

void sfresidue_write_value(SoOutput * out, const SbResidue &val)
{
}

void SFResidue::writeValue(SoOutput * out) const
{
  sfresidue_write_value(out, this->getValue());
}

//////////////////////////////////////////////////////////////////////////////
//
// MFResidue class
//
//////////////////////////////////////////////////////////////////////////////

SO_MFIELD_SOURCE(MFResidue, SbResidue, const SbResidue &);

SbBool MFResidue::read1Value(SoInput *in, int index)
{
    SbResidue val;
  if (!sfresidue_read_value(in, val)) return FALSE;
  this->set1Value(index, val);
  return TRUE;
}

void MFResidue::write1Value(SoOutput *out, int index) const
{
    sfresidue_write_value(out, (*this)[index]);
}
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//====================================================================================
//====================================================================================









