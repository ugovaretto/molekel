//
// Copyright (c) 2006, 2007, 2008, 2009 - Ugo Varetto and 
// Swiss National Supercomputing Centre (CSCS)
//
// This source code is free; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This source code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this source code; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
// 


// bug in OpenBabel code: obiter.h has references to the following two classes
// which are not forward declared in the header.
class OBNodeBase;
class OBEdgeBase;
#include <openbabel/obconversion.h>
#include <openbabel/obiter.h>
#include <openbabel/data.h>
#include <openbabel/mol.h>
#include <openbabel/obmolecformat.h>

#include <iostream>

using namespace std;

using namespace OpenBabel;

//==============================================================================
/// Class to output a molecule in XYZR MSMS input format for further computation
/// of Connolly surface.
/// Michel Sanner page with info on MSMS:
/// http://www.scripps.edu/~sanner/
class OBMSMSFormat : public OpenBabel::OBMoleculeFormat
{
public:
    /// Constructor: register 'msms' and "MSMS" format.
    OBMSMSFormat()
    {
        OpenBabel::OBConversion::RegisterFormat( "msms", this );
        OpenBabel::OBConversion::RegisterFormat( "MSMS", this );
    }

    /// Return description.
    virtual const char* Description() //required
    {
        return
        "M.F. Sanner's MSMS input format"
        "Write only.\n"
        "a output atom names\n";
    }

    /// Return a specification url, not really a specification since
    /// I couldn't find it but close enough.
    virtual const char* SpecificationURL()
    {
        return "http://www.scripps.edu/~sanner";
    }

    /// Return MIME type, NULL in this case.
    virtual const char* GetMIMEType() { return 0; };

      /// Return read/write flag: read only.
    virtual unsigned int Flags()
    {
        return WRITEONEONLY;
    };

    /// Skip to object: used for multi-object file formats.
    virtual int SkipObjects( int n, OpenBabel::OBConversion* pConv ) { return 0; }

    /// Read: always return false.
    virtual bool ReadMolecule( OpenBabel::OBBase*, OpenBabel::OBConversion* )
    {
        return false;
    }

    /// Write.
    virtual bool WriteMolecule( OpenBabel::OBBase* , OpenBabel::OBConversion* );
};

//------------------------------------------------------------------------------

namespace
{
    // Global variable used to register MSMS format.
    OBMSMSFormat msmsFormat__;
}

//------------------------------------------------------------------------------


//==============================================================================

//------------------------------------------------------------------------------
bool OBMSMSFormat::WriteMolecule( OBBase* pOb, OBConversion* pConv )
{
    OBMol* pmol = dynamic_cast< OBMol* >(pOb);
    if( pmol == 0 ) return false;

    ostream& os = *pConv->GetOutStream();

    const bool atomNames = pConv->IsOption( "a", OBConversion::OUTOPTIONS ) != 0;

    // write header ?

    // iterate through atoms and write <atom x> <atom y> <atom z> <atom radius>
    // and optionally <atomic number> in case atomNames == true

    FOR_ATOMS_OF_MOL( a, *pmol )
    {
        const double* c = a->GetCoordinate();
        os << c[ 0 ] << '\t' << c[ 1 ] << '\t' << c[ 2 ] << '\t' <<
        etab.GetVdwRad( a->GetAtomicNum() );
        if( atomNames ) os << '\t' << a->GetAtomicNum();
        os << '\n';
    }
    os.flush();
    return true;
}
