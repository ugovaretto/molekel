#ifndef MOLECULETRAJECTORYWIDGET_H_
#define MOLECULETRAJECTORYWIDGET_H_
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

class MolekelMolecule;
class MainWindow;
class AtomTrajectoryAnimator;
class QString;
class QComboBox;
class QSpinBox;

//
//------------------------------
//
// Number of Frames: xx   <-- label
//             ________ _
// Direction  |________|v|
//             ________ _
// Loop Mode  |________|v|
//                 ___ __
// Step           |___|v^|
//
//------------------------------
//

/// Allows user to change trajectory animation parameters.
class MoleculeTrajectoryWidget : public QWidget
{
    Q_OBJECT
private slots:
    void DirectionIndexChangedSlot( int );
    void LoopModeIndexChangedSlot( int );
    void StepChangedSlot( int );
public:
    /// Updates GUI reading data from MolekelMolecule instance.
    void UpdateGUI();
    /// Constructor. Calls CreateGUI();
    MoleculeTrajectoryWidget( QWidget* parent = 0 );
    /// Sets reference to molecule.
    void SetMolecule( MolekelMolecule* mol ) { mol_ = mol; UpdateGUI(); }
    /// Sets reference to animator.
    void SetAnimator( AtomTrajectoryAnimator* a ) { animator_ = a; UpdateGUI(); }
private:
    /// Creates GUI.
    void CreateGUI();
private:
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Reference to animator whose property values are displayed in widget.
    AtomTrajectoryAnimator* animator_;
    /// Animation direction.
    QComboBox* directionComboBox_;
    /// Loop mode:
    /// - REPEAT: repeat indefinitely from first frame  to last frame.
    ///           @note first ==  last and last == first if direction == backward.
    /// - SWING: go indefinitely from first frame to last frame and back to first frame.
    ///          @note first ==  last and last == first if direction == backward.
    QComboBox* loopModeComboBox_;
    /// Step == frames per time step.
    QSpinBox* stepSpinBox_;
    /// Set to true when UI is being updated Update/CreateGUI to
    /// avoid executing code in slot methods.
    bool updatingGUI_;
    //@{ Indices of combo box items used to map between combo box items and
    ///  animator properties.
    static const int DIRECTION_FORWARD_INDEX = 0;
    static const int DIRECTION_BACKWARD_INDEX = 1;
    static const int LOOPMODE_SWING_INDEX = 0;
    static const int LOOPMODE_REPEAT_INDEX = 1;
    static const int LOOPMODE_ONE_TIME_INDEX = 2;
    //@}
};

#endif /*MOLECULETRAJECTORYWIDGET_H_*/
