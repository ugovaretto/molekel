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
 */

/* reading molden format */
#define _USE_MATH_DEFINES
#include <cstring>
#include <cmath>
#include <algorithm>
#include <map>

#include <stdio.h>
#include <stdlib.h>

#include "constant.h"
#include "molekeltypes.h"

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
extern Dynamics CopyDynamics( const Dynamics& );
//------------------------------------------------------------------------------

// from Graphviz
int strucmp(const char *s1, const char *s2)
{
    while( ( *s1 != '\0' ) && ( tolower( *s1 ) == tolower( *s2 ) ) )
    {
        s1++;
        s2++;
    }
    return tolower( *s1 ) - tolower( *s2 );
}

//------------------------------------------------------------------------------
int get_ordinal(char *sym)
{
   int num, len;
   register short i;
   char line[100];

   if(isdigit(sym[0]) || sym[0] == ' ' || sym[0] == '.') sym++;
   len = strlen(sym);
   if(len > 1 && (isdigit(sym[1]) || sym[1] == '.')) len = 1;
   if(len > 2) len = 2;

   num = -1;
   for(i=0; i<NLIST; i++){
      if(len != strlen(element[i].symbol)) continue;
      if( strucmp( sym, element[ i ].symbol ) )
      {
         num = i;
         break;
      }
   }
   if(num < 0){
      sprintf(line, "Unknown atom type %s,", sym);
      logprint(line);
      sprintf(line, "   set as hydrogen!", sym);
      logprint(line);
      num = 1;
   }

   return num;
}


//------------------------------------------------------------------------------

typedef struct
{
  int ord;
  double x, y, z;
} Xyzatm;

typedef struct
{
  char var[12];
  double val;
} Zvar;

typedef struct
{
  double r;
  double w;
  double t;
  int  na;
  int  nb;
  int  nc;
} Zmat;

static Mol *read_atomic_coordinates(const char *name);
static char *find_string(char *s);
static void addTrajectoryStep(int natoms, Xyzatm *atmArray);
static int read_atomic_charges(Mol *mol);
static int read_basis_set(Mol *mol);
static int read_coefficients(Mol *mol);
static int read_occupations(Mol *mol);
//static Mol * read_frequencies(const char *name);
static int read_frequencies( Mol* mol );
static int read_dipole(Mol *mol);

static FILE *fp;
static char line[256];
static long previous_line = 0, preprevious = 0;
static int orbtype = GAUSS_ORB;

Molecule *read_molden(const char *name)
{
   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      return NULL;
   }
   if(fgets(line, 255, fp) == 0 ||
       (strstr(line, "[Molden Format]") == 0 && strstr(line, "[MOLDEN FORMAT]") == 0 &&
         strstr(line, "[Title]") == 0)) {
      sprintf(line, "is not a molden format file !\n");
      showinfobox(line);
      fclose(fp);
      return NULL;
   }

   Mol *mol = read_atomic_coordinates(name);
   if(!mol){
      showinfobox("can't read the atomic coordinates\nfile contains probably z-matrix info");
      fclose(fp);
      update_logs();
      return NULL;
   }

   if(!mol->natoms){
      sprintf(line, "No atoms in %s!", name);
      showinfobox(line);
      fclose(fp);
      return NULL;
   }

   create_bonds(mol);
   find_multiplebonds(mol);
   new_mole(mol,name);
   logprint("[ATOMS] section read!");

   rewind(fp);
   if(!read_basis_set(mol)){
      showinfobox("can't read the basis-set");
      //fclose(fp);
      update_logs();
      //return NULL;
   }

   rewind(fp);
   if(!read_coefficients(mol)){
      showinfobox("can't read the MO-coefficients");
      //fclose(fp);
      update_logs();
      //return NULL;
   }

   rewind( fp );
   if( !read_frequencies( mol ) )
   {
	   showinfobox("Can't read the frequencies");
	   update_logs();
   }
   fclose(fp);
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

static int isEmptyLine(char *line)
{
   unsigned int i;

   for(i=0; i<strlen(line); i++) {
      if(!isspace(line[i])) return 0;
   }
   return 1;
}

static int compare_orbitals(const void *aa, const void *bb)
{
    MolecularOrbital *a, *b;

    a = (MolecularOrbital *)aa;
    b = (MolecularOrbital *)bb;

   if(a->eigenvalue < b->eigenvalue) return -1;
   if(a->eigenvalue > b->eigenvalue) return  1;

//   return strcmp(a->type, b->type);
   return 0;
}

