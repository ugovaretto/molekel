/// @note original molekel file will be kept until Gaussian I/O is added
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

/* reading GAUSSIAN output G92 - G03 should work */
#define _USE_MATH_DEFINES
#include <cstring>
#include <cmath>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cassert>

#define RHF          1
#define ROHF         2
#define UHF          4

#include "molekeltypes.h"
#include "constant.h"
//------------------------------------------------------------------------------
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
extern void *alloc_trimat(int n, size_t size);

//------------------------------------------------------------------------------

Molecule *read_gauss(const char *name);
Mol *read_atomic_coordinates(const char *file);
char *find_string(char *s);
int read_charge(Mol *mol);
int read_basis_set(Mol *mol);
int  read_coefficients(Mol *mol);
int read_density_matrix(Mol *mol);
float **read_trimat(Mol *mol);
void read_coeffs(char *s, double *v1, double *v2, double *v3,
                                double *v4, double *v5);
double get_value(char *s, int n);
void all_uppercase(char *s);
int read_frequencies(Mol *mol);
int read_dipole(Mol *mol);
void print_frequencies(Mol *mol);
int read_atomic_charges(Mol *mol);
int addTrajectoryStep();

void print_basis_set(Mol *mol);
void print_coefficients(Mol *mol);
void print_density_matrix(Mol *mol);

static int read_frequency_IR_intensities(Mol *mol);
static int read_frequency_raman_activities(Mol *mol);
static int read_frequency_reduced_masses(Mol *mol);

int n_primitive_gaussians;

static FILE *fp;
static char line[256];
static int nblocks, d_type, f_type;
static long previous_line = 0, preprevious = 0, preprepre = 0;
unsigned short flagG98 = 1;
unsigned short flagG03 = 1;
unsigned short flagG9803 = 0;

/**** lecture of gaussian output ****/


/// @note added by UV
/// Creates a copy of Dynamics struct
Dynamics CopyDynamics( const Dynamics& d )
{
    Dynamics c = d;
    c.freeat = NULL;
    c.trajectory =(Vector**) malloc( d.ntotalsteps * sizeof( Vector* ) );
    for( int i = 0; i != d.ntotalsteps; ++i )
    {
        c.trajectory[ i ] = ( Vector* ) malloc( d.molecule->natoms * sizeof( Vector ) );
        for( int j = 0; j != d.molecule->natoms; ++j )
        {
            c.trajectory[ i ][ j ].x = d.trajectory[ i ][ j ].x;
            c.trajectory[ i ][ j ].y = d.trajectory[ i ][ j ].y;
            c.trajectory[ i ][ j ].z = d.trajectory[ i ][ j ].z;
        }
    }

    return c;
}
/// @note added by UV
/// Frees memory allocated for Dynamics members
void FreeDynamics( Dynamics& d )
{
    for( int i = 0; i != d.ntotalsteps; ++i )
    {
        free( d.trajectory[ i ] );
    }
    free( d.trajectory );
    memset( &d, 0, sizeof( Dynamics ) );
}


Molecule *read_gauss(const char *name)
{
   static bool initAtoms = true;
   if( initAtoms )
   {
        InitAtoms();
        initAtoms = false;
   }
   unsigned long position;
   int basisread = 1;

   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "read_gauss : can't open %s\n", name);
      showinfobox(line);
      return NULL;
   }
   if(!find_string("Gaussian")) {
      showinfobox("read_gauss : Gaussian output only !");
      fclose(fp);
      return NULL;
   }
   if(!find_string("Gaussian 98")) flagG98 = 0;
   else {
      flagG98 = 1;
      flagG9803 = 1;
   }
   rewind (fp);
   if(!find_string("Gaussian 03")) flagG03 = 0;
   else {
      flagG03 = 1;
      flagG9803 = 1;
   }
   rewind (fp);
   if(flagG98) logprint("G98");
   if(flagG03) logprint("G03");
   rewind (fp);

   Mol *mol = read_atomic_coordinates(name);
   if(!mol){
      showinfobox("can't read the atomic coordinates");
      fclose(fp);
      return NULL;
   }

   if(!mol->natoms){
      sprintf(line, "No atoms in\n%s !", name);
      showinfobox(line);
      return NULL;
   }

   create_bonds(mol);
   find_multiplebonds(mol);
