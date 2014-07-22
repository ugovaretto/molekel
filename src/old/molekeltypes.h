#ifndef _MOLEKELSTRUCTS_H_
#define _MOLEKELSTRUCTS_H_
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

//Previous copyright notice from molekel 4.6
///*  MOLEKEL, Version 4.4, Date: 10.Dec.03
// *  Copyright (C) 2002-2003 Claudio Redaelli (CSCS)
// *  (original IRIX GL implementation, concept and data structure
// *   by Peter F. Fluekiger, CSCS/UNI Geneva, OpenGL/Mesa extensions
// *   and revisions by Stefan Portmann, CSCS/ETHZ)
// */


// UV first cleanup: removed all data types not used by readgauss.cpp
// and calcdens.cpp which are the only two original source files left.
// Will probably disappear as soon as proper support for orbitals and dynamics
// is added into OpenBabel.

#include <vector>
#include <list>
#include <string>
#include <fstream>


//----------------------------------------------------------------------------
// Some of the structs present have been converted to classes
// with memory handled by the class. But most is still in general.cpp
// Use of std::vector or std::list for all lists is intended in the future
//----------------------------------------------------------------------------

struct Molecule;

typedef Molecule Mol;

/// Holds information about gaussian basis functions used
/// for MO computation.
class Gauss {
  public:
    // One day these will be private
    double exponent, coeff, coeff2;
};

/// List of Gauss objects.
typedef std::vector<Gauss> GaussList;

/// Orbital shell; holds information used for MO computation/
class Shell {
  public:
    Shell() : n_base(0), scale_factor(0), gaussians() {};
    Shell(const Shell &s) : n_base(s.n_base), scale_factor(s.scale_factor), gaussians(s.gaussians) {};
  public:
    // One day these will be private
    short      n_base;
    float      scale_factor;
    GaussList  gaussians;
};

/// Shell list.
typedef std::vector<Shell> ShellList;

/// Holds information about Slater basis functions used for MO computation
class Slater {
  public:
    // One day these will be private
    short  n;
    char   type[2];
    unsigned a : 4;
    unsigned b : 4;
    unsigned c : 4;
    unsigned d : 4;
    float  exponent, norm[5];
};

/// List of Slater objects.
typedef std::vector<Slater> SlaterList;

/// Valence Shell
class ValenceShell {
  public:
   ValenceShell();
  ~ValenceShell();
  public:
    // One day these will be private
    bool  defined;
    short ns, np, nd;
    float exps, expp, expd;
    float couls, coulp, could;
    double *norm;
};

/// Amoss basis @warning UV do not know how this is currently used
class Amoss_basis {
  public:
    // One day these will be private
    char center[3], basis[21];
    ShellList Shells;
};

/// List of Amoss basis
typedef std::vector<Amoss_basis> AmossBasisList;

/// Used for MO computation with Slater basis functions.
class Basis {
  public:
    // One day these will be private
    char basisname[21];
    short ord;
    ShellList Shells;
    SlaterList Slaters;
};

/// Basis list.
typedef std::vector<Basis> BasisList;


/// Atom definition
class MolekelAtom {
  public:
               MolekelAtom(int o, float x, float y, float z); //<--
    virtual   ~MolekelAtom();
    Shell     *add_shell();
    Shell     *add_shell(Shell *sp);
    Slater    *add_slater();

  public:
    // One day these will be private
    short ord, nbonds, name;
    unsigned picked  : 1;
    unsigned het     : 1;
    unsigned planar  : 1;
    unsigned main    : 1;
    unsigned fixed	 : 1;
    float coord[3], charge, coordination, spin;
    float force[3];
    ShellList  Shells;
    ValenceShell valence;
    SlaterList Slaters;
};

/// Atom list
typedef std::vector<MolekelAtom> MolekelAtomList;

// For each molecule, store iterators which point to atoms in lists
// which are organised by Atom Type.
// This way all the Hydrogen, Oxygen, Carbon, etc can be traversed
// by type, rather than their order in the Molecule
class AtomType {
  public:
    short ord;
    std::vector<int> atomList;
};

/// Atom types
typedef std::vector<AtomType> AtomTypeList;

/// Bond
class Bond {
  public:
    MolekelAtom *a, *b;
    /* float middle[3]; */
    short type, name, picked;
#ifdef BOND_MATRIX
    Matrix bondMatrix;
#endif
};

/// Bond list
typedef std::vector<Bond> BondList;


typedef struct TER   { MolekelAtom *first, *last;
                       short colindex;
                       struct TER *next;
                     } Ter;

typedef struct RES   { MolekelAtom *first, *last;
                       char type[4];
                       struct RES *next;
                     } Residue;


typedef struct { float v[3]; float n[3];}   Surfdot;

typedef struct { unsigned int p1, p2, p3; } Triangle;

/// 3d vector
class Vector {
  public:
    float x, y, z;
    Vector() : x(0), y(0), z(0) { }
    Vector(float X, float Y, float Z) { x=X; y=Y; z=Z; }
} ;

