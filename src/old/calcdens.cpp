// UV original molekel file with bug fixes - will be kept until proper support
// for Gaussian and Gamess I/O is added to OpenBabel

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

//////// Previous copyright notices /////////

/*  MOLEKEL, Version 4.4, Date: 10.Dec.03
 *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
 *  (original IRIX GL implementation, concept and data structure
 *   by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
 *   and revisions by Stefan Portmann, CSCS/ETHZ)
 */

/*  MOLEKEL, Version 4.4, Date: 10.Dec.03
 *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
 *  (original IRIX GL implementation, concept and data structure
 *  by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
 *  and revisions by Stefan Portmann, CSCS/ETHZ)
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


#ifndef WIN32
#include <signal.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/param.h>
#include <sys/stat.h>
#endif
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <limits>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include "constant.h"
////////////////////////////////////////////////
extern void logprint( const char* );
extern void showinfobox( const char* );
extern void update_logs();

// #include "drawing.h"

extern unsigned long my_pick(short key);
#ifdef AIX
extern double std::ceil(double);
#endif

/*----------------------------------------*/

struct box {
    float x1, x2;
    float y1, y2;
    float z1, z2;
    float cubesize;
};

Box box;
float orbcubesize = 0.25;
int order(float *small_, float *big_);
/*----------------------------------------*/

void create_box(Mol *mol)
{
   if(!mol) return;

   MolekelAtomList::iterator ap = mol->Atoms.begin();
   box.x1 = box.x2 = ap->coord[0];
   box.y1 = box.y2 = ap->coord[1];
   box.z1 = box.z2 = ap->coord[2];
   for (++ap; ap!=mol->Atoms.end(); ++ap) {
      if(ap->coord[0] < box.x1) box.x1 = ap->coord[0];
      if(ap->coord[0] > box.x2) box.x2 = ap->coord[0];
      if(ap->coord[1] < box.y1) box.y1 = ap->coord[1];
      if(ap->coord[1] > box.y2) box.y2 = ap->coord[1];
      if(ap->coord[2] < box.z1) box.z1 = ap->coord[2];
      if(ap->coord[2] > box.z2) box.z2 = ap->coord[2];
   }

   box.x1 -= 1.5; box.x2 += 1.5;
   box.y1 -= 1.5; box.y2 += 1.5;
   box.z1 -= 1.5; box.z2 += 1.5;
   box.cubesize = orbcubesize;

}


void reset_box(Mol *mol)
{
   create_box(mol);
}


void define_box(Mol *mol, float *dim, int *ncub)
{
   float x, y, z;

   if(!mol) return;

   dim[0] = box.x1;   dim[1] = box.x2;
   dim[2] = box.y1;   dim[3] = box.y2;
   dim[4] = box.z1;   dim[5] = box.z2;

   x = dim[1] - dim[0]; y = dim[3] - dim[2]; z = dim[5] - dim[4];

   ncub[0] = (int)std::ceil(x/box.cubesize) + 1;
   ncub[1] = (int)std::ceil(y/box.cubesize) + 1;
   ncub[2] = (int)std::ceil(z/box.cubesize) + 1;
}


int correct_box()
{
   return (
      order(&box.x1, &box.x2) ||
      order(&box.y1, &box.y2) ||
      order(&box.z1, &box.z2) );
}




int order(float *small_, float *big_)
{
   float temp;

   if(*small_ <= *big_)return 0;

   temp = *small_;
   *small_ = *big_;
   *big_ = temp;
   return 1;
}


////////////////////////////////////////////////


void calc_chi(Mol *mol, float x, float y, float z);
double calc_mep(Mol *mol, float x, float y, float z);
double calc_point(Mol *mol, float x, float y, float z);
double calc_prddo_density(Mol *mol, float x, float y, float z);
double calc_prddo_spindensity(Mol *mol, float x, float y, float z);
double calc_sltr_point(Mol *mol, float x, float y, float z);
double calc_sltr_spindensity(Mol *mol, float x, float y, float z);
double calc_sltr_density(Mol *mol, float x, float y, float z);
double calc_prddo_point(Mol *mol, float x, float y, float z);
double calculate_density(Mol *mol, float x, float y, float z);
int generate_density_matrix(Mol *mol, int key);
double calculateSomo(Mol *mol, float x, float y, float z);
extern void *alloc_trimat(int n, size_t size);
extern Element element[ 105 ];
// from chooseinterf
int datasource = USE_COEFFS;

#define NOSOCK

#define POW_5(x,y) ((y)>0?(x):1)
#define POW_4(x,y) ((y)>0?(x)*POW_5((x),(y)-1):1)
#define POW_3(x,y) ((y)>0?(x)*POW_4((x),(y)-1):1)
#define POW_2(x,y) ((y)>0?(x)*POW_3((x),(y)-1):1)
#define POW_1(x,y) ((y)>0?(x)*POW_2((x),(y)-1):1)
#define POW(x,y)  ((y)>0?(x)*POW_1((x),(y)-1):1)