//   create_box();
   new_mole(mol, name);


   if(!read_atomic_charges(mol)){
      logprint("unable to read");
      logprint("the atomic charges");
      update_logs();
      mol->charges = 0;
   }

   rewind(fp);

   if(!find_string("----")) return NULL;
   fgets(line, 255, fp);
   if(line[1] != '#'){
      while(find_string("----")) {
         fgets(line, 255, fp);
         if(line[1] == '#') break;
      }
   }
   all_uppercase(line);
   logprint(line);
   if(strlen(line) > 29) logprint(line+29);
   if(strlen(line) > 58) logprint(line+58);

   if(!strstr(line, "GFPRINT") || !strstr(line, "POP")) {
      logprint("");
      logprint("for visualizing orbitals");
      logprint("and densities, you must");
      logprint("run the GAUSSIAN-job");
      logprint("with the keywords");
      logprint("GFPRINT,");
      logprint("  POP = FULL or POP = REGULAR");
      logprint("");
   }

   find_string("-\n");
   if(!find_string("-\n")) {
      logprint("can't read the title");
   }
   else {
      fgets(line, 255, fp);
      logprint("title:");
      logprint(line);     /* the title */
   }
   update_logs();

   position = ftell(fp);
   if(!read_charge(mol)){
      logprint("can't read the charge");
      logprint("  and multiplicity");
      fseek(fp, position, SEEK_SET);
   }

   position = ftell(fp);
   if(!read_basis_set(mol)){
      logprint("can't read the basis-set");
      fseek(fp, position, SEEK_SET);
      basisread = 0;
   }

/*
   print_basis_set();
*/

   position = ftell(fp);
   if(basisread) {
      if(!read_coefficients(mol)){
         logprint("can't read");
         logprint("  the MO-coefficients");
         fseek(fp, position, SEEK_SET);
      }
      computeOccupations(mol);


/*
   print_coefficients();
*/
      position = ftell(fp);
      if(!read_density_matrix(mol)){
         logprint("can't read");
         logprint("  the density matrix");
         fseek(fp, position, SEEK_SET);
      }
/*
   print_density_matrix();
*/
   }

   if(read_frequencies(mol)){
      logprint("frequencies present");
/*
      print_frequencies();
*/
   }

   if(read_dipole(mol)) {
      logprint("dipole moment present");
   }

   fclose(fp);
   update_logs();
   mol->dynamics = CopyDynamics( dynamics );
   free_dyna();
   return mol;

}


char *find_string(char *s)
{
   previous_line = ftell(fp);
   do {
      if(!fgets(line, 255, fp)) return NULL;
      if(strstr(line, s)) return line;
      preprepre = preprevious;
      preprevious = previous_line;
      previous_line = ftell(fp);
   } while (1);
}


Mol *read_atomic_coordinates(const char *file)
{
   long fpos;
   float x, y, z;
   int ord, natoms;

   free_dyna();

   if(find_string("Standard orientation")) {
      do {
         fpos = ftell(fp);
         natoms = addTrajectoryStep();
         } while(find_string("Standard orientation"));
   }
   else {
      rewind(fp);
      if(find_string("Z-Matrix orientation")) {
         do {
            fpos = ftell(fp);
            natoms = addTrajectoryStep();
         } while(find_string("Z-Matrix orientation"));
      }
      else {
           rewind(fp);
           if(find_string("Input orientation")) {
                do {
                 fpos = ftell(fp);
                 natoms = addTrajectoryStep();
                } while(find_string("Input orientation"));
           }
      }
   }

   Mol *mol = add_mol(file);
   dynamics.molecule = mol;
   dynamics.current = dynamics.ntotalsteps - 1;

   fseek(fp, fpos, SEEK_SET);

   if(!find_string("Coordinates (Angstroms)")) return 0;
   if(!find_string("-----")) return 0;

   fgets(line, 255, fp);
   do {
      if(flagG9803) {
      if(sscanf(line, "%*d %d %*d %f %f %f", &ord, &x, &y, &z) != 4) return 0;
      }
      else {
      if(sscanf(line, "%*d %d %f %f %f", &ord, &x, &y, &z) != 4) return 0;
      }
      if(ord >= 0) mol->AddNewAtom(ord, x, y, z);
      fgets(line, 255, fp);
   } while(!strstr(line, "------"));



   return mol;
}




