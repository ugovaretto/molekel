#ifndef COMPUTEELDENSSURFACEDIALOG_H_
#define COMPUTEELDENSSURFACEDIALOG_H_
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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QCheckBox>
#include <QLabel>
#include <QColor>

#include "../MainWindow.h"
#include "../MolekelMolecule.h"
#include "../widgets/MoleculeElDensSurfaceWidget.h"

// Forward declarations
class vtkLookupTable;


/// Class used to generate orbital surfaces.
class ComputeElDensSurfaceDialog : public QDialog
{
    Q_OBJECT

private:
	/// Converts from QColor to double[3] array.
	void QColorToRgb( const QColor& c, double color[ 3 ] )
	{
		color[ 0 ] = c.redF();
		color[ 1 ] = c.greenF();
		color[ 2 ] = c.blueF();
	}
	/// Converts from double[ 3 ] to QColor.
	QColor RgbToQColor( const double color[ 3 ] )
	{
		QColor c;
		c.setRgbF( color[ 0 ], color[ 1 ], color[ 2 ] );
		return c;
	}
    
private slots:

    /// Called when density matrix checkbox toggled.
    void DensityMatrixToggledSlot( bool v )
    {
        useDensityMatrix_ = v;
        if( mol_->HasElectronDensitySurface() ) removeButton_->setEnabled( true );
        else removeButton_->setEnabled( false );
        if( mol_->CanComputeElectronDensity() ) generateButton_->setEnabled( true );
        else generateButton_->setEnabled( false );

        if( v && mol_->HasElectronDensitySurface() )
        {
        	ow_->SetRenderingStyle( mol_->GetElDensSurfaceRenderingStyle() );
        	ow_->SetDMTransparency( 1.0 - mol_->GetElDensSurfaceOpacity() );
        	double color[ 3 ];
        	mol_->GetElDensSurfaceColor( color );
        	QColor c = RgbToQColor( color );
        	if( c.isValid() ) ow_->SetDensityMatrixColor( c );
        }     
    }

    /// Called when MEP mapping checkbox toggled.
    void MEPMappingToggledSlot( bool v )
    {
        mapMEP_ = v;
        // if density matrix selected
        if( useDensityMatrix_ && mol_->HasElectronDensitySurface()
            && realTimeCheckBox_->checkState() == Qt::Checked )
        {
            int steps[ 3 ];
            double bboxSize[ 3 ];
            if( !ow_->GetSteps( steps ) ) return;
            if( !ow_->GetBBoxSize( bboxSize ) ) return;
            MapMEPOnElDensSurface( steps, bboxSize );
            mw_->SetMEPScalarBarLUT( mol_->GetElectronDensitySurfaceLUT() );
            mw_->Refresh();
        }
        else if( selectedOrbital_ >= 0 &&
                mol_->HasOrbitalSurface( selectedOrbital_ ) &&
                realTimeCheckBox_->checkState() == Qt::Checked )
        {
            int steps[ 3 ];
            double bboxSize[ 3 ];
            if( !ow_->GetSteps( steps ) ) return;
            if( !ow_->GetBBoxSize( bboxSize ) ) return;
            MapMEPOnOrbitalSurface( steps, bboxSize );
            mw_->SetMEPScalarBarLUT( mol_->GetOrbitalSurfaceLUT( selectedOrbital_ ) );
            mw_->Refresh();
        }

    }
   
    /// Stop processing.
    void StopProcessing()
    {
    	mol_->StopElectronDensityDataGeneration();
    	mol_->StopMEPDataGeneration();
    	mol_->StopMOGridDataGeneration();    	
    }
    
