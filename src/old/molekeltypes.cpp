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
///*  MOLEKEL, Version 4.4, Date: 10.Dec.03
// *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
// *  (original IRIX GL implementation, concept and data structure
// *   by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
// *   and revisions by Stefan Portmann, CSCS/ETHZ)
// */


// UV minimal set of functions required to support Gaussian I/O and
// MO computation.

#include <cstring>
#include "molekeltypes.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace std;


//----------------------------------------------------------------------------
// normalize contracted gaussians
void norm1(Shell *sp, double fac, double ex)
{
//   register Gauss *i, *j;
   double sum, norm;

   sum = 0;
   for (GaussList::iterator i=sp->gaussians.begin(); i!=sp->gaussians.end(); ++i) {
    for (GaussList::iterator j=sp->gaussians.begin(); j!=sp->gaussians.end(); ++j) {
         sum += i->coeff * j->coeff * pow(i->exponent + j->exponent, ex);
      }
   }
   sum *= pow(M_PI, 1.5) * fac;
   norm = 1.0/sqrt(sum);

   for (GaussList::iterator i=sp->gaussians.begin(); i!=sp->gaussians.end(); ++i) i->coeff *= norm;
}
void norm2(Shell *sp, double fac, double ex)
{
//   register Gauss *i, *j;
   double sum, norm;

   sum = 0;
   for (GaussList::iterator i=sp->gaussians.begin(); i!=sp->gaussians.end(); ++i) {
    for (GaussList::iterator j=sp->gaussians.begin(); j!=sp->gaussians.end(); ++j) {
         sum += i->coeff2 * j->coeff2 * pow(i->exponent + j->exponent, ex);
      }
   }
   sum *= pow(M_PI, 1.5) * fac;
   norm = 1.0/sqrt(sum);

   for (GaussList::iterator i=sp->gaussians.begin(); i!=sp->gaussians.end(); ++i) i->coeff2 *= norm;
}
void Molecule::normalize_gaussians(void)
{
  for (MolekelAtomList::iterator ap=this->Atoms.begin(); ap!=this->Atoms.end(); ++ap) {
    for (ShellList::iterator sp=ap->Shells.begin(); sp!=ap->Shells.end(); ++sp) {
        switch(sp->n_base){
          case  1 : norm1(&(*sp), 1.0, -1.5); break;
          case  3 : norm1(&(*sp), 0.5, -2.5); break;
          case  4 : norm1(&(*sp), 1.0, -1.5);
                    norm2(&(*sp), 0.5, -2.5); break;
          case  5 :
          case  6 : norm1(&(*sp), 0.25, -3.5); break;
          case  7 :
          case 10 : norm1(&(*sp), 0.125, -4.5); break;
        }
    }
  }
}

//----------------------------------------------------------------------------
MolekelAtom *Molecule::AddNewAtom(int ord, float x, float y, float z)
{
  MolekelAtom atom(ord, x, y, z);
  atom.name = ++(this->natoms);
  this->Atoms.push_back(atom);
  return &this->Atoms.back();
}

//----------------------------------------------------------------------------
Shell *MolekelAtom::add_shell()
{
  this->Shells.push_back(Shell());
  Shell *shell = &Shells.back();
  shell->gaussians.reserve(5);
  return shell;
}

//----------------------------------------------------------------------------
Slater* MolekelAtom::add_slater()
{
    Slater temp;
    Slaters.push_back(temp);
    // UV: original code: adds object at the end and initializes the one
    // at the front!!  Slater *result = Slaters.front()
    Slater *result = &Slaters.back();
    // UV why cannot the following initialization code be executed
    // in the Slater constructor ?
    result->n = 0;
    result->type[0] = ' ';
    result->type[1] = 0;
    result->exponent = 0;
    result->norm[0] = result->norm[1] = result->norm[2] = 1;
    return result;
}