static double get_var(char *var, Zvar *zvar, int natoms)
{
   int i, pre;
   double value;

   if(sscanf(var, "%lf", &value) == 1) return value;
   pre = 1;
   if(strncmp(var, "-", 1) == 0) {
      pre = -1;
      var++;
   }
   for(i=0; i<(natoms-2)*3; i++) {
      if(strcmp(zvar[i].var, var) == 0) return pre * zvar[i].val;
   }
   return 0.0;

}

static int int_to_cart(Xyzatm *atmArray, Zmat *zmat, int natoms)
{
// more or less taken from babel
  double cosph,sinph,costh,sinth,coskh,sinkh;
  double cosa,sina,cosd,sind;
  double dist,angle,dihed;

  double xpd,ypd,zpd,xqd,yqd,zqd;
  double xa,ya,za,xb,yb,zb;
  double rbc,xyb,yza,temp;
  double xpa,ypa,zqa;
  double xd,yd,zd;
  int flag;
  int i, na, nb, nc;

  /* Atom #1 */
  atmArray[0].x = 0.0;
  atmArray[0].y = 0.0;
  atmArray[0].z = 0.0;

  if (natoms == 1)
    return 1;

  /* Atom #2 */
  atmArray[1].x = zmat[1].r;
  atmArray[1].y = 0.0;
  atmArray[1].z = 0.0;

  if( natoms == 2 )
    return 1;

  /* Atom #3 */
  dist = zmat[2].r;
  angle = zmat[2].w * M_PI/180.;
  cosa = cos(angle);
  sina = sin(angle);

  if(  zmat[2].na == 1 )
  {
    atmArray[2].x = atmArray[0].x + cosa*dist;
  }
  else
  {
    atmArray[2].x = atmArray[1].x - cosa*dist;
  }
  atmArray[2].y = sina*dist;
  atmArray[2].z = 0.0;

  for (i = 3; i < natoms; i++)
  {
    dist = zmat[i].r;
    angle = zmat[i].w * M_PI/180.;
    dihed = zmat[i].t * M_PI/180.;

    na = zmat[i].na - 1;
    nb = zmat[i].nb - 1;
    nc = zmat[i].nc - 1;

    xb = atmArray[nb].x - atmArray[na].x;
    yb = atmArray[nb].y - atmArray[na].y;
    zb = atmArray[nb].z - atmArray[na].z;

    rbc = xb*xb + yb*yb + zb*zb;
    if( rbc < 0.0001 )
      return 0;
    rbc = 1.0/sqrt(rbc);

    cosa = cos(angle);
    sina = sin(angle);


    if( fabs(cosa) >= 0.999999 )
    {
      /* Colinear */
      temp = dist*rbc*cosa;
      atmArray[i].x = atmArray[na].x + temp*xb;
      atmArray[i].y = atmArray[na].y + temp*yb;
      atmArray[i].z = atmArray[na].z + temp*zb;
    }
    else
    {
      xa = atmArray[nc].x - atmArray[na].x;
      ya = atmArray[nc].y - atmArray[na].y;
      za = atmArray[nc].z - atmArray[na].z;

      sind = -sin(dihed);
      cosd = cos(dihed);

      xd = dist*cosa;
      yd = dist*sina*cosd;
      zd = dist*sina*sind;

      xyb = sqrt(xb*xb + yb*yb);
      if( xyb < 0.1 )
      {
  /* Rotate about y-axis! */
  temp = za; za = -xa; xa = temp;
  temp = zb; zb = -xb; xb = temp;
  xyb = sqrt(xb*xb + yb*yb);
  flag = 1;
      }
      else
  flag = 0;

      costh = xb/xyb;
      sinth = yb/xyb;
      xpa = costh*xa + sinth*ya;
      ypa = costh*ya - sinth*xa;

      sinph = zb*rbc;
      cosph = sqrt(1.0 - sinph*sinph);
      zqa = cosph*za  - sinph*xpa;

      yza = sqrt(ypa*ypa + zqa*zqa);

      if( yza > 1.0E-10 )
      {
  coskh = ypa/yza;
  sinkh = zqa/yza;

  ypd = coskh*yd - sinkh*zd;
  zpd = coskh*zd + sinkh*yd;
      }
      else
      {
  ypd = yd;
  zpd = zd;
      }

      xpd = cosph*xd  - sinph*zpd;
      zqd = cosph*zpd + sinph*xd;
      xqd = costh*xpd - sinth*ypd;
      yqd = costh*ypd + sinth*xpd;

      if( flag )
      {
  /* Rotate about y-axis! */
        atmArray[i].x = atmArray[na].x - zqd;
        atmArray[i].y = atmArray[na].y + yqd;
        atmArray[i].z = atmArray[na].z + xqd;
      }
      else
      {
        atmArray[i].x = atmArray[na].x + xqd;
        atmArray[i].y = atmArray[na].y + yqd;
        atmArray[i].z = atmArray[na].z + zqd;
      }
    }
  }
  return 1;
}