int did_sigset = 0, pipein = 0, pipeout = 0;
int child_action_key = 0;
float **density;
double *chi;
MolecularOrbital *molOrb;
char timestring[300];


//-----------------------------------------------------------------------------
/// Set this variable to false from a separate thread to stop computation.
static bool stop = false;

/// Called from a thread different from the one that started vtk_process_calc
/// to interrrupt computation.
void StopProcessCalc() { stop = true; }

/// Returns value of stop variable.
bool ProcessCalcStopped() { return stop; }

namespace
{
    double minValue = double();
    double maxValue = double();
    int type = -1;
}

/// Returns min, max value in dataset computed by vtk_process_calc.
void GetProcessCalcMinMax( double& minVal, double& maxVal ) { minVal = minValue; maxVal = maxValue; }

/// Returns type of data generated by last call to vtk_process_calc.
int GetProcessCalcDataType() { return type; }

/// returns vtk image data instead of writing to macu file.
/// @param progressCBack pointer to function that will be called to notify
///        observer of completed step; steps go from 0 to ncubes[ 2 ] - 1.
/// @param cbackData data provided by calling function that will be returned
///        in a call to progressCBack function.
vtkImageData* vtk_process_calc( Mol *mol,
                                float *dim,
                                int *ncubes,
                                int key,
                                void ( *progressCBack )( int completedStep,
                                                         int totalSteps,
                                                         void* cbackData ) = 0,
                                void* cbackData = 0 )
{

  stop = false;
  float x, y, z, dx, dy, dz;
  short i, j, k;
  int len, ncub[3];

  type = -1;

  double (*funct)(Mol *mol, float x, float y, float z);

  minValue = std::numeric_limits< double >::max();
  maxValue = std::numeric_limits< double >::min();

  ncub[0] = *ncubes++;
  ncub[1] = *ncubes++;
  ncub[2] = *ncubes++;


  // UV why do we need alha/beta orbital information when key == MEP ?
  if( key != MEP )
  {
  if((mol->alphaOrbital[0].flag == ADF_ORB_A ||
   mol->alphaOrbital[0].flag == ADF_ORB_B ) &&
   mol->alphaOrbital[0].coefficient == NULL) {
   //executeAdfUtilities(mol, s, dim, ncub, key);
   return 0;
  }

  chi = (double *)calloc(mol->nBasisFunctions, sizeof(double));
  // UV following commented code is executed in calc_chi
  //memset( chi, 0, mol->nBasisFunctions * sizeof( double ) );
  if (!chi && !(mol->alphaOrbital[0].flag == EHT_ORB)) {
   fprintf(stderr, "can't allocate chi\n");
   return 0;
  }
  } // if( key != MEP )
  switch(key) {
   case CALC_ORB  :
    switch(mol->alphaOrbital[0].flag) {
      case GAMESS_ORB :
      case HONDO_ORB  :
      case GAUSS_ORB  : funct = calc_point; break;
  /* to be fixed
          case ADF_ORB_A  :
  case ADF_ORB_B  : funct = calc_adf_point; break;

  case EHT_ORB   : funct = calc_eht_point; break;
  */
      case MOS_ORB   :
      case ZINDO_ORB  :
      case PRDDO_ORB  : funct = calc_prddo_point; break;
      case MLD_SLATER_ORB  : funct = calc_sltr_point; break;
    }
    break;

   case EL_DENS :
    switch(mol->alphaOrbital[0].flag) {
      case GAMESS_ORB :
      case HONDO_ORB  :
      case GAUSS_ORB  :
       if (!generate_density_matrix(mol, key)) {
        fprintf(stderr, "Can't generate the density matrix!\n");
        strcpy(timestring, "Can't generate the density matrix!");
        return 0;
       }
       else {
        printf("density matrix generated...\n");
        funct = calculate_density;
       }
       break;

  /* to be fixed
          case ADF_ORB_A  :
  case ADF_ORB_B  : funct = calc_adf_density; break;

  case EHT_ORB   : funct = calc_eht_density; break;
  */
      case MOS_ORB   :
      case ZINDO_ORB  :
      case PRDDO_ORB  : funct = calc_prddo_density; break;
      case MLD_SLATER_ORB  : funct = calc_sltr_density; break;
    }
    break;

   case SPIN_DENS :
    switch(mol->alphaOrbital[0].flag) {
      case GAMESS_ORB :
      case HONDO_ORB  :
      case GAUSS_ORB  :
       if (!mol->alphaBeta) {
        funct = calculateSomo;
       }
       else if(!generate_density_matrix(mol, key)) {
        fprintf(stderr, "Can't generate the density matrix!\n");
        strcpy(timestring, "Can't generate the density matrix!");
        return 0;
       }
       else {
        printf("density matrix generated...\n");
        funct = calculate_density;
       }
       break;
  /* to be fixed
  case ADF_ORB_A  :
          case ADF_ORB_B  :
  */
      case MOS_ORB   :
      case ZINDO_ORB  :
      case PRDDO_ORB  : funct = calc_prddo_spindensity; break;
      case MLD_SLATER_ORB  : funct = calc_sltr_spindensity; break;
    }
    break;

   case MEP :
    funct = calc_mep;
    break;
  }


  dx = (dim[1]-dim[0])/(ncub[0]-1);
  dy = (dim[3]-dim[2])/(ncub[1]-1);
  dz = (dim[5]-dim[4])/(ncub[2]-1);
  len = 4*ncub[0]*ncub[1];

  vtkSmartPointer< vtkImageData > image( vtkImageData::New() );
  image->SetDimensions( ncub[ 0 ], ncub[ 1 ], ncub[ 2 ] );
  image->SetOrigin( dim[0],
                    dim[2],
                    dim[4] );
  image->SetSpacing( dx, dy, dz );

  const int totalSteps = ncub[ 0 ] * ncub[ 1 ] * ncub[ 2 ];
  if( progressCBack ) progressCBack( 0, totalSteps, cbackData );
  for (i=0, z=dim[4]; i<ncub[2]; i++, z += dz) {
   for (j=0, y=dim[2]; j<ncub[1]; j++, y += dy) {
	for (k=0, x=dim[0]; k<ncub[0]; k++, x += dx) {
      const double s = (*funct)(mol, x, y, z);
      if( s < minValue ) minValue = s;
      if( s > maxValue ) maxValue = s;
      image->SetScalarComponentFromDouble( k, j, i, 0, s );
      if( stop == true ) goto stopped; // forward jump to stopped label
    }
   }
 
// Execution will jump to this label iff stop requested   
stopped: 

   const int idx = ncub[ 0 ] * ncub[ 1 ] * ( i + 1 );
   // invoke progress callback function.
   if( progressCBack ) progressCBack( idx, totalSteps, cbackData );
  }
  printf("\n");

  if(density){
   free(density[0]);
   free(density);
  }
  density = NULL;
  if(chi)
  {
     free(chi);
     chi = NULL;
  }
  type = key;
  return image;
}

