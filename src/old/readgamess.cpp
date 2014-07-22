/// @note original molekel file will be kept until proper Gamess I/O is added
/// to OpenBabel or new codebase
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


// Previous copyright notice
/*  MOLEKEL, Version 4.4, Date: 10.Dec.03
 *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
 *  (original IRIX GL implementation, concept and data structure
 *   by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
 *   and revisions by Stefan Portmann, CSCS/ETHZ)
 *
 *  This software makes use of the
 *  GLUT http://reality.sgi.com/mjk/glut3/glut3.html
 *  GLUI http://www.cs.unc.edu/~rademach/glui/
 *  libtiff http://www.libtiff.org/tiff-v3.5.5.tar.gz
 *  libjpeg ftp://ftp.uu.net/graphics/jpeg
 *  and in some versions of the
 *  Mesa http://www.mesa3d.org/
 *  and the
 *  libimage https://toolbox.sgi.com/toolbox/src/haeberli/libimage/index.html#dl
 *  libraries.
 *  An adapted version of the tr library by Brian Paul
 *  (http://www.mesa3d.org/brianp/TR.html)
 *  is part of the distribution.
 *
 *  The binary code is available free of charge but is not in the
 *  public domain. See license for details on conditions and restrictions.
 *  Source code is only available in the framework of a collaboration.
 *
 *  Info: http://www.cscs.ch/molekel/
**/


/* lecture of GAMESS output */
#define _USE_MATH_DEFINES
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "constant.h"
#include "molekeltypes.h"
#include <cctype>
#include <cassert>

using namespace std;

extern Element element[ 105 ];
extern Dynamics dynamics;
extern int InitAtoms();
extern void free_dyna();
extern void showinfobox( const char* msg );
extern void update_logs();
extern void logprint( const char* m );
extern Molecule* add_mol( const char* fname );
extern float dist(float *a, float *b);
extern void new_mole( Molecule* , const char* );
extern void create_bonds( Molecule* );
extern void find_multiplebonds( Molecule* );
extern void computeOccupations(Mol *mp);
extern MolecularOrbital *allocOrbital(int nOrbitals, int nBasis, int flag);
extern void FreeDipole( Dipole* d );
extern Dipole *add_dipole(Mol *mol, float x, float y, float z);
extern Dynamics CopyDynamics( const Dynamics& d );
//------------------------------------------------------------------------------




/////////////////////
static int read_atomic_coordinates(Mol *mol);
static char *find_string(char *s);
static int read_charge(Mol *mol);
static int read_basis_set(Mol *mol);
static int read_eigenvectors(Mol *mol);
static int read_frequencies(Mol *mol);
static int read_dipole(Mol *mol);
static int addGMTrajectoryStep(void);

static int read_frequency_IR_intensities(Mol *mol);
static int read_frequency_reduced_masses(Mol *mol);

static FILE *fp;
static char line[256];
static int nblocks;
static long previous_line = 0, preprevious = 0;


/**** lecture of GAMESS output ****/

void print_amoss_basis_set(Mol *mol)
{
//   MolekelAtom *basis;
//   Shell *shell;
//   Gauss *gauss;

   for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
      printf("Atom %s\n", element[ap->ord].symbol);
      for (ShellList::iterator shell=ap->Shells.begin(); shell!=ap->Shells.end(); ++shell) {
         printf("          Shell (%d), scale %f\n",
            shell->n_base, shell->scale_factor);
         for (GaussList::iterator it=shell->gaussians.begin(); it!=shell->gaussians.end(); ++it) {
            printf("                         %12.5f %8.4f %8.4f\n",it->exponent, it->coeff, it->coeff2);
         }
      }
   }
}