int addTrajectoryStep(void)
{
   long fpos, index, i;
   float x, y, z;
   static int natoms;
   char str[30];

   index = dynamics.ntotalsteps++;

   if(!dynamics.trajectory){
      if((dynamics.trajectory = (Vector**) malloc(sizeof(Vector *))) == NULL){
         showinfobox("can't allocate dyna pointer\n");
         return 0;
      }

/* count nr of atoms */
      fpos = ftell(fp);
      if(!find_string("Coordinates (Angstroms)")) return 0;
      if(!find_string("-----")) return 0;
      fgets(line, 255, fp);
      natoms = 0;
      do {
         natoms++;
         if(!fgets(line, 255, fp)) break;
      } while(!strstr(line, "------"));
      fseek(fp, fpos, SEEK_SET);

   }
   else {
      if((dynamics.trajectory = (Vector**) realloc(dynamics.trajectory,
          dynamics.ntotalsteps*sizeof(Vector *))) == NULL){
         showinfobox("can't reallocate dyna pointer\n");
         dynamics.ntotalsteps--;
         free_dyna();
         return 0;
      }
      fpos = ftell(fp);
   }

   if((dynamics.trajectory[index] = (Vector*) malloc(natoms*sizeof(Vector))) == NULL){
      sprintf(str, "can't allocate timestep dyna[%d]\n", index);
      showinfobox(str);
      dynamics.ntotalsteps--;
      free_dyna();
      return 0;
   }

   if(!find_string("Coordinates (Angstroms)")) return 0;
   if(!find_string("-----")) return 0;

   fgets(line, 255, fp);
   i = 0;
   do {
      if(flagG9803) {
      if(sscanf(line, "%*d %*d %*d %f %f %f", &x, &y, &z) != 3) return 0;
      }
      else {
      if(sscanf(line, "%*d %*d %f %f %f", &x, &y, &z) != 3) return 0;
      }
      dynamics.trajectory[index][i].x = x;
      dynamics.trajectory[index][i].y = y;
      dynamics.trajectory[index][i].z = z;
      if(!fgets(line, 255, fp)) break;
      i++;
   } while(!strstr(line, "------"));

   fseek(fp, fpos, SEEK_SET);
   return natoms;
}




int read_charge(Mol *mol)
{
   char str[64] = "";
   if(!find_string("Multiplicity =")) return 0;

   sscanf(line, "%*s %*c%f %*s %*c%d", &mol->charge, &mol->multiplicity);
   sprintf(str, "charge: %3.1f, multiplicity: %2d", mol->charge, mol->multiplicity);
   logprint(str);

   return 1;
}