//-----------------------------------------------------------------------------
void process_calc(Mol *mol, const char *s, float *dim, int *ncubes, int key)
{
  float x, y, z, dx, dy, dz;
  float **slice, *array;
  short i, j, k;
  int len, ncub[3];
  FILE *fp;
  #ifndef NOSOCK
  struct tms starttime, endtime;
  float systime, cputime;
  #endif
  double (*funct)(Mol *mol, float x, float y, float z);

  ncub[0] = *ncubes++;
  ncub[1] = *ncubes++;
  ncub[2] = *ncubes++;

  if((mol->alphaOrbital[0].flag == ADF_ORB_A ||
   mol->alphaOrbital[0].flag == ADF_ORB_B ) &&
   mol->alphaOrbital[0].coefficient == NULL) {
   //executeAdfUtilities(mol, s, dim, ncub, key); UV XXX REMOVED
   return;
  }

  if((fp = fopen(s, "wb")) == NULL){
   fprintf(stderr, "can't open .macu file\n");
   return;
  }

  if ((array = (float *)malloc(ncub[0]*ncub[1]*sizeof(float))) == NULL){
   fprintf(stderr, "can't allocate slice\n");
   exit(-1);
  }
  if ((slice = (float **)malloc(ncub[1]*sizeof(float *))) == NULL){
   fprintf(stderr, "can't allocate slice\n");
   exit(-1);
  }
  for (i=0; i<ncub[1]; i++) slice[i] = array + (i * ncub[0]);

  chi = (double *)calloc(mol->nBasisFunctions, sizeof(double));
  if (!chi && !(mol->alphaOrbital[0].flag == EHT_ORB)) {
   fprintf(stderr, "can't allocate chi\n");
   return;
  }

  switch(key) {
   case CALC_ORB  :
    switch(mol->alphaOrbital[0].flag) {
      case GAMESS_ORB :
      case HONDO_ORB  :
      case GAUSS_ORB  : funct = calc_point; break;
  /* to be fixed
          case ADF_ORB_A  :
  case ADF_ORB_B  : funct = calc_adf_point; break;

  case EHT_ORB   : funct = calc_eht_point; break;
  */
      case MOS_ORB   :
      case ZINDO_ORB  :
      case PRDDO_ORB  : funct = calc_prddo_point; break;
      case MLD_SLATER_ORB  : funct = calc_sltr_point; break;
    }
    break;

   case EL_DENS :
    switch(mol->alphaOrbital[0].flag) {
      case GAMESS_ORB :
      case HONDO_ORB  :
      case GAUSS_ORB  :
       if (!generate_density_matrix(mol, key)) {
        fprintf(stderr, "Can't generate the density matrix!\n");
        strcpy(timestring, "Can't generate the density matrix!");
        return;
       }
       else {
        printf("density matrix generated...\n");
        funct = calculate_density;
       }
       break;

  /* to be fixed
          case ADF_ORB_A  :
  case ADF_ORB_B  : funct = calc_adf_density; break;

  case EHT_ORB   : funct = calc_eht_density; break;
  */
      case MOS_ORB   :
      case ZINDO_ORB  :
      case PRDDO_ORB  : funct = calc_prddo_density; break;
      case MLD_SLATER_ORB  : funct = calc_sltr_density; break;
    }
    break;

   case SPIN_DENS :
    switch(mol->alphaOrbital[0].flag) {
      case GAMESS_ORB :
      case HONDO_ORB  :
      case GAUSS_ORB  :
       if (!mol->alphaBeta) {
        funct = calculateSomo;
       }
       else if(!generate_density_matrix(mol, key)) {
        fprintf(stderr, "Can't generate the density matrix!\n");
        strcpy(timestring, "Can't generate the density matrix!");
        return;
       }
       else {
        printf("density matrix generated...\n");
        funct = calculate_density;
       }
       break;
  /* to be fixed
  case ADF_ORB_A  :
          case ADF_ORB_B  :
  */
      case MOS_ORB   :
      case ZINDO_ORB  :
      case PRDDO_ORB  : funct = calc_prddo_spindensity; break;
      case MLD_SLATER_ORB  : funct = calc_sltr_spindensity; break;
    }
    break;

   case MEP :
    funct = calc_mep;
    break;
  }

  len = 36;
  fwrite(&len, sizeof(int), 1, fp);
  fwrite(dim, sizeof(float), 6, fp);
  fwrite(ncub, sizeof(int), 3, fp);
  fwrite(&len, sizeof(int), 1, fp);

  dx = (dim[1]-dim[0])/(ncub[0]-1);
  dy = (dim[3]-dim[2])/(ncub[1]-1);
  dz = (dim[5]-dim[4])/(ncub[2]-1);
  len = 4*ncub[0]*ncub[1];
  #ifndef NOSOCK
  times(&starttime);
  #endif

  for (i=0, z=dim[4]; i<ncub[2]; i++, z += dz) {
   for (j=0, y=dim[2]; j<ncub[1]; j++, y += dy) {
    for (k=0, x=dim[0]; k<ncub[0]; k++, x += dx) {
      slice[j][k] = (*funct)(mol, x, y, z);
    }
   }
   fwrite(&len, sizeof(int), 1, fp);
   fwrite(slice[0], sizeof(float), ncub[0]*ncub[1], fp);
   fwrite(&len, sizeof(int), 1, fp);
   fflush(fp);
   printf(".");
   fflush(stdout);
  }
  printf("\n");

  #ifndef NOSOCK
  putc('\7', stdout);        /* the machine that goes "beep" */
  fflush(stdout);

  times(&endtime);
  cputime = (endtime.tms_utime - starttime.tms_utime)/(float)HZ;
  systime = (endtime.tms_stime - starttime.tms_stime)/(float)HZ;
  printf("  time : cpu %.2fs, sys %.2fs\n", cputime, systime);
  sprintf(timestring, " : cpu %.2fs, sys %.2fs\0", cputime, systime);
  #endif

  fclose(fp);
  free(slice);
  free(array);
  if(density){
   free(density[0]);
   free(density);
  }
  density = NULL;
  if(chi) free(chi);
}




