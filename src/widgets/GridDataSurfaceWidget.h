#ifndef GRIDDATASURFACEWIDGET_H_
#define GRIDDATASURFACEWIDGET_H_
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


class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;
class QDoubleSpinBox;
class QSpinBox;


//-------------------------------------
//  Grid Data -----------------------
// |                                 |
// | Step Size: x: 0.2 y: 0.2 z: 0.2 |
// |                                 |
// | Steps: x: 30 y: 30 z: 30        |
// |                                 |
// | Value Range: (-0.1, +0.1)       |
// |                                 |
//  ---------------------------------
//        ____ __
// Value |____|^v|
//                  ____ __
// Step Multiplier |____|^v|
//                  ___________ _
// Rendering Style |_Select..._|v|
//
// Smoothing -----------------------
// |              ____ __           |
// | Iterations: |____|^v|          |
// |              ____ __           |
// | Relaxation: |____|^v|          |
// |                                |
//  --------------------------------
//-------------------------------------

///Allows user to generate an isosurface from the OBGridData
///object (optionally) contained inside an OBMol molecule instance.
///It is possible to smooth the generated iso-surface with
///Laplacian smoothing by specifying the number of iterations
///and relaxation factor.
/// @see OBGridData
class GridDataSurfaceWidget : public QWidget
{
    Q_OBJECT
private slots:
    /// Emits a RenderingStyleChanged signal.
    void RenderingStyleChangedSlot( int );
    /// Emits a ValueChanged signal.
    void ValueChangedSlot( double  );
    /// Emits a StepMultipllierChanged signal.
    void StepMultiplierChangedSlot( int );
    /// Emits a SurfaceChanged signal.
    void SurfaceChangedSlot( int );
    /// Invoked when color button pressed.
    void SetColorSlot();
signals:
    /// Emitted whenever the isoruface value changes.
    void ValueChanged( double value );
    /// Emitted whenever the step multiplier value changes.
    void StepMultiplierChanged( int sm );
    /// Emitted whenever the rendering style changes.
    void RenderingStyleChanged( MolekelMolecule::RenderingStyle rs );
    /// Emitted whenever the selected surface is selected.
    void SurfaceChanged( const std::string& );
public:
    /// Constructor; creates GUI.
    GridDataSurfaceWidget( QWidget* parent = 0 );
    /// Updates GUI reading data from MolekelMolecule instance.
    void UpdateGUI();
    /// Sets reference to molecule.
    void SetMolecule( MolekelMolecule* mol ) { mol_ = mol; UpdateGUI(); }
    /// Returns isosurface value;
    double GetValue() const;
    /// Returns step multiplier.
    int GetStepMultiplier() const;
    /// Returns rendering style.
    MolekelMolecule::RenderingStyle GetRenderingStyle() const;
    /// Sets rendering style
    void SetRenderingStyle( MolekelMolecule::RenderingStyle rs );
    /// Returns the number of iterations used for Laplacian smoothing.
    int GetIterations() const;
    /// Returns the relaxation factor used for Laplacian smoothing.
    double GetRelaxationFactor() const;
    /// Returns the label of the currently selected surface.
    std::string GetSurfaceLabel() const;
    /// Returns current color as a QColor object.
    QColor GetColor() const;
    /// Returns current color as r, g, b floats.
    void GetColor( float& r, float& g, float& b ) const;
private:

    /// Set color from QColor
    void SetColor( const QColor& );
    /// Set color from r, g, b floats/
    void SetColor( float r, float g, float b );
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Isosurface value.
    QDoubleSpinBox* valueSpinBox_;
    /// Step multiplier value.
    QSpinBox* stepMulSpinBox_;
    /// Rendering style.
    QComboBox* renderingStyleComboBox_;
    /// Iterations.
    QSpinBox* iterationsSpinBox_;
    /// Relaxation factor.
    QDoubleSpinBox* relFactorSpinBox_;
    /// Utility variable to store the QColor returned from QColorDialog::getColor().
    QColor color_;
    /// Current surface color.
    QLabel* colorLabel_;
    // @{ Grid data info
    QLabel* stepSizeLabel_;
    QLabel* stepNumLabel_;
    QLabel* minMaxLabel_;
    // @}
    /// List of available grids.
    QComboBox* labelsComboBox_;
    /// Set to true when UI is being updated Update/CreateGUI to
    /// avoid executing code in slot methods.
    bool updatingGUI_;
};

#endif /*GRIDDATASURFACEWIDGET_H_*/
