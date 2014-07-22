#ifndef SPECTRUMWIDGET_H_
#define SPECTRUMWIDGET_H_
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
#include <vector>

// QT
#include <QWidget>
#include <QString>

#include "../utility/LorentzianInterpolator.h"

class MolekelMolecule;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotPicker;
class QwtPlotZoomer;
class QwtPlotGrid;
class QComboBox;
class QLayout;
class QSpinBox;
class QDoubleSpinBox;

/// Widget containing a 2D plot of interpolated radiation spectra.
/// Infrared intensities and Raman activities are supported.
/// Supported basis function types are Gaussian and Lorentzian.
/// In past versions it was possbile to use Lorentzian basis functions
/// with fixed aspect ratio; this feature is currently no more accessible
/// from the GUI but still supported by the class methods.
/// The aspect ratio for Lorentzian functions is computed as:
/// |Height-at-peak / Half-width-at-half-height|.
class SpectrumWidget : public QWidget
{
	Q_OBJECT
	
public:
	/// Constructor. All the needed data are copied into data members and no
	/// reference to the passed molecule is kept. The molecule can therefore
	/// safely be deleted after the constructor returns.
	SpectrumWidget( MolekelMolecule* m, const QString& outDataDirKey, QWidget* parent = 0 ); 

	/// Destructor.
	~SpectrumWidget();
	
private:
	
	/// IR or Raman activities plot.
	enum RadiationType { IR, RAMAN };
	
	/// Graph type: histogram, lorentzian or gaussian.
	/// HISTOGRAM type is not accessible anymore since a similar
	/// effect can be achieved by setting the half width value to 0.5.
	enum GraphType { HISTOGRAM, LORENTZIAN, GAUSSIAN };	
	
	/// Sets radiation type.
	void SetRadiationType( RadiationType rt ) { radiationType_ = rt; }
	
	/// Sets graph type.
	void SetGraphType( GraphType gt ) { graphType_ = gt; }
		
	/// Generate histogram data.
	void GenerateHistogramData( std::vector< double >& intensities,
								double& xmin, double& xmax,
								double& ymin, double& ymax );
	
	/// Generate interpolated intensities.
	void GenerateLorentzianData( std::vector< double >& intensities,
								 double& xmin, double& xmax,
								 double& ymin, double& ymax );
	
	/// Generate interpolated data.
	void GenerateInterpolatedData( std::vector< double >& intensities,
								   double& xmin, double& xmax,
								   double& ymin, double& ymax );
	
	
	/// Generates data, properly rescales axes and redraws plot.
	void GenerateData();
	
	/// Creates top toolbar and adds GUI controls.
	void AddTopToolbar( QLayout* );
	
	/// Creates bottom toolbar and adds GUI controls.
	void AddBottomToolbar( QLayout* );
	
	/// Lorentzian function.
	double Lorentzian( double x ) const
	{
		const double d = 1.0 + oneOverHalfWidthSquared_ * x * x;
		return 1. / d;
	}
	
	/// Gaussian function.
	double Gaussian( double x ) const
	{
		const double log2 = std::log( 2.0 );
		return std::exp( -log2 * oneOverHalfWidthSquared_ * x * x );
	}
	
private slots:

	/// Enables/disables zoom mode. When zoom mode is
	/// enabled user can zoom in with a rubber band and
	/// out by clicking on the right mouse button.
    void EnableZoomModeSlot( int checkBoxState );
    
    /// Exports to PS or PDF.
    void SaveSlot();
    
    /// Shows/hides grid.
    void ShowGridSlot( int checkBoxState );
    
    /// Sets steps.
    void SetStepsSlot( int steps );
    
    /// Sets graph type: lorentzian, histogram or gaussian.
    void GraphTypeSlot( int comboBoxIndex );
    
    /// Sets radiation type: infrared or Raman.
    void RadiationTypeSlot( int comboBoxIndex );
    
    /// Sets orientation of x axis to right-left.
    void SetXAxisRightLeftSlot( int checkBoxState );
    
    /// Sets orientation of y axis to top-down.
    void SetYAxisTopDownSlot( int checkBoxState );
    
    /// Sets aspect ration of Lorentzian basis functions.
    /// @param r aspect ratio: Height-at-peak / Half-width-at-half-height.
    void SetHWRatioSlot( double r );
    
    /// Sets the coordinate hw at which Lorentzian( hw ) = 0.5 * Lorentzian( x_max )
    /// where x_max is the coordinate of the global maximum for the Lorentzian function.   
    void SetHalfWidthSlot( double hw );
    
private:	
	/// Intensity interpolator.
	LorentzianInterpolator lorentzianInterpolator_;
	/// 2D Plot widget.
	QwtPlot *plot_;
	/// Plot.
	QwtPlotCurve *curve_;
	/// Interpolated frequencies.
	std::vector< double > xdata_;
	/// Interpolated intensities.
	std::vector< double > ydata_;
	/// Frequencies.
	std::vector< double > frequencies_;
	/// IR intensities.
	std::vector< double > irIntensities_;
	/// Raman activities.
	std::vector< double > ramanActivities_;
	/// Displays coordinates at cursor position.
	QwtPlotPicker* picker_;
	/// Zoom in/out.
	QwtPlotZoomer* zoomer_;
	/// Grid.
	QwtPlotGrid* grid_;
	/// Interpolation steps.
	int steps_;
	/// Lorentzian aspect ratio.
	double aspectRatio_;
	/// Molecule name recorded at construction time.
	QString moleculeName_;
	/// File format; needed to set the y-axis dimensional units.
	QString format_;
	/// Current graph type.
	GraphType graphType_;
	/// Current radiation type.
	RadiationType radiationType_;
	/// Orientation of x axis.
	bool xAxisOrientationRightLeft_;
	/// Orientation of y axis.
	bool yAxisOrientationTopDown_;
	/// Graph type combo box.
	QComboBox* graphTypeCBox_;
	/// Radiation type combo box.
	QComboBox* radiationTypeCBox_;
	/// Steps spin box.
	QSpinBox* stepsSpinBox_;
	/// Lorentzian basis function's aspect ratio, used only if molden style disabled.
	QDoubleSpinBox* aspectRatioSpinBox_;
	/// Lorentzian half width at half height, used only if molden style enabled.
	double halfWidth_;
	/// Lorentzian half width at half height editor.
	QDoubleSpinBox* halfWidthSpinBox_;
	/// Utility variable set each time half width changes.
	mutable double oneOverHalfWidthSquared_;
	/// QSettings key for default output directory.
	QString outDataDirKey_;
	
};

#endif /*SPECTRUMWIDGET_H_*/