double calc_point(Mol *mol, float x, float y, float z)
/* calculate the MO-value at given point */
/* no functions for speed */
{
   register int i;
   double value, *ao_coeff;

   ao_coeff = molOrb->coefficient;
   calc_chi(mol, x, y, z);

   value = 0;
   for(i=0; i<mol->nBasisFunctions; i++) {
      value += ao_coeff[i]*chi[i];
   }

   return value;
}






double calculateSomo(Mol *mol, float x, float y, float z)
/* calculate the spin density of the singly occ. orbitals */
{
  register short i;
  double value, point;

  value = 0;

  for(i=mol->nBeta; i<mol->nAlpha; i++){
    molOrb = mol->alphaOrbital + i - mol->firstOrbital + 1;
    point = calc_point(mol, x, y, z);
    value += point*point;;
  }

  return value;
}






double calculate_density(Mol *mol, float x, float y, float z)
/* calculate the electron or spin density at given point */
{
  register short i, j;
  double value;

  value = 0;
  calc_chi(mol, x, y, z);

  for(i=0; i<mol->nBasisFunctions; i++){
    value += density[i][i] * chi[i] * chi[i];
    for(j=0; j<i; j++)
      value += density[i][j] * chi[i] * chi[j] * 2.0;
  }

  return value;
}