static Xyzatm *read_zmat(int *natoms, long *eof)
{


   Zmat *zmat;
   Zvar *zvar, *pzvar;
   Xyzatm *atmArray;
   long fpos;
   int i;
   char symb[100], rvar[12], wvar[12], tvar[12];

   fpos = ftell(fp);
   fgets(line, 255, fp);
   *natoms = 0;
   while(!strstr(line, "variables") && !strstr(line, "VARIABLES")) {
     (*natoms)++;
     fgets(line, 255, fp);
   }

   if((zmat = (Zmat *)malloc(*natoms * sizeof(Zmat))) == NULL){
      showinfobox("can't allocate the z-matrix");
      return NULL;
   }
   if((zvar = (Zvar *)malloc((*natoms - 2)*3 * sizeof(Zvar))) == NULL){
      showinfobox("can't allocate the z-variables");
      return NULL;
   }
   if((atmArray = (Xyzatm *)malloc(*natoms * sizeof(Xyzatm))) == NULL){
      showinfobox("can't allocate the xyz-atoms");
      return NULL;
   }

   fgets(line, 255, fp);
   pzvar = zvar;
   while(!strstr(line, "end") && !strstr(line, "END")) {
      if(strstr(line, "constants") || strstr(line, "CONSTATNTS")) {
         fgets(line, 255, fp);
         continue;
      }
      if(sscanf(line, "%s %lf", pzvar->var, &pzvar->val) != 2) return NULL;
      pzvar++;
      fgets(line, 255, fp);
   }

   *eof = ftell(fp);

   fseek(fp, fpos, SEEK_SET);
   for(i = 0; i<*natoms; i++) {
      fgets(line, 255, fp);
      if(i==0) {
         if(sscanf(line, "%s", symb) != 1) return NULL;
         atmArray[i].ord = get_ordinal(symb);
         zmat[i].r = zmat[i].w = zmat[i].t = zmat[i].na = zmat[i].nb = zmat[i].nc = 0;
      } else if(i==1) {
         if(sscanf(line, "%s %d %s", symb, &zmat[i].na, rvar) != 3) return NULL;
         atmArray[i].ord = get_ordinal(symb);
         zmat[i].r = get_var(rvar, zvar, *natoms);
         zmat[i].w = zmat[i].t = zmat[i].nb = zmat[i].nc = 0;
      } else if(i==2) {
         if(sscanf(line, "%s %d %s %d %s", symb, &zmat[i].na, rvar, &zmat[i].nb, wvar) != 5) return NULL;
         atmArray[i].ord = get_ordinal(symb);
         zmat[i].r = get_var(rvar, zvar, *natoms);
         zmat[i].w = get_var(wvar, zvar, *natoms);
         zmat[i].t = zmat[i].nc = 0;
      } else {
         if(sscanf(line, "%s %d %s %d %s %d %s",
           symb, &zmat[i].na, rvar, &zmat[i].nb, wvar, &zmat[i].nc, tvar) != 7) return NULL;
         atmArray[i].ord = get_ordinal(symb);
         zmat[i].r = get_var(rvar, zvar, *natoms);
         zmat[i].w = get_var(wvar, zvar, *natoms);
         zmat[i].t = get_var(tvar, zvar, *natoms);
      }
   }

   if(!int_to_cart(atmArray, zmat, *natoms)) return NULL;

   free(zmat);
   free(zvar);

   return atmArray;
}