Molecule *read_gamess(const char *file)
{

   if((fp = fopen(file, "r")) == NULL){
      sprintf(line, "Can't open file\n%s !", file);
      showinfobox(line);
      return NULL;
   }

   if(!find_string("GAMESS")) {
      showinfobox("read_gamess : GAMESS (US) output only!");
      fclose(fp);
      return NULL;
   }

   Mol *mol = add_mol(file);

   if(!read_basis_set(mol)){
      showinfobox("Can't read the basis-set!");
      rewind(fp);
   }

   if(!read_atomic_coordinates(mol)){
      showinfobox("Can't read the atomic coordinates!");
      fclose(fp);
      delete mol;
      //Globals::Molecules.remove(mol);
      return NULL;
   }

   if(!mol->natoms){
      sprintf(line, "No atoms in %s!", file);
      showinfobox(line);
      //Globals::Molecules.remove(mol);
      fclose(fp);
      delete mol;
      return NULL;
   }


   create_bonds(mol);
   find_multiplebonds(mol);
//   create_box();
   new_mole(mol,file);

/*
   print_amoss_basis_set();
*/

   if(!read_eigenvectors(mol)){
      logprint("can't read the eigenvalues!");
   }
   computeOccupations(mol);

/*
   print_coefficients();
*/

   if(!read_charge(mol)){
      logprint("unable to read");
      logprint("   the atomic charges!");
      mol->charges = 0;
   }

   if(read_frequencies(mol)){
      logprint("frequencies present");
   }

   if(read_dipole(mol)) {
      logprint("dipole moment present");
   }

   fclose(fp);
   mol->dynamics = CopyDynamics( dynamics );
   free_dyna();
   update_logs();
   return mol;
}


static char *find_string(char *s)
{
   previous_line = ftell(fp);
   do {
      if(!fgets(line, 255, fp)) return NULL;
      if(strstr(line, s)) return line;
      preprevious = previous_line;
      previous_line = ftell(fp);
   } while (1);
}





Shell *add_shell_to_amoss(Amoss_basis *ap)
/* add shell to linked list of AMOSS basis-set shells */
{
   Shell shell/*, *sp*/;
   ap->Shells.push_back(shell);
   return &ap->Shells.back();
}


static int read_atomic_coordinates(Mol *mol)
{
   long fpos;
   float x, y, z;
   char basis_symbol[20], *format;
   float ord;
   unsigned angst = 0;
   int natoms;

   free_dyna();

   rewind(fp);

   if(find_string("COORDINATES (BOHR)")) {
      do {
         fpos = ftell(fp);
      } while(find_string("COORDINATES (BOHR)"));

      fseek(fp, fpos, SEEK_SET);
      if(!find_string("CHARGE         X                   Y ")){
         fseek(fp, fpos, SEEK_SET);
         if(!find_string("ATOM     ZNUC       X             Y")) return 0;
         fgets(line, 255, fp);
         fgets(line, 255, fp);
         format = "%*d%s %f %f %f %f";
      }
      else format = "%s %f %f %f %f";
      fpos = ftell(fp);
   }
   else return 0;

   rewind(fp);
   if(find_string("COORDINATES OF ALL ATOMS ARE (ANGS)")) {
      do {
         angst = 1;
         fpos = ftell(fp);
         natoms = addGMTrajectoryStep();
      } while(find_string("COORDINATES OF ALL ATOMS ARE (ANGS)"));
      fseek(fp, fpos, SEEK_SET);
      fgets(line, 255, fp);
      fgets(line, 255, fp);
      fpos = ftell(fp);
   }

   dynamics.molecule = mol;
   dynamics.current = dynamics.ntotalsteps - 1;

   fseek(fp, fpos, SEEK_SET);
   while (1) {
      if (!fgets(line, 255, fp)) return 0;
      if (sscanf(line, format, basis_symbol, &ord, &x, &y, &z)!= 5) break;
      MolekelAtom *atom;
      if (angst) atom = mol->AddNewAtom((int)ord, x, y, z);
      else atom = mol->AddNewAtom((int)ord, float(x*BOHR), float(y*BOHR), float(z*BOHR) );
      ShellList *sp = mol->get_Amossbasis(basis_symbol);
      if ( sp && sp->size()) {
        // copy the shell list across : @TODO check this
        atom->Shells = *sp;
      }
   }

   return 1;
}