void calc_chi(Mol *mol, float x, float y, float z)
/* calculate sum of AO-contributions chi for each MO at given point */
/* no functions for speed */
{
//  MolekelAtom *ap;
//  register Shell    *sp;
//  register Gauss    *gp;
  double radial_part, *cp;
  float xa, ya, za, ra2;  /* atomic units !! */

  memset(chi, 0, mol->nBasisFunctions * sizeof(double));
  cp = chi;
  for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
    xa = (x - ap->coord[0]) * _1_BOHR;
    ya = (y - ap->coord[1]) * _1_BOHR;
    za = (z - ap->coord[2]) * _1_BOHR;

    ra2 = xa*xa + ya*ya + za*za;    /* cutoff-distance ? */

    for (ShellList::iterator sp=ap->Shells.begin(); sp!=ap->Shells.end(); ++sp) {
      switch(sp->n_base){
        case 1  :        /*** S-orbital ***/
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = exp(-ra2*it->exponent);
            *cp += it->coeff * radial_part;
          }
          cp++;
          break;

        case 4 :        /*** SP-orbital ***/
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = exp(-ra2*it->exponent);
            *cp     += it->coeff * radial_part;
            *(cp+1) += it->coeff2 * xa * radial_part;
            *(cp+2) += it->coeff2 * ya * radial_part;
            *(cp+3) += it->coeff2 * za * radial_part;
          }
          cp += 4;
          break;

        case 3  :        /*** P-orbital ***/
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = it->coeff * exp(-ra2*it->exponent);
            *cp     += xa * radial_part;
            *(cp+1) += ya * radial_part;
            *(cp+2) += za * radial_part;
          }
          cp += 3;
          break;

        case 5  :        /*** D-orbital (5) ***/
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = it->coeff * exp(-ra2*it->exponent);
            *cp    += 0.288675135 *
                   (2*za*za - xa*xa - ya*ya) * radial_part;
            *(cp+3) += 0.5 * (xa*xa - ya*ya) * radial_part;
            *(cp+4) += xa * ya * radial_part;
            *(cp+1) += xa * za * radial_part;
            *(cp+2) += ya * za * radial_part;
          }
          cp += 5;
          break;

        case 6  :        /*** D-orbital (6) ***/
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = it->coeff * exp(-ra2*it->exponent);
            *cp     += radial_part * xa * xa * 0.57735027;
            *(cp+1) += radial_part * ya * ya * 0.57735027;
            *(cp+2) += radial_part * za * za * 0.57735027;
            *(cp+3) += radial_part * xa * ya;
            *(cp+4) += radial_part * xa * za;
            *(cp+5) += radial_part * ya * za;
          }
          cp += 6;
          break;

        case 7  :        /*** F-orbital (7) ***/
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = exp(-ra2*it->exponent) * it->coeff;
            *cp     += radial_part * za * (5. * za * za - 3. * ra2)/* * k */;
            *(cp+1) += radial_part * xa * (5. * za * za - ra2)/* * k */;
            *(cp+2) += radial_part * ya * (5. * za * za - ra2)/* * k */;
            *(cp+3) += radial_part * za * (xa * xa - ya * ya)/* * k */;
            *(cp+4) += radial_part * xa * ya * za;
            *(cp+5) += radial_part * (xa * xa * xa - 3. * xa * ya * ya)/* * k */;
            *(cp+6) += radial_part * (3. * xa * xa * ya - ya * ya * ya)/* * k */;
          }
          cp += 7;
          break;

        case 10 :        /*** F-orbital (10) ***/
               /* correct order ??? */
          for (GaussList::iterator it=sp->gaussians.begin(); it!=sp->gaussians.end(); ++it) {
            radial_part = it->coeff * exp(-ra2*it->exponent);
            *cp     += radial_part * xa * xa * xa * .25819889;
            *(cp+1) += radial_part * ya * ya * ya * .25819889;
            *(cp+2) += radial_part * za * za * za * .25819889;
            *(cp+3) += radial_part * xa * xa * ya * .57735027;
            *(cp+4) += radial_part * xa * xa * za * .57735027;
            *(cp+5) += radial_part * xa * ya * ya * .57735027;
            *(cp+6) += radial_part * ya * ya * za * .57735027;
            *(cp+7) += radial_part * xa * za * za * .57735027;
            *(cp+8) += radial_part * ya * za * za * .57735027;
            *(cp+9) += radial_part * xa * ya * za;
          }
          cp += 10;
          break;

      } /* end of switch */
    } /* end of loop over the shells (for(sp...) */
  } /* end of loop over the atoms (for(ap...)*/

  return;
}




