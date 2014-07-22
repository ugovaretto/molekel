#ifndef MOLEKELDATA_H_
#define MOLEKELDATA_H_
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

// STD
#include <map>
#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <limits>

/// @todo add smart pointers

// Forward declaration

#include "MolekelMolecule.h"
#include "MolekelException.h"

class vtkRenderer;

typedef MolekelMolecule* MolekelMoleculePtr;
typedef const MolekelMolecule* MolekelMoleculeConstPtr;



/// Molecule file format; used to store information about
/// a specific file format and the capabilities of the
/// file format handler.
/// @todo move in separate file after creating a generic
/// MoleculeData interface and deriving MolekelData from this interface.
class MoleculeFileFormat
{
public:
    /// Supported I/O actions: read, write read & write
    typedef enum { READ, WRITE, READ_WRITE } IOMode;
    /// Returns format id as a string.
    const std::string& GetFormat() const { return format_; }
    /// Returns list of file extensions associated with this
    /// format as a single string e.g. *.pdb *.mol.
    std::string GetExtensionsAsString() const
    {
        typedef std::vector< std::string > StringVector;
        std::ostringstream buf;
        for( StringVector::const_iterator ei = extensions_.begin();
             ei != extensions_.end();
             ++ei )
        {
            buf << '*' << *ei << ' ';
        }
        return buf.str();
    }
    /// Returns a short description of the file format.
    const std::string& GetDescription() const { return description_; }
    /// Returns supported I/O actions.
    IOMode GetIOSupport() const  { return supportedIOMode_; }
    /// Returns true if and only if this file format can be read.
    bool CanRead() const { return supportedIOMode_ == READ || supportedIOMode_ == READ_WRITE; }
    /// Returns true if and only if this file format can written.
    bool CanWrite() const { return supportedIOMode_ == WRITE || supportedIOMode_ == READ_WRITE; }
    /// Returns the chemical MIME type if available.
    const std::string& GetMIMEType() const { return MIMEType_; }
    /// Returns specification info e.g. the URL where the format is described.
    const std::string& GetSpecificationInfo() const { return specificationInfo_; }
    /// Constructor.
    MoleculeFileFormat( const std::string& format,
                        const std::vector< std::string >& extensions,
                        const std::string& description,
                        IOMode supportedIOMode,
                        const std::string MIMEType = "",
                        const std::string specificationInfo = "" )
                        : format_( format ), extensions_( extensions ),
                          description_( description ), supportedIOMode_( supportedIOMode ),
                          MIMEType_( MIMEType ), specificationInfo_( specificationInfo )
                        {}

private:
    std::string format_;
    std::vector< std::string > extensions_;
    std::string description_;
    IOMode supportedIOMode_;
    std::string MIMEType_;
    std::string specificationInfo_;
};


/// Holds molecule data; performs basic DB operations.
class MolekelData
{
public:
    /// Molecule index/key type.
    typedef unsigned int IndexType;
    /// Molecule database format: associative container.
    typedef std::map< IndexType, MolekelMolecule* > Molecules;
    /// Size type for database.
    typedef Molecules::size_type SizeType;
    /// Type for file format list.
    typedef std::vector< MoleculeFileFormat > FileFormats;
    /// Invalid index.
    /// Same concept as string::npos: max index value == max(string::size_type) - 1
    static IndexType InvalidIndex();
    /// Constructor.
    MolekelData() {}
    /// Destructor.
    ~MolekelData();
    /// Reads molecule from file, adds it to database and provided VTK renderer.
    /// @param fileName file name (.pdb .mol .g98 ...).
    /// @param renderer vtkRenderer to which molecule actor will be added.
    /// @param cb callback used to report progress.
    /// @return molecule index, use the returned value as a reference/handle
    /// to the newly added molecule.
    /// @throw MolekelException in case a problem occurs.
    IndexType AddMolecule( const char* fileName,
                           vtkRenderer* renderer,
                           ILoadMoleculeCallback* cb,
                           bool computeBonds ); // throws MolekelException
    /// Reads molecule from file, adds it to database and provided VTK renderer.
    /// @param fileName file name (.pdb .mol .g98 ...).
    /// @param renderer vtkRenderer to which molecule actor will be added.
    /// @param format file format, used to explicitly set the file format.
    /// @param cb callback used to report progress.
    /// @return molecule index, use the returned value as a reference/handle
    /// to the newly added molecule.
    /// @throw MolekelException in case a problem occurs.
    IndexType AddMolecule( const char* fileName,
                           const char* format,
                           vtkRenderer* renderer,
                           ILoadMoleculeCallback* cb,
                           bool computeBonds ); // throws MolekelException
    /// Remove molecule.
    /// @param id molecule id @see AddMolecule
    /// @throw MolekelException in case the index is invalid.
    void RemoveMolecule( IndexType id );
    /// Save molecule to a file; output format is inferred from filename extension.
    /// @param id molecule id @see AddMolecule.
    /// @param fileName file name.
    /// @throw MolekelException in case a problem occurs.
    void SaveMolecule( IndexType id, const char* fileName ) const; // throws MolekelException
    /// Save molecule to a file. 
    /// @param id molecule id @see AddMolecule.
    /// @param fileName file name.
    /// @param type OpenBabel file type.
    /// @throw MolekelException in case a problem occurs.
    void SaveMolecule( IndexType id, const char* fileName, const char* type ) const; // throws MolekelException
    /// Returns reference to molecule.
    /// @param id molecule id.
    /// @throw MolekelException if id invalid.
    MolekelMolecule* GetMolecule( IndexType id );
    /// Removes all molecules from database; in case molecules are added to
    /// a VTK renderer they have to be removed from the VTK renderer before
    /// calling this method.
    void Clear();
    /// Applies a unary function object to all molecules in database.
    template < class UnaryFunT > void Apply( UnaryFunT fun )
    {
        for( Molecules::iterator i = molecules_.begin();
             i != molecules_.end();
             ++i )
        {
            fun( i->second );
        }
    }
    /// Returns number of molecules in database.
    SizeType GetNumberOfMolecules() { return molecules_.size(); }
    /// Returns list of supported file formats.
    const FileFormats& GetSupportedFileFormats() const { InitFileFormats(); return formats_; }

private:
    /// Check if molecule id is valid.
    /// @param id molecule id @see AddMolecule
    /// @throw MolekelException if index invalid.
    void CheckIndex( IndexType id ) const;
    /// Returns new unique value used as a molecule's id.
    static IndexType GetNewID();
    /// Initialize file format list. File format database is built at first invocation;
    /// subsequent invocations have no effect and the method immediately returns.
    static void InitFileFormats();
    /// Database, implemented as a map< IndexType, MolekelMolecule* >.
    Molecules molecules_;
    /// Last generated id.
    static IndexType lastId_;
    /// File formats.
    static FileFormats formats_;
    /// Set true after first initialization performed.
    static bool initialized_;
};

/// Returns value for invalid index.
inline MolekelData::IndexType MolekelData::InvalidIndex()
{
     return std::numeric_limits< IndexType >::max();
}

/// Returns new unique id.
inline MolekelData::IndexType MolekelData::GetNewID()
{
    /// @todo add atomic increment when porting to thread-safe (if ever needed)
    if( lastId_ == InvalidIndex() ) throw MolekelException( "Invalid Molecule id" );
    IndexType id = lastId_;
    ++lastId_;
    return id;
}

#endif /*MOLEKELDATA_H_*/
