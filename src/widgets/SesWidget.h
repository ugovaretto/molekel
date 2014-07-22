#ifndef SESWIDGET_H_
#define SESWIDGET_H_
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

#include <QWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>

#include "../MolekelMolecule.h"
#include "../utility/RAII.h"

#include <cassert>

//
//-----------------------------------------
//
// [ ] Map MEP
//                   ______ __
// Probe Radius     |______|^v| (spin box)
//
//                   ______ __
// Point density    |______|^v| (spin box)
//
//  --[ ]Use MSMS------------------------
// |                                     |
// | MSMS Path                           |
// |  ___________________   ___________  |
// | |___________________| |_Select..._| |
//  -------------------------------------
//                   _______ _
// Rendering Style  |_______|v|
//
//-----------------------------------------
//
/// GUI widget for specifying SES parameters.
class SesWidget : public QWidget
{
    Q_OBJECT

private slots:

    /// Called when "..." button is pressed: used to select MSMS executable.
    void SelectMSMSPathSlot()
    {
        QString msmsPath = QFileDialog::getOpenFileName( this, "Select MSMS Executable" );
        if( msmsPath.size() > 0 )
        {
            msmsPathLineEdit_->setText( msmsPath ); // doesn't emit signal
            emit MSMSExecutablePathChanged( msmsPath );
        }
    }

    /// Called when rendering style changed.
    void RenderingStyleChangedSlot( int index )
    {
        if( updatingGUI_  ) return;
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

    /// Called MSMS file path edited.
    void MSMSEditingFinishedSlot()
    {
        emit MSMSExecutablePathChanged( msmsPathLineEdit_->text() );
    }
signals:
     /// Emitted when solvent radius changes.
    void RadiusChanged( double );
     /// Emitted when step changes.
    void StepChanged( double );
    /// Emitted when rendering style changes.
    void RenderingStyleChanged( MolekelMolecule::RenderingStyle );
    /// Emitted when the MSMS executable path is changed.
    /// This signal is emitted only after the user finishes editing this field i.e.
    /// enter is pressed, the line edit loses focus or the file is selected through
    /// a file dialog.
    void MSMSExecutablePathChanged( const QString& );
public:
    /// Constructor.
    SesWidget( MolekelMolecule* m,
               const QString& msmsFilePath = QString(),
               QWidget* parent = 0  )
                                       : mol_( m ),
                                       radiusSpinBox_( 0 ),
                                       stepSpinBox_( 0 ),
                                       renderingStyleComboBox_( 0 ),
                                       mepCheckBox_( 0 ),
                                       msmsGroupBox_( 0 ),
                                       msmsPathLineEdit_( 0 ),
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
        radiusSpinBox_->setDecimals( 5 );
        radiusSpinBox_->setMinimum( 0. );
        radiusSpinBox_->setValue( 1.4 );
        radiusSpinBox_->setSingleStep( 0.1 );
        mainLayout->addWidget( new QLabel( "Probe Radius (Angstrom)" ), 1, 0 );
        mainLayout->addWidget( radiusSpinBox_, 1, 1 );
        // step
        stepSpinBox_ = new QDoubleSpinBox;
        stepSpinBox_->setMinimum( 0 );
        stepSpinBox_->setMaximum( 100000 );
        stepSpinBox_->setValue( 10 );
        stepSpinBox_->setSingleStep( 0.5 );
        stepSpinBox_->setDecimals( 1 );
        mainLayout->addWidget( new QLabel( "Point Density" ), 2, 0 );
        mainLayout->addWidget( stepSpinBox_, 2, 1 );
        // rendering style
        renderingStyleComboBox_ = new QComboBox;
        /// MSMS
        msmsGroupBox_ = new QGroupBox( "Use MSMS" );
        msmsGroupBox_->setCheckable( true );
        msmsGroupBox_->setChecked( true );
        QVBoxLayout* msmsLayout = new QVBoxLayout;
        msmsLayout->addWidget( new QLabel( "MSMS Executable" ) );
        QHBoxLayout* msmsPathLayout = new QHBoxLayout;
        msmsPathLineEdit_ = new QLineEdit;
        msmsPathLineEdit_->setText( msmsFilePath );
        msmsPathLayout->addWidget( msmsPathLineEdit_ );
        QPushButton* selectMSMSPathButton = new QPushButton( "Select..." );
        connect( selectMSMSPathButton, SIGNAL( released() ),
                 this, SLOT( SelectMSMSPathSlot() ) );
        msmsPathLayout->addWidget( selectMSMSPathButton );
        msmsLayout->addItem( msmsPathLayout );
        msmsGroupBox_->setLayout( msmsLayout );
        mainLayout->addWidget( msmsGroupBox_, 3, 0, 1, 2 );
        // only points supported for internal SES computation for now
        renderingStyleComboBox_->addItem( "Solid (MSMS)", int( MolekelMolecule::SOLID ) );
        renderingStyleComboBox_->addItem( "Points", int( MolekelMolecule::POINTS ) );
        renderingStyleComboBox_->addItem( "Wireframe (MSMS)", int( MolekelMolecule::WIREFRAME ) );
        renderingStyleComboBox_->addItem( "Transparent Solid (MSMS)", int( MolekelMolecule::TRANSPARENT_SOLID ) );
        mainLayout->addWidget( new QLabel( "Rendering style" ), 4, 0 );
        mainLayout->addWidget( renderingStyleComboBox_, 4, 1 );
        setLayout( mainLayout );

        // connect signals to slots
        connect( renderingStyleComboBox_, SIGNAL( currentIndexChanged( int ) ),
                 this, SLOT( RenderingStyleChangedSlot( int ) ) );
        connect( msmsPathLineEdit_, SIGNAL( editingFinished() ),
                 this, SLOT( MSMSEditingFinishedSlot() ) );
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
    /// Returns true if MSMS check box checked; false otherwise.
    bool UseMSMS() const { return msmsGroupBox_->isChecked(); }
    /// Returns MSMS file path.
    QString GetMSMSFilePath() const { return msmsPathLineEdit_->text(); }
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
    /// Use MSMS group box.
    QGroupBox* msmsGroupBox_;
    /// MSMS executable path.
    QLineEdit* msmsPathLineEdit_;
    /// Internal variable used to detect from within slots when UI is being updated.
    bool updatingGUI_;
};

#endif /*SESWIDGET_H_*/