int generate_density_matrix(Mol *mol, int key)
{
  register short i, j, k;
  float adder;

  if(!mol->alphaDensity && datasource == USE_MATRICES) return 0;

  if(density){
    free(density[0]);
    free(density);
    density = NULL;
  }

  if((density = (float **)alloc_trimat(mol->nBasisFunctions, sizeof(float))) == NULL){
    fprintf(stderr, "can't allocate density-matrix\n");
    return 0;
  }

/* use the alpha (and beta) density matrices from the gaussian output file */
  if(datasource == USE_MATRICES){
    if(key == EL_DENS){
      if(mol->betaDensity){
        for(i=0; i<mol->nBasisFunctions; i++){
          for(j=0; j<=i; j++)
            density[i][j] = mol->alphaDensity[i][j] + mol->betaDensity[i][j];
        }
      }
      else {
        for(i=0; i<mol->nBasisFunctions; i++){
          for(j=0; j<=i; j++) density[i][j] = mol->alphaDensity[i][j];
        }
      }
    }
    if(key == SPIN_DENS){
      if(!mol->betaDensity) return 0;
      for(i=0; i<mol->nBasisFunctions; i++){
        for(j=0; j<=i; j++)
          density[i][j]  = mol->alphaDensity[i][j] - mol->betaDensity[i][j];
      }
    }
  }

/* generate the density matrices with the coefficients */
  else if(datasource == USE_COEFFS) {
    if(key == EL_DENS){
      for(i=0; i<mol->nBasisFunctions; i++){
        for(j=0; j<=i; j++){
          density[i][j] = 0;
          for(k=0; k<mol->nBeta; k++){
            adder = mol->alphaOrbital[k].coefficient[i] *
                  mol->alphaOrbital[k].coefficient[j];
            if(mol->alphaBeta)
              adder += mol->betaOrbital[k].coefficient[i] *
                    mol->betaOrbital[k].coefficient[j];
            else adder *= 2.0;
            density[i][j] += adder;
          }
          for(; k<mol->nAlpha; k++)
            density[i][j] += mol->alphaOrbital[k].coefficient[i] *
                        mol->alphaOrbital[k].coefficient[j];
        }
      }
    }
    if(key == SPIN_DENS){
      if(!mol->alphaBeta) {
        for(i=0; i<mol->nBasisFunctions; i++){
          for(j=0; j<=i; j++){
            density[i][j] = 0;
            for(k=mol->nBeta; k<mol->nAlpha; k++)
              density[i][j] += mol->alphaOrbital[k].coefficient[i] *
                          mol->alphaOrbital[k].coefficient[j];
          }
        }
      }
      else {
        for(i=0; i<mol->nBasisFunctions; i++){
          for(j=0; j<=i; j++){
            density[i][j] = 0;
            for(k=0; k<mol->nAlpha; k++)
              density[i][j] += mol->alphaOrbital[k].coefficient[i] *
                          mol->alphaOrbital[k].coefficient[j];
            for(k=0; k<mol->nBeta; k++)
              density[i][j] -= mol->betaOrbital[k].coefficient[i] *
                          mol->betaOrbital[k].coefficient[j];
          }
        }
      }
    }
  }
  else return 0;

  return 1;
}

void mep_dot_surface(Mol *mol, Surface *surf)
{

  if(!surf->val){
    if((surf->val = (float *)malloc(surf->npts * sizeof(float))) == NULL){
      showinfobox("can't allocate memory for the quality");
      return;
    }
  }
  {
    Surfdot *sp;
    float *vp;
    register int i;

    sp = surf->dot;
    vp = surf->val;
    for(i=0; i<surf->npts; i++, sp++, vp++){
       *vp = calc_mep(mol, sp->v[0], sp->v[1], sp->v[2]);
    }

    surf->vmin = surf->vmax = *surf->val;
    for(i=1, vp=surf->val+1; i<surf->npts; i++, vp++){
      if(*vp < surf->vmin) surf->vmin = *vp;
      if(*vp > surf->vmax) surf->vmax = *vp;
    }
    //UV XXX REMOVED
    //if(!Globals::Textures.size()) init_texture();
    //surf->texture = Globals::Textures.firsttexture;
    //surf->texenv = GL_MODULATE;
    //surf->textype = TEX_MAP;
    //GUI::set_vmin_vmax(surf);
  }
}

void mep_dot_surface_no_pick(Mol *mol)
{
  Surface *surf;
  int count;
  char str[100];

  int surface_live_var=0;
  surf = mol->firstsurf;
  for(count=1; count < surface_live_var; count++) surf = surf->next;

  if(!surf->val){
    if((surf->val = (float *)malloc(surf->npts * sizeof(float))) == NULL){
      showinfobox("can't allocate memory for the quality");
      return;
    }
  }
  {
    Surfdot *sp;
    float *vp;
    register int i;

    sp = surf->dot;
    vp = surf->val;
    for(i=0; i<surf->npts; i++, sp++, vp++){
       *vp = calc_mep(mol, sp->v[0], sp->v[1], sp->v[2]);
    }

    surf->vmin = surf->vmax = *surf->val;
    for(i=1, vp=surf->val+1; i<surf->npts; i++, vp++){
      if(*vp < surf->vmin) surf->vmin = *vp;
      if(*vp > surf->vmax) surf->vmax = *vp;
    }
// UV REMOVED
//    if(!Globals::Textures.size()) init_texture();
//
//    surf->texture = Globals::Textures.firsttexture;
//    surf->texenv = GL_MODULATE;
//    surf->textype = TEX_MAP;
  }

//  glutPostRedisplay();
  sprintf(str, "vmin = %f  vmax = %f", surf->vmin, surf->vmax);
  logprint("");
  logprint("MEP:");
  logprint(str);
  update_logs();
}

