/*  MOLEKEL, Version 4.3, Date: 11.Nov.02
 *  Copyright (C) 2000-2002 Stefan Portmann (CSCS/ETHZ)
 *  (original IRIX GL implementation, concept and data structure
 *   by Peter F. Fluekiger, CSCS/UNI Geneva)
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


/* reading molden format */
#include "main.h"
#include "constant.h"
#include "molekel.h"
#include "general.h"
#include "maininterf.h"
#include "readmolden.h"
#include "box.h"
#include "utils.h"

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

static int read_atomic_coordinates(char *name);
static int read_geom(char *name);
static char *find_string(char *s);
static void addTrajectoryStep(int natoms, Xyzatm *atmArray);
static int read_atomic_charges(void);
static int read_basis_set(void);
static int read_coefficients(void);
static int read_occupations(void);
static int read_frequencies(char *name);
static int read_dipole(void);

static FILE *fp;
static char line[256];
static long previous_line = 0, preprevious = 0;
static int orbtype = GAUSS_ORB;

void read_molden(char *name)
{
   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      return;
   }
   if(fgets(line, 255, fp) == 0 || 
       (strstr(line, "[Molden Format]") == 0 && strstr(line, "[MOLDEN FORMAT]") == 0 &&
         strstr(line, "[Title]") == 0)) {
      sprintf(line, "is not a molden format file !\n");
      showinfobox(line);
      fclose(fp);
      return;
   }

   if(!read_atomic_coordinates(name)){
      showinfobox("can't read the atomic coordinates\nfile contains probably z-matrix info");
      fclose(fp);
      update_logs();
      return;
   }

   if(!actualmol->natoms){
      sprintf(line, "No atoms in %s!", name);
      showinfobox(line);
      fclose(fp);
      return;
   }

   create_bonds();
   find_multiplebonds();
   new_mole(name);
   logprint("[ATOMS] section read!");

   rewind(fp);
   if(!read_basis_set()){
      showinfobox("can't read the basis-set");
      fclose(fp);
      update_logs();
      return;
   }

   rewind(fp);
   if(!read_coefficients()){
      showinfobox("can't read the MO-coefficients");
      fclose(fp);
      update_logs();
      return;
   }

   fclose(fp);
   update_logs();

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
   int i;

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


static int read_atomic_coordinates(char *name)
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
            factor = BOHR;
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
               factor = BOHR;
            } else return 0;
            fpos = ftell(fp);
         }
      } 
      else return 0;
   }

   fseek(fp, fpos, SEEK_SET);
   add_mol(name);
   dynamics.molecule = actualmol;     
   fgets(line, 255, fp);
   do {
      if(sscanf(line, "%*s %*d %d %f %f %f", &ord, &x, &y, &z) != 4) return 0;
      if(ord >= 0) add_atom(ord, factor*x, factor*y, factor*z);
      if(!fgets(line, 255, fp)) return 1;
   } while(strstr(line, "[") == NULL);

   return 1;
}

static int read_geom(char *name)
{
 


   long fpos, last;
   float x, y, z;
   char symb[100];
   int i, natoms, cp_natoms, ord, eof = 0;
   Xyzatm *atmArray = NULL;

   free_dyna();

   if(!find_string("[GEOMETRIES]")) {
      return 0;
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
      add_mol(name);
      dynamics.molecule = actualmol;
      fseek(fp, fpos, SEEK_SET);
      for(i=0; i<cp_natoms; i++) {
         fgets(line, 255, fp);
         if(sscanf(line, "%s %f %f %f", symb, &x, &y, &z) != 4) return 0;
         ord = get_ordinal(symb);
         add_atom(ord, x, y, z);
      }
      dynamics.current = dynamics.ntotalsteps - 1;
      return 1;
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
      add_mol(name);
      dynamics.molecule = actualmol;
      for(i=0; i<natoms; i++) {
         add_atom(atmArray[i].ord, atmArray[i].x, atmArray[i].y, atmArray[i].z); 
      }
      dynamics.current = dynamics.ntotalsteps - 1;
      free(atmArray);
      return 1;
   }

   return 0;
}