    /// Called whenever an orbital has been selected.
    void OrbitalSelectedSlot( int orbitalIndex )
    {
    	selectedOrbital_ = orbitalIndex;
        if( selectedOrbital_ < 0 )
        {
            removeButton_->setEnabled( false );
            generateButton_->setEnabled( false );
            return;
        }

        if( mol_->HasOrbitalSurface( selectedOrbital_ ) )
        {
            removeButton_->setEnabled( true );
            generateButton_->setEnabled( true );
            ow_->SetRenderingStyle( mol_->GetOrbitalRenderingStyle( selectedOrbital_ ) );
            ow_->SetNegTransparency( 1.0 - mol_->GetOrbitalOpacity( selectedOrbital_, MolekelMolecule::ORBITAL_MINUS ) );
            ow_->SetNodalTransparency( 1.0 - mol_->GetOrbitalOpacity( selectedOrbital_, MolekelMolecule::ORBITAL_NODAL ) );
            ow_->SetPosTransparency( 1.0 - mol_->GetOrbitalOpacity( selectedOrbital_, MolekelMolecule::ORBITAL_PLUS ) );
            
            double color[ 3 ];
            mol_->GetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_MINUS, color );
            QColor c = RgbToQColor( color );
            if( c.isValid() ) ow_->SetNegativeOrbitalColor( RgbToQColor( color ) );
            mol_->GetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_NODAL, color );
            c = RgbToQColor( color );
            if( c.isValid() ) ow_->SetNodalSurfaceColor( c );
            mol_->GetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_PLUS, color );
            c = RgbToQColor( color );
            if( c.isValid() ) ow_->SetPositiveOrbitalColor( c );             
        }
        else
        {
            removeButton_->setEnabled( false );
            generateButton_->setEnabled( true );
        }
    }
    /// Called when remove button pressed.
    void RemoveSlot()
    {
    	StopProcessing();
        if( !useDensityMatrix_ )
        {
            if( selectedOrbital_ < 0 ) return;
            if( mol_->HasOrbitalSurface( selectedOrbital_ ) )
            {
                mol_->RemoveOrbitalSurface( selectedOrbital_ );
                removeButton_->setEnabled( false );
            }
            ow_->UpdateGUI( true, false );
        }
        else
        {
            if( mol_->HasElectronDensitySurface() )
            {
                mol_->RemoveElectronDensitySurface();
                removeButton_->setEnabled( false );
            }
        }
        mw_->Refresh();
    }

    /// Called when generate button pressed.
    void GenerateSlot()
    {
    	StopProcessing();
        ProgressCallback pcb = MainWindow::ProgressCallback;
        if( !useDensityMatrix_ )
        {
            if( selectedOrbital_ < 0 ) return;
            if( mol_->HasOrbitalSurface( selectedOrbital_ ) )
            {
                mol_->RemoveOrbitalSurface( selectedOrbital_ );
            }
            double value, bboxSize[ 3 ];
            int steps[ 3 ];
            bool bothSigns;
            bool nodalSurface;
            MolekelMolecule::RenderingStyle rs;
            double dmTr = 0.; // density matrix transparency
            double nTr  = 0.; // negative transparency
            double noTr = 0.; // nodal transparency
            double pTr  = 0.; // positive transparency
            if( !ow_->GetData( value, bboxSize, steps, bothSigns, nodalSurface,
            				   rs, dmTr, nTr, noTr, pTr ) ) return;
            if( mol_->AddOrbitalSurface( selectedOrbital_,
                                         bboxSize,
                                         steps,
                                         value,
                                         bothSigns,
                                         nodalSurface,
                                         pcb,
                                         mw_ ) )
            {
                mol_->SetOrbitalRenderingStyle( selectedOrbital_, rs );
                mol_->SetOrbitalOpacity( selectedOrbital_, 1.0 - nTr, MolekelMolecule::ORBITAL_MINUS );
                mol_->SetOrbitalOpacity( selectedOrbital_, 1.0 - noTr, MolekelMolecule::ORBITAL_NODAL );
                mol_->SetOrbitalOpacity( selectedOrbital_, 1.0 - pTr, MolekelMolecule::ORBITAL_PLUS );
                if( mapMEP_ )
                {
                    MapMEPOnOrbitalSurface( steps, bboxSize );
                    mw_->SetMEPScalarBarLUT( mol_->GetOrbitalSurfaceLUT( selectedOrbital_ ) );
                }
                removeButton_->setEnabled( true );
                double color[ 3 ];
                QColor c;
                c = ow_->GetNegativeOrbitalColor();
                QColorToRgb( c, color );
                mol_->SetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_MINUS, color );
                c = ow_->GetNodalSurfaceColor();
                QColorToRgb( c, color );
                mol_->SetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_NODAL, color );
                c = ow_->GetPositiveOrbitalColor(); 
                QColorToRgb( c, color );
                mol_->SetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_PLUS, color ); 
            }
            else
            {
                removeButton_->setEnabled( false );
            }
            ow_->UpdateGUI( true, false );            
            
        }
        else
        {
            double value, bboxSize[ 3 ];
            int steps[ 3 ];
            bool bothSigns;
            bool nodalSurface;
            MolekelMolecule::RenderingStyle rs;
            double dmTr = 0.; // density matrix transparency
            double nTr  = 0.; // negative transparency
            double noTr = 0.; // nodal transparency
            double pTr  = 0.; // positive transparency
            if( !ow_->GetData( value, bboxSize, steps, bothSigns, nodalSurface, 
            				   rs, dmTr, nTr, noTr, pTr ) ) return;
            if( mol_->AddElectronDensitySurface( bboxSize,
                                                 steps,
                                                 value,
                                                 pcb,
                                                 mw_ ) )
            {
                mol_->SetElDensSurfaceRenderingStyle( rs );
                mol_->SetElDensSurfaceOpacity( 1.0 - dmTr );
                if( mapMEP_ )
                {
                    MapMEPOnElDensSurface( steps, bboxSize );
                    mw_->SetMEPScalarBarLUT( mol_->GetElectronDensitySurfaceLUT() );
                }
                removeButton_->setEnabled( true );
                double color[ 3 ];
                const QColor c = ow_->GetDensityMatrixColor();
                QColorToRgb( c, color );
                mol_->SetElDensSurfaceColor( color );
            }
            else
            {
                removeButton_->setEnabled( false );
            }           
        }

        mw_->Refresh();

    }
    /// Called whenever a parameter value changes.
    void ValuesChangedSlot()
    {
        if( realTimeCheckBox_->checkState() == Qt::Checked ) GenerateSlot();
        else mw_->Refresh();
    }
    /// Called when rendering style changes.
    void RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle rs )
    {
        if( useDensityMatrix_ )
        {
            if( !mol_->HasElectronDensitySurface() ) return;
            mol_->SetElDensSurfaceRenderingStyle( rs );
        }
        else
        {
            if( selectedOrbital_ < 0 ) return;
            if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
            mol_->SetOrbitalRenderingStyle( selectedOrbital_, rs );
        }
        mw_->Refresh();
    }
    
    /// Called when D.M. transparency changes.
    void DMTransparencyChangedSlot( double t )
    {
    	if( !mol_->HasElectronDensitySurface() ) return;
    	mol_->SetElDensSurfaceOpacity( 1.0 - t );
        mw_->Refresh();
    }
   
    /// Called when negative orbital's transparency changes.
    void NegTransparencyChangedSlot( double t )
    {
    	if( selectedOrbital_ < 0 ) return;
    	if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
    	mol_->SetOrbitalOpacity( selectedOrbital_, 1.0 - t, MolekelMolecule::ORBITAL_MINUS );
    	mw_->Refresh();
    }
    
    /// Called when nodal surface's transparency changes.
    void NodalTransparencyChangedSlot( double t )
    {
       	if( selectedOrbital_ < 0 ) return;
       	if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
       	mol_->SetOrbitalOpacity( selectedOrbital_, 1.0 - t, MolekelMolecule::ORBITAL_NODAL );
    	mw_->Refresh();
    }

    /// Called when positive orbital's transparency changes.
    void PosTransparencyChangedSlot( double t )
    {
     	if( selectedOrbital_ < 0 ) return;
      	if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
       	mol_->SetOrbitalOpacity( selectedOrbital_, 1.0 - t, MolekelMolecule::ORBITAL_PLUS );
    	mw_->Refresh();
    }

    /// Called when density matrix surface color changes.
    void DensityMatrixColorChangedSlot( const QColor& c )
    {
    	if( !mol_->HasElectronDensitySurface() ) return;
       	double color[3];
       	QColorToRgb( c, color );
    	mol_->SetElDensSurfaceColor( color );
        mw_->Refresh();
     }
    
    /// Called when negative orbital color changes.
    void NegativeOrbitalColorChangedSlot( const QColor& c )
    {
    	if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
      	double color[3];
      	QColorToRgb( c, color );
      	mol_->SetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_MINUS, color );
      	mw_->Refresh();
    }
    
    /// Called when nodal surface color changes.
    void NodalSurfaceColorChangedSlot( const QColor& c )
    {
      	if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
       	double color[3];
       	QColorToRgb( c, color );
       	mol_->SetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_NODAL, color );
       	mw_->Refresh();
    }
    
    /// Called when positive orbital color changes.
    void PositiveOrbitalColorChangedSlot( const QColor& c )
    {
      	if( !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
       	double color[3];
       	QColorToRgb( c, color );
       	mol_->SetOrbitalColor( selectedOrbital_, MolekelMolecule::ORBITAL_PLUS, color );
       	mw_->Refresh();
    }
    
    /// Called when ok button pressed.
    void AcceptSlot()
    {
    	StopProcessing();
        selectedOrbital_ = -1;
        removeButton_->setEnabled( false );
        generateButton_->setEnabled( false );
        mol_->SetIsoBBoxVisible( false );
        accept();
    }

public:
    /// Constructor.
    ComputeElDensSurfaceDialog( MolekelMolecule* mol,
                          MainWindow* mw,
                          QWidget* parent = 0,
                          vtkLookupTable* mepLUT = 0 )
                          : QDialog( parent ),
                              ow_( 0 ), mw_( mw ), mol_( mol ),
                            generateButton_( 0 ), removeButton_( 0 ), selectedOrbital_( -1 ),
                            mapMEP_( false ), useDensityMatrix_( false ), mepLUT_( mepLUT )
    {
        assert( mol && "NULL Molecule" );
        assert( mw &&  "NULL Main Window" );
        // Orbital widget
        ow_ = new MoleculeElDensSurfaceWidget;
        ow_->SetMolecule( mol );
        connect( ow_, SIGNAL( OrbitalSelected( int ) ),
                 this, SLOT( OrbitalSelectedSlot( int ) ) );
        QVBoxLayout* mainLayout = new QVBoxLayout;
        mainLayout->addWidget( ow_ );
        // Separator
        QFrame* line = new QFrame;
        line->setFrameShape( QFrame::HLine );
        mainLayout->addWidget( line );
        // Real-time checkbox.
        QHBoxLayout* realTimeLayout = new QHBoxLayout;
        realTimeLayout->addWidget( new QLabel( tr( "Real-time Update" ) ) );
        realTimeCheckBox_ = new QCheckBox;
        realTimeLayout->addWidget( realTimeCheckBox_ );
        mainLayout->addItem( realTimeLayout );
        // Separator
        mainLayout->addWidget( line );
        // Generate, Remove, Close buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        generateButton_ = new QPushButton( tr( "Generate" ) );
        generateButton_->setEnabled( false );
        connect( generateButton_, SIGNAL( released() ), this, SLOT( GenerateSlot() ) );
        buttonLayout->addWidget( generateButton_ );
        removeButton_ = new QPushButton( tr( "Remove" ) );
        removeButton_->setEnabled( false );
        connect( removeButton_, SIGNAL( released() ), this, SLOT( RemoveSlot() ) );
        buttonLayout->addWidget( removeButton_ );
        QPushButton* okButton = new QPushButton( tr( "Close" ) );
        connect( okButton, SIGNAL( released() ), this, SLOT( AcceptSlot() ) );
        buttonLayout->addWidget( okButton );
        mainLayout->addItem( buttonLayout );
        this->setLayout( mainLayout );
        connect( ow_, SIGNAL( ValuesChanged() ), this, SLOT( ValuesChangedSlot() ) );
        connect( ow_, SIGNAL( RenderingStyleChanged( MolekelMolecule::RenderingStyle ) ),
                 this, SLOT( RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle ) ) );
        connect( ow_, SIGNAL( DensityMatrixToggled( bool ) ),
                 this, SLOT( DensityMatrixToggledSlot( bool ) ) );
        connect( ow_, SIGNAL( MEPMappingToggled( bool ) ),
                 this, SLOT( MEPMappingToggledSlot( bool ) ) );
        connect( ow_, SIGNAL( DMTransparencyChanged( double ) ),
                 this, SLOT( DMTransparencyChangedSlot( double ) ) );
        connect( ow_, SIGNAL( NegTransparencyChanged( double ) ),
                        this, SLOT( NegTransparencyChangedSlot( double ) ) );
        connect( ow_, SIGNAL( NodalTransparencyChanged( double ) ),
                        this, SLOT( NodalTransparencyChangedSlot( double ) ) );
        connect( ow_, SIGNAL( PosTransparencyChanged( double ) ),
                        this, SLOT( PosTransparencyChangedSlot( double ) ) );
        connect( ow_, SIGNAL( DensityMatrixColorChanged( const QColor& ) ),
                 this, SLOT( DensityMatrixColorChangedSlot( const QColor& ) ) );
        connect( ow_, SIGNAL( NegativeOrbitalColorChanged( const QColor& ) ),
                 this, SLOT( NegativeOrbitalColorChangedSlot( const QColor& ) ) );
        connect( ow_, SIGNAL( NodalSurfaceColorChanged( const QColor& ) ),
                 this, SLOT( NodalSurfaceColorChangedSlot( const QColor& ) ) );
        connect( ow_, SIGNAL( PositiveOrbitalColorChanged( const QColor& ) ),
                 this, SLOT( PositiveOrbitalColorChangedSlot( const QColor& ) ) );        
        if( !mol_->CanComputeElectronDensity() && mol_->GetNumberOfOrbitals() == 0 )
        {
            ow_->setEnabled( false );
            realTimeCheckBox_->setEnabled( false );
        }
        mol->SetIsoBBoxVisible( true );
    }