double calc_mep(Mol *mol, float x, float y, float z)
/* calculate the MEP at given point based on the point charges
 * of the atoms
 */
{
//  MolekelAtom *ap;
  double nuclear;
  double xa, ya, za, ra2;

  nuclear = 0;
  for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
    xa = (x - ap->coord[0]) * _1_BOHR; /* atomic units */
    ya = (y - ap->coord[1]) * _1_BOHR;
    za = (z - ap->coord[2]) * _1_BOHR;
    ra2 = xa*xa + ya*ya + za*za;
    if(!ra2) { // UV ??? double used as int!!!
      if(ap->charge > 0) return 10000;
      else if(ap->charge < 0) return -10000;
    }
    else nuclear += ap->charge/sqrt(ra2);
  }

  return nuclear;
}

double calc_mep(const Mol *mol, const double xyz[ 3 ])
/* calculate the MEP at given point based on the point charges
 * of the atoms
 */
{
//  MolekelAtom *ap;
  double nuclear;
  double xa, ya, za, ra2;

  nuclear = 0;
  for (MolekelAtomList::const_iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
    xa = (xyz[ 0 ] - ap->coord[0]) * _1_BOHR; /* atomic units */
    ya = (xyz[ 1 ] - ap->coord[1]) * _1_BOHR;
    za = (xyz[ 2 ] - ap->coord[2]) * _1_BOHR;
    ra2 = xa*xa + ya*ya + za*za;
    if(!ra2) { // UV ??? double used as int!!!
      if(ap->charge > 0) return 10000;
      else if(ap->charge < 0) return -10000;
    }
    else    nuclear += ap->charge/sqrt(ra2);
  }

  return nuclear;
}


void spin_on_atoms(Mol *mol)
{
  register short i;
  char str[40];

  if(!generate_density_matrix(mol, SPIN_DENS)){
    showinfobox("Can't generate the density matrix!");
    return;
  }

  chi = (double *)calloc(mol->nBasisFunctions, sizeof(double));
  if(!chi) { showinfobox("Can't allocate chi"); }

  printf("Spin-densities on the atoms :\n");
  i=0;
  for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap, i++) {
      ap->spin = calculate_density(mol, ap->coord[0], ap->coord[1], ap->coord[2]);
      sprintf(str, "   %2s%-3d  %12.6f", element[ap->ord].symbol, i+1,
            ap->spin);
      printf("%s\n", str);
  }
  mol->atm_spin = 1;
  // UV XXX REMOVED
  //update_interface_flags();

  free(chi);
}


double calc_prddo_point(Mol *mol, float x, float y, float z)
/* calculate the MO-value at given point */
{
//  Slater *vp;
  double value, *ao_coeff, angular_part;
  float xa, ya, za, ra2, ra;  /* atomic units !! */

  value = 0;
  ao_coeff = molOrb->coefficient;

  for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
    xa = (x - ap->coord[0]) * _1_BOHR;
    ya = (y - ap->coord[1]) * _1_BOHR;
    za = (z - ap->coord[2]) * _1_BOHR;

    ra2 = xa*xa + ya*ya + za*za;
    ra = sqrt(ra2);

    for (SlaterList::iterator vp=ap->Slaters.begin(); vp!=ap->Slaters.end(); ++vp) {
      switch(vp->type[0]) {
        case 'S' :
          if(*ao_coeff) value += *ao_coeff * POW(ra, vp->n - 1) *
                    exp(-vp->exponent * ra) * vp->norm[0];
          ao_coeff++;
          break;
        case 'P' :
          angular_part = ao_coeff[0] * za + ao_coeff[1] * xa +
                    ao_coeff[2] * ya;
          value += angular_part * POW(ra, vp->n - 2) *
                exp(-vp->exponent * ra) * vp->norm[1];
          ao_coeff += 3;
          break;
        case 'D' :
          angular_part =  ao_coeff[0] * (3.*za*za - ra2) * vp->norm[3] +
                     ao_coeff[2] * (xa*xa - ya*ya) * vp->norm[2] +
                    (ao_coeff[1] * xa*za +
                     ao_coeff[3] * ya*za +
                     ao_coeff[4] * xa*ya) * vp->norm[4];

          value += angular_part * POW(ra, vp->n - 3) *
                exp(-vp->exponent * ra);
          ao_coeff += 5;
          break;
      }
    }
  } /* end of loop over the atoms (for(ap...)*/

  return value;
}