static int addGMTrajectoryStep(void)
{
   long fpos, index, i;
   float x, y, z;
   static int natoms;


   index = dynamics.ntotalsteps++;

   if(!dynamics.trajectory){
      if((dynamics.trajectory = (Vector **)malloc(sizeof(Vector *))) == NULL){
         fprintf(stderr, "can't allocate dyna pointer\n");
         return 0;
      }
/* count nr of atoms */
      fpos = ftell(fp);
      fgets(line, 255, fp);
      fgets(line, 255, fp);
      fgets(line, 255, fp);
      natoms = 0;
      do {
         natoms++;
         if(!fgets(line, 255, fp)) break;
      } while(strlen(line) > 1);
      fseek(fp, fpos, SEEK_SET);

   }
   else {
      if((dynamics.trajectory = (Vector **)realloc(dynamics.trajectory,
          dynamics.ntotalsteps*sizeof(Vector *))) == NULL){
         fprintf(stderr, "can't reallocate dyna pointer\n");
         dynamics.ntotalsteps--;
         free_dyna();
         return 0;
      }
      fpos = ftell(fp);
   }

   if((dynamics.trajectory[index] = (Vector *)malloc(natoms*sizeof(Vector))) == NULL){
      fprintf(stderr, "can't allocate timestep dyna[%d]\n", index);
      dynamics.ntotalsteps--;
      free_dyna();
      return 0;
   }

   fgets(line, 255, fp);
   fgets(line, 255, fp);
   fgets(line, 255, fp);

   i = 0;
   do {
      if(sscanf(line, "%*s %*f %f %f %f", &x, &y, &z) != 3) return 0;
      dynamics.trajectory[index][i].x = x;
      dynamics.trajectory[index][i].y = y;
      dynamics.trajectory[index][i].z = z;
      if(!fgets(line, 255, fp)) break;
      i++;
   } while(strlen(line) > 1);



   fseek(fp, fpos, SEEK_SET);
   return natoms;
}


