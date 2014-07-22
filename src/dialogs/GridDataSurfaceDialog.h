#ifndef GRIDDATASURFACEDIALOG_H_
#define GRIDDATASURFACEDIALOG_H_
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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QCheckBox>
#include <QLabel>

#include "../MainWindow.h"
#include "../MolekelMolecule.h"
#include "../widgets/GridDataSurfaceWidget.h"

/// Container fro GridDataSurfaceWidget: generates isosurfaces
/// from grid data.
class GridDataSurfaceDialog : public QDialog
{
    Q_OBJECT

private slots:

    /// Called whenever the isosurface value changes.
    void ValueChangedSlot( double v )
    {
        if( realTimeCheckBox_->checkState() == Qt::Checked ) GenerateSlot();
    }

    /// Called whenever the step multiplier changes.
    void StepMultiplierChangedSlot( int s )
    {
        if( realTimeCheckBox_->checkState() == Qt::Checked ) GenerateSlot();
    }

    /// Called whenever the rendering style changes.
    void RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle rs )
    {
        mol_->SetGridDataSurfaceRenderingStyle( rs, sw_->GetSurfaceLabel() );
        mw_->Refresh();
    }

    /// Called when 'Close' button released.
    void AcceptSlot()
    {
        accept();
    }

    /// Called when 'Generate' button released: invokes MolekelMolecule methods
    /// to generate isosurface.
    void GenerateSlot()
    {
        ProgressCallback pcb = MainWindow::ProgressCallback;
        const std::string sl = sw_->GetSurfaceLabel();
        if( mol_->GenerateGridDataSurface( sl,
                                           sw_->GetValue(),
                                           sw_->GetStepMultiplier(),
                                           sw_->GetIterations(),
                                           sw_->GetRelaxationFactor(),
                                           pcb, mw_ ) )
        {
            mol_->SetGridDataSurfaceRenderingStyle( sw_->GetRenderingStyle(), sl );
            float r, g, b;
            sw_->GetColor( r, g, b );
            mol_->SetGridDataSurfaceColor( r, g, b, sl );
            mw_->SetMEPScalarBarLUT( mol_->GetGridDataSurfaceLUT( sl ) );
        }

        if( mol_->HasGridDataSurface( sl ) )
        {
            removeButton_->setEnabled( true );
            sw_->SetRenderingStyle( mol_->GetGridDataSurfaceRenderingStyle( sl ) ) ;
        }
        mw_->Refresh();
    }

    /// Called when 'Remove' button released: invokes MolekelMolecule methods
    /// to remove the generated isosurface from scene.
    void RemoveSlot()
    {
        mol_->RemoveGridDataSurface( sw_->GetSurfaceLabel() );
        removeButton_->setEnabled( false );
        mw_->Refresh();
    }

    /// Called when a different surface is selected.
    void SurfaceChangedSlot( const std::string& label )
    {
        removeButton_->setEnabled( mol_->HasGridDataSurface( label ) );
    }

public:
    /// Constructor. Creates UI.
    /// @param mol reference to molecule from which grid data are read.
    /// @param mw reference to main window.
    /// @param parent parent widget.
    GridDataSurfaceDialog( MolekelMolecule* mol,
                           MainWindow* mw,
                           QWidget* parent = 0 )
                           : QDialog( parent ), mol_( mol ), mw_( mw ),
                                sw_( 0 ), removeButton_( 0 ), generateButton_( 0 )
    {
        assert( mol && "NULL Molecule" );
        assert( mw && "NULL Main Window" );

        sw_ = new GridDataSurfaceWidget;
        sw_->SetMolecule( mol_ );
        connect( sw_, SIGNAL( ValueChanged( double ) ), this, SLOT( ValueChangedSlot( double ) ) );
        connect( sw_, SIGNAL( RenderingStyleChanged( MolekelMolecule::RenderingStyle ) ),
                 this, SLOT( RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle ) ) );
        connect( sw_, SIGNAL( StepMultiplierChanged( int ) ),
                 this, SLOT( StepMultiplierChangedSlot( int ) ) );
        connect( sw_, SIGNAL( SurfaceChanged( const std::string& ) ),
                 this, SLOT( SurfaceChangedSlot( const std::string& ) ) );
        // main layout: create and add GridDataSurfaceWidget into it
        QVBoxLayout* mainLayout = new QVBoxLayout;
        mainLayout->addWidget( sw_ );

        // separator
        QFrame* line = new QFrame;
        line->setFrameShape( QFrame::HLine );
        mainLayout->addWidget( line );

        // real time checkbox
        QHBoxLayout* realTimeLayout = new QHBoxLayout;
        realTimeLayout->addWidget( new QLabel( tr( "Real-time Update" ) ) );
        realTimeCheckBox_ = new QCheckBox;
        realTimeLayout->addWidget( realTimeCheckBox_ );
        mainLayout->addItem( realTimeLayout );

        mainLayout->addWidget( line );

        // Generate, Remove, Close buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        generateButton_ = new QPushButton( tr( "Generate" ) );
        connect( generateButton_, SIGNAL( released() ), this, SLOT( GenerateSlot() ) );
        buttonLayout->addWidget( generateButton_ );
        removeButton_ = new QPushButton( tr( "Remove" ) );
        removeButton_->setEnabled( mol_->HasGridDataSurface( sw_->GetSurfaceLabel() ) );
        connect( removeButton_, SIGNAL( released() ), this, SLOT( RemoveSlot() ) );
        buttonLayout->addWidget( removeButton_ );
        QPushButton* okButton = new QPushButton( tr( "Close" ) );
        connect( okButton, SIGNAL( released() ), this, SLOT( AcceptSlot() ) );
        buttonLayout->addWidget( okButton );
        mainLayout->addItem( buttonLayout );
        this->setLayout( mainLayout );

        if( !mol_->HasGridData() )
        {
            sw_->setEnabled( false );
            generateButton_->setEnabled( false );
            realTimeCheckBox_->setEnabled( false );
        }
        sw_->SetRenderingStyle( mol_->GetGridDataSurfaceRenderingStyle() ) ;
    }

private:
    /// GridDataSurfaceWidget istance: ownned by this object.
    GridDataSurfaceWidget* sw_;
    /// Reference to main window.
    MainWindow* mw_;
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Button for generation of isosurface.
    QPushButton* generateButton_;
    /// Button for removal of isosurface.
    QPushButton* removeButton_;
    /// Real-time check box: if checked a surface is generated
    /// each time a parameter's value is changed.
    QCheckBox* realTimeCheckBox_;
};
#endif /*GRIDDATASURFACEDIALOG_H_*/