static Mol *read_atomic_coordinates(const char *name)
{
   long fpos;
   float x, y, z, factor;
   int ord;

   free_dyna();

   if(find_string("[Atoms]")) {
      if(line[0] != '#'){
         if(strstr(line, "Angs")) {
            factor = 1;
         } else if(strstr(line, "AU")) {
            factor = float(BOHR);
         } else factor = 1;
         fpos = ftell(fp);
      }
   }
   else {
      rewind(fp);
      if(find_string("[ATOMS]")) {
         if(line[0] != '#'){
            if(strstr(line, "Angs")) {
               factor = 1;
            } else if(strstr(line, "AU")) {
               factor = float(BOHR);
            } else return 0;
            fpos = ftell(fp);
         }
      }
      else return 0;
   }

   fseek(fp, fpos, SEEK_SET);
   Mol *mol = add_mol(name);
   dynamics.molecule = mol;
   fgets(line, 255, fp);
   do {
      if(sscanf(line, "%*s %*d %d %f %f %f", &ord, &x, &y, &z) != 4) return 0;
      if(ord >= 0) mol->AddNewAtom(ord, factor*x, factor*y, factor*z);
      if(!fgets(line, 255, fp)) return mol;
   } while(strstr(line, "[") == NULL);

   return mol;
}

static Mol *read_geom(const char *name)
{
   long fpos, last;
   float x, y, z;
   char symb[100];
   int i, natoms, cp_natoms, ord, eof = 0;
   Xyzatm *atmArray = NULL;

   free_dyna();

   if(!find_string("[GEOMETRIES]")) {
      return NULL;
   }

   if(strstr(line, "XYZ")) {
      fgets(line, 255, fp);
      if(sscanf(line, "%d", &natoms) != 1) return 0;
      cp_natoms = natoms;
      do {
         fgets(line, 255, fp);
         fpos = ftell(fp);
         addTrajectoryStep(natoms, atmArray);
         if(fgets(line, 255, fp)) {
            if(sscanf(line, "%d", &natoms) != 1) natoms = 0;
         }
         else natoms = 0;
      } while(natoms != 0);

      Mol *mol = add_mol(name);

      dynamics.molecule = mol;
      fseek(fp, fpos, SEEK_SET);
      for(i=0; i<cp_natoms; i++) {
         fgets(line, 255, fp);
         if(sscanf(line, "%s %f %f %f", symb, &x, &y, &z) != 4) return 0;
         ord = get_ordinal(symb);
         mol->AddNewAtom(ord, x, y, z);
      }
      dynamics.current = dynamics.ntotalsteps - 1;
      return mol;
   }
   else if(strstr(line, "ZMAT")) {
      fgets(line, 255, fp);
      do {
         last = ftell(fp);
         if((atmArray = read_zmat(&natoms, &fpos)) == NULL) return 0;
         addTrajectoryStep(natoms, atmArray);
         free(atmArray);
         fseek(fp, fpos, SEEK_SET);
         if(fgets(line, 255, fp) == NULL) eof = 1;
      } while(!eof && !strstr(line, "[") && !isEmptyLine(line));

      fseek(fp, last, SEEK_SET);
      if((atmArray = read_zmat(&natoms, &fpos)) == NULL) return 0;

      Mol *mol = add_mol(name);

      dynamics.molecule = mol;
      for(i=0; i<natoms; i++) {
         mol->AddNewAtom(atmArray[i].ord, float(atmArray[i].x), float(atmArray[i].y), float(atmArray[i].z) );
      }
      dynamics.current = dynamics.ntotalsteps - 1;
      
      mol->dynamics = CopyDynamics( dynamics );
      free_dyna();
      free(atmArray);
      return mol;
   }

   return NULL;
}

static void addTrajectoryStep(int natoms, Xyzatm *atmArray)
{
   long /*fpos,*/ index, i;
   float x, y, z;

   index = dynamics.ntotalsteps++;

   if(!dynamics.trajectory){
      if((dynamics.trajectory = (Vector **)malloc(sizeof(Vector *))) == NULL){
         fprintf(stderr, "can't allocate dyna pointer\n");
         return;
      }
   }
   else {
      if((dynamics.trajectory = (Vector **)realloc(dynamics.trajectory,
          dynamics.ntotalsteps*sizeof(Vector *))) == NULL){
         fprintf(stderr, "can't reallocate dyna pointer\n");
         dynamics.ntotalsteps--;
         free_dyna();
         return;
      }
   }

   if((dynamics.trajectory[index] = (Vector *)malloc(natoms*sizeof(Vector))) == NULL){
      fprintf(stderr, "can't allocate timestep dyna[%d]\n", index);
      dynamics.ntotalsteps--;
      free_dyna();
      return;
   }

   if(atmArray){
      for(i=0; i<natoms; i++) {
         dynamics.trajectory[index][i].x = float(atmArray[i].x);
         dynamics.trajectory[index][i].y = float(atmArray[i].y);
         dynamics.trajectory[index][i].z = float(atmArray[i].z);
      }
   } else {
      for(i=0; i<natoms; i++) {
         fgets(line, 255, fp);
         sscanf(line, "%*s %f %f %f", &x, &y, &z);
         dynamics.trajectory[index][i].x = x;
         dynamics.trajectory[index][i].y = y;
         dynamics.trajectory[index][i].z = z;
      }
   }

   return;

}