static void addTrajectoryStep(int natoms, Xyzatm *atmArray)
{
   long fpos, index, i;
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
         dynamics.trajectory[index][i].x = atmArray[i].x;
         dynamics.trajectory[index][i].y = atmArray[i].y;
         dynamics.trajectory[index][i].z = atmArray[i].z;
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

static int read_basis_set(void)
{
   AtoM *ap;
   Gauss *gp;
   Shell *sp;
   Slater *slp;
   double s_coeff, p_coeff, d_coeff, f_coeff, alpha, norm;
   char type[5], cp[256];
   int d_type = 6, f_type = 10, nbr, i, prevatm = 1, atm;
   unsigned int kx, ky, kz, kr;
   float sa, sn;

   if(find_string("[5D]")) d_type = 5;
   rewind(fp);
// what is the default for f_type, it looks like it is 7
//   if(find_string("[7F]")) f_type = 7;
   f_type = 7;
   if(find_string("[GTO]")) {
      orbtype = GAUSS_ORB;
      for(ap = actualmol->firstatom; ap; ap = ap->next){
         fgets(line, 255, fp);
         fgets(line, 255, fp);
         do {
            if(!(sp = add_shell(ap))) return 0;
            sscanf(line, "%s %d", type, &nbr);
            sp->scale_factor = 1.0;
            if(!fgets(line, 255, fp)) return 0;
            for(i=0; i<nbr; i++) {
               gp = add_gauss(sp);
               if(strcmp(type, "s") == 0) {
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &s_coeff);
                  gp->exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gp->exponent;
                  norm  = pow(2.0 * alpha / M_PI, 0.75);
                  gp->coeff  = s_coeff * norm;
                  sp->n_base = 1;
               }
               else if(strcmp(type, "sp") == 0) {
                  subst();
                  sscanf(line, "%lf %lf %lf", &alpha, &s_coeff, &p_coeff);
                  gp->exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gp->exponent;
                  norm  = pow(2.0 * alpha / M_PI, 0.75);
                  gp->coeff  = s_coeff * norm;
                  norm  = pow(128.0 * pow(alpha, 5) / pow(M_PI, 3), 0.25);
                  gp->coeff2 = p_coeff * norm;
                  sp->n_base = 4;
               }
               else if(strcmp(type, "p") == 0){
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &p_coeff);
                  gp->exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gp->exponent;
                  norm  = pow(128.0 * pow(alpha, 5) / pow(M_PI, 3), 0.25);
                  gp->coeff  = p_coeff * norm;
                  sp->n_base = 3;
               }
               else if(strcmp(type, "d") == 0){
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &d_coeff);
                  gp->exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gp->exponent;
                  norm = pow(2048. * pow(alpha, 7) / pow(M_PI, 3), .25);
                  gp->coeff  = d_coeff * norm;
                  sp->n_base = d_type;
               }
               else if(strcmp(type, "f") == 0){
                  subst();
                  sscanf(line, "%lf %lf", &alpha, &f_coeff);
                  gp->exponent = alpha * sp->scale_factor * sp->scale_factor;
                  alpha = gp->exponent;
                  norm = pow(32768. * pow(alpha, 9) / pow(M_PI, 3), .25);
                  gp->coeff = f_coeff * norm;
                  sp->n_base = f_type;
               }
               else {
                  showinfobox("No coefficients in gaussian primitive");
                  return 0;
               }
               if(!fgets(line, 255, fp)) return 0;
            }
            actualmol->nBasisFunctions += sp->n_base;
         } while(!isEmptyLine(line));
      }
   
      normalize_gaussians();
   
      return 1;
   }
   else {
      rewind(fp);
      if(!find_string("[STO]")) return 0;
      orbtype = MLD_SLATER_ORB;
      if(!fgets(line, 255, fp)) return 0;
      ap = actualmol->firstatom;
      do {
         while(strncmp(line, "#", 1) == 0) fgets(line, 255, fp);
         sscanf(line, "%d %d %d %d %d %f %f", &atm, &kx, &ky, &kz, &kr, &sa, &sn);
         if(prevatm != atm) {
            if(ap->next) ap = ap->next;
            prevatm = atm;
         }
         slp = add_slater(ap);
         slp->a = kx;
         slp->b = ky;
         slp->c = kz;
         slp->d = kr;
         slp->exponent = sa;
         slp->norm[0] = sn;
         actualmol->nBasisFunctions++;
         if(!fgets(line, 255, fp)) return 0;
      } while(!strstr(line, "[") && !isEmptyLine(line));
      return 1;
   }


}

