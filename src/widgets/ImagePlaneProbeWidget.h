#ifndef IMAGEPLANEPROBEWIDGET_H_
#define IMAGEPLANEPROBEWIDGET_H_
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

// STD
#include <string>

// QT
#include <QWidget>

#include "../MolekelMolecule.h"

// Forward declarations.
class MainWindow;
class QTableWidget;
class QTableWidgetItem;
class QDoubleSpinBox;
class QComboBox;
class QGroupBox;
class QLabel;

//
//--------------------------------
//               ________ _
// Visibility:  |__Show__|v| (REMOVED)
//
// [x]=Orbitals===================
// _______________________________
// Selected | Eigenvalue | ...
// -------------------------------
//   [v]    |  2.0       | ...
// -------------------------------
//   [ ]    |  1.0       | ...
// -------------------------------
//  ...     |   ...      | ...
// _______________________________
//          ______ __
// step    |______|^v| (spin box)
//
// ===============================
//
// [ ]=Density Matrix=============
//                 _______ _
// Density Matrix |_alpha_|v|
//
// ===============================
//
// [ ]=Grid Data==================
// ===============================
//
//--------------------------------
//

/// Adds a vtkImagePlaneWidget to scene: user can select one
/// density function to be analyzed with widget.
/// When the dialog opens a list of density data types is displayed:
/// - orbitals
/// - alpha/beta density matrices
/// - generic grid data read e.g. from a Gaussian cube file
/// User can select one and only one type and one and only one
/// density data set within the selected type.
/// Data are molecule-specific: only the data actually available for
/// specific molecule are displayed.
/// Orbital grid data have to be generated while other density data are
/// simply read from the molecule; it is therefore required that the user
/// specify also a bounding box and step used to generate the grid data;
/// currently only the step can be specified, the bounding box used is
/// the current molecule bounding box.
/// Molecule bounding box = <atoms' bounding box> + <generated surfaces' bounding box>.
/// @todo finish implementation for density matrix and generic grid data
/// @todo consider giving access to vtkWidget also from within surface generation
/// bounding dialogs.
/// @todo add option to change the vtkWidget rendering style: e.g. colormap.
class ImagePlaneProbeWidget : public QWidget
{
    Q_OBJECT
private slots:
    /// Called whenever a cell in the orbitals table is selected.
    /// Selects/deselects orbital and makes sure that one and only one
    /// orbital is selected at any given time.
    /// @todo add option to highlight orbital surface if orbital surface
    /// was generated.
    void TableItemChangedSlot( QTableWidgetItem* i );
    /// Called each time the step changes.
    void StepChangedSlot( double s  );
    /// Called when grid data checkbox toggled.
    void GridDataGroupToggledSlot( bool v );
    /// Called when electron density checkbox toggled.
    void ElectronDensityGroupToggledSlot( bool v );
    /// Called when Molecular Electrostatic Potential checkbox toggled.
    void MEPGroupToggledSlot( bool v );
    /// Called when spin density checkbox toggled.
    void SpinDensityGroupToggledSlot( bool v );
    /// Called when orbitals checkbox toggled.
    void OrbitalsGroupToggledSlot( bool v );
    /// Invoked whenever a different grid data set is selected.
    void GridDataChangedSlot( int );
signals:
    /// Emitted whenever an orbital is selected i.e. the corresponding checkbox is checked.
    void OrbitalSelected( int );
    /// Emitted whenever the step changes.
    void StepChanged( double );
    /// Emitted when grid data checkbox toggled.
    void GridDataToggled( bool );
    /// Emitted when electron density checkbox toggled.
    void ElectronDensityToggled( bool );
    /// Emitted when Molecular Electrostatic Potential checkbox toggled.
    void MEPToggled( bool );
    /// Emitted when spin density checkbox toggled.
    void SpinDensityToggled( bool );
    /// Emitted when orbitals checkbox toggled.
    void OrbitalsToggled( bool );
public:

    /// User selectable data types.
    typedef enum  { NONE,
                    ORBITAL,
                    GRID_DATA,
                    ELECTRON_DENSITY,
                    SPIN_DENSITY,
                    MEP } DataType;

    /// Returns selected data type.
    DataType GetSelectedDataType() const;
    /// Updates GUI reading data from MolekelMolecule instance.
    void UpdateGUI();
    /// Constructor; Calls CreateGUI().
    ImagePlaneProbeWidget( MolekelMolecule* mol,
                           QWidget* parent = 0 );
    /// Sets reference to molecule.
    void SetMolecule( MolekelMolecule* mol )
    {
        mol_ = mol;
        selectedOrbital_ = -1;
        UpdateGUI();
    }
    /// Updates the electron density value range label.
    void SetElectronDensityRange( double minVal, double maxVal );
    /// Updates the spin density value range label.
    void SetSpinDensityRange( double minVal, double maxVal );
    /// Updates the MEP value range label.
    void SetMEPRange( double minVal, double maxVal );
    /// Return currently selected grid name.
    std::string GetGridLabel() const;
private:
    /// Creates GUI.
    void CreateGUI();
    /// Unchecks all groups.
    void UnselectAll();
private:
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Table used to select the vibration mode to animate.
    QTableWidget* table_;
    /// Step size.
    QDoubleSpinBox* stepSizeSpinBox_;
    /// Orbitals group: if enabled the plane probe will probe the grid generated
    /// from orbital coefficients.
    QGroupBox* orbitalsGroup_;
    /// Grid data group: if enabled the plane probe will probe the grid data
    /// read from the molecule file (e.g. Gaussian cube grid data).
    QGroupBox* gridDataGroup_;
    /// Grids.
    QComboBox* gridsComboBox_;
    /// Electron density group: if enabled the plane probe will probe the electron
    /// density data generated from alpha and beta density matrices.
    QGroupBox* eldensGroup_;
    /// MEP group: if enabled the plane probe will probe the Molecular Electrostatic
    /// Potential data.
    QGroupBox* mepGroup_;
    /// Spin density group: if enabled the plane probe will probe the spin
    /// density data generated from alpha and beta density matrices.
    QGroupBox* spindensGroup_;
    /// Label displaying range (min, max) of grid data values.
    QLabel* gridDataRangeLabel_;
    /// Label displaying range (min, max) of electron density values.
    QLabel* eldensRangeLabel_;
    /// Label displaying range (min, max) of spin density values.
    QLabel* spindensRangeLabel_;
    /// Label displaying range (min, max) of MEP values.
    QLabel* mepRangeLabel_;
    /// Set to true when UI is being updated by Update/CreateGUI to
    /// avoid executing code in slot methods.
    bool updatingGUI_;
    /// Selected orbital.
    int selectedOrbital_;
};

#endif /*IMAGEPLANEPROBEWIDGET_H_*/
