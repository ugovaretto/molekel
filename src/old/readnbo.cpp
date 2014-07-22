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


//To display the NBO's calculated with Gaussian:
//
//Run Gaussian with the keyworkd "PLOT" in the NBO section.
//This creates several FORTRAN output files.
//Load the regular Gaussian logfile fist.
//Load the corresponding FORTRAN file (probably rename it first to have the extension .orb)
//  containing the NBO orbital coefficients with nbo orb.
//Select and display the orbitals as you do for regular orbitals.

/* lecture of orbital coeffs form files generated
 * with the NBO program as part of g98
 * replaces existing coeffs in an existing molecule
*/


#include <cassert>
#include <cstdio>

#include "constant.h"
#include "molekeltypes.h"

using namespace std;

static char *find_string(char *s);
static int read_alpha_orb(void);
static int read_beta_orb(void);

static FILE *fp;
static char line[256];
static long previous_line = 0, preprevious = 0, preprepre = 0;


void read_nbo(char *name, Molecule* mol )
{
   assert( mol );

   if(!mol->alphaOrbital) {
      printf("Load an output-file first\n");
      return;
   }

   if((fp = fopen(name, "r")) == NULL){
      printf("can't open file\n%s!\n", name );
      return;
   }

   if(find_string("ALPHA SPIN")) {
      if(!mol->alphaBeta) {
         printf("orbitals do not match!\nnot an open shell system!\n");
         fclose(fp);
         return;
      }
   }
   else {
      if(mol->alphaBeta) {
         printf("orbitals do not match!\nnot a closed shell system!\n");
         fclose(fp);
         return;
      }
   }

   rewind (fp);

   fgets(line, 255, fp);
   fgets(line, 255, fp);
   
   if( feof( fp ) ) {
	   printf( "Not an NBO file\n" );
	   return;
   }
   
   fgets(line, 255, fp);
   if(mol->alphaBeta) fgets(line, 255, fp); 

   if( !read_alpha_orb( mol ) ) {
      printf("nbo orbitals to not macht to the actual molecule\n");
      fclose(fp);
      return;
   }

   if(mol->alphaBeta) {
      fgets(line, 255, fp);
      if(!read_beta_orb( mol ) ) {
         printf("nbo orbitals to not macht to the actual molecule\n");
         fclose(fp);
         return;
      }
   }

   fclose(fp);
}

int read_alpha_orb(Molecule* mol)
{
   register int i, j;

   for(i=0; i<mol->nMolecularOrbitals; i++){
      mol->alphaOrbital[i].eigenvalue = i+1;
      mol->alphaOrbital[i].occ = 1;
      for(j=0; j<mol->nBasisFunctions; j+=5){
         if(fgets(line, 255, fp) == NULL) return 0;
          sscanf(line, "%lf %lf %lf %lf %lf",
            &mol->alphaOrbital[i].coefficient[j],
            &mol->alphaOrbital[i].coefficient[j+1],
            &mol->alphaOrbital[i].coefficient[j+2],
            &mol->alphaOrbital[i].coefficient[j+3],
            &mol->alphaOrbital[i].coefficient[j+4]);
      }
   }
   return 1;
}

int read_beta_orb(Molecule* mol)
{
   register int i, j;

   for(i=0; i<mol->nMolecularOrbitals; i++){
      mol->betaOrbital[i].eigenvalue = i+1;
      mol->betaOrbital[i].occ = 1;
      for(j=0; j<mol->nBasisFunctions; j+=5){
         if(fgets(line, 255, fp) == NULL) return 0;
          sscanf(line, "%lf %lf %lf %lf %lf",
            &mol->betaOrbital[i].coefficient[j],
            &mol->betaOrbital[i].coefficient[j+1],
            &mol->betaOrbital[i].coefficient[j+2],
            &mol->betaOrbital[i].coefficient[j+3],
            &mol->betaOrbital[i].coefficient[j+4]);
      }
   }
   return 1;
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