//----------------------------------------------------------------------------
MolekelAtom::MolekelAtom(int o, float x, float y, float z) {
  this->ord     = o;
  this->picked  = 0;
  this->het     = 0;
  this->planar  = 0;
  this->main    = 0;
  this->fixed   = 1;
  this->nbonds  = 0;
#ifdef ATOM_NEIGHBORS
  this->nneighbors = 0;
#endif
  this->coord[0]      = x;
  this->coord[1]      = y;
  this->coord[2]      = z;
  this->coordination  = 0.0;
  this->charge        = 0.0;
  this->spin          = 0.0;
  this->force[0] = this->force[1] = this->force[2] = 0.0;
}

//----------------------------------------------------------------------------
ValenceShell::~ValenceShell()
{
  delete []norm;
}

//----------------------------------------------------------------------------
ValenceShell::ValenceShell()
{
  this->defined = 0;
  this->norm = NULL;
}

//----------------------------------------------------------------------------
MolekelAtom::~MolekelAtom()
{
}

//----------------------------------------------------------------------------
Vibration::Vibration() : frequency(0.0f), ir_intensity(0.0f),
						 raman_activity(0.0f), reduced_mass(0.0f) {
  memcpy(this->type, "\0\0\0\0\0", 5);
}
//----------------------------------------------------------------------------
Molecule::Molecule()
{

  //
  this->firstsurf          = NULL;
  this->firstter           = NULL;
  this->firstresidue       = NULL;
  this->firstdist          = NULL;
  this->firstang           = NULL;
  this->firsttor           = NULL;
  this->alphaOrbital       = this->betaOrbital = NULL;
  this->alphaDensity       = this->betaDensity = NULL;
//  this->vibration          = std::vector<Vibration>(0);
//  this->freq_arrow         = this->vibration.begin() - this->vibration.begin(); // we want NULL
//  this->freq_arrow = (this->vibration.end()); // UV why ?

//  this->sc_freq_ar         = 5 * sc_freq_ar; // ???? UV in this context sc_freq_ar IS an uninitialized
//                                             // data member not a global variable
  const float def_sc_freq_ar = 0.5f; // UV copied from Molekel 4.3 - general.C
  this->sc_freq_ar = 5 * def_sc_freq_ar;
  this->nMolecularOrbitals = this->nBasisFunctions = this->multiplicity = 0;
  this->n_frequencies      = 0;
  this->plane_iz = this->plane_iy = this->plane_ix = 0;
  this->firstOrbital       = this->lastOrbital = 1;
  this->nAlpha   = this->nBeta = this->nElectrons = this->alphaBeta = 0;
  this->filename = "";
  this->tvec[0]  = this->tvec[1] = this->tvec[2] = 0.0;
  this->rvec[0]  = this->rvec[1] = this->rvec[2] = 0.0;
  this->rvec[3]  = 1.0;
  this->centervec[0] = this->centervec[1] = this->centervec[2] = 0.0;
  this->dipole       = NULL;
  this->sc_dipole_ar = 2 * sc_dipole_ar;
  this->plane        = NULL;
  this->mass         = 0;
  this->charge       = 0;
  this->dvs          = 0;
  this->natoms       = 0;
  this->nbonds       = 0;
  this->n_h_bonds    = 0;
  this->nsurfs       = 0;
  this->nters = this->nresidues = 0;

  this->got_macufile   = 0;
  this->cubeplanes     = 0;

  this->show_freq_arrow= 0;

  //
  this->cube_value     = NULL;
  this->cubemin = this->cubemax = this->cutoff = 0;
}


//----------------------------------------------------------------------------
Molecule::~Molecule()
{
extern void FreeDynamics( Dynamics& );
    FreeDynamics( dynamics );
extern void FreeDipole( Dipole* d );
    FreeDipole( dipole );
}

//----------------------------------------------------------------------------
Amoss_basis *Molecule::add_amoss()
{
   Amoss_basis amoss;
   Amoss.push_back(amoss);
   return &Amoss.back();
}
//----------------------------------------------------------------------------
ShellList *Molecule::get_Amossbasis(char *basis)
{
  for (AmossBasisList::iterator bp=this->Amoss.begin(); bp!=this->Amoss.end(); ++bp) {
    if (!strcmp(basis, bp->basis)) return &bp->Shells;
  }
  return NULL;
}

