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
//#define MOLEKEL_USE_COMMON_FILE_FORMATS // <- #define this for faster builds on MinGW

// VTK
#include <vtkRenderer.h>
#include <vtkProperty.h>

// STD
#include <exception>
#include <algorithm>

// Molekel
#include "MolekelData.h"
#include "MolekelMolecule.h"
#include "MolekelException.h"


MolekelData::IndexType MolekelData::lastId_ = 0;

//------------------------------------------------------------------------------
void MolekelData::CheckIndex( IndexType id ) const
{
    if( molecules_.find( id ) == molecules_.end() )
    {
         throw MolekelException( "Invalid molecule index" );
    }
}

//------------------------------------------------------------------------------
MolekelData::IndexType MolekelData::AddMolecule( const char* fname,
                                                 vtkRenderer* ren,
                                                 ILoadMoleculeCallback* cb,
                                                 bool computeBonds )
{
    MolekelMolecule* mol = MolekelMolecule::New( fname, cb, computeBonds );
    std::pair< IndexType, MolekelMolecule* > np( GetNewID(), mol );
    molecules_.insert( np );
    double r, g, b;
    ren->GetBackground( r, g, b );
    r = 1.0 - r;
    g = 1.0 - g;
    b = 1.0 - b;
    mol->GetBBoxActor()->GetProperty()->SetColor( r, g, b );
    ren->AddActor( mol->GetAssembly() );
    return np.first;
}

//------------------------------------------------------------------------------
MolekelData::IndexType MolekelData::AddMolecule( const char* fname,
                                                 const char* format,
                                                 vtkRenderer* ren,
                                                 ILoadMoleculeCallback* cb,
                                                 bool computeBonds )
{
    MolekelMolecule* mol = MolekelMolecule::New( fname, format, cb, computeBonds );
    std::pair< IndexType, MolekelMolecule* > np( GetNewID(), mol );
    molecules_.insert( np );
    double r, g, b;
    ren->GetBackground( r, g, b );
    r = 1.0 - r;
    g = 1.0 - g;
    b = 1.0 - b;
    mol->GetBBoxActor()->GetProperty()->SetColor( r, g, b );
    ren->AddActor( mol->GetAssembly() );
    return np.first;
}

//------------------------------------------------------------------------------
void MolekelData::RemoveMolecule( IndexType id )
{
    CheckIndex( id );
    molecules_.erase( id );
}


//------------------------------------------------------------------------------
void MolekelData::SaveMolecule( IndexType id,
                                const char* fileName ) const
{
    CheckIndex( id );
    const MolekelMolecule* mol = molecules_.find( id )->second;
    mol->Save( fileName );
}

//------------------------------------------------------------------------------
void MolekelData::SaveMolecule( IndexType id,
                                const char* fileName,
                                const char* type ) const
{
    CheckIndex( id );
    const MolekelMolecule* mol = molecules_.find( id )->second;
    mol->Save( fileName, type );
}

//------------------------------------------------------------------------------
MolekelMolecule* MolekelData::GetMolecule( MolekelData::IndexType id )
 {
       CheckIndex( id );
       return molecules_[ id ];
}

//------------------------------------------------------------------------------
namespace
{
    struct DeleteMolecule
    {
        void operator()( MolekelData::Molecules::value_type& v )
        {
            delete v.second;
            v.second = 0;
        }
    };
}
void MolekelData::Clear()
{
    /// @todo remove as soon as smart pointers are added
    for_each( molecules_.begin(), molecules_.end(), DeleteMolecule() );
    molecules_.clear();
    lastId_ = 0;
}

//------------------------------------------------------------------------------
MolekelData::~MolekelData()
{
    Clear();
}