void subst(void) {
// the double precision FORTRAN format is not recognized, exchange D with e
   char *letter;

   for(letter=line; *letter; letter++) {
      if(*letter == 'D') *letter = 'e';
   }
}

static int read_basis_set(Mol *mol)
{
   Shell *sp;
   Slater *slp;
   double s_coeff, p_coeff, d_coeff, f_coeff, alpha, norm;
   char type[5]/*, cp[256]*/;
   int d_type = 6, f_type = 10, nbr, i, prevatm = 1, atm;
   unsigned int kx, ky, kz, kr;
   float sa, sn;
   
   std::vector< int > basisIndices; // holds indices of basis set elements
   
   if(find_string("[5D]")) d_type = 5;
   rewind(fp);
// what is the default for f_type, it looks like it is 7
//   if(find_string("[7F]")) f_type = 7;
   f_type = 7;
   if(find_string("[GTO]")) {
      orbtype = GAUSS_ORB;
      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
         fgets(line, 255, fp);
         int atomIndex = -1;
         int dummy = -1;
         const int scanned = sscanf(line, "%d %d", &atomIndex, &dummy);
                
         // record basis set element index 
         basisIndices.push_back( atomIndex - 1 );
         
         if( scanned != 2 ) return 0;
         fgets(line, 255, fp);
         do {
            if(!(sp = /*ap->add_shell()*/ mol->Atoms[ atomIndex - 1 ].add_shell())) return 0;
            sscanf(line, "%s %d", type, &nbr);
            sp->scale_factor = 1.0;
            if(!fgets(line, 255, fp)) return 0;
            for(i=0; i<nbr; i++) {
              Gauss gauss;
               if(strcmp(type, "s") == 0) {
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &s_coeff);
                  gauss.exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gauss.exponent;
                  norm  = pow(2.0 * alpha / M_PI, 0.75);
                  gauss.coeff  = s_coeff * norm;
                  sp->n_base = 1;
               }
               else if(strcmp(type, "sp") == 0) {
                  subst();
                  sscanf(line, "%lf %lf %lf", &alpha, &s_coeff, &p_coeff);
                  gauss.exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gauss.exponent;
                  norm  = pow(2.0 * alpha / M_PI, 0.75);
                  gauss.coeff  = s_coeff * norm;
                  norm  = pow(128.0 * pow(alpha, 5) / pow(M_PI, 3), 0.25);
                  gauss.coeff2 = p_coeff * norm;
                  sp->n_base = 4;
               }
               else if(strcmp(type, "p") == 0){
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &p_coeff);
                  gauss.exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gauss.exponent;
                  norm  = pow(128.0 * pow(alpha, 5) / pow(M_PI, 3), 0.25);
                  gauss.coeff  = p_coeff * norm;
                  sp->n_base = 3;
               }
               else if(strcmp(type, "d") == 0){
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &d_coeff);
                  gauss.exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gauss.exponent;
                  norm = pow(2048. * pow(alpha, 7) / pow(M_PI, 3), .25);
                  gauss.coeff  = d_coeff * norm;
                  sp->n_base = d_type;
               }
               else if(strcmp(type, "f") == 0){
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &f_coeff);
                  gauss.exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gauss.exponent;
                  norm = pow(32768. * pow(alpha, 9) / pow(M_PI, 3), .25);
                  gauss.coeff = f_coeff * norm;
                  sp->n_base = f_type;
               }
               else {
                  showinfobox("No coefficients in gaussian primitive");
                  return 0;
               }
               sp->gaussians.push_back(gauss);
               if(!fgets(line, 255, fp)) return 0;
            }
            mol->nBasisFunctions += sp->n_base;
         } while(!isEmptyLine(line));
      }

      mol->normalize_gaussians();
      
      
   }
   else {
      rewind(fp);
      if(!find_string("[STO]")) return 0;
      orbtype = MLD_SLATER_ORB;
      if(!fgets(line, 255, fp)) return 0;
      MolekelAtomList::iterator ap = mol->Atoms.begin();
      do {
         while(strncmp(line, "#", 1) == 0) fgets(line, 255, fp);
         sscanf(line, "%d %d %d %d %d %f %f", &atm, &kx, &ky, &kz, &kr, &sa, &sn);
         if (prevatm != atm) {
           if (ap != mol->Atoms.end()) ++ap;
            prevatm = atm;
         }
         
         slp = /*ap->add_slater()*/ mol->Atoms[ atm - 1 ].add_slater();
         slp->a = kx;
         slp->b = ky;
         slp->c = kz;
         slp->d = kr;
         slp->exponent = sa;
         slp->norm[0] = sn;
         mol->nBasisFunctions++;
         if(!fgets(line, 255, fp)) return 0;
      } while(!strstr(line, "[") && !isEmptyLine(line));
     
   }

   if( basisIndices.size() )
   {
	   // order molecule's atoms according to basis set order
	   // basisIndices contains the correct order information: iterate over basisIndices
	   // and add atom at position basisIndices[ i ] in new array 
	   MolekelAtomList orderedAtoms;
	   for( std::vector< int >::const_iterator i = basisIndices.begin();
    		i != basisIndices.end();
    		++i )
	   {
		   orderedAtoms.push_back(  mol->Atoms[ *i ] );
	   }
   
	   // change order of atoms according to basis set order
	   mol->Atoms = orderedAtoms;
   }
   
   return 1;
}