/// Surface - not used; all computation/visualization done with VTK or IV
class Surface {
  public:
    Surface(Mol *mol, char type, Surfdot  *dot, Triangle *tri, float contour, float *val,
              int npts, int ntri, char color, const char *name); //<--
   virtual ~Surface(); //<--
  public:
    Surfdot  *dot;
    Triangle *tri;
    Vector   *trinorm;
    float contour, *val, vmin, vmax;
    float *val1, vmin1, vmax1; /* second property */
    int   npts, ntri, matindex;
    std::string name;
    Surface *next, *second;
};

/// Cutplane - to be removed - use VTK widgets instead
typedef struct CUTPLANE { float alpha, beta, a[3];
                       int npts, ntri;
                       float dd;
                       float (**plane_point)[7];
                       unsigned int (*tri)[3][2];
                     } Cutplane;

/// Holds information about atom trajectories
struct Dynamics
{
    Vector **trajectory;
    MolekelAtom **freeat;
    Molecule *molecule;
    long ntotalsteps, nfreat;
    long start, end, current;
    int isrunning, runtype, direction;
    float stepsize, timestep;

     Dynamics() : trajectory( 0 ), freeat( 0 ),
                  ntotalsteps( 0 ), nfreat( 0 ),
                  start( -1 ), end( -1 ), current( -1 ),
                  isrunning( 0 ), runtype( 0 ), direction( 0 ),
                  stepsize( 0 ), timestep( 0 ) {}

 };

/// Bounding box
typedef struct      { float x1, x2, y1, y2, z1, z2, cubesize;
                      short nx, ny, nz;
                      short flag;
                    } Box;

/// ???
typedef struct MOND { MolekelAtom *a, *b;
                      struct MOND *next;
                    } Mon_dist;

/// ???
typedef struct MONA { MolekelAtom *a, *b, *c;
                      struct MONA *next;

                    } Mon_ang;

/// ???
typedef struct MONT { MolekelAtom *a, *b, *c, *d;
                      struct MONT *next;
                    } Mon_tor;

/// Molecular Orbital (MO)
 struct  MolecularOrbital {
                 char type[8];
                 int flag;
                 int number;
                 float  occ;
                 double eigenvalue;
                 double *coefficient;
                 MolecularOrbital() : flag( 0 ),
                                      number( -1 ),
                                      occ( 0.f ),
                                      eigenvalue( 0. ),
                                      coefficient( 0 ) {}
               };

/// Vibration information
class Vibration {
  public:
    Vibration();
    char                type[5];
    float               frequency; //cm^-1
    float				ir_intensity; // KM/Mole
    float				raman_activity; // A^4/AMU
    float				reduced_mass; //AMU
    std::vector<Vector> coord;
};

/// List of vibration parameters
typedef std::vector<Vibration> VibrationList;

/// Dipole
struct Dipole
{
     float start[3];
     float end[3];
     float absolute;
};

/// Main class to store Molecular data.
struct Molecule  {
    Molecule();
    ~Molecule();
    MolekelAtom* AddNewAtom(int ord, float x, float y, float z);
    void normalize_gaussians();
    std::string fname; /// @todo UV remove this and use filename instead
    Amoss_basis *add_amoss();
    ShellList   *get_Amossbasis(char *basis);
    MolekelAtomList                   Atoms;
    AtomTypeList                      AtomTypes;
    BondList                          Bonds;
    AmossBasisList                    Amoss;
    BasisList                         Basisset;
    VibrationList                     vibration;
    VibrationList::iterator           freq_arrow;

    Dynamics dynamics;

    Surface      *firstsurf;
    Ter          *firstter;
    Residue      *firstresidue;
    Mon_dist    *firstdist;
    Mon_ang     *firstang;
    Mon_tor     *firsttor;
    Box         box;
    int         plane_iz, plane_iy, plane_ix;
    int         nMolecularOrbitals;
    int         nBasisFunctions;
    MolecularOrbital *alphaOrbital;
    MolecularOrbital *betaOrbital;
    int       firstOrbital;
    int       lastOrbital;
    int       n_frequencies;
    int       multiplicity;
    float   **alphaDensity;
    float   **betaDensity;
    float     charge;
    float     mass;
    int       nAlpha;
    int       nBeta;
    int       nElectrons;
    int       alphaBeta;
    Dipole   *dipole;
    float     sc_freq_ar;
    float     sc_dipole_ar;
    Cutplane *plane;

    double point[3];

    std::string filename;
    void *dvs;
    void *anim_data;
    short natoms, nbonds, n_h_bonds, nsurfs, nters, nresidues;
    float tvec[3], rvec[4], centervec[3];
    float ***cube_value, cubemin, cubemax, cutoff;
      int atm_spin       ;
    int got_macufile   ;
    int cubeplanes     ;
    int charges        ;
    int show_freq_arrow;
};

// GLOBAL ATTRIBUTES

typedef struct INTD { MolekelAtom *a, *b;
                      Mol *ma, *mb;
                      struct INTD *next;
                    } Int_dist;

typedef struct { int bury;
                 int  lon;
                 int  bin;
               } Conflag;

typedef struct { float mass, rvdw, coval;
                 short col;
                 int textype;
                 short nbond;
                 char  symbol[3];
               } Element;

typedef float Matrix[4][4];

typedef struct { MolekelAtom **a;
                 short na;
               } Atomcube;


#endif
