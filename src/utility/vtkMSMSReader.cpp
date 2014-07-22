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

//STD
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

// VTK
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include "vtkMSMSReader.h"

using namespace std;

//------------------------------------------------------------------------------
vtkCxxRevisionMacro( vtkMSMSReader, " $Revision$ ");

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkMSMSReader );

//------------------------------------------------------------------------------
vtkMSMSReader::vtkMSMSReader()
{
  SetNumberOfInputPorts( 0 );
}

//------------------------------------------------------------------------------
int vtkMSMSReader::RequestData( vtkInformation *vtkNotUsed( request ),
                                vtkInformationVector **vtkNotUsed( inputVector ),
                                vtkInformationVector *outputVector )
{

    // get info object
    vtkInformation *outInfo = outputVector->GetInformationObject( 0 );

    // get ouptut
    vtkPolyData *output = vtkPolyData::SafeDownCast( outInfo->Get( vtkDataObject::DATA_OBJECT() ) );

    if( fileName_.size() == 0 )
    {
        vtkErrorMacro( << "A FileName must be specified." );
        return 0;
    }

    // open vert file
    ifstream vert_in( ( fileName_ + ".vert" ).c_str() );

    if(!vert_in )
    {
        vtkErrorMacro( << "File " << ( fileName_ + ".vert" ).c_str() << " not found" );
        return 0;
    }

    vtkDebugMacro( << "Reading .vert file" );

    // intialize some structures to store the file contents in
    vtkSmartPointer< vtkPoints > points( vtkPoints::New() ); // vertices
    vtkSmartPointer< vtkFloatArray > normals( vtkFloatArray::New() ); // normals
    normals->SetNumberOfComponents( 3 );
    vtkSmartPointer< vtkCellArray > polys( vtkCellArray::New() ); // triangles

    // read vert header
    int numVertices = 0;
    int numSpheres = 0;
    double density = 0.;
    double probeRadius = 0;
    string lineBuffer;
    istringstream line;
    getline( vert_in, lineBuffer ); // 1st
    getline( vert_in, lineBuffer ); // 2nd
    getline( vert_in, lineBuffer ); // 3rd
    line.str( lineBuffer );

    line >> numVertices >> numSpheres >> density >> probeRadius;

    while( vert_in )
    {
        getline( vert_in, lineBuffer );
        if( lineBuffer.size() == 0 || lineBuffer[ 0 ] == '#' ) continue;
        istringstream line;
        line.str( lineBuffer );
        double xyz[ 3 ]  = { 0., 0., 0. };
        double nxyz[ 3 ] = { 0., 0., 0. };
        int faceId;
        int closestSphereId;
        int type;
        string atomName;
        line >> xyz[ 0 ] >> xyz[ 1 ] >> xyz[ 2 ] >> nxyz[ 0 ] >> nxyz[ 1 ] >> nxyz[ 2 ] >> faceId
             >> closestSphereId >> type;
//printf( "%f\t%f\t%f\t%f\t%f\t%f\t%d\t%d\t%d\n", xyz[ 0 ], xyz[ 1 ], xyz[ 2 ], nxyz[ 0 ], nxyz[ 1 ], nxyz[ 2 ],
//                                        faceId, closestSphereId, type );
        if( line ) line >> atomName;
        points->InsertNextPoint( xyz );
        normals->InsertNextTuple( nxyz );
    }

    // done reading vertex information
    vert_in.close();

    // read faces

    // open face file
    ifstream face_in( ( fileName_ + ".face" ).c_str() );

    if(!face_in )
    {
        vtkErrorMacro(<< "File " << ( fileName_ + ".face" ).c_str() << " not found" );
        return 0;
    }

    vtkDebugMacro( << "Reading .face file" );

    // read face header

    int numFaces = 0;
    getline( face_in, lineBuffer ); // 1st
    getline( face_in, lineBuffer ); // 2nd
    getline( face_in, lineBuffer ); // 3rd
    istringstream line2;
    line2.str( lineBuffer );
    line2 >> numFaces>> numSpheres >> density >> probeRadius;

//printf( "\n\n=============================================================\n\n" );
    output->SetPoints( points );
    while( face_in )
    {
        getline( face_in, lineBuffer );
        if( lineBuffer.size() == 0 || lineBuffer[ 0 ] == '#' ) continue;
        istringstream line;
        line.str( lineBuffer );
        int triangle[ 3 ]  = { 0, 0, 0 };
        int faceId;
        int type;
        line >> triangle[ 0 ] >> triangle[ 1 ] >> triangle[ 2 ] >> type >> faceId;
        --triangle[ 0 ]; // convert to zero based index
        --triangle[ 1 ]; // convert to zero based index
        --triangle[ 2 ]; // convert to zero based index
        polys->InsertNextCell( 0 );
        polys->InsertCellPoint( triangle[ 0 ] );
        polys->InsertCellPoint( triangle[ 1 ] );
        polys->InsertCellPoint( triangle[ 2 ] );
        polys->UpdateCellCount( 3 );
 //printf( "%d\t%d\t%d\t%d\t%d\n", triangle[ 0 ], triangle[ 1 ], triangle[ 2 ], type, faceId );
    }

    // done
    //polys->UpdateCellCount( numFaces * 3 );

    face_in.close();

    vtkDebugMacro( << "Copying file data into the output" );
    output->SetPoints( points );
    output->SetPolys( polys );
    output->GetPointData()->SetNormals( normals );
    output->Squeeze();
    return 1;
}

//------------------------------------------------------------------------------
void vtkMSMSReader::PrintSelf( ostream& os, vtkIndent indent )
{
  Superclass::PrintSelf( os, indent );
  os << indent << "File Name: " << fileName_ << '\n';
}