namespace 
{
bool EmptyString( const char* s )
{
	if( !s ) return true;
	while( *s )
	{
		if( !std::iscntrl( *s ) && !std::isspace( *s ) ) return false;
		++s;	
	}
	return true;
}
}
static int read_basis_set(Mol *mol)
{
   Amoss_basis *ap;
   Shell *sp;
   int i1, i2;
   float f1, f2, f3;
   double s_coeff, p_coeff, d_coeff, f_coeff, /*alpha,*/ norm;
   int new_gamess = 1;

   if(!find_string("ATOMIC BASIS SET")) return 0;
   if(!find_string("CONTRACTED PRIMITIVE FUNCTIONS")) return 0;
   
   const long fpos = ftell( fp );
   if(!find_string("CONTRACTION COEFFICIENT(S)") )
   {
	   fseek( fp, fpos, SEEK_SET );
       if( !find_string("CONTRACTION COEFFICIENTS") ) return 0;
   }
   fgets(line, 255, fp); /*  blank line */

   while (!strstr(line, "TOTAL NUMBER OF")) {

      while(fgets(line, 255, fp)) {

         //if(line[0] == '\n' || line[1] == '\n') continue;
    	 if( EmptyString( line ) ) continue; 	
         if(strlen(line) < 16) { /* new basis set symbol */
            if (!(ap = mol->add_amoss())) return 0;
            sscanf(line, "%s", ap->basis);
            break;
         }

         if(strchr(line, '(')){
            new_gamess = 0;
         }

         if(new_gamess) {
            if(sscanf(line, "%d %*s %d %f %f",
                           &i1, &i2, &f1, &f2) != 4) break;
         }
         else {
            if(sscanf(line, "%d %*s %d %f %f %*s %f",
                           &i1, &i2, &f1, &f2, &f3) != 5) break;
         }

         if(!(sp = add_shell_to_amoss(ap))) return 0;
         sp->scale_factor = 1.0;

         while( !EmptyString( line ) /*line[0] != '\n' && line[1] != '\n'*/) {
            Gauss gauss;
            gauss.coeff2 = 0;

            if(strchr(line, 'S')){
               if(new_gamess) {
                  sscanf(line, "%*d %*s %*d %lf %lf",
                     &gauss.exponent, &s_coeff);
                  norm  = pow(2.0 * gauss.exponent / M_PI, 0.75);
                  gauss.coeff  = s_coeff * norm;
               }
               else {
                  sscanf(line, "%*d %*s %*d %lf %lf %*s %*f",
                     &gauss.exponent, &gauss.coeff);
               }
               sp->n_base = 1;
            }
            else if(strchr(line, 'P')){
               if(new_gamess) {
                  sscanf(line, "%*d %*s %*d %lf %lf",
                     &gauss.exponent, &p_coeff);
                  norm  = pow(128.0 * pow(gauss.exponent, 5) / pow(M_PI, 3), 0.25);
                  gauss.coeff  = p_coeff * norm;
               }
               else {
                  sscanf(line, "%*d %*s %*d %lf %lf %*s %*f",
                     &gauss.exponent, &gauss.coeff);
               }
               sp->n_base = 3;
            }
            else if(strchr(line, 'L')){
/*
               sscanf(line, "%*d %*s %*d %lf %lf %*s %*f",
                  &gauss.exponent, &gauss.coeff);
               sscanf(line+54, "%lf", &gauss.coeff2);
*/
               if(new_gamess) {
                  sscanf(line, "%*d %*s %*d %lf %lf %lf",
                     &gauss.exponent, &s_coeff, &p_coeff);
                  norm  = pow(2.0 * gauss.exponent / M_PI, 0.75);
                  gauss.coeff  = s_coeff * norm;
                  norm  = pow(128.0 * pow(gauss.exponent, 5) / pow(M_PI, 3), 0.25);
                  gauss.coeff2 = p_coeff * norm;
               }
               else {
                  if(sscanf(line, "%*d %*s %*d %lf %lf %*s %*f%*s %lf %*s %*f",
                     &gauss.exponent, &gauss.coeff, &gauss.coeff2) == 2) {
                       fgets(line, 255, fp);
                       sscanf(line, "%lf", &gauss.coeff2);
                  }
               }
               sp->n_base = 4;
            }
            else if(strchr(line, 'D')){
               if(new_gamess) {
                  sscanf(line, "%*d %*s %*d %lf %lf",
                     &gauss.exponent, &d_coeff);
                  norm = pow(2048. * pow(gauss.exponent, 7) / pow(M_PI, 3), .25);
                  gauss.coeff  = d_coeff * norm;
// 4.4.01 STP: the line below gives the same number, that was present in
// the gamess output before. But what I need is above.
//                  gauss.coeff  = d_coeff * norm / sqrt(3);
               }
               else {
                  sscanf(line, "%*d %*s %*d %lf %lf %*s %*f",
                     &gauss.exponent, &d_coeff);
                     gauss.coeff  = d_coeff * 1.7320508;
// 4.4.01 STP: d_coeff seems not to be what was thought it to be:
// d_coeff is:  CDINP * (2048 EX^7/PI^3)^1/4 *(1/3)^1/2
// need to correct with factor 3^1/2
//                     gauss.coeff  = d_coeff;
               }
               sp->n_base = 6;
            }
            else if(strchr(line, 'F')){
               if(new_gamess) {
                  sscanf(line, "%*d %*s %*d %lf %lf",
                     &gauss.exponent, &f_coeff);
                  norm = pow(32768. * pow(gauss.exponent, 9) / pow(M_PI, 3), .25);
                  gauss.coeff  = f_coeff * norm;
// 4.4.01 STP: the line below gives the same number, that was present in
// the gamess output before. But what I need is above.
//                  gauss.coeff  = f_coeff * norm / sqrt(15);
               }
               else {
                  sscanf(line, "%*d %*s %*d %lf %lf %*s %*f",
                     &gauss.exponent, &f_coeff);
                     gauss.coeff = f_coeff * 3.8729833;
// 4.4.01 STP: f_coeff seems not to be what was thought it to be:
// f_coeff is: CFINP * (32768 EX^9/PI^3)^1/4 *(1/15)^1/2
// need to correct with factor 15^1/2
//                     gauss.coeff = f_coeff;
               }
               sp->n_base = 10;
            }
            else {
               logprint("can't read");
               logprint("   gaussian primitives");
               return 0;
            }
            sp->gaussians.push_back(gauss);
            if(!fgets(line, 255, fp)) return 0;
         }                                /* end of gaussian primitives */
      }                                   /* end of shell */
   }                                      /* end of basis-set */

   mol->normalize_gaussians();

   return 1;
}


