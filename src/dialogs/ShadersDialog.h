#ifndef SHADERSDIALOG_H_
#define SHADERSDIALOG_H_
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

// QT
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTabWidget>


#include "../widgets/ShaderWidget.h"
#include "../MolekelMolecule.h"

class MainWindow;


//
//----------------------------------
//   ________ ___ ___ _______ ___
//  |Molecule|SAS|SES|Orbital|...
//  |         ----------------------
//  |                              |
//  |                              |
//  |                              |
//  |                              |
//  |                              |
//  |                              |
//   ------------------------------
//   ____
//  |_Ok_|
//
//----------------------------------
//

/// Container for Shader widget: one tab per surface type.
class ShadersDialog : public QDialog
{
    Q_OBJECT
public:

    ShadersDialog( MainWindow* mw,
                   MolekelMolecule *mol,
                   QWidget* parent = 0 ) : QDialog( parent )
    {
        typedef MolekelMolecule::ShaderProgram Sh;
        typedef ShaderWidget Sw;
        QVBoxLayout* mainLayout = new QVBoxLayout();
        QTabWidget* tabWidget = new QTabWidget( this );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::MOLECULE_SURFACE ), "Molecule" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::SESMS_SURFACE ), "SES(MS)" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::SAS_SURFACE ), "SAS" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::DENSITY_MATRIX_SURFACE ), "Density Matrix" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::ORBITAL_POSITIVE_SURFACE ), "Orbital(+)" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::ORBITAL_NODAL_SURFACE ), "Orbital(0)" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::ORBITAL_NEGATIVE_SURFACE ), "Orbital(-)" );
        tabWidget->addTab( new Sw( mw, mol, MolekelMolecule::GRID_DATA_SURFACE ), "Grid Data (.cube, .t41)" );

        mainLayout->addWidget( tabWidget );
        QPushButton* b = new QPushButton( tr( "Close" ) );
        connect( b, SIGNAL( released() ), this, SLOT( AcceptSlot() ) );
        mainLayout->addWidget( b );
        this->setLayout( mainLayout );
    }
public slots:
    /// Called when 'Ok' clicked.
    void AcceptSlot() { accept(); }
};

#endif /*SHADERSDIALOG_H_*/