int read_basis_set(Mol *mol)
{
//   MolekelAtom *ap;
   Shell *sp;
   double s_coeff, p_coeff, d_coeff, f_coeff, alpha, norm;
   char *cp;
   char tmpline[256];
   long fpos;
   if(!find_string(" Basis read")) {
      rewind(fp);
      if(!find_string(" basis")) return 0;

   }
   fpos = ftell(fp);
   fgets(tmpline, 255, fp);
   if(strstr(tmpline, " ******")) {
      fseek(fp, fpos, SEEK_SET);
   }
   else if(strstr(tmpline, " AO bas")) {
           fseek(fp, fpos, SEEK_SET);
   }
   else {
      rewind(fp);
      if(!find_string(" basis")) return 0;
   }

   if(!strstr(line, "General basis") &&
      !strstr(line, "Standard basis") &&
      !strstr(line, "from chk"))
      return 0;
   logprint(line);
   if(strlen(line) > 29) logprint(line+29);

   if(!(cp = strrchr(line, '('))) return 0;
   if(!(cp = strchr(cp, 'D'))) return 0;
   cp--;
   if(*cp == '5') d_type = 5;
   else if(*cp == '6') d_type = 6;
   else {
      showinfobox("weird d-orbital");
      return 0;
   }
   if(!(cp = strchr(cp, 'F'))) return 0;
   cp--;
   if(*cp == '7') f_type = 7;
   else if(*cp == '0') f_type = 10;
   else {
      showinfobox("weird f-orbital");
      return 0;
   }
   if(!find_string("GAUSSIAN FUNCTIONS")) {
     return 0;
   }
   if(!find_string("*    ATOM   X-COORD  Y-COORD  Z-COORD   *  NUMBER     TYPE\
     FACTOR *  EXPONENT    S-COEF      P-COEF      D-COEF      F-COEF   *"))
      return 0;
   if(!find_string("****")) return 0;

   for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
      fgets(line, 255, fp); /* atom - symbol, x, y, z */
      fgets(line, 255, fp);
      do {
         if(!(sp = ap->add_shell())) return 0;
/*         sscanf(line+52, "%s", sp->type); */
         sscanf(line+63, "%f", &sp->scale_factor);
         fgets(line, 255, fp);
         while(strstr(line, "                                             \
                       "))
         {
            Gauss gauss;
            alpha = 0.0; s_coeff = 0.0; p_coeff = 0.0;
            d_coeff = 0.0; f_coeff = 0.0;
            read_coeffs(line+2, &alpha, &s_coeff, &p_coeff,
                                               &d_coeff, &f_coeff);
            gauss.exponent = alpha * sp->scale_factor * sp->scale_factor;
            alpha = gauss.exponent;
            if (s_coeff) {
               norm  = pow(2.0 * alpha / M_PI, 0.75);
               gauss.coeff  = s_coeff * norm;
               gauss.coeff2 = 0.0;
               sp->n_base = 1;
               if(p_coeff){
                  norm  = pow(128.0 * pow(alpha, 5) / pow(M_PI, 3), 0.25);
                  gauss.coeff2 = p_coeff * norm;
                  sp->n_base = 4;
               }
            }
            else if(p_coeff){
               norm  = pow(128.0 * pow(alpha, 5) / pow(M_PI, 3), 0.25);
               gauss.coeff  = p_coeff * norm;
               gauss.coeff2  = 0.0;
               sp->n_base = 3;
            }
            else if(d_coeff){
               norm = pow(2048. * pow(alpha, 7) / pow(M_PI, 3), .25);
               gauss.coeff  = d_coeff * norm;
               gauss.coeff2  = 0.0;
               sp->n_base = d_type;
            }
            else if(f_coeff){
               norm = pow(32768. * pow(alpha, 9) / pow(M_PI, 3), .25);
               gauss.coeff = f_coeff * norm;
               gauss.coeff2  = 0.0;
               sp->n_base = f_type;
            }
            else {
               showinfobox("No coefficients in gaussian primitive");
               return 0;
            }

            sp->gaussians.push_back(gauss);

            if(!fgets(line, 255, fp)) return 0;
         }                                /* end of gaussian primitives */
      } while(!strstr(line, "-----") &&
              !strstr(line, "*****"));    /* end of atom */
   }                                      /* end of basis-set */

   if(!strstr(line, "****")) return 0;

   mol->normalize_gaussians();

   return 1;
}



void read_coeffs(char *s, double *v1, double *v2, double *v3,
                                double *v4, double *v5)
/* read FORTRAN-type double-precision numbers */
{
   while(*s == ' ') s++;

   *v1 = get_value(s, 12);    s += 12;
   *v2 = get_value(s, 12);    s += 12;
   *v3 = get_value(s, 12);    s += 12;
   *v4 = get_value(s, 12);    s += 12;
   *v5 = get_value(s, 12);    s += 12;
}



double get_value(char *s, int n)
{
   char digits[20];
   register char *letter;
   double v;

   strncpy(digits, s, n);
   digits[n] = 0;
   for(letter=digits; *letter; letter++){
      if(*letter == 'D') *letter = 'e';
   }

   if(sscanf(digits, "%le", &v) != 1){
      showinfobox("problems reading the coefficients...");
      return 0;
   }
   return v;
}



void print_basis_set(Mol *mol)
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



void all_uppercase(char *s)
{
   while(*s) {
      *s = toupper(*s);
      s++;
   }
}



