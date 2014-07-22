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

// STD
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
// reference: http://www.cmbi.ru.nl/molden/molden_format.html

#include <openbabel/obconversion.h>
#include <openbabel/obmolecformat.h>
#include <openbabel/mol.h>

using namespace std;
using namespace OpenBabel;

/// Molden input reader: reads atoms from [Atoms] section of Molden input file.
class OBZmatrixFormat : public OpenBabel::OBMoleculeFormat
{
public:
    /// Constructor: register 'cube' and "CUBE" format.
    OBZmatrixFormat()
    {
        OBConversion::RegisterFormat( "zmatrix", this );
    }

    /// Return description.
    virtual const char* Description() //required
    {
        return
        "Z-Matrix  format\n"
        "Read only.\n"
        "b no bonds\n"
        "s no multiple bonds\n\n";
    }

    /// Return a specification url
    virtual const char* SpecificationURL()
    {
        return 0;
    }

    /// Return MIME type, NULL in this case.
    virtual const char* GetMIMEType() { return 0; };

      /// Return read/write flag: read only.
    virtual unsigned int Flags()
    {
        return READONEONLY;
    };

    /// Skip to object: used for multi-object file formats.
    virtual int SkipObjects( int n, OpenBabel::OBConversion* pConv ) { return 0; }

    /// Read.
    virtual bool ReadMolecule( OpenBabel::OBBase* pOb, OpenBabel::OBConversion* pConv );

    /// Write: always returns false.
    virtual bool WriteMolecule( OpenBabel::OBBase* , OpenBabel::OBConversion* )
    {
        return false;
    }

private:

    /// Maps atom name to atomic number.
    int GetAtomicNumber( const string& name ) const
    {
        int iso;
        return etab.GetAtomicNum( name.c_str(), iso );
    }

    /// Adjust angle: currently not used; when exporting from
    /// Molden. It seems that <OpenBabel angle> = <360 - Molden angle> when
    /// Molden angle > 180. ???
    double AdjustAngle( double a ) const
    {
        return a;
        //if( a > 180. ) return ( 360. - a );
        //return a;
    }
};

//------------------------------------------------------------------------------

namespace
{
    // Global variable used to register Z-Matrix format.
    OBZmatrixFormat zmatixFormat__;
}

//------------------------------------------------------------------------------


//==============================================================================

//------------------------------------------------------------------------------
bool OBZmatrixFormat::ReadMolecule( OBBase* pOb, OBConversion* pConv )
{
    OBMol* pmol = dynamic_cast< OBMol* >(pOb);
    if( pmol == 0 ) return false;

    istream& ifs = *pConv->GetInStream();

    pmol->BeginModify();
    pmol->SetDimension( 3 );
    string lineBuffer;
    getline( ifs, lineBuffer );

    while( ifs && ( lineBuffer.size() == 0 ) ) getline( ifs, lineBuffer );

    if( !ifs ) return false;

    vector< OBInternalCoord* > atoms;
    vector< OBAtom* > atomData;
    atomData.push_back( 0 ); // 1 atom offset

    // read first z-matrix line
    istringstream is1( lineBuffer );
    string a1Name;
    is1 >> a1Name;
    if( a1Name.size() == 0 ) return false;

    OBAtom* a1 = pmol->NewAtom();
    a1->SetAtomicNum( GetAtomicNumber( a1Name ) ); //a1Name can be in the form 'C12'
    atoms.push_back( new OBInternalCoord );
    atomData.push_back( a1 );
    //cout << a1Name << endl;
    lineBuffer = "";

    // read second z-matrix line
    while( ifs && ( lineBuffer.size() == 0 ) ) getline( ifs, lineBuffer );
    if( ifs )
    {
        istringstream is2( lineBuffer );
        string a2Name;
        int i1;
        double dist = 0.;
        is2 >> a2Name;
        if( a2Name.size() == 0 ) return false;
        is2 >> i1 >> dist;

        OBAtom* a2 = pmol->NewAtom();
        a2->SetAtomicNum( GetAtomicNumber( a2Name ) ); //a1Name can be in the form 'C12'
        atomData.push_back( a2 );
        atoms.push_back( new OBInternalCoord( atomData[ i1 ] ) );
        atoms.back()->_dst = dist;
        //cout << a2Name << ' ' << i1 << ' ' << dist << endl;
    }
    lineBuffer = "";

    // read third z-matix line
    while( ifs && ( lineBuffer.size() == 0 ) ) getline( ifs, lineBuffer );
    if( ifs )
    {
        istringstream is3( lineBuffer );
        string a3Name;
        int i1, i2;
        double dist;
        double angle = 0.;
        is3 >> a3Name;
        if( a3Name.size() == 0 ) return false;
        is3 >> i1 >> dist >> i2 >> angle;

        OBAtom* a3 = pmol->NewAtom();
        atomData.push_back( a3 );
        a3->SetAtomicNum( GetAtomicNumber( a3Name ) ); //a1Name can be in the form 'C12'
        atoms.push_back( new OBInternalCoord( atomData[ i1 ], atomData[ i2 ] ) );
        atoms.back()->_dst = dist;
        atoms.back()->_ang = AdjustAngle( angle );
        //cout << a3Name << ' ' << i1 << ' ' << dist << ' ' << i2 << ' ' << angle << endl;
    }
    lineBuffer = "";
    // read subsequent lines
    while( getline( ifs, lineBuffer ) )
    {
        if( lineBuffer.size() == 0 ) continue;
        string aName;
        int i1, i2, i3;
        double dist, angle, dangle;
        istringstream is( lineBuffer );
        is >> aName;
        if( aName.size() == 0 ) continue;
        is >> i1 >> dist >> i2 >> angle >> i3 >> dangle;
        OBAtom* atom = pmol->NewAtom();
        atomData.push_back( atom );
        atom->SetAtomicNum( GetAtomicNumber( aName ) );
        atoms.push_back( new OBInternalCoord( atomData[ i1 ],
                                              atomData[ i2 ],
                                              atomData[ i3 ] ) );
        atoms.back()->_dst = dist;
        atoms.back()->_ang = AdjustAngle( angle );
        atoms.back()->_tor = AdjustAngle( dangle );
        //cout << aName << ' ' << i1 << ' ' << dist << ' ' << i2 << ' ' << angle << ' ' << i3 << ' ' << dangle << endl;
        lineBuffer = "";
    }

    InternalToCartesian( atoms, *pmol );

    if( !pConv->IsOption( "b", OBConversion::INOPTIONS ) ) pmol->ConnectTheDots();
    if (!pConv->IsOption( "s", OBConversion::INOPTIONS )
        && !pConv->IsOption( "b", OBConversion::INOPTIONS ) )
    {
        pmol->PerceiveBondOrders();
    }
    pmol->EndModify();

    return true;
}