///@todo Coefficients need to be reordered according to basis set order:
/// # create basis set order array: each element is the index of the atom
///   corresponding to the basis element
/// # create (atom index, coefficient range) array: each element contains
///   the atom index and corresponding basis set coefficients range
/// # order (atom index, coefficient range) array by atom index in ascending order
///   i.e. 0, 1, 2...num atoms - 1
/// # create new coefficient array by iterating over the (atom index, coefficient range)
///   array and copying the coefficients in the range into new array
/// # replace old coefficient array with new array
static int read_coefficients(Mol *mol)
{
   //MolekelAtom *ap;
   long fpos;
   MolecularOrbital *alphaOrb, *betaOrb;
   char o_type[8];
   double o_eigenvalue, coeff;
   float o_occ;
   int nalpha = 0, nbeta = 0;
   float aele = 0.0, bele= 0.0, sumord = 0.0;
   int eof, h, i, j, a, b, rohf = 0, spin = 0;

   if(!find_string("[MO]")) return 0;
   fpos = ftell(fp);

   while(find_string("Spin= Alpha")) nalpha++;
   rewind(fp);
   while(find_string("Spin= Beta")) nbeta++;

   if(nbeta) mol->alphaBeta = 1;
   if(mol->alphaBeta) {
      if(nalpha != nbeta) {
         printf("Numbers of Alpha and Beta orbitals need to be the same\n");
         return 0;
      }
   }

   mol->nMolecularOrbitals = nalpha;
   mol->lastOrbital = mol->firstOrbital + mol->nMolecularOrbitals;

   if((alphaOrb = allocOrbital(mol->nMolecularOrbitals, mol->nBasisFunctions, orbtype)) == NULL){
      showinfobox("can't allocate the MO-structures");
      return 0;
   }
   for(a=0; a<mol->nMolecularOrbitals; a++) {
      for(i=0; i<mol->nBasisFunctions; i++) {
         alphaOrb[a].coefficient[i] = 0;
      }
   }

   if(mol->alphaBeta) {
      if((betaOrb = allocOrbital(mol->nMolecularOrbitals, mol->nBasisFunctions, orbtype)) == NULL){
         showinfobox("can't allocate the MO-structures");
         return 0;
      }
      for(b=0; b<mol->nMolecularOrbitals; b++) {
         for(i=0; i<mol->nBasisFunctions; i++) {
            betaOrb[b].coefficient[i] = 0;
         }
      }
   }

   fseek(fp, fpos, SEEK_SET);
   a = b = h = j = eof = 0;
   do {
      sprintf(o_type, "-");
      do {
         if(!j) if(!fgets(line, 255, fp)) {
            eof = 1;
            break;
         }
         j = 0;
         if(h != 0 && (strstr(line, "=") || strstr(line, "[") || isEmptyLine(line))) {
             j = 1;
             break;
         }
         while(h == 0 && strstr(line, "=")) {
            if(strstr(line, "Sym=")) {
               sscanf(line, "%*s %s", o_type);
            } else if(strstr(line, "Ene=")) {
               sscanf(line, "%*s %lf", &o_eigenvalue);
            } else if(strstr(line, "Occup=")) {
               sscanf(line, "%*s %f", &o_occ);
            } else if(strstr(line, "Spin=")) {
               if(strstr(line, "Alpha")) {
                  spin = 1;
               }
               else if(strstr(line, "Beta")) {
                  spin = 2;
               }
            }
            if(!fgets(line, 255, fp)) return 0;
         }
         if(spin == 1) {
            sscanf(line, "%d %lf", &i, &coeff);
            alphaOrb[a].coefficient[i-1] = coeff;
         } else if(spin == 2) {
            sscanf(line, "%d %lf", &i, &coeff);
            betaOrb[b].coefficient[i-1] = coeff;
         }
         h++;
      } while(!strstr(line, "=") && !strstr(line, "[") && !isEmptyLine(line));

      h = 0;
      if(spin == 1) {
         strcpy(alphaOrb[a].type, o_type);
         alphaOrb[a].eigenvalue = o_eigenvalue;
         alphaOrb[a].occ = o_occ;
         if(alphaOrb[a].occ == 1) rohf++;
         aele += alphaOrb[a].occ;
         a++;
      } else if(spin == 2) {
         strcpy(betaOrb[b].type, o_type);
         betaOrb[b].eigenvalue = o_eigenvalue;
         betaOrb[b].occ = o_occ;
         bele += betaOrb[b].occ;
         b++;
      }
   } while(!strstr(line, "[") && !isEmptyLine(line) && !eof);
/*
   for(j=0; j<(nalpha+nbeta); j++) {
      sprintf(o_type, "-");
      for(i=0; i<mol->nBasisFunctions; i++) {
         if(!fgets(line, 255, fp)) return 0;
         while(strstr(line, "=")) {
            if(strstr(line, "Sym=")) {
               sscanf(line, "%*s %s", o_type);
            } else if(strstr(line, "Ene=")) {
               sscanf(line, "%*s %lf", &o_eigenvalue);
            } else if(strstr(line, "Occup=")) {
               sscanf(line, "%*s %f", &o_occ);
            } else if(strstr(line, "Spin=")) {
               if(strstr(line, "Alpha")) {
                  spin = 1;
               }
               else if(strstr(line, "Beta")) {
                  spin = 2;
               }
            }
            if(!fgets(line, 255, fp)) return 0;
         }
         if(spin == 1) {
            sscanf(line, "%*d %lf", &alphaOrb[a].coefficient[i]);
         } else if(spin == 2) {
            sscanf(line, "%*d %lf", &betaOrb[b].coefficient[i]);
         }
      }
      if(spin == 1) {
         strcpy(alphaOrb[a].type, o_type);
         alphaOrb[a].eigenvalue = o_eigenvalue;
         alphaOrb[a].occ = o_occ;
         if(alphaOrb[a].occ == 1) rohf++;
         aele += alphaOrb[a].occ;
         a++;
      } else if(spin == 2) {
         strcpy(betaOrb[b].type, o_type);
         betaOrb[b].eigenvalue = o_eigenvalue;
         betaOrb[b].occ = o_occ;
         bele += betaOrb[b].occ;
         b++;
      }
   }
*/
   mol->alphaOrbital = alphaOrb;
   if(mol->alphaBeta) mol->betaOrbital = betaOrb;

   for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
      sumord += ap->ord;
   }
   mol->nElectrons = int( aele + bele );
   mol->charge = sumord - mol->nElectrons;
   if(bele != 0) {
      mol->nAlpha = int( aele );
      mol->nBeta = int( bele );
   } else if(rohf) {
      mol->nBeta = int( (aele - rohf)/2 );
      mol->nAlpha = int( mol->nBeta + rohf );
   } else {
      mol->nAlpha = mol->nBeta = int( aele / 2 );
   }
   mol->multiplicity = mol->nAlpha - mol->nBeta + 1;

   qsort(mol->alphaOrbital,
         nalpha,
         sizeof(MolecularOrbital),
         compare_orbitals);
   qsort(mol->betaOrbital,
         nbeta,
         sizeof(MolecularOrbital),
         compare_orbitals);

   sprintf(line, "Charge: %2.0f", mol->charge);
   logprint(line);
   sprintf(line, "Multiplicity: %d", mol->multiplicity);
   logprint(line);
   sprintf(line, "Electrons: %d", mol->nElectrons);
   logprint(line);
   sprintf(line, "Alpha Electrons: %d", mol->nAlpha);
   logprint(line);
   sprintf(line, "Beta Electrons: %d", mol->nBeta);
   logprint(line);
   return 1;
}