double calc_prddo_density(Mol *mol, float x, float y, float z)
{
  int i/*, norbs*/;
  double value, contr;

  value = 0;
  for(i=0; i<mol->nMolecularOrbitals; i++){
    if(mol->alphaOrbital[i].occ > 0){
      molOrb = mol->alphaOrbital + i;
      contr = calc_prddo_point(mol, x, y, z);
      value += mol->alphaOrbital[i].occ*contr*contr;
    }
    if(mol->betaOrbital && mol->betaOrbital[i].occ > 0){
      molOrb = mol->betaOrbital + i;
      contr = calc_prddo_point(mol, x, y, z);
      value += mol->betaOrbital[i].occ*contr*contr;
    }
  }

  return value;
}




double calc_prddo_spindensity(Mol *mol, float x, float y, float z)
{
  int i/*, norbs*/;
  double value, contr;

  value = 0;
  if(mol->betaOrbital) {
    for(i=0; i<mol->nMolecularOrbitals; i++){
      if(mol->alphaOrbital[i].occ > 0){
        molOrb = mol->alphaOrbital + i;
        contr = calc_prddo_point(mol, x, y, z);
        value += mol->alphaOrbital[i].occ*contr*contr;
      }
      if(mol->betaOrbital[i].occ > 0){
        molOrb = mol->betaOrbital + i;
        contr = calc_prddo_point(mol, x, y, z);
        value -= mol->betaOrbital[i].occ*contr*contr;
      }
    }
  }
  else if (mol->nAlpha != mol->nBeta) {
    for(i=0; i<mol->nMolecularOrbitals; i++){
      if(mol->alphaOrbital[i].occ == 1){
        molOrb = mol->alphaOrbital + i;
        contr = calc_prddo_point(mol, x, y, z);
        value += contr*contr;
      }
    }
  }

  return value;
}


double calc_sltr_point(Mol *mol, float x, float y, float z)
{
  float xa, ya, za, ra2, ra;
  double value = 0, exponent = 0, *ao_coeff;

  ao_coeff = molOrb->coefficient;

  for (MolekelAtomList::iterator ap=mol->Atoms.begin(); ap!=mol->Atoms.end(); ++ap) {
    xa = (x - ap->coord[0]) * _1_BOHR;
    ya = (y - ap->coord[1]) * _1_BOHR;
    za = (z - ap->coord[2]) * _1_BOHR;
    ra2 = xa*xa + ya*ya + za*za;
    ra = sqrt(ra2);

    for (SlaterList::iterator vp=ap->Slaters.begin(); vp!=ap->Slaters.end(); ++vp) {
      if(*ao_coeff) {
        exponent = exp(-(vp->exponent*ra));
        value += *ao_coeff * vp->norm[0] * POW(xa,vp->a) *
          POW(ya,vp->b) * POW(za,vp->c) * POW(ra,vp->d) * exponent;
      }
      ao_coeff++;
    }

  }
  return value;
}


double calc_sltr_density(Mol *mol, float x, float y, float z)
{
  int i/*, norbs*/;
  double value, contr;

  value = 0;
  for(i=0; i<mol->nMolecularOrbitals; i++){
    if(mol->alphaOrbital[i].occ > 0){
      molOrb = mol->alphaOrbital + i;
      contr = calc_sltr_point(mol, x, y, z);
      value += mol->alphaOrbital[i].occ*contr*contr;
    }
    if(mol->betaOrbital && mol->betaOrbital[i].occ > 0){
      molOrb = mol->betaOrbital + i;
      contr = calc_sltr_point(mol, x, y, z);
      value += mol->betaOrbital[i].occ*contr*contr;
    }
  }

  return value;
}

double calc_sltr_spindensity(Mol *mol, float x, float y, float z)
{
  int i/*, norbs*/;
  double value, contr;

  value = 0;
  if(mol->betaOrbital) {
    for(i=0; i<mol->nMolecularOrbitals; i++){
      if(mol->alphaOrbital[i].occ > 0){
        molOrb = mol->alphaOrbital + i;
        contr = calc_sltr_point(mol, x, y, z);
        value += mol->alphaOrbital[i].occ*contr*contr;
      }
      if(mol->betaOrbital[i].occ > 0){
        molOrb = mol->betaOrbital + i;
        contr = calc_sltr_point(mol, x, y, z);
        value -= mol->betaOrbital[i].occ*contr*contr;
      }
    }
  }
  else if (mol->nAlpha != mol->nBeta) {
    for(i=0; i<mol->nMolecularOrbitals; i++){
      if(mol->alphaOrbital[i].occ == 1){
        molOrb = mol->alphaOrbital + i;
        contr = calc_sltr_point(mol, x, y, z);
        value += contr*contr;
      }
    }
  }

  return value;
}