static int read_eigenvectors(Mol *mol)
{
   long fpos, itest;
   MolecularOrbital *mo, *mbeta;
   int i1, i2, i3, i4, i5, i6, i7, i8, i9, i0, nnn, n_mo;
   int incr = 38;
   register int i, j;
   char *eigenstr = "EIGENVECTORS",
               *alphastr = "----- ALPHA SET -----",
               *betastr  = "----- BETA SET -----";

   char *pkey;
   unsigned equgeo = 0;

   rewind(fp);

   if(!find_string("TOTAL NUMBER OF BASIS FUNCTIONS")) {
      incr = 47;
      rewind(fp);
      if(!find_string("NUMBER OF CARTESIAN GAUSSIAN BASIS FUNCTIONS")) return 0;
   }
   sscanf(line + incr, "%d", &mol->nBasisFunctions);
   if(!find_string("NUMBER OF OCCUPIED ORBITALS (ALPHA)")) return 0;
   sscanf(line + incr, "%d", &mol->nAlpha);
   if(!find_string("NUMBER OF OCCUPIED ORBITALS (BETA )")) return 0;
   sscanf(line + incr, "%d", &mol->nBeta);

   if(find_string("EQUILIBRIUM GEOMETRY LOCATED")) {
      alphastr = "**** ALPHA SET ****";
      betastr = "**** BETA SET ****";
      eigenstr = "MOLECULAR ORBITALS\n";
      equgeo = 1;
   }

   rewind(fp);

   if(find_string(alphastr)) {
      mol->alphaBeta = 1;
      pkey = alphastr;
   }
   else {
      mol->alphaBeta = 0;
      pkey = eigenstr;
   }
   rewind(fp);

   if(!find_string(eigenstr)) {
      rewind(fp);
      pkey = " ORBITALS\n";
      if(!find_string(pkey)) return 0;
   }

   do {
      fpos = ftell(fp);
   } while(find_string(pkey));
   fseek(fp, fpos, SEEK_SET);

   if(mol->alphaBeta){
      if(!equgeo) if(!find_string(eigenstr))  return 0;
   }
   nblocks = (mol->nBasisFunctions - 1)/5 + 1;

   nnn = mol->nBasisFunctions;


   if((mo = allocOrbital(nnn, nnn, GAMESS_ORB)) == NULL){
      logprint("can't allocate the MO-structures");
      return 0;
   }

   fseek(fp, fpos, SEEK_SET);

   j = nblocks;
   n_mo = 0;

   i1 = i2 = i3 = i4 = i5 = i6 = i7 = i8 = i9 = i0 = 0;

   if(!(equgeo && mol->alphaBeta)) fgets(line, 255, fp); /* ------------ */

   while(j--){
      fgets(line, 255, fp); /* blank */
      fgets(line, 255, fp); /* MO-numbers */
      itest = n_mo + 1;
      n_mo += sscanf(line, " %d %d %d %d %d %d %d %d %d %d",
         &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &i9, &i0);
      if(i1 != itest) break;

      fgets(line, 255, fp); /* Eigenvalues */
      sscanf(line, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf", &mo[--i1].eigenvalue,
         &mo[--i2].eigenvalue, &mo[--i3].eigenvalue, &mo[--i4].eigenvalue,
         &mo[--i5].eigenvalue, &mo[--i6].eigenvalue, &mo[--i7].eigenvalue,
         &mo[--i8].eigenvalue, &mo[--i9].eigenvalue, &mo[--i0].eigenvalue);

      fgets(line, 255, fp); /* Symmetries */

      for(i=0; i<mol->nBasisFunctions; i++){
         fgets(line, 255, fp);
         sscanf(line+15, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",  &mo[i1].coefficient[i],
            &mo[i2].coefficient[i], &mo[i3].coefficient[i], &mo[i4].coefficient[i],
            &mo[i5].coefficient[i], &mo[i6].coefficient[i], &mo[i7].coefficient[i],
            &mo[i8].coefficient[i], &mo[i9].coefficient[i], &mo[i0].coefficient[i]);
      }
   }
   mol->nMolecularOrbitals = n_mo;
   mol->alphaOrbital = mo;

   if(!mol->alphaBeta) return 1;

   if((mbeta = allocOrbital(nnn, nnn, GAMESS_ORB)) == NULL){
      logprint("can't allocate the MO-structures");
      return 0;
   }

   if(!find_string(betastr)) return 0;
   if(!equgeo) if(!find_string(eigenstr)) return 0;

   j = nblocks;
   n_mo = 0;
   i1 = i2 = i3 = i4 = i5 = i6 = i7 = i8 = i9 = i0 = 0;

   if(!equgeo) fgets(line, 255, fp); /* ------------ */

   while(j--){
      fgets(line, 255, fp); /* blank */
      fgets(line, 255, fp); /* MO-numbers */
      itest = n_mo + 1;
      n_mo += sscanf(line, " %d %d %d %d %d %d %d %d %d %d",
         &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &i9, &i0);
      if(i1 != itest) break;

      fgets(line, 255, fp); /* Eigenvalues */
      sscanf(line, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf", &mbeta[--i1].eigenvalue,
         &mbeta[--i2].eigenvalue, &mbeta[--i3].eigenvalue, &mbeta[--i4].eigenvalue,
         &mbeta[--i5].eigenvalue, &mbeta[--i6].eigenvalue, &mbeta[--i7].eigenvalue,
         &mbeta[--i8].eigenvalue, &mbeta[--i9].eigenvalue, &mbeta[--i0].eigenvalue);

      fgets(line, 255, fp); /* Symmetries */

      for(i=0; i<nnn; i++){
         fgets(line, 255, fp);
         sscanf(line+15, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",  &mbeta[i1].coefficient[i],
            &mbeta[i2].coefficient[i], &mbeta[i3].coefficient[i], &mbeta[i4].coefficient[i],
            &mbeta[i5].coefficient[i], &mbeta[i6].coefficient[i], &mbeta[i7].coefficient[i],
            &mbeta[i8].coefficient[i], &mbeta[i9].coefficient[i], &mbeta[i0].coefficient[i]);
      }
   }

   mol->nMolecularOrbitals = n_mo;
   mol->alphaOrbital = mo;
   mol->betaOrbital = mbeta;

   return 1;
}