static int read_frequencies( Mol* mol )
{
   long fpos;
   int n_freq = 0, /*ord,*/ i, j;
//   float x, y, z;
//   char symb[100];
//   MolekelAtom *ap;

   if(!find_string("[FR-COORD]")) return 0;
   //Mol *mol = add_mol(name);
   //dynamics.molecule = mol;
   //fgets(line, 255, fp);
   //while(strstr(line, "[FR") == 0 && !isEmptyLine(line)) {
   //   if(sscanf(line, "%s %f %f %f", symb, &x, &y, &z) != 4) return 0;
   //   ord = get_ordinal(symb);
   //   mol->AddNewAtom(ord, float(BOHR*x), float(BOHR*y), float(BOHR*z) );
   //   fgets(line, 255, fp);
   //}

   //if(!mol->natoms){
   //   sprintf(line, "No atoms in %s!", name);
   //   showinfobox(line);
   //   fclose(fp);
   //   return 0;
   //}

   //create_bonds(mol);
   //find_multiplebonds(mol);
   //new_mole(mol,name);
   //logprint("[FR-COORD] section read!");

   rewind(fp);
   if(!find_string("[FREQ]")) return 0;
   fpos = ftell(fp);
   fgets(line, 255, fp);
   while(strstr(line, "[FR") == 0 && !isEmptyLine(line)) {
      n_freq++;
      fgets(line, 255, fp);
   }
   
   mol->vibration.resize(n_freq);
   mol->n_frequencies = n_freq;

   fseek(fp, fpos, SEEK_SET);

   for(i=0; i<n_freq; i++) {
      if(!fgets(line, 255, fp)) return 0;
     // sscanf(line, "%f", mol->vibration[i].frequency);
	  sscanf(line, "%f", &mol->vibration[i].frequency);	
   }

   rewind(fp);
   if(!find_string("[FR-NORM-COORD]")) return 0;

   for(i=0; i<n_freq; i++) {
      if (!fgets(line, 255, fp)) return 0;
      //sscanf(line, " %*s %s", mol->vibration[i].type);
	  sscanf(line, " %*s %s", &mol->vibration[i].type); 
      mol->vibration[i].coord.resize(mol->natoms);
      j=0;
      for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap, j++) {
         if (!fgets(line, 255, fp)) return 0;
         //sscanf(line, "%f %f %f", mol->vibration[i].coord[j].x, mol->vibration[i].coord[j].y, mol->vibration[i].coord[j].z);
		 sscanf(line, "%f %f %f", &mol->vibration[i].coord[j].x, &mol->vibration[i].coord[j].y, &mol->vibration[i].coord[j].z); 
      }
   }

   return 1;
}


