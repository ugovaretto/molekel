#ifndef VTKMSMSREADER_H_
#define VTKMSMSREADER_H_
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
#include <string>

// VTK
#include <vtkPolyDataAlgorithm.h>

/// Reader for  MSMS output files.
/// The msms program generate a mesh representing the Connolly surface
/// of a molecule.
/// Reference: Michel Senner home page - http://www.scripps.edu/~sanner/
/// MSMS output is composed of two files:
/// - <out file>.face faces
/// - <in file>.vert verices + normals
class vtkMSMSReader : public vtkPolyDataAlgorithm
{
public:
    /// Factory method.
    static vtkMSMSReader *New();
    /// Version info.
    vtkTypeRevisionMacro( vtkMSMSReader, vtkPolyDataAlgorithm );
    /// Overriden method for printing object information.
    void PrintSelf(ostream& os, vtkIndent indent);
    /// Returns file name.
    const std::string& GetFileName() const { return fileName_; }
    /// Sets file name.
    void SetFileName( const std::string& fname   ) { fileName_ = fname; }
private:
    /// Input file name.
    std::string fileName_;
    /// Private default constructor.
    vtkMSMSReader();
    /// Private default destructor - use Delete() method to delete this object.
    ~vtkMSMSReader() {}
    /// Overridden method required by VTK.
    int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
    /// Disable copy contructor.
    vtkMSMSReader( const vtkMSMSReader& );
    /// Disable assignment.
    vtkMSMSReader& operator=( const vtkMSMSReader& );
};

#endif // VTKMSMSREADER_H_