int read_coefficients(Mol *mol)
{
   long fpos, denspos, coefficientsPos;
   int i1, i2, i3, i4, i5, n_mo, norbs;
   int firstOrb, firstPass;
   register int i, j;
   static char *keystr = "Alpha Molecular Orbital Coefficients", *pkey;
   MolecularOrbital *alphaOrb, *betaOrb;

   if(!find_string("primitive gaussians")) return 0;
   logprint(line);
   sscanf(line, "%d %*s %*s %d", &mol->nBasisFunctions, &n_primitive_gaussians);
   sprintf(line, "%3d basis functions",
          mol->nBasisFunctions);
   logprint(line);
   sprintf(line, "  and %3d primitive gaussians",
          n_primitive_gaussians);
   logprint(line);
   fgets(line, 255, fp);
   sscanf(line, "%d %*s %*s %d", &mol->nAlpha, &mol->nBeta);
   logprint(line);

   pkey = strchr(keystr, 'O');
   if(!find_string(pkey)) return 0;
   if(strstr(line, keystr)) {
      mol->alphaBeta = 1;
      pkey = keystr;
   }

   do {
      fpos = ftell(fp);
   } while(find_string(pkey));
   fseek(fp, fpos, SEEK_SET);
   coefficientsPos = fpos;

   find_string("DENSITY MATRIX.");
   denspos = ftell(fp);
   fseek(fp, fpos, SEEK_SET);

   nblocks = 0;
   i1 = i2 = i3 = i4 = i5 = 0;
   n_mo = firstPass = 0;
   while(find_string("EIGENVALUES")){
      if(ftell(fp) >= denspos) break;
      nblocks++;

   if(mol->firstOrbital == 0) {
      fseek(fp, coefficientsPos, SEEK_SET);
      fgets(line, 255, fp); /* contains the orbital numbers */
  }
  else {
      fseek(fp, preprevious, SEEK_SET);
      fgets(line, 255, fp);   /* contains the orbital numbers or the symmetry */
      if(strchr(line, '(') || strchr(line, 'O') || strchr(line, 'V')) {
      /* contains symmetry symbols */
    fseek(fp, preprepre, SEEK_SET);
          fgets(line, 255, fp); /* contains the orbital numbers */
      }
  }

      n_mo += sscanf(line, " %d %d %d %d %d", &i1, &i2, &i3, &i4, &i5);
      if(!firstPass) mol->firstOrbital = firstPass = i1;
      find_string("EIGENVALUES"); /* "EIGENVALUES" string */
   }
   mol->lastOrbital = mol->firstOrbital + n_mo;

   norbs = (mol->alphaBeta ? n_mo/2 : n_mo);

   if((alphaOrb = allocOrbital(norbs, mol->nBasisFunctions, GAUSS_ORB)) == NULL){
      showinfobox("can't allocate the MO-structures");
      return 0;
   }

   if(mol->alphaBeta) {
      if((betaOrb = allocOrbital(norbs, mol->nBasisFunctions, GAUSS_ORB)) == NULL){
         showinfobox("can't allocate the MO-structures");
         return 0;
      }
   }

   fseek(fp, fpos, SEEK_SET);

   firstOrb = mol->firstOrbital;
   j = nblocks;
   if(mol->alphaBeta) j = nblocks/2;
   while(j--){
      fgets(line, 255, fp);
      sscanf(line, " %d %d %d %d %d", &i1, &i2, &i3, &i4, &i5);
      find_string("EIGENVALUES");
      sscanf(line+20, "%lf %lf %lf %lf %lf", &alphaOrb[i1-firstOrb].eigenvalue,
          &alphaOrb[i2-firstOrb].eigenvalue, &alphaOrb[i3-firstOrb].eigenvalue,
          &alphaOrb[i4-firstOrb].eigenvalue, &alphaOrb[i5-firstOrb].eigenvalue);
      for(i=0; i<mol->nBasisFunctions; i++){
         fgets(line, 255, fp);
         sscanf(line+20, "%lf %lf %lf %lf %lf", &alphaOrb[i1-firstOrb].coefficient[i],
         &alphaOrb[i2-firstOrb].coefficient[i], &alphaOrb[i3-firstOrb].coefficient[i],
         &alphaOrb[i4-firstOrb].coefficient[i], &alphaOrb[i5-firstOrb].coefficient[i]);
      }
   }

   if(!mol->alphaBeta) {
      mol->alphaOrbital = alphaOrb;
      mol->nMolecularOrbitals = n_mo;
      return 1;
   }

   if(!find_string("Beta Molecular Orbital Coefficients")) return 0;
   j = nblocks/2;
   while(j--){
      fgets(line, 255, fp);
      sscanf(line, " %d %d %d %d %d", &i1, &i2, &i3, &i4, &i5);
      find_string("EIGENVALUES");
      sscanf(line+20, "%lf %lf %lf %lf %lf", &betaOrb[i1-firstOrb].eigenvalue,
           &betaOrb[i2-firstOrb].eigenvalue, &betaOrb[i3-firstOrb].eigenvalue,
           &betaOrb[i4-firstOrb].eigenvalue, &betaOrb[i5-firstOrb].eigenvalue);
      for(i=0; i<mol->nBasisFunctions; i++){
         fgets(line, 255, fp);
         sscanf(line+20, "%lf %lf %lf %lf %lf", &betaOrb[i1-firstOrb].coefficient[i],
          &betaOrb[i2-firstOrb].coefficient[i], &betaOrb[i3-firstOrb].coefficient[i],
          &betaOrb[i4-firstOrb].coefficient[i], &betaOrb[i5-firstOrb].coefficient[i]);
      }
   }

   mol->alphaOrbital = alphaOrb;
   mol->betaOrbital = betaOrb;
   mol->nMolecularOrbitals = norbs;
   return 1;
}


