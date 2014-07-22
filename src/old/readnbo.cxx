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

#include "main.h"
#include "molekel.h"
#include "constant.h"
#include "readnbo.h"
#include "general.h"
#include "maininterf.h"
#include "utils.h"


static char *find_string(char *s);
static int read_alpha_orb(void);
static int read_beta_orb(void);

static FILE *fp;
static char line[256];
static long previous_line = 0, preprevious = 0, preprepre = 0;


void read_nbo(char *name)
{
   if(!firstmol) {
      logprint("No molecule loaded");
      update_logs();
      return;
   }

   if(!actualmol || !actualmol->alphaOrbital) {
      logprint("Load an output-file first");
      update_logs();
      return;
   }

   if((fp = fopen(name, "r")) == NULL){
      sprintf(line, "can't open file\n%s !", name);
      showinfobox(line);
      update_logs();
      return;
   }

   if(find_string("ALPHA SPIN")) {
      if(!actualmol->alphaBeta) {
         showinfobox("orbitals do not match!\nnot an open shell system!");
         fclose(fp);
         update_logs();
         return;
      }
   }
   else {
      if(actualmol->alphaBeta) {
         showinfobox("orbitals do not match!\nnot a closed shell system!");
         fclose(fp);
         update_logs();
         return;
      }
   }

   rewind (fp);

   fgets(line, 255, fp);
   fgets(line, 255, fp);
   if((strchr) == NULL) {
      showinfobox("not a nbo orbital file!");
      fclose(fp);
      update_logs();
      return;
   }
   logprint(line);
   fgets(line, 255, fp);
   if(actualmol->alphaBeta) fgets(line, 255, fp); 

   if(!read_alpha_orb()){
      showinfobox("nbo orbitals to not macht to the actual molecule");
      fclose(fp);
      update_logs();
      return;
   }

   if(actualmol->alphaBeta) {
      fgets(line, 255, fp);
      if(!read_beta_orb()){
         showinfobox("nbo orbitals to not macht to the actual molecule");
         fclose(fp);
         update_logs();
         return;
      }
   }

   fclose(fp);
   update_logs();
}

int read_alpha_orb(void)
{
   register int i, j;

   for(i=0; i<actualmol->nMolecularOrbitals; i++){
      actualmol->alphaOrbital[i].eigenvalue = i+1;
      actualmol->alphaOrbital[i].occ = 1;
      for(j=0; j<actualmol->nBasisFunctions; j+=5){
         if(fgets(line, 255, fp) == NULL) return 0;
          sscanf(line, "%lf %lf %lf %lf %lf",
            &actualmol->alphaOrbital[i].coefficient[j],
            &actualmol->alphaOrbital[i].coefficient[j+1],
            &actualmol->alphaOrbital[i].coefficient[j+2],
            &actualmol->alphaOrbital[i].coefficient[j+3],
            &actualmol->alphaOrbital[i].coefficient[j+4]);
      }
   }
   return 1;
}

int read_beta_orb(void)
{
   register int i, j;

   for(i=0; i<actualmol->nMolecularOrbitals; i++){
      actualmol->betaOrbital[i].eigenvalue = i+1;
      actualmol->betaOrbital[i].occ = 1;
      for(j=0; j<actualmol->nBasisFunctions; j+=5){
         if(fgets(line, 255, fp) == NULL) return 0;
          sscanf(line, "%lf %lf %lf %lf %lf",
            &actualmol->betaOrbital[i].coefficient[j],
            &actualmol->betaOrbital[i].coefficient[j+1],
            &actualmol->betaOrbital[i].coefficient[j+2],
            &actualmol->betaOrbital[i].coefficient[j+3],
            &actualmol->betaOrbital[i].coefficient[j+4]);
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

