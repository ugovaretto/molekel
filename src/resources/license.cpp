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

static const char LICENSE[] =
    "Molekel is Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)\n"
    "Portions of the source code are Copyright (C) by Ugo Varetto\n"
    "\n"
    "CSCS website: http://www.cscs.ch\n"
    "\n"
    "This program is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU General Public License\n"
    "as published by the Free Software Foundation; either version 2\n"
    "of the License, or (at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program; if not, write to the Free Software\n"
    "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,\n"
    "MA  02110-1301, USA.\n"
    "\n"
    "Molekel is built on top of the following open source libraries/tools; please\n"
    "refer to the specific tool's website for license/copyright information:\n"
    "\n"
    "Qt:\n http://www.trolltech.com/products/qt\n"
    "VTK: \n http://www.vtk.org\n"
    "OpenBabel:\n http://openbabel.sourceforge.net\n"
    "Coin3D:\n http://www.coin3d.org\n"
    "OpenMOIV:\n http://www.tecn.upf.es/openMOIV\n"
    "GLEW:\n http://glew.sourceforge.net";

//------------------------------------------------------------------------------
/// Returns license info.
const char* GetMolekelLicense()
{
    return &LICENSE[ 0 ];
}

