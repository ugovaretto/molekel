#ifndef SASWIDGET_H_
#define SASWIDGET_H_
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

#include <QWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>

#include "../MolekelMolecule.h"
#include "../utility/RAII.h"
#include <cassert>


//
//-----------------------------------------
//
// [ ] Map MEP
//                   ______ __
// Solvent Radius   |______|^v| (spin box)
//
//                   ______ __
// Step             |______|^v| (spin box)
//
//                   _______ _
// Rendering Style  |_______|v|
//
//-----------------------------------------
//
/// GUI widget for specifying SAS parameters.
class SasWidget : public QWidget
{
    Q_OBJECT

private slots:
    /// Called when rendering style changed.
    void RenderingStyleChangedSlot( int index )
    {
        if( updatingGUI_ ) return;
        emit RenderingStyleChanged
        (
            MolekelMolecule::RenderingStyle
            (
                renderingStyleComboBox_->itemData( index ).toInt()
             )
        );
    }
    /// Called when radius changed.
    void RadiusChangedSlot( double r )
    {
        if( updatingGUI_ ) return;
        emit RadiusChanged( r );
    }
    /// Called when step changed.
    void StepChangedSlot( double v )
    {
        if( updatingGUI_ ) return;
        emit StepChanged( v );
    }

signals:
     /// Emitted when solvent radius changes.
    void RadiusChanged( double );
     /// Emitted when step changes.
    void StepChanged( double );
    /// Emitted when rendering style changes.
    void RenderingStyleChanged( MolekelMolecule::RenderingStyle );
public:
    /// Constructor.
    SasWidget( MolekelMolecule* m,  QWidget* parent = 0  )
                                       : mol_( m ),
                                       radiusSpinBox_( 0 ),
                                       stepSpinBox_( 0 ),
                                       renderingStyleComboBox_( 0 ),
                                       mepCheckBox_( 0 ),
                                       updatingGUI_( false )

    {
        assert( m && "NULL Molecule" );
        ResourceHandler< bool > ug( updatingGUI_, true, false );
        QGridLayout* mainLayout = new QGridLayout;
        // MEP
        mepCheckBox_ = m->CanComputeMEP() || !m->HasGridData() ?
                       new QCheckBox( "Map MEP" ) :
                       new QCheckBox( "Map MEP (From Grid Data)" );
        mepCheckBox_->setEnabled( m->CanComputeMEP() || m->HasGridData() );
        mepCheckBox_->setCheckState( Qt::Unchecked );
        mainLayout->addWidget( mepCheckBox_, 0, 0 );
        // radius
        radiusSpinBox_ = new QDoubleSpinBox;
        radiusSpinBox_->setMinimum( 0. );
        radiusSpinBox_->setValue( 0. );
        radiusSpinBox_->setSingleStep( 0.05 );
        mainLayout->addWidget( new QLabel( "Solvent Radius (Angstrom)" ), 1, 0 );
        mainLayout->addWidget( radiusSpinBox_, 1, 1 );
        // step
        stepSpinBox_ = new QDoubleSpinBox;
        stepSpinBox_->setMinimum( 0.01 );
        stepSpinBox_->setValue( 0.25 );
        stepSpinBox_->setSingleStep( 0.05 );
        mainLayout->addWidget( new QLabel( "Step" ), 2, 0 );
        mainLayout->addWidget( stepSpinBox_, 2, 1 );
        // rendering style
        renderingStyleComboBox_ = new QComboBox;
        renderingStyleComboBox_->addItem( "Solid", int( MolekelMolecule::SOLID ) );
        renderingStyleComboBox_->addItem( "Points", int( MolekelMolecule::POINTS ) );
        renderingStyleComboBox_->addItem( "Wireframe", int( MolekelMolecule::WIREFRAME ) );
        renderingStyleComboBox_->addItem( "Transparent Solid", int( MolekelMolecule::TRANSPARENT_SOLID ) );
        mainLayout->addWidget( new QLabel( "Rendering style" ), 3, 0 );
        mainLayout->addWidget( renderingStyleComboBox_, 3, 1 );
        setLayout( mainLayout );

        // connect signals to slots
        connect( renderingStyleComboBox_, SIGNAL( currentIndexChanged( int ) ),
                 this, SLOT( RenderingStyleChangedSlot( int ) ) );
        connect( radiusSpinBox_, SIGNAL( valueChanged( double ) ),
                 this, SLOT( RadiusChangedSlot( double ) ) );
        connect( stepSpinBox_, SIGNAL( valueChanged( double ) ),
                 this, SLOT( StepChangedSlot( double ) ) );

    }
    /// Returns current rendering style (points, wireframe, solid, transparent solid)
    MolekelMolecule::RenderingStyle GetRenderingStyle() const
    {
        return MolekelMolecule::RenderingStyle(
                    renderingStyleComboBox_->itemData(
                        renderingStyleComboBox_->currentIndex()
                    ).toInt()
               );
    }
    /// Returns solvent radius.
    double GetRadius() const { return radiusSpinBox_->value(); }
    /// Returns step.
    double GetStep() const { return stepSpinBox_->value(); }
    /// Sets current molecule.
    void SetMolecule( MolekelMolecule* m ) { mol_ = m; }
    /// Returns true if compute MEP checkbox is checked.
    bool ComputeMep() const { return mepCheckBox_->checkState() == Qt::Checked; }
    /// Sets the current rendering style.
    void SetRenderingStyle( MolekelMolecule::RenderingStyle rs )
    {
        const int i = renderingStyleComboBox_->findData( int( rs ) );
        if( i < 0 ) return;
        renderingStyleComboBox_->setCurrentIndex( i );
    }
private:
    /// Reference to molecule.
    MolekelMolecule* mol_;
    /// Radius.
    QDoubleSpinBox* radiusSpinBox_;
    /// Step.
    QDoubleSpinBox* stepSpinBox_;
    /// Rendering style.
    QComboBox* renderingStyleComboBox_;
    /// MEP
    QCheckBox* mepCheckBox_;
    /// Internal variable used to detect from within slots when UI is being updated.
    bool updatingGUI_;
};

#endif /*SASWIDGET_H_*/