/* reads the alpha- (and beta-) matrices */
int read_density_matrix(Mol *mol)
{
   if(!find_string("DENSITY MATRIX.")) return 0;

   if(mol->alphaDensity) {
      free(mol->alphaDensity[0]);
      free(mol->alphaDensity);
   }
   if(mol->betaDensity) {
      free(mol->betaDensity[0]);
      free(mol->betaDensity);
   }
   mol->alphaDensity = mol->betaDensity = NULL;

   if(!strstr(line, "ALPHA")){                 /* only one matrix */
      if((mol->alphaDensity = read_trimat(mol)) == NULL){
         showinfobox("can't allocate density matrix");
         return 0;
      }
      return 1;
   }

   /* two matrices */
   if((mol->alphaDensity = read_trimat(mol)) == NULL){
      showinfobox("can't allocate alpha density matrix");
      return 0;
   }
   if(!find_string("BETA DENSITY MATRIX.")) return 0;
   if((mol->betaDensity = read_trimat(mol)) == NULL){
      showinfobox("can't allocate beta density matrix");
      return 0;
   }

   return 1;
}



float **read_trimat(Mol *mol)
{
   register short i, j;
   float **trimat;

   if((trimat = (float**) alloc_trimat(mol->nBasisFunctions, sizeof(float))) == NULL){
      showinfobox("can't allocate density matrix");
      return NULL;
   }

   for(i=0; i<=((mol->nBasisFunctions-1)/5); i++){
      fgets(line, 255, fp);
      for(j=5*i; j<mol->nBasisFunctions; j++){
         fgets(line, 255, fp);
         sscanf(line + 20, "%f %f %f %f %f", trimat[j]+5*i, trimat[j]+5*i+1,
                          trimat[j]+5*i+2, trimat[j]+5*i+3, trimat[j]+5*i+4);
      }
   }

   return trimat;
}




void print_density_matrix(Mol *mol)
{
   register short i, j;

   for(i=0; i<mol->nBasisFunctions; i++){
      for(j=0; j<=i; j++) printf("%8.5f", mol->alphaDensity[i][j]);
      printf("\n");
   }
   if(mol->betaDensity){
      for(i=0; i<mol->nBasisFunctions; i++){
         for(j=0; j<=i; j++) printf("%8.5f", mol->betaDensity[i][j]);
         printf("\n");
      }
   }
}



void print_coefficients(Mol *mol)
{
   register int i, j;

   printf("\n   MO  eigenvalue  occupation\n\n");
   for(i=0; i<mol->nMolecularOrbitals; i++){
      printf(" %3d  %10.5f      %1f", i+1,
            mol->alphaOrbital[i].eigenvalue,
            mol->alphaOrbital[i].occ);

      for(j=0; j<mol->nBasisFunctions; j++){
         if(!(j%10)) printf("\n");
         printf(" %8.5f", mol->alphaOrbital[i].coefficient[j]);
      }
      printf("\n");

   }
   printf("\n");
}