Molecule *read_molden_freq(const char *name)
{
   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      return NULL;
   }
   if(fgets(line, 255, fp) == 0 ||
       (strstr(line, "[Molden Format]") == 0 && strstr(line, "[MOLDEN FORMAT]") == 0)) {
      sprintf(line, "is not a molden format file !\n");
      showinfobox(line);
      fclose(fp);
      return NULL;
   }

//   Mol *mol = read_frequencies(name);
   Mol* mol = add_mol( name );
   /*if(!mol){
      logprint("frequencies present");
   }*/

   fclose(fp);
   update_logs();
   return mol;
}


Molecule *read_molden_geom(const char *name)
{
   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      return NULL;
   }
   if(fgets(line, 255, fp) == 0 ||
       (strstr(line, "[Molden Format]") == 0 && strstr(line, "[MOLDEN FORMAT]") == 0)) {
      sprintf(line, "is not a molden format file !\n");
      showinfobox(line);
      fclose(fp);
      update_logs();
      return NULL;
   }

   Mol *mol = read_geom(name);
   if(!mol){
      showinfobox("can't read the atomic coordinates\nfile contains probably z-matrix info");
      fclose(fp);
      update_logs();
      return NULL;
   }

   if(!mol->natoms){
      sprintf(line, "No atoms in %s!", name);
      showinfobox(line);
      fclose(fp);
      update_logs();
      return NULL;
   }

   create_bonds(mol);
   find_multiplebonds(mol);
   new_mole(mol,name);
   logprint("[GEOMETRIES] section read!");

   fclose(fp);
   update_logs();
   return mol;

}
