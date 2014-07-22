#ifndef MOLECULEVIBRATIONWIDGET_H_
#define MOLECULEVIBRATIONWIDGET_H_
//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008 Swiss National Supercomputing Centre (CSCS)
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
class AtomVibrationAnimator;
class QDoubleSpinBox;
class QTableWidget;
class QTableWidgetItem;
class QString;
class QCheckBox;

//---------------------------------------
//                 ________ __
// Scaling        |________|^v| (spinbox)
//
// [ ] Show arrows
//
// [ ] Constant arrow length
//                 ________ __
// Arrow scaling  |________|^v| (spinbox)
//
// __________________________
// Frequency |     Type      |
// --------------------------
//  1.22     |      A        |
// --------------------------
//  1234     |      A        |
// --------------------------
//  ...      |     ...
// __________________________
//
//---------------------------------------

/// Allows user to select the current vibration mode
/// and scaling factor.
class MoleculeVibrationWidget : public QWidget
{
    Q_OBJECT
private slots:
    void DoubleValueChangedSlot( double v );
    void ArrowScalingChangedSlot( double v );
    void StringValueChangedSlot( const QString& s );
    void ShowArrowsSlot( int );
    void ConstArrowLengthSlot( int );
    void TableItemChangedSlot( QTableWidgetItem* i  );
public:
    /// Updates GUI reading data from MolekelMolecule instance.
    void UpdateGUI();
    /// Constructor. Calls CreateGUI();
    MoleculeVibrationWidget( QWidget* parent = 0 );
    /// Sets reference to molecule.
    void SetMolecule( MolekelMolecule* mol ) { mol_ = mol; UpdateGUI(); }
    /// Sets reference to animator.
    void SetAnimator( AtomVibrationAnimator* a ) { animator_ = a; UpdateGUI(); }
private:
    /// Creates GUI.
    void CreateGUI();
private:
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Reference to animator whose property values are being displayed in widget.
    AtomVibrationAnimator* animator_;
    /// Vibration amplitude: vibration  = Amplitude * sin( Frequency );
    QDoubleSpinBox* scalingSpinBox_;
    /// Table used to select the vibration mode to animate.
    QTableWidget* table_;
    /// Checkbox used to toggle arrows visibility.
    QCheckBox* showArrowsCheckBox_;
    /// Checkbox used to toggle fixed/variable arrow size.
    QCheckBox* constArrowLengthCheckBox_;
    /// Arrow scaling factor.
    QDoubleSpinBox* arrowScalingSpinBox_;
    /// Set to true when UI is being updated Update/CreateGUI to
    /// avoid executing code in slot methods.
    bool updatingGUI_;
};

#endif /*MOLECULEVIBRATIONWIDGET_H_*/
