#ifndef MOLECULEANIMATIONDIALOG_H_
#define MOLECULEANIMATIONDIALOG_H_
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


#include "../widgets/MoleculeVibrationWidget.h"
#include "../widgets/MoleculeTrajectoryWidget.h"
#include "../widgets/MoleculeAnimationModeWidget.h"

class MolekelMolecule;
class AtomVibrationAnimator;
class AtomTrajectoryAnimator;
class AbstractAtomAnimator;
class MainWindow;
//
//----------------------------------
//   _________ __________ _________
//  |Animation|Trajectory|Vibration|
//  |          --------------------
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

/// Container for Molecule animation widgets.
class MoleculeAnimationDialog : public QDialog
{
    Q_OBJECT
public:
    /// Constructor.
    /// @param mol reference to molecule to edit
    /// @param aaa reference to atom animator.
    ///            used to edit timestap & animation enable
    //             properties.
    /// @param ava reference to vibration animator.
    ///            used to edit vibration amplitude and mode.
    /// @param ata reference to trajectory animator.
    ///            used to edit animation mode and loop mode.
    /// @param parent parent widget
    MoleculeAnimationDialog( MainWindow* mw,
    						 MolekelMolecule *mol,
                             AbstractAtomAnimator* aaa,
                             AtomVibrationAnimator* ava,
                             AtomTrajectoryAnimator* ata,
                             QWidget* parent = 0 ) : QDialog( parent )
    {
        QVBoxLayout* mainLayout = new QVBoxLayout();
        QTabWidget* tabWidget = new QTabWidget( this );
        MoleculeVibrationWidget* mvw = new MoleculeVibrationWidget;
        MoleculeTrajectoryWidget* mtw = new MoleculeTrajectoryWidget;
        MoleculeAnimationModeWidget* maw = new MoleculeAnimationModeWidget( mw );
        maw->SetAnimator( aaa );
        mvw->SetMolecule( mol );
        mvw->SetAnimator( ava );
        mtw->SetMolecule( mol );
        mtw->SetAnimator( ata );
        tabWidget->addTab( maw, tr( "Animation" ) );
        tabWidget->addTab( mvw, tr( "Vibration" ) );
        tabWidget->addTab( mtw, tr( "Trajectory" ) );
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

#endif /*MOLECULEANIMATIONDIALOG_H_*/
