/*  MOLEKEL, Version 4.4, Date: 10.Dec.03
 *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
 *  (original IRIX GL implementation, concept and data structure
 *   by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
 *   and revisions by Stefan Portmann, CSCS/ETHZ)
 */

#include <cstdlib>
#include <iostream>
#include <cmath>

#include "constant.h"
#include "molekeltypes.h"

using namespace std;

Element element[ 105 ];

int InitAtoms()
{

extern const char* GetAtomsDataPtr();
extern const char* GetNextAtomPtr( const char* );

  const char* ptr = GetAtomsDataPtr();
  int ord = -1;
  Element *ep = 0;
  while( ptr = GetNextAtomPtr( ptr ) ) {
    sscanf(ptr , "%d", &ord);
    ep = element + ord;
    sscanf(ptr , "%d %s %f %f %hd %hd %f", &ord, ep->symbol,
        &ep->coval, &ep->rvdw, &ep->nbond, &ep->col, &ep->mass);
// Using the following code to test since data are not currently used anywhere.
//    printf( "\n=====\n%d %s %f %f %hd %hd %f",    ord, ep->symbol,
//        ep->coval, ep->rvdw, ep->nbond, ep->col, ep->mass );
  }
  return 1;
}

void free_dyna();

void showinfobox( const char* msg )
{
    cerr << msg << endl;
}

void update_logs() {}

void logprint( const char* m ) { showinfobox( m ); }

Molecule* add_mol( const char* fname )
{
    Molecule* mol = new Molecule;
    mol->fname = fname;
    return mol;
}

/* return the distance between two points */
float dist(float *a, float *b)
{
   return sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) +
               (a[2]-b[2])*(a[2]-b[2]));
}

void new_mole( Molecule* , const char* ) {}

void create_bonds( Molecule* ) {}
void find_multiplebonds( Molecule* ) {}

Dynamics dynamics;


void computeOccupations(Mol *mp)  /* only depending on nr. of electrons! */
{
   int i;

   if(!mp->alphaOrbital) return;

   for(i=1; i<=mp->nAlpha; i++) {
      if(i < mp->firstOrbital) continue;
      mp->alphaOrbital[i - mp->firstOrbital].occ = 1;
   }

   if(mp->alphaBeta) {
      if(!mp->betaOrbital) return;
      for(i=1; i<=mp->nBeta; i++) {
         if(i < mp->firstOrbital) continue;
         mp->betaOrbital[i - mp->firstOrbital].occ = 1;
      }
   }

   else {
      for(i=1; i<=mp->nBeta; i++) {
         if(i < mp->firstOrbital) continue;
         mp->alphaOrbital[i - mp->firstOrbital].occ += 1;
      }
   }
}

MolecularOrbital *allocOrbital(int nOrbitals, int nBasis, int flag)
{
   MolecularOrbital *orbital;
   register int i;

   if((orbital = (MolecularOrbital*) malloc(nOrbitals*sizeof(MolecularOrbital))) == NULL){
      return 0;
   }
   for(i=0; i<nOrbitals; i++) {
      if(nBasis) {
         if((orbital[i].coefficient = (double*) malloc(nBasis*sizeof(double))) == NULL) {
            while(i--) free(orbital[i].coefficient);
            free(orbital);
            return 0;
         }
      }
      else orbital[i].coefficient = NULL;

      orbital[i].eigenvalue = 0;
      orbital[i].number = 0;
      orbital[i].occ = 0;
      orbital[i].type[0] = 0;
      orbital[i].flag = flag;
   }

   return orbital;
}

/// Frees memory allocated for dipole moment.
void FreeDipole( Dipole* d )
{
    if( d ) free( d );
}

Dipole *add_dipole(Mol *mol, float x, float y, float z)
{
   Dipole *dp;
   //char str[30];

   if((dp = (Dipole*) malloc(sizeof(Dipole))) == NULL) {
      fprintf(stderr, " can't allocate memory for the dipole moment\n");
      return NULL;
   }

   dp->start[0] = -mol->centervec[0] - x/2;
   dp->start[1] = -mol->centervec[1] - y/2;
   dp->start[2] = -mol->centervec[2] - z/2;
   dp->end[0] = -mol->centervec[0] + x/2;
   dp->end[1] = -mol->centervec[1] + y/2;
   dp->end[2] = -mol->centervec[2] + z/2;

   dp->absolute = dist(dp->start, dp->end);

/*
printf("Center Vec   : %f %f %f\n", mol->centervec[0], mol->centervec[1], mol->centervec[2]);
printf("Dipole Moment: %f %f %f\n", dp->start[0], dp->start[1], dp->start[2]);
printf("               %f %f %f\n", dp->end[0], dp->end[1], dp->end[2]);
printf("              |%f|\n", dist(dp->start, dp->end));
*/

   return dp;
}

void *alloc_trimat(int n, size_t size)
{
   void **pointerarray;
   char *array;
   register short i;

   if((array = (char*) malloc((n*(n+1))/2*size)) == NULL) return NULL;
      /* array will hold the data */
   if((pointerarray = (void**) malloc(n*sizeof(char *))) == NULL) return NULL;
      /* pointerarray will hold the pointers to the rows of data */
   for(i=0; i<n; i++) pointerarray[i] = array + (i*(i+1))/2*size;

   return pointerarray;
}