static int read_coefficients(void)
{
   AtoM *ap;
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

   if(nbeta) actualmol->alphaBeta = 1;
   if(actualmol->alphaBeta) {
      if(nalpha != nbeta) {
         printf("Numbers of Alpha and Beta orbitals need to be the same\n");
         return 0;
      }
   }

   actualmol->nMolecularOrbitals = nalpha;
   actualmol->lastOrbital = actualmol->firstOrbital + actualmol->nMolecularOrbitals;

   if((alphaOrb = allocOrbital(actualmol->nMolecularOrbitals, actualmol->nBasisFunctions, orbtype)) == NULL){
      showinfobox("can't allocate the MO-structures");
      return 0;
   }
   for(a=0; a<actualmol->nMolecularOrbitals; a++) {
      for(i=0; i<actualmol->nBasisFunctions; i++) {
         alphaOrb[a].coefficient[i] = 0;
      }
   }

   if(actualmol->alphaBeta) {
      if((betaOrb = allocOrbital(actualmol->nMolecularOrbitals, actualmol->nBasisFunctions, orbtype)) == NULL){
         showinfobox("can't allocate the MO-structures");
         return 0;
      }
      for(b=0; b<actualmol->nMolecularOrbitals; b++) {
         for(i=0; i<actualmol->nBasisFunctions; i++) {
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
      for(i=0; i<actualmol->nBasisFunctions; i++) {
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
   actualmol->alphaOrbital = alphaOrb;
   if(actualmol->alphaBeta) actualmol->betaOrbital = betaOrb;

   for(ap=actualmol->firstatom; ap; ap = ap->next) {
      sumord += ap->ord;
   }
   actualmol->nElectrons = aele + bele;
   actualmol->charge = sumord - actualmol->nElectrons;
   if(bele != 0) {
      actualmol->nAlpha = aele;
      actualmol->nBeta = bele;
   } else if(rohf) {
      actualmol->nBeta = (aele - rohf)/2;
      actualmol->nAlpha = actualmol->nBeta + rohf;
   } else {
      actualmol->nAlpha = actualmol->nBeta = aele / 2;
   }
   actualmol->multiplicity = actualmol->nAlpha - actualmol->nBeta + 1;

   qsort(actualmol->alphaOrbital,
         nalpha,
         sizeof(MolecularOrbital),
         compare_orbitals);
   qsort(actualmol->betaOrbital,
         nbeta,
         sizeof(MolecularOrbital),
         compare_orbitals);

   sprintf(line, "Charge: %2.0f", actualmol->charge);
   logprint(line);
   sprintf(line, "Multiplicity: %d", actualmol->multiplicity);
   logprint(line);
   sprintf(line, "Electrons: %d", actualmol->nElectrons);
   logprint(line);
   sprintf(line, "Alpha Electrons: %d", actualmol->nAlpha);
   logprint(line);
   sprintf(line, "Beta Electrons: %d", actualmol->nBeta);
   logprint(line);
   return 1;
}

static int read_frequencies(char *name)
{
   long fpos;
   int n_freq = 0, ord, i, j;
   float x, y, z;
   char symb[100];
   AtoM *ap;
   Vibration *vib;

   if(!find_string("[FR-COORD]")) return 0;
   add_mol(name);
   dynamics.molecule = actualmol;     
   fgets(line, 255, fp);
   while(strstr(line, "[FR") == 0 && !isEmptyLine(line)) {
      if(sscanf(line, "%s %f %f %f", symb, &x, &y, &z) != 4) return 0;
      ord = get_ordinal(symb);
      add_atom(ord, BOHR*x, BOHR*y, BOHR*z);
      fgets(line, 255, fp);
   }

   if(!actualmol->natoms){
      sprintf(line, "No atoms in %s!", name);
      showinfobox(line);
      fclose(fp);
      return 0;
   }

   create_bonds();
   find_multiplebonds();
   new_mole(name);
   logprint("[FR-COORD] section read!");

   rewind(fp);
   if(!find_string("[FREQ]")) return 0;
   fpos = ftell(fp);
   fgets(line, 255, fp);
   while(strstr(line, "[FR") == 0 && !isEmptyLine(line)) {
      n_freq++;
      fgets(line, 255, fp);
   }

   if((actualmol->vibration = (Vibration *)malloc(n_freq*sizeof(Vibration))) == NULL){
      showinfobox("can't realloc vibrational frequency");
      return 0;
   }
   for(i=0, vib=actualmol->vibration; i<n_freq; i++, vib++){
      if((vib->coord = (Vector *)malloc(actualmol->natoms*sizeof(Vector))) == NULL){
         showinfobox("can't allocate vibration");
         return 0;
      }
   }

   actualmol->n_frequencies = n_freq;

   fseek(fp, fpos, SEEK_SET);

   for(i=0, vib=actualmol->vibration; i < n_freq; i++, vib++) {
      if(!fgets(line, 255, fp)) return 0;
      sscanf(line, "%f", &vib->frequency);
   }

   rewind(fp);
   if(!find_string("[FR-NORM-COORD]")) return 0;

   for(i=0, vib=actualmol->vibration; i < n_freq; i++, vib++) {
      if(!fgets(line, 255, fp)) return 0;
      sscanf(line, " %*s %s", vib->type);
      for(ap=actualmol->firstatom, j=0; ap; ap=ap->next, j++){
         if(!fgets(line, 255, fp)) return 0;
         sscanf(line, "%f %f %f", &vib->coord[j].x, &vib->coord[j].y, &vib->coord[j].z);
      }
   }

   return 1;
}


void read_molden_freq(char *name)
{
   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      return;
   }
   if(fgets(line, 255, fp) == 0 || 
       (strstr(line, "[Molden Format]") == 0 && strstr(line, "[MOLDEN FORMAT]") == 0)) {
      sprintf(line, "is not a molden format file !\n");
      showinfobox(line);
      fclose(fp);
      return;
   }

   if(read_frequencies(name)){
      logprint("frequencies present");
   }

   fclose(fp);
   update_logs();
}


void read_molden_geom(char *name)
{
   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      return;
   }
   if(fgets(line, 255, fp) == 0 || 
       (strstr(line, "[Molden Format]") == 0 && strstr(line, "[MOLDEN FORMAT]") == 0)) {
      sprintf(line, "is not a molden format file !\n");
      showinfobox(line);
      fclose(fp);
      update_logs();
      return;
   }

   if(!read_geom(name)){
      showinfobox("can't read the atomic coordinates\nfile contains probably z-matrix info");
      fclose(fp);
      update_logs();
      return;
   }

   if(!actualmol->natoms){
      sprintf(line, "No atoms in %s!", name);
      showinfobox(line);
      fclose(fp);
      update_logs();
      return;
   }

   create_bonds();
   find_multiplebonds();
   new_mole(name);
   logprint("[GEOMETRIES] section read!");

   fclose(fp);
   update_logs();

}