private:

    /// Maps MEP onto electron density surface computed from density matrix.
    void MapMEPOnElDensSurface( int steps[ 3 ], double bboxSize[ 3 ] )
    {
        if( !useDensityMatrix_ || !mol_->HasElectronDensitySurface() ) return;
        mol_->MapMEPOnElDensSurface( bboxSize, steps, mepLUT_ );
    }

    /// Maps MEP onto orbital surface.
    void MapMEPOnOrbitalSurface( int steps[ 3 ], double bboxSize[ 3 ] )
    {
        if( selectedOrbital_ <= 0 || useDensityMatrix_ ||
            !mol_->HasOrbitalSurface( selectedOrbital_ ) ) return;
        mol_->MapMEPOnOrbitalSurface( selectedOrbital_,  bboxSize, steps, mepLUT_ );
    }

    /// Electron density surface widget instance contained in this window.
    MoleculeElDensSurfaceWidget* ow_;
    /// Reference to main window; used to invoke a repaint action.
    MainWindow* mw_;
    /// Reference to molecule whose information is displayed in this window.
    MolekelMolecule* mol_;
    /// Generate orbital surface.
    QPushButton* generateButton_;
    /// Remove orbital surface.
    QPushButton* removeButton_;
    /// Real-time check box: if selected changes are immediately applied and
    /// scene re-rendered.
    QCheckBox* realTimeCheckBox_;
    /// Set to true when density matrix is used to compute electron density.
    bool useDensityMatrix_;
    /// Currently selected orbital.
    int selectedOrbital_;
    /// If true maps MEP onto selected surface.
    /// @todo add option to remove mep mapping (vtkMapper::SetScalarVisibilityOff())
    /// currently the surface has to be re-geneated to remove scalar mapping.
    bool mapMEP_;
    /// Lookup table used to map MEP values to colors.
    vtkLookupTable* mepLUT_;
};


#endif /*COMPUTEELDENSSURFACEDIALOG_H_*/
