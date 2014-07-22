#ifndef MOLECULEANIMATIONMODEWIDGET_H_
#define MOLECULEANIMATIONMODEWIDGET_H_
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

class QComboBox;
class QSpinBox;
class AbstractAtomAnimator;
class MainWindow;


/// Per- molecule animation preferences: animation on/off,
/// animation mode: vibration, trajectory, mixed (vibration + trajectory)
class MoleculeAnimationModeWidget : public QWidget
{
    Q_OBJECT
public:
    /// Constructor.
    MoleculeAnimationModeWidget( MainWindow* mw, QWidget* parent = 0 );
    /// Set animator and update GUI.
    void SetAnimator( AbstractAtomAnimator* a ) { animator_ = a; UpdateGUI(); }
private slots:
    /// Called when animation on/off option changed.
    void AnimationEnableIndexChangedSlot( int );
    /// Called whne animation mode changed.
    void AnimationModeIndexChangedSlot( int );
    /// Called when timestep value changed by clicking on
    /// arrow buttons.
    void TimeIntValueChangedSlot( int );
    /// Called when timestep value changed by changing the text
    /// in the edit box.
    void TimeStringValueChangedSlot( const QString& );
private:
    /// Create GUI.
    void CreateGUI();
    /// Update GUI with values read from animator.
    void UpdateGUI();
    /// Timestep.
    QSpinBox* timeSpinBox_;
    /// Animation enable/disable.
    QComboBox* animationEnableComboBox_;
    /// Animation mode: Vibration, Trajectory, Mixed.
    QComboBox* animationModeComboBox_;
    /// Reference to animator whose property values are displayed
    /// in widget.
    AbstractAtomAnimator* animator_;
    /// Set to true when UI is being updated Update/CreateGUI to
    /// avoid executing code in slot methods.
    bool updatingGUI_;
    /// Reference to main window.
    MainWindow* mw_;
    
    //@{ Indices of combo box items used to
    /// map between combo box items and animator properties.
    static const int ANIMATION_ON_INDEX = 0;
    static const int ANIMATION_OFF_INDEX = 1;
    static const int VIBRATION_INDEX = 0;
    static const int TRAJECTORY_INDEX = 1;
    //@}

};

#endif /*MOLECULEANIMATIONMODEWIDGET_H_*/
