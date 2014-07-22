#ifndef MOLECULEELDENSSURFACEWIDGET_H_
#define MOLECULEELDENSSURFACEWIDGET_H_
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

#include "../MolekelMolecule.h"

class MainWindow;
class QTableWidget;
class QTableWidgetItem;
class QLineEdit;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QColor;
class QLabel;

//--------------------------------
// _______________________________
// Eigenvalue | Occupancy |	Type
// -------------------------------
//  -1.22     |  2.0      |  A
// -------------------------------
//      1     |  1.0      |  A
// -------------------------------
//  ...       |   ...     |
// _______________________________
//

//--------------------------------

/// Allows user to render iso-surfaces from electron density
/// data (density matrix or orbitals).
class MoleculeElDensSurfaceWidget : public QWidget
{
    Q_OBJECT
private slots:
    /// Called whenever a cell in the orbitals table is selected.
    void TableCellClickedSlot( int row, int column );
    /// Computes steps from step size and bounding box.
    void ComputeSteps();
    /// Move center of bounding box used to compute iso surfaces.
    void MoveCenterSlot();
    /// Emits a RenderingStyleChanged signal.
    void RenderingStyleChangedSlot( int );
    /// Called when density matrix checkbox toggled.
    void DensityMatrixToggledSlot( bool );
    /// Called when MEP mapping checkbox toggled.
    void MEPMappingToggledSlot( bool );
    /// Called when density matrix transparency slider moved.
    void DMTransparencyChangedSlot( double );
    /// Called when negative transparency slider moved.
    void NegTransparencyChangedSlot( double );
    /// Called when nodal transparency slider moved.
    void NodalTransparencyChangedSlot( double );
    /// Called when positive transparency slider moved.
    void PosTransparencyChangedSlot( double );
    /// Called when density matrix color button released.
    void DensityMatrixColorSlot();
    /// Called when negative orbital color button released.
    void NegativeOrbitalColorSlot();
    /// Called when nodal surface color button released.
    void NodalSurfaceColorSlot();
    /// Called when positive orbital color button released.
    void PositiveOrbitalColorSlot();
    /// Called when 'Both signs' check box toggled. Emits
    /// a ValuesChanged signal.
    void BothSignsSlot( int );
    /// Called when 'Nodal surface' check box toggled. Emits
    /// a ValuesChanged signal.
    void NodalSurfaceSlot( int );
    
signals:
    /// Emitted whenever an orbital in the cell is selected.
    void OrbitalSelected( int orbitalIndex );
    /// Emitted whenever a parameter changes its value.
    void ValuesChanged();
    /// Emitted whenever the rendering style changes.
    void RenderingStyleChanged( MolekelMolecule::RenderingStyle rs );
    /// Emitted when density matrix checkbox toggled.
    void DensityMatrixToggled( bool );
    /// Emitted when MEP mapping checkbox toggled.
    void MEPMappingToggled( bool );
    /// Emitted when density matrix transparency changed.
    void DMTransparencyChanged( double );
    /// Emitted when negative transparency changed.
    void NegTransparencyChanged( double );
    /// Emitted when nodal transparency changed.
    void NodalTransparencyChanged( double );
    /// Emitted when posiitve transparency changed.
    void PosTransparencyChanged( double );
    /// Emitted when color for density matrix surface changed.
    void DensityMatrixColorChanged( const QColor& );
    /// Emitted when color for negative orbital changed.
    void NegativeOrbitalColorChanged( const QColor& );
    /// Emitted when color for nodal surface changed.
    void NodalSurfaceColorChanged( const QColor& );
    /// Emitted when color for positive orbital changed.
    void PositiveOrbitalColorChanged( const QColor& );
    
public:
    /// Updates GUI reading data from MolekelMolecule instance.
    void UpdateGUI( bool updateTable = true, bool updateBBox = true );
    /// Constructor. Calls CreateGUI();
    MoleculeElDensSurfaceWidget( QWidget* parent = 0 );
    /// Sets reference to molecule.
    void SetMolecule( MolekelMolecule* mol ) { mol_ = mol; UpdateGUI(); }
    /// Validates and returns data in widget.
    bool GetData( double& value,
                  double bboxSize[ 3 ],
                  int steps[ 3 ],
                  bool& bothSigns,
                  bool& nodalSurface,
                  MolekelMolecule::RenderingStyle& rs,
                  double& mdTransparency,
                  double& negTransparency,
                  double& nodalTransparency,
                  double& posTransparency ) const;
    /// Return rendering style.
    MolekelMolecule::RenderingStyle GetRenderingStyle() const;
    /// Returns steps.
    bool GetSteps( int steps[ 3 ] ) const;
    /// Returns bounding box size.
    bool GetBBoxSize( double bboxSize[ 3 ] ) const;
    /// Sets current rendering style.
    void SetRenderingStyle( MolekelMolecule::RenderingStyle rs );
    /// Sets current transparency for density matrix.
    void SetDMTransparency( double );
    /// Sets current transparency for negative orbitals.
    void SetNegTransparency( double );
    /// Sets current transparency for nodal surface.
    void SetNodalTransparency( double );
    /// Sets current transparency for positive orbitals.
    void SetPosTransparency( double );
    /// Sets current color for density matrix surface.
    void SetDensityMatrixColor( const QColor& c ) { densityMatrixColor_ = c; }
    /// Sets current color for negative orbital.
    void SetNegativeOrbitalColor( const QColor& c ) { negativeOrbitalColor_ = c; } 
    /// Sets current color for nodal surface.
    void SetNodalSurfaceColor( const QColor& c ) { nodalSurfaceColor_ = c; }
    /// Sets current color for positive orbital.
    void SetPositiveOrbitalColor( const QColor& c ) { positiveOrbitalColor_ = c; }
    /// Returns current color for density matrix surface.
    QColor GetDensityMatrixColor() const  { return densityMatrixColor_ ; }
    /// Retunrs current color for negative orbital.
    QColor GetNegativeOrbitalColor() const { return negativeOrbitalColor_; } 
    /// Returns current color for nodal surface.
    QColor GetNodalSurfaceColor() const { return nodalSurfaceColor_; }
    /// Returns current color for positive orbital.
    QColor GetPositiveOrbitalColor() const { return positiveOrbitalColor_; }   
    
private:
    /// Creates GUI.
    void CreateGUI();
    /// Validate data.
    bool ValidateData() const;
    
private:
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Table used to select the vibration mode to animate.
    QTableWidget* table_;
    /// Isosurface value.
    QDoubleSpinBox* valueSpinBox_;
    /// Bounding box size along x axis.
    QDoubleSpinBox* dxSpinBox_;
    /// Bounding box size along y axis.
    QDoubleSpinBox* dySpinBox_;
    /// Bounding box size along z axis.
    QDoubleSpinBox* dzSpinBox_;
    /// Bounding box x center coordinate.
    QDoubleSpinBox* xSpinBox_;
    /// Bounding box y center coordinate.
    QDoubleSpinBox* ySpinBox_;
    /// Bounding box z center coordinate.
    QDoubleSpinBox* zSpinBox_;
    /// Number of steps along x axis.
    QLabel* nxLineEdit_;
    /// Number of steps along y axis.
    QLabel* nyLineEdit_;
    /// Number of steps along z axis.
    QLabel* nzLineEdit_;
    /// Step size.
    QDoubleSpinBox* stepSizeSpinBox_;
    /// Both signs.
    QCheckBox* signCheckBox_;
    /// Nodal surface.
    QCheckBox* nodalSurfaceCheckBox_;
    /// Rendering style.
    QComboBox* renderingStyleComboBox_;
    /// Density matrix checkbox.
    QCheckBox* densityMatrixCheckBox_;
    /// Density matrix transparency slider.
    QDoubleSpinBox* dmTransparencyWidget_;
    /// Negative orbital transparency slider.
    QDoubleSpinBox* negTransparencyWidget_;
    /// Positive orbital transparency slider.
    QDoubleSpinBox* posTransparencyWidget_;
    /// Nodal surface transparency slider.
    QDoubleSpinBox* nodTransparencyWidget_;
    /// Set to true when UI is being updated Update/CreateGUI to
    /// avoid executing code in slot methods.
    bool updatingGUI_;
    /// Current color for density matrix surface.
    QColor densityMatrixColor_;
    /// Current color for negative molecular orbital.
    QColor negativeOrbitalColor_;
    /// Current color for nodal surface.
    QColor nodalSurfaceColor_;
    /// Current color for positive molecular orbital.
    QColor positiveOrbitalColor_;
    
    //@{ Persistent preferences
    static const QString POSITIVE_ORBITAL_COLOR_KEY;
    static const QString NEGATIVE_ORBITAL_COLOR_KEY;
    static const QString NODAL_SURFACE_COLOR_KEY;
    static const QString DENSITY_MATRIX_COLOR_KEY;
    static const QString NEGATIVE_TRANSPARENCY_KEY;
    static const QString POSITIVE_TRANSPARENCY_KEY;
    static const QString NODAL_TRANSPARENCY_KEY;
    static const QString DENSITY_MATRIX_TRANSPARENCY_KEY;
    static const QString ISOVALUE_KEY;
    static const QString BOTH_SIGNS_KEY;
    static const QString NODAL_SURFACE_KEY;
    static const QString STEP_SIZE_KEY;
#ifdef PERSISTENT_ISOBBOX
    static const QString BBOX_DX_KEY;
    static const QString BBOX_DY_KEY;
    static const QString BBOX_DZ_KEY;
#endif
    //@}
    
};

#endif /*MOLECULEELDENSSURFACEWIDGET_H_*/