struct CompareFrequencies
{
	bool operator()( const Vibration& v1, const Vibration& v2 ) const
	{
		return v1.frequency < v2.frequency;
	}
};
int read_frequencies(Mol *mol)
{
   long fpos;
   int n_freq;
   register short i, j;
//   MolekelAtom *ap;

   rewind(fp);

   if(!find_string("Harmonic frequencies (cm**-1)")) return 0;

   fpos = ftell(fp);
   if(!find_string(" Frequencies ---")) fseek(fp, fpos, SEEK_SET);
   else fpos = ftell(fp);
   n_freq = 0;
   while(find_string(" Frequencies -- ")) n_freq += 3;

   mol->vibration.resize(n_freq);

   fseek(fp, fpos, SEEK_SET);
   for(i=0; i<n_freq/3; i++) {
      find_string(" Frequencies -- ");
      mol->vibration[i*3].coord.resize(mol->natoms);
      mol->vibration[i*3+1].coord.resize(mol->natoms);
      mol->vibration[i*3+2].coord.resize(mol->natoms);
      mol->n_frequencies += sscanf(line+15, "%f %f %f", &mol->vibration[i*3].frequency, &mol->vibration[i*3+1].frequency, &mol->vibration[i*3+2].frequency);

      fseek(fp, preprevious, SEEK_SET);
      fgets(line, 255, fp);
      sscanf(line, "%s %s %s", &mol->vibration[i*3].type, &mol->vibration[i*3+1].type, &mol->vibration[i*3+2].type);
      if (!find_string("Atom AN      X      Y      Z        X      Y      Z"))
         return 0;
      j=0;
      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap, j++) {
         if(!fgets(line, 255, fp)) return 0;
         sscanf(line, "%*d %*d %f %f %f %f %f %f %f %f %f",
            &mol->vibration[i*3].coord[j].x,   &mol->vibration[i*3].coord[j].y,   &mol->vibration[i*3].coord[j].z,
            &mol->vibration[i*3+1].coord[j].x, &mol->vibration[i*3+1].coord[j].y, &mol->vibration[i*3+1].coord[j].z,
            &mol->vibration[i*3+2].coord[j].x, &mol->vibration[i*3+2].coord[j].y, &mol->vibration[i*3+2].coord[j].z);
      }
   }

   if( read_frequency_IR_intensities( mol ) ) 	printf( "Read IR intensities\n" );
   if( read_frequency_raman_activities( mol ) ) printf( "Read Raman activities\n" );
   if( read_frequency_reduced_masses( mol ) ) 	printf( "Read reduced masses\n" );
   fflush( stdout );
   
   std::sort( mol->vibration.begin(), mol->vibration.end(), CompareFrequencies() );
   
   return 1;
}

int read_dipole(Mol *mol)
{
   long fpos;
   float x, y, z;

   rewind(fp);
   if(!find_string("Dipole moment")) return 0;
   fpos = ftell(fp);
   while(find_string("Dipole moment")) fpos = ftell(fp);
   fseek(fp, fpos, SEEK_SET);
   fgets(line, 255, fp);

   sscanf(line, "%*s %f %*s %f %*s %f", &x, &y, &z);
   mol->dipole = add_dipole(mol, x, y, z);

   return 1;
}

void print_frequencies(Mol *mol)
{
   register short i, j;
//   MolekelAtom *ap;

   printf("%d vibrational modes\n", mol->n_frequencies);

   for(i=0; i<mol->n_frequencies; i++) {
      printf("%3d %10.3f \n", i+1, mol->vibration[i].frequency);
      j=0;
      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap, j++) {
         printf("                %5.2f%5.2f%5.2f\n",
            mol->vibration[i].coord[j].x,
            mol->vibration[i].coord[j].y,
            mol->vibration[i].coord[j].z);
      }
   }
}