static int read_charge(Mol *mol)
{
   long fpos;
   float ch;
//   MolekelAtom *ap;

   rewind(fp);
   if(!find_string("TOTAL MULLIKEN AND LOWDIN ATOMIC POPULATIONS")) return 0;
   do {
      fpos = ftell(fp);
   } while(find_string("TOTAL MULLIKEN AND LOWDIN ATOMIC POPULATIONS"));
   fseek(fp, fpos, SEEK_SET);

   fgets(line, 255, fp);
   for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
      if(!fgets(line, 255, fp)) return 0;
      if(!sscanf(line, "%*d%*s%*f%f", &ch)) return 0;
      ap->charge = ch;
   }
   mol->charges = 1;
   return 1;
}



struct CompareFrequencies
{
	bool operator()( const Vibration& v1, const Vibration& v2 ) const
	{
		return v1.frequency < v2.frequency;
	}
};
static int read_frequencies(Mol *mol)
{
   long fpos;
   int n_freq;
   register short i, j;
   char *iptr = NULL;

   rewind(fp);

   if(!find_string("FREQUENCIES IN CM**-1")) return 0;
   fpos = ftell(fp);

   n_freq = 0;
   while(find_string(" FREQUENCY: ")) n_freq += 5;
   if(!n_freq) return 0;

   mol->vibration.resize(n_freq);

   fseek(fp, fpos, SEEK_SET);
   for (i=0; i<n_freq/5; i++) {
      find_string(" FREQUENCY: ");
      /* replace all I for imaginary freq with a blank */
      iptr = strchr(line, 'I');
      while (iptr) {
         *iptr = ' ';
         iptr = strchr(line, 'I');
      }
      mol->vibration[i*5].coord.resize(mol->natoms);
      mol->vibration[i*5+1].coord.resize(mol->natoms);
      mol->vibration[i*5+2].coord.resize(mol->natoms);
      mol->vibration[i*5+3].coord.resize(mol->natoms);
      mol->vibration[i*5+4].coord.resize(mol->natoms);

      mol->n_frequencies += sscanf(line+20, "%f%f%f%f%f",
         &mol->vibration[i*5].frequency, &mol->vibration[i*5+1].frequency, &mol->vibration[i*5+2].frequency,
         &mol->vibration[i*5+3].frequency, &mol->vibration[i*5+4].frequency);

      fgets(line, 255, fp);
      /* newer gamess versions print also the reduced mass -> additional fgets */
      if (strstr(line, "REDUCED MASS")) {
         fgets(line, 255, fp); /* either empty or with Intensities */
      }
      if (strlen(line) > 1) fgets(line, 255, fp);

      j=0;
      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap, j++) {
         if(!fgets(line, 255, fp)) return 0;
         if(line[19] != 'X')    return 0;
         sscanf(line+20, "%f%f%f%f%f",
            &mol->vibration[i*5].coord[j].x, &mol->vibration[i*5+1].coord[j].x, &mol->vibration[i*5+2].coord[j].x,
            &mol->vibration[i*5+3].coord[j].x, &mol->vibration[i*5+4].coord[j].x);

         if(!fgets(line, 255, fp)) return 0;
         if(line[19] != 'Y')    return 0;
         sscanf(line+20, "%f%f%f%f%f",
            &mol->vibration[i*5].coord[j].y, &mol->vibration[i*5+1].coord[j].y, &mol->vibration[i*5+2].coord[j].y,
            &mol->vibration[i*5+3].coord[j].y, &mol->vibration[i*5+4].coord[j].y);

         if(!fgets(line, 255, fp)) return 0;
         if(line[19] != 'Z')    return 0;
         sscanf(line+20, "%f%f%f%f%f",
            &mol->vibration[i*5].coord[j].z, &mol->vibration[i*5+1].coord[j].z, &mol->vibration[i*5+2].coord[j].z,
            &mol->vibration[i*5+3].coord[j].z, &mol->vibration[i*5+4].coord[j].z);

      }
   }

   if( read_frequency_IR_intensities( mol ) ) 	printf( "Read IR intensities\n" );
   if( read_frequency_reduced_masses( mol ) ) 	printf( "Read reduced masses\n" );
   fflush( stdout );
   
   std::sort( mol->vibration.begin(), mol->vibration.end(), CompareFrequencies() );
   
   
   return 1;
}