//------------------------------------------------------------------------------
MolekelData::FileFormats MolekelData::formats_;
bool MolekelData::initialized_ = false;
/// @todo properly initialize using OpenBabel OBConversion.
void MolekelData::InitFileFormats()
{
    if( initialized_ ) return;
    formats_.clear();
    typedef MoleculeFileFormat MFF;
    typedef std::vector< std::string > SV;

#ifdef MOLEKEL_USE_COMMON_FILE_FORMATS
//==============================================================================
// COMMON FORMATS - use this during development on MinGW to save build time
//==============================================================================
    //    * car -- Accelrys/MSI Biosym/Insight II CAR format [Read-only]
    SV car; car.push_back( ".car" );
    formats_.push_back( MFF( "car", car, "Accelrys/MSI Biosym/Insight II CAR format", MFF::READ ) );
    //    * com -- Gaussian 98/03 Cartesian Input [Write-only]
    SV com; com.push_back( ".com" );
    formats_.push_back( MFF( "com", com, "Gaussian 98/03 Cartesian Input", MFF::WRITE ) );
    //    * csr -- Accelrys/MSI Quanta CSR format [Write-only]
    SV csr; csr.push_back( ".csr" );
    formats_.push_back( MFF( "csr", csr, "Accelrys/MSI Quanta CSR format", MFF::WRITE ) );
    /// @note currently using custom OpenBabel reader for Gaussian cubes
    // Gaussian cube.
    SV gcube; gcube.push_back( ".cube" ); gcube.push_back( ".grd" );
    formats_.push_back( MFF( "cube", gcube, "Gaussian Cube format", MFF::READ ) );
    //    * ent -- Protein Data Bank format
    SV ent; ent.push_back( ".ent" );
    formats_.push_back( MFF( "ent", ent, "Protein Data Bank format", MFF::READ_WRITE ) );
     //    * fh -- Fenske-Hall Z-Matrix format [Write-only]
    SV fh; fh.push_back( ".fh" );
    formats_.push_back( MFF( "fh", fh, "Fenske-Hall Z-Matrix format", MFF::WRITE ) );
     //    * g03 -- Gaussian98/03 Output [Read-only]
    SV g03; g03.push_back( ".g03" ); g03.push_back( ".out" ); g03.push_back( ".log" );
    formats_.push_back( MFF( "g03", g03, "Gaussian98/03 Output", MFF::READ ) );
    //    * g98 -- Gaussian98/03 Output [Read-only]
    SV g98; g98.push_back( ".g98" ); g98.push_back( ".out" ); g98.push_back( ".log" );
    formats_.push_back( MFF( "g98", g98, "Gaussian98/03 Output", MFF::READ ) );
    //    * gam -- GAMESS Output [Read-only]
    SV gam; gam.push_back( ".gam" ); gam.push_back( ".out" ); gam.push_back( ".log" );
    formats_.push_back( MFF( "gam", gam, "GAMESS Output", MFF::READ ) );
    //    * gamin -- GAMESS Input [Write-only]
    SV gamin; gamin.push_back( ".gamin" );
    formats_.push_back( MFF( "gamin", gamin, "GAMESS Input", MFF::WRITE ) );
    //    * gamout -- GAMESS Output [Read-only]
    SV gamout; gamout.push_back( ".gamout" ); gamout.push_back( ".out" ); gamout.push_back( ".log" );
    formats_.push_back( MFF( "gamout", gamout, "GAMESS Output", MFF::READ ) );
    //    * gau -- Gaussian 98/03 Cartesian Input [Write-only]
    SV gau; gau.push_back( ".gau" );
    formats_.push_back( MFF( "gau", gau, "Gaussian 98/03 Cartesian Input", MFF::WRITE ) );
    //    * mdl -- MDL MOL format
    SV mdl; mdl.push_back( ".mdl" );
    formats_.push_back( MFF( "mdl", mdl, "MDL MOL format", MFF::READ_WRITE ) );
    //    * mmd, mmod -- MacroModel format
    SV mmd; mmd.push_back( ".mmd" ); mmd.push_back( ".mmod" );
    formats_.push_back( MFF( "mmd", mmd, "MacroModel format", MFF::READ_WRITE ) );
    //    * mol -- MDL MOL format
    SV mol; mol.push_back( ".mol" );
    formats_.push_back( MFF( "mol", mol, "MDL MOL format", MFF::READ_WRITE ) );
    //    * mol2 -- Sybyl Mol2 format
    SV mol2; mol2.push_back( ".mol2" );
    formats_.push_back( MFF( "mol2", mol2, "Sybyl Mol2 format", MFF::READ_WRITE ) );
    //    * molden -- Molden input format
    SV molden; molden.push_back( ".molden" ); molden.push_back( ".input" );
    formats_.push_back( MFF( "molden", molden, "Molden input format", MFF::READ ) );
    //    * msms -- Michel Sanner's MSMS input format [Write-only]
    SV msms; msms.push_back( ".msms" ); msms.push_back( ".xyzr" );
    formats_.push_back( MFF( "msms", msms, "Michel Sanner's MSMS input format", MFF::WRITE ) );
    //    * pdb -- Protein Data Bank format
    SV pdb; pdb.push_back( ".pdb" );
    formats_.push_back( MFF( "pdb", pdb, "Protein Data Bank format", MFF::READ_WRITE ) );
    //    * pov -- POV-Ray input format [Write-only]
    SV pov; pov.push_back( ".pov" );
    formats_.push_back( MFF( "pov", pov, "POV format", MFF::WRITE ) );
    //    * t41 -- ADF ASCII format
    SV t41; t41.push_back( ".t41" );
    formats_.push_back( MFF( "t41", t41, "ADF ASCII format", MFF::READ ) );
    //    * xyz -- XYZ cartesian coordinates format
    SV xyz; xyz.push_back( ".xyz" );
    formats_.push_back( MFF( "xyz", xyz, "XYZ cartesian coordinates format", MFF::READ_WRITE ) );
    //    * zmatrix -- plain Z-matrix format
    SV zmat; zmat.push_back( ".zmat" ); zmat.push_back( ".zmatrix" );
    formats_.push_back( MFF( "zmatrix", zmat, "Z-matrix format", MFF::READ ) );
#else
//==============================================================================
// ALL FORMATS - use this when building official releases
//==============================================================================
    //    OpenBabel 2.0 Supported Molecular Formats:
    //    * alc -- Alchemy format
    SV alc; alc.push_back( ".alc" );
    formats_.push_back( MFF( "alc", alc, "Alchemy format", MFF::READ_WRITE ) );
    //    * bgf -- MSI BGF format
    SV bgf; bgf.push_back( ".bgf" );
    formats_.push_back( MFF( "bgf", bgf, "MSI BGF format", MFF::READ_WRITE ) );
    //    * box -- Dock 3.5 Box format
    SV box; box.push_back( ".box" );
    formats_.push_back( MFF( "box", box, "Dock 3.5 Box format", MFF::READ_WRITE ) );
    //    * bs -- Ball and Stick format
    SV bs; bs.push_back( ".bs" );
    formats_.push_back( MFF( "bs", bs, "Ball and Stick format", MFF::READ_WRITE ) );
    //    * c3d1 -- Chem3D Cartesian 1 format
    SV c3d1; c3d1.push_back( ".c3d1" );
    formats_.push_back( MFF( "c3d1", c3d1, "Chem3D Cartesian 1 format", MFF::READ_WRITE ) );
    //    * c3d2 -- Chem3D Cartesian 2 format
    SV c3d2; c3d2.push_back( ".c3d2" );
    formats_.push_back( MFF( "c3d2", c3d2, "Chem3D Cartesian 2 format", MFF::READ_WRITE ) );
    //    * caccrt -- Cacao Cartesian format
    SV caccrt; caccrt.push_back( ".caccrt" );
    formats_.push_back( MFF( "caccrt", caccrt, "Cacao Cartesian format", MFF::READ_WRITE ) );
    //    * cache -- CAChe MolStruct format [Write-only]
    SV cache; cache.push_back( ".cache" );
    formats_.push_back( MFF( "cache", cache, "CAChe MolStruct format", MFF::WRITE ) );
    //    * cacint -- Cacao Internal format [Write-only]
    SV cacint; cacint.push_back( ".cacint" );
    formats_.push_back( MFF( "cacint", cacint, "Cacao Internal format", MFF::WRITE ) );
    //    * car -- Accelrys/MSI Biosym/Insight II CAR format [Read-only]
    SV car; car.push_back( ".car" );
    formats_.push_back( MFF( "car", car, "Accelrys/MSI Biosym/Insight II CAR format", MFF::READ ) );
    //    * ccc -- CCC format [Read-only]
    SV ccc; ccc.push_back( ".ccc" );
    formats_.push_back( MFF( "ccc", ccc, "CCC format", MFF::READ ) );
    //    * cht -- Chemtool format [Write-only]
    SV cht; cht.push_back( ".cht" );
    formats_.push_back( MFF( "cht", cht, "Chemtool format", MFF::WRITE ) );

    //    * cml -- Chemical Markup Language
    //SV cml; cml.push_back( ".cml" );
    //formats_.push_back( MFF( "cml", cml, "Chemical Markup Language", MFF::READ_WRITE ) );

    //    * cmlr -- CML Reaction format
    //SV cmlr; cml.push_back( ".cmlr" );
    //formats_.push_back( MFF( "cmlr", cmlr, "CML Reaction format", MFF::READ_WRITE ) );

    //    * com -- Gaussian 98/03 Cartesian Input [Write-only]
    SV com; com.push_back( ".com" );
    formats_.push_back( MFF( "com", com, "Gaussian 98/03 Cartesian Input", MFF::WRITE ) );
    //    * copy -- Copies raw text [Write-only]
    SV copy; com.push_back( ".copy" ); com.push_back( ".txt" );
    formats_.push_back( MFF( "copy", copy, "Copies raw text", MFF::WRITE ) );
    //    * crk2d -- Chemical Resource Kit 2D diagram format
    SV crk2d; crk2d.push_back( ".crk2d" );
    formats_.push_back( MFF( "crk2d", crk2d, "Chemical Resource Kit 2D diagram format", MFF::READ_WRITE ) );
    //    * crk3d -- Chemical Resource Kit 3D format
    SV crk3d; crk3d.push_back( ".crk3d" );
    formats_.push_back( MFF( "crk3d", crk3d, "Chemical Resource Kit 3D format", MFF::READ_WRITE ) );
    //    * csr -- Accelrys/MSI Quanta CSR format [Write-only]
    SV csr; csr.push_back( ".csr" );
    formats_.push_back( MFF( "csr", csr, "Accelrys/MSI Quanta CSR format", MFF::WRITE ) );
    //    * cssr -- CSD CSSR format [Write-only]
    SV cssr; cssr.push_back( ".cssr" );
    formats_.push_back( MFF( "cssr", cssr, "CSD CSSR format", MFF::WRITE ) );
    //    * ct -- ChemDraw Connection Table format
    SV ct; ct.push_back( ".ct" );
    formats_.push_back( MFF( "ct", ct, "ChemDraw Connection Table format", MFF::READ_WRITE ) );

    /// @note currently using custom OpenBabel reader for Gaussian cubes
    // Gaussian cube.
    SV gcube; gcube.push_back( ".cube" ); gcube.push_back( ".grd" );
    formats_.push_back( MFF( "cube", gcube, "Gaussian Cube format", MFF::READ ) );

    //    * dmol -- DMol3 coordinates format
    SV dmol; dmol.push_back( ".dmol" );
    formats_.push_back( MFF( "dmol", dmol, "DMol3 coordinates format", MFF::READ_WRITE ) );
    //    * ent -- Protein Data Bank format
    SV ent; ent.push_back( ".ent" );
    formats_.push_back( MFF( "ent", ent, "Protein Data Bank format", MFF::READ_WRITE ) );
    //    * feat -- Feature format
    SV feat; feat.push_back( ".feat" );
    formats_.push_back( MFF( "feat", feat, "Feature format", MFF::READ_WRITE ) );
    //    * fh -- Fenske-Hall Z-Matrix format [Write-only]
    SV fh; fh.push_back( ".fh" );
    formats_.push_back( MFF( "fh", fh, "Fenske-Hall Z-Matrix format", MFF::WRITE ) );
    //    * fix -- SMILES FIX format [Write-only]
    SV fix; fix.push_back( ".fix" );
    formats_.push_back( MFF( "fix", fix, "SMILES FIX format", MFF::WRITE ) );
    //    * fpt -- Fingerprint format [Write-only]
    SV fpt; fpt.push_back( ".fpt" );
    formats_.push_back( MFF( "fpt", fpt, "Fingerprint format", MFF::WRITE ) );
    //    * fract -- Free Form Fractional format
    SV fract; fract.push_back( ".fract" );
    formats_.push_back( MFF( "fract", fract, "Free Form Fractional format", MFF::READ_WRITE ) );
    //    * fs -- FastSearching Index
    SV fs; fs.push_back( ".fs" );
    formats_.push_back( MFF( "fs", fs, "FastSearching Index", MFF::READ_WRITE ) );
    //    * g03 -- Gaussian98/03 Output [Read-only]
    SV g03; g03.push_back( ".g03" ); g03.push_back( ".out" ); g03.push_back( ".log" );
    formats_.push_back( MFF( "g03", g03, "Gaussian98/03 Output", MFF::READ ) );
    //    * g98 -- Gaussian98/03 Output [Read-only]
    SV g98; g98.push_back( ".g98" ); g98.push_back( ".out" ); g98.push_back( ".log" );
    formats_.push_back( MFF( "g98", g98, "Gaussian98/03 Output", MFF::READ ) );
    //    * gam -- GAMESS Output [Read-only]
    SV gam; gam.push_back( ".gam" ); gam.push_back( ".out" ); gam.push_back( ".log" );
    formats_.push_back( MFF( "gam", gam, "GAMESS Output", MFF::READ ) );
    //    * gamin -- GAMESS Input [Write-only]
    SV gamin; gamin.push_back( ".gamin" );
    formats_.push_back( MFF( "gamin", gamin, "GAMESS Input", MFF::WRITE ) );
    //    * gamout -- GAMESS Output [Read-only]
    SV gamout; gamout.push_back( ".gamout" ); gamout.push_back( ".out" ); gamout.push_back( ".log" );
    formats_.push_back( MFF( "gamout", gamout, "GAMESS Output", MFF::READ ) );
    //    * gau -- Gaussian 98/03 Cartesian Input [Write-only]
    SV gau; gau.push_back( ".gau" );
    formats_.push_back( MFF( "gau", gau, "Gaussian 98/03 Cartesian Input", MFF::WRITE ) );
    //    * gpr -- Ghemical format
    SV gpr; gpr.push_back( ".gpr" );
    formats_.push_back( MFF( "gpr", gau, "Ghemical format", MFF::READ_WRITE ) );
    //    * gr96 -- GROMOS96 format [Write-only]
    SV gr96; gr96.push_back( ".gr96" );
    formats_.push_back( MFF( "gr96", gr96, "GROMOS96 format", MFF::WRITE ) );
    //    * hin -- HyperChem HIN format
    SV hin; hin.push_back( ".hin" );
    formats_.push_back( MFF( "hin", hin, "HyperChem HIN format", MFF::READ_WRITE ) );
    //    * inchi -- IUPAC InChI descriptor [Write-only]
    SV inchi; inchi.push_back( ".inchi" );
    formats_.push_back( MFF( "inchi", inchi, "IUPAC InChI descriptor", MFF::WRITE ) );
    //    * inp -- GAMESS Input [Write-only]
    SV inp; inp.push_back( ".inp" );
    formats_.push_back( MFF( "inp", inp, "GAMESS Input", MFF::WRITE ) );
    //    * ins -- ShelX format [Read-only]
    SV ins; ins.push_back( ".ins" );
    formats_.push_back( MFF( "ins", ins, "ShelX format", MFF::READ ) );
    //    * jin -- Jaguar input format [Write-only]
    SV jin; jin.push_back( ".jin" );
    formats_.push_back( MFF( "jin", jin, "Jaguar input format", MFF::WRITE ) );
    //    * jout -- Jaguar output format [Read-only]
    SV jout; jout.push_back( ".jout" );
    formats_.push_back( MFF( "jout", jout, "Jaguar output forma", MFF::READ ) );
    //    * mdl -- MDL MOL format
    SV mdl; mdl.push_back( ".mdl" );
    formats_.push_back( MFF( "mdl", mdl, "MDL MOL format", MFF::READ_WRITE ) );
    //    * mmd, mmod -- MacroModel format
    SV mmd; mmd.push_back( ".mmd" ); mmd.push_back( ".mmod" );
    formats_.push_back( MFF( "mmd", mmd, "MacroModel format", MFF::READ_WRITE ) );
    //    * mol -- MDL MOL format
    SV mol; mol.push_back( ".mol" );
    formats_.push_back( MFF( "mol", mol, "MDL MOL format", MFF::READ_WRITE ) );
    //    * mol2 -- Sybyl Mol2 format
    SV mol2; mol2.push_back( ".mol2" );
    formats_.push_back( MFF( "mol2", mol2, "Sybyl Mol2 format", MFF::READ_WRITE ) );
    //    * molden -- Molden input format
    SV molden; molden.push_back( ".molden" ); molden.push_back( ".input" );
    formats_.push_back( MFF( "molden", molden, "Molden input format", MFF::READ ) );
    //    * mopcrt -- MOPAC Cartesian format
    SV mopcrt; mopcrt.push_back( ".mopcrt" );
    formats_.push_back( MFF( "mopcrt", mopcrt, "MOPAC Cartesian format", MFF::READ_WRITE ) );
    //    * mopout -- MOPAC Output format [Read-only]
    SV mopout; mopout.push_back( ".mopout" );
    formats_.push_back( MFF( "mopout", mopout, "MOPAC Output format", MFF::READ ) );
    //    * mpd -- Sybyl descriptor format [Write-only]
    SV mpd; mpd.push_back( ".mpd" );
    formats_.push_back( MFF( "mpd", mpd, "Sybyl descriptor format", MFF::WRITE ) );
    //    * mpqc -- MPQC output format [Read-only]
    SV mpqc; mpqc.push_back( ".mpqc" );
    formats_.push_back( MFF( "mpqc", mpqc, "MPQC output format", MFF::READ ) );
    //    * mpqcin -- MPQC simplified input format [Write-only]
    SV mpqcin; mpqcin.push_back( ".mpqcin" );
    formats_.push_back( MFF( "mpqcin", mpqcin, "MPQC simplified input format", MFF::WRITE ) );
    //    * msms-- Michel Sanner's MSMS input format [Write-only]
    SV msms; msms.push_back( ".msms" ); msms.push_back( ".xyzr" );
    formats_.push_back( MFF( "msms", msms, "Michel Sanner's MSMS input format", MFF::WRITE ) );
    //    * nw -- NWChem input format [Write-only]
    SV nw; mpqcin.push_back( ".nw" );
    formats_.push_back( MFF( "nw", nw, "NWChem input format", MFF::WRITE ) );
    //    * nwo -- NWChem output format [Read-only]
    SV nwo; nwo.push_back( ".nwo" );
    formats_.push_back( MFF( "nwo", nwo, "NWChem output format", MFF::READ ) );
    //    * pc -- PubChem format [Read-only]
    SV pc; pc.push_back( ".pc" );
    formats_.push_back( MFF( "pc", pc, "PubChem format", MFF::READ ) );
    //    * pcm -- PCModel format
    SV pcm; pcm.push_back( ".pcm" );
    formats_.push_back( MFF( "pcm", pcm, "PCModel format", MFF::READ_WRITE ) );
    //    * pdb -- Protein Data Bank format
    SV pdb; pdb.push_back( ".pdb" );
    formats_.push_back( MFF( "pdb", pdb, "Protein Data Bank format", MFF::READ_WRITE ) );
    //    * pov -- POV-Ray input format [Write-only]
    SV pov; pov.push_back( ".pov" );
    formats_.push_back( MFF( "pov", pov, "POV format", MFF::WRITE ) );
    //    * pqs -- Parallel Quantum Solutions format
    SV pqs; pqs.push_back( ".pqs" );
    formats_.push_back( MFF( "pqs", pqs, "Parallel Quantum Solutions format", MFF::READ_WRITE ) );
    //    * prep -- Amber Prep format [Read-only]
    SV prep; prep.push_back( ".prep" );
    formats_.push_back( MFF( "prep", prep, "Amber Prep format", MFF::READ ) );
    //    * qcin -- Q-Chem input format [Write-only]
    SV qcin; qcin.push_back( ".qcin" );
    formats_.push_back( MFF( "qcin", qcin, "Q-Chem input format", MFF::WRITE ) );
    //    * qcout -- Q-Chem output format [Read-only]
    SV qcout; qcout.push_back( ".qcout" );
    formats_.push_back( MFF( "qcout", qcout, "Q-Chem output format", MFF::READ ) );
    //    * report -- Open Babel report format [Write-only]
    SV report; report.push_back( ".report" );
    formats_.push_back( MFF( "report", report, "Open Babel report format", MFF::WRITE ) );
    //    * res -- ShelX format [Read-only]
    SV res; res.push_back( ".res" );
    formats_.push_back( MFF( "res", res, "ShelX format", MFF::READ ) );
    //    * rxn -- MDL RXN format
    SV rxn; rxn.push_back( ".rxn" );
    formats_.push_back( MFF( "rxn", rxn, "MDL RXN format", MFF::READ_WRITE ) );
    //    * sd, sdf -- MDL MOL format
    SV sdf; sdf.push_back( ".sd" ); sdf.push_back( ".sdf" );
    formats_.push_back( MFF( "sdf", sdf, "MDL MOL format", MFF::READ_WRITE ) );
    //    * smi -- SMILES format
    SV smi; smi.push_back( ".smi" );
    formats_.push_back( MFF( "smi", smi, "SMILES format", MFF::READ_WRITE ) );
    //    * t41 -- ADF ASCII format
    SV t41; t41.push_back( ".t41" );
    formats_.push_back( MFF( "t41", t41, "ADF ASCII format", MFF::READ ) );
    //    * test -- Test format [Write-only]
    SV test; test.push_back( ".test" );
    formats_.push_back( MFF( "test", test, "Test format", MFF::WRITE ) );
    //    * tmol -- TurboMole Coordinate format
    SV tmol; tmol.push_back( ".tmol" );
    formats_.push_back( MFF( "tmol", tmol, "TurboMole Coordinate format", MFF::READ_WRITE ) );
    //    * txyz -- Tinker MM2 format [Write-only]
    SV txyz; txyz.push_back( ".txyz" );
    formats_.push_back( MFF( "txyz", txyz, "Tinker MM2 format", MFF::WRITE ) );
    //    * unixyz -- UniChem XYZ format
    SV unixyz; unixyz.push_back( ".unixyz" );
    formats_.push_back( MFF( "unixyz", unixyz, "UniChem XYZ format", MFF::READ_WRITE ) );
    //    * vmol -- ViewMol format
    SV vmol; vmol.push_back( ".vmol" );
    formats_.push_back( MFF( "vmol", vmol, "ViewMol format", MFF::READ_WRITE ) );
    //    * xed -- XED format [Write-only]
    SV xed; xed.push_back( ".xed" );
    formats_.push_back( MFF( "xed", vmol, "XED format", MFF::WRITE ) );
    //    * xml -- General XML format [Read-only]
    //SV xml; xml.push_back( ".xml" );
    //formats_.push_back( MFF( "xml", xml, "General XML format", MFF::READ ) );
    //    * xyz -- XYZ cartesian coordinates format
    SV xyz; xyz.push_back( ".xyz" );
    formats_.push_back( MFF( "xyz", xyz, "XYZ cartesian coordinates format", MFF::READ_WRITE ) );
    //    * yob -- YASARA.org YOB format
    SV yob; yob.push_back( ".yob" );
    formats_.push_back( MFF( "yob", yob, "YASARA.org YOB format", MFF::READ_WRITE ) );
    //    * zin -- ZINDO input format [Write-only]
    SV zin; zin.push_back( ".zin" );
    formats_.push_back( MFF( "zin", zin, "ZINDO input format", MFF::WRITE ) );
    //    * zmatrix -- plain Z-matrix format
    SV zmat; zmat.push_back( ".zmat" ); zmat.push_back( ".zmatrix" );
    formats_.push_back( MFF( "zmatrix", zmat, "Z-matrix format", MFF::READ ) );
#endif
    initialized_ = true;

}



