int read_atomic_charges(Mol *mol)
{
   long total, fitted, choice;

   total = fitted = 0;
   rewind(fp);
   if(find_string("Total atomic charges")) {
      do {
   total = ftell(fp);
      } while(find_string("Total atomic charges"));
   }
   rewind(fp);
   if(find_string("Mulliken atomic charges")) {
      do {
   total = ftell(fp);
      } while(find_string("Mulliken atomic charges"));
   }
   rewind(fp);
   if(find_string("Charges from ESP fit")) {
      do {
         fitted = ftell(fp);
      } while(find_string("Charges from ESP fit"));
   }

   if(total && fitted){   /* select which set to read */
/* to be fixed
      choice = selection(&xselection, &yselection,
          "Choose one of the two sets of atomic charges", 2, list);
*/
      choice = 1;
      if(choice < 0) return 0;
      if(choice == 0) {
         fitted = 0;
         logprint("Reading the total atomic charges");
      }
      else if(choice == 1) {
         total = 0;
         logprint("Reading the atomic charges");
         logprint("   from ESP fit");
      }
   }

   if(total){
      fseek(fp, total, SEEK_SET);
      fgets(line, 255, fp);

      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
         if(!fgets(line, 255, fp)) return 0;
         if(sscanf(line, "%*d %*s %f", &ap->charge) != 1) return 0;

  //    printf("=====\nAtom ord: %d Atom %s, charge = %f\n", ap->ord, element[ap->ord].symbol, ap->charge);

      }
   }
   else if(fitted){
      fseek(fp, fitted, SEEK_SET);
      fgets(line, 255, fp);
      fgets(line, 255, fp);

      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
         if(!fgets(line, 255, fp)) return 0;
         if(sscanf(line, "%*d %*s %f", &ap->charge) != 1) return 0;
      }
   }
   else return 0;

   mol->charges = 1;
   return 1;
}

void free_dyna()
{
   int i;

   if(!dynamics.trajectory) return;
   for(i=0; i<dynamics.ntotalsteps; i++) {
      if(dynamics.trajectory[i]) free(dynamics.trajectory[i]);
   }
   if(dynamics.freeat) {
      for(i=0; i<dynamics.nfreat; i++) dynamics.freeat[i]->fixed = 1;
      free(dynamics.freeat);
   }
   dynamics.nfreat = 0;
   dynamics.freeat = NULL;
   dynamics.ntotalsteps = 0;
   dynamics.trajectory = NULL;
   dynamics.start = dynamics.end = dynamics.current = 0;
   dynamics.molecule = NULL;
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

	if(!find_string("IR intensities (KM/Mole)")) return 0;

	if(!find_string(" IR Inten    --")) return 0;
	
	const int n_freq = mol->n_frequencies;
   
	for(i=0; i<n_freq/3; i++) {
		if( sscanf(line+15, "%f %f %f", &mol->vibration[i*3].ir_intensity,
				&mol->vibration[i*3+1].ir_intensity,
				&mol->vibration[i*3+2].ir_intensity) != 3 ) return 0;
		find_string(" IR Inten    --");	
    
	}
	return ( i == n_freq/3 );
}

static int read_frequency_raman_activities(Mol *mol)
{
   assert( mol );
   if( mol->n_frequencies <=0 ) return 0;
   int i = 0;
  
   rewind(fp);

   if(!find_string(" Raman scattering")) return 0;
  
   if(!find_string(" Raman Activ --")) return 0;
   
   const int n_freq = mol->n_frequencies;
   
   for(i=0; i<n_freq/3; i++) {
      if( sscanf(line+15, "%f %f %f", &mol->vibration[i*3].raman_activity,
    		 &mol->vibration[i*3+1].raman_activity,
    		 &mol->vibration[i*3+2].raman_activity) != 3 ) return 0;
      find_string(" Raman Activ --");
    
   }
   return ( i == n_freq/3 );
}

static int read_frequency_reduced_masses(Mol *mol)
{
   assert( mol );
   if( mol->n_frequencies <= 0 ) return 0;	
   int i = 0;
  
   rewind(fp);

   if(!find_string(" reduced masses")) return 0;

   if(!find_string(" Red. masses --")) return 0;
   
   const int n_freq = mol->n_frequencies;
   
   for(i=0; i<n_freq/3; i++) {
      if( sscanf(line+15, "%f %f %f", &mol->vibration[i*3].reduced_mass,
    		 &mol->vibration[i*3+1].reduced_mass,
    		 &mol->vibration[i*3+2].reduced_mass) != 3 ) return 0;

      find_string(" Red. masses --");
   }

   return ( i == n_freq/3 );
}