static int read_dipole(Mol *mol)
{
   long fpos;
   float x, y, z;

   rewind(fp);
   if(!find_string("(DEBYE)")) return 0;
   fpos = ftell(fp);
   while(find_string("(DEBYE)")) fpos = ftell(fp);
   fseek(fp, fpos, SEEK_SET);
   fgets(line, 255, fp);

   sscanf(line, "%f %f %f", &x, &y, &z);
   mol->dipole = add_dipole(mol, x, y, z);

   return 1;
}

//==============================================================================
// Code for reading IR, Raman activities and reduced masses
//==============================================================================
static int read_frequency_IR_intensities(Mol *mol)
{
	assert( mol );
	if( mol->n_frequencies <=0 ) return 0;
	int i = 0;
  
	rewind(fp);

	if(!find_string(" IR INTENSITIES")) return 0;

	if(!find_string(" IR INTENSITY:")) return 0;
	
	const int n_freq = mol->n_frequencies;
   
	for(i=0; i<n_freq/5; i++) {
		if( sscanf(line+20, "%f %f %f %f %f",
				&mol->vibration[i*3].ir_intensity,
				&mol->vibration[i*3+1].ir_intensity,
				&mol->vibration[i*3+2].ir_intensity,
				&mol->vibration[i*3+3].ir_intensity,
				&mol->vibration[i*3+4].ir_intensity ) != 5 ) return 0;
		find_string(" IR INTENSITY:");	
    
	}
	return ( i == n_freq/5 );
}


static int read_frequency_reduced_masses(Mol *mol)
{
	assert( mol );
	if( mol->n_frequencies <=0 ) return 0;
	int i = 0;
  
	rewind(fp);

	if(!find_string(" REDUCED MASSES")) return 0;

	if(!find_string(" REDUCED MASS:")) return 0;
	
	const int n_freq = mol->n_frequencies;
   
	for(i=0; i<n_freq/5; i++) {
		if( sscanf(line+20, "%f %f %f %f %f",
				&mol->vibration[i*3].reduced_mass,
				&mol->vibration[i*3+1].reduced_mass,
				&mol->vibration[i*3+2].reduced_mass,
				&mol->vibration[i*3+3].reduced_mass,
				&mol->vibration[i*3+4].reduced_mass ) != 5 ) return 0;
		find_string(" REDUCED MASS:");	
    
	}
	return ( i == n_freq/5 );
}
