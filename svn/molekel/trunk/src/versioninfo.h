#ifndef VERSIONINFO_H_
#define VERSIONINFO_H_
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

/// Returns information about the current Molekel build.
void GetMolekelVersionInfo( int& major,
                            int& minor,
                            int& patchLevel,
                            int& buildNumber );

/// Returns build type: nightly, alpha, beta, release, debug, development...
const char* GetMolekelVersionType();

/// Returns copyright info
const char* GetMolekelCopyrightInfo();

/// Returns build date info.
const char* GetMolekelBuildDate();

/// Returns license info.
const char* GetMolekelLicense();

#endif /*VERSIONINFO_H_*/

