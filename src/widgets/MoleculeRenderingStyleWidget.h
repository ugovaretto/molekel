#ifndef MOLECULERENDERINGSTYLEWIDGET_H_
#define MOLECULERENDERINGSTYLEWIDGET_H_
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

class MainWindow;
class MolekelMolecule;
class QComboBox;
class QSlider;
class QLabel;
class QCheckBox;


//------------------------------
//                   ________ _
// Molecule         |________|v|
//                   ________ _
// Atom             |________|v|
//                   ________ _
// Bond             |________|v|
//                   ________ _
// Residue          |________|v|
//                   ________ _
// Dipole Moment    |________|v|
//
// Atom Detail      ===||======
//
// Bond Detail      ===||======
//
// Atom Scaling     ===||======
//
// Bond Scaling     ===||======
//  _______
// |_Color_|          0xFFAABB
//  _____________
// |_Atom Colors_|    Default
//
// -----------------------------
//
// [ ] Show bounding box
//
// [ ] Real-time update
//  ____    _______    ________
// |_Ok_|  |_Apply_|  |_Cancel_|
//
//------------------------------

// set to either QWidget or QDialog
typedef QDialog BaseWidget;

/// @todo consider using policy based approach:
/// template < class BaseT > class MoleculeRenderingStyleWidget : BaseT
/// will have to include .cpp into .h.
/// OR #ifdef, in both cases the type is resolved at compile time
/// but policy based design allows client classes to decide
/// how to use this widget e.g. derive from QWidget to add to QDockingWidget
/// or fron QDialog to use open a stand-alone dialog

/// Rendering style widget. Used to change molecule/atom/bond/residue
/// display style.
class MoleculeRenderingStyleWidget : public BaseWidget
{
    Q_OBJECT

public slots:
    /// Called when 'Ok' clicked.
    void AcceptSlot();
    /// Called when 'Cancel' clicked.
    void ApplySlot();
    /// Called when "Molecule Color" button released.
    void SetColorSlot();
    /// Invoked when values changed by user: molecule is updated
    /// only of real-time update is checked.
    void RealTimeUpdateSlot();
    /// Loads atom colors from file
    void LoadAtomColorsSlot();
private:
    /// Called to update UI reading parameter values from
    /// MolekelMolecule class instance.
    void UpdateUI();
    /// Called to update MolekelMolecule class instance
    /// with values read from widget.
    void UpdateMolecule();
    /// Initializes UI controls.
    void Init();
    /// Creates widget layout using instances of QBoxLayout derived
    /// classes and fills widget with UI controls. @todo remove!
    void CreateBoxLayout();
    /// Creates widget layout using a QGridLayout.
    void CreateGridLayout();
public:
    /// Data structure used to pass registry keys
    /// for persistent settings to widget.
    struct PersistentSettingsKeys
    {
        std::string atomDetail;
        std::string bondDetail;
        std::string atomScalingFactor;
        std::string bondScalingFactor;
        std::string displayStyle;
        std::string atomDisplayStyle;
        std::string bondDisplayStyle;
        PersistentSettingsKeys( 
            const std::string& ad,
            const std::string& bd,
            const std::string& asf,
            const std::string& bsf,
            const std::string& ds,
            const std::string& ads,
            const std::string& bds ) : 
                                      atomDetail( ad ),
                                      bondDetail( bd ),
                                      atomScalingFactor( asf ),
                                      bondScalingFactor( bsf ),
                                      displayStyle( ds ),
                                      atomDisplayStyle( ads ),
                                      bondDisplayStyle( bds )
        {}
    };
    /// Constructor.
    /// @param mw reference to MainWindow instance; used
    /// to refresh the rendering widget when 'Apply' is clicked.
    /// @param mol reference to MolekelMolecule instance to be
    /// edited with this widget.
    /// @param parent parent widget.
    MoleculeRenderingStyleWidget( MainWindow* mw,
                                  MolekelMolecule* mol,
                                  const PersistentSettingsKeys& psk,
                                  QWidget* parent = 0 );

    /// Sets molecule to be edited.
    /// @param m reference to molecule instance.
    void SetMolecule( MolekelMolecule* m )
    {
        molecule_ = m;
        UpdateUI();
    }
    
private:

    /// Change molecule display style.
    QComboBox* displayStyle_;
    /// Change atom display style.
    QComboBox* atom_;
    /// Change bond display style.
    QComboBox* bond_;
    /// Change residue display style.
    QComboBox* residue_;
    /// Change atom complexity.
    QSlider* atomDetail_;
    /// Change bond complexity.
    QSlider* bondDetail_;
    /// Show/hide dipole moment.
    QComboBox* dipoleMoment_;
    /// Atom radius scaling in 'Ball and Stick/Wire' mode.
    QSlider* atomScaling_;
    /// Bond cylinder radius scaling.
    QSlider* bondScaling_;
    /// Molecule color: this label shows the current molecule
    /// color used for atoms and bonds when the display style
    /// is set to SoSphere (atoms) and SoCylinder (bonds).
    QLabel* colorLabel_;
    /// Real-time update check box: if checked changes are applied immediately.
    QCheckBox* rtUpdateCheckBox_;
    /// Reference to molecule.
    MolekelMolecule* molecule_;
    /// Reference to MainWindow instance.
    MainWindow* mainWin_;
    /// Utility variable to store the QColor returned from QColorDialog::getColor().
    QColor color_;
    /// Label displaying the filename from which atom color were read or "Default"
    /// if no file selected.
    QLabel* atomColorsLabel_;
    /// Set to true when UI is being updated to avoid executing slots.
    bool updatingGUI_;
    /// Margin.
    static const int MARGIN = 10;
    /// Spacing between UI controls.
    static const int SPACING = 12;
    /// Max complexity value for atoms and bonds.
    static const int MAX_COMPLEXITY = 100;
    /// Max scaling % for atoms in "Ball and Stick/Wire" mode;
    /// default is 20% i.e. displayed atom's radius = 0.2 * covalent radius.
    static const int MAX_ATOM_SCALING = 100;
    /// Max scaling % for bond cylinder radius;
    /// 100% == 1.
    static const int MAX_BOND_SCALING = 100;
    /// Number of marks on atom, bond and scaling sliders.
    static const int SLIDER_TICKS = 20;
    /// Vertical padding between atom and bond sliders.
    static const int VPADDING = 4;
    /// Keys in persistent settings map
    PersistentSettingsKeys psKeys_;
};


#endif /*MOLECULERENDERINGSTYLEWIDGET_H_*/
