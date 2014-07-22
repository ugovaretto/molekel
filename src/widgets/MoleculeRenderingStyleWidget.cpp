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
#include <QComboBox>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QVariant>
#include <QLabel>
#include <QGridLayout>
#include <QPalette>
#include <QColor>
#include <QColorDialog>
#include <QFrame>
#include <QCheckBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>


// OPENMOIV
#include <ChemKit2/ChemDisplayParam.h>

// STD
#include <cassert>
#include <exception>

#include "MoleculeRenderingStyleWidget.h"
#include "../MainWindow.h"
#include "../MolekelMolecule.h"
#include "../utility/RAII.h"

//------------------------------------------------------------------------------
MoleculeRenderingStyleWidget::MoleculeRenderingStyleWidget( MainWindow* mw,
                                                  MolekelMolecule* mol,
                                                  const PersistentSettingsKeys& psk,
                                                  QWidget* parent ) :
                                                  QDialog( parent ),
                                                  updatingGUI_( false ),
                                                  mainWin_( mw ),
                                                  molecule_( mol ),
                                                  psKeys_( psk )

{
    // checks
    assert( mainWin_ && "NULL MainWindow" );
    assert( molecule_ && "NULL Molecule" );
    CreateGridLayout();
    Init();
    UpdateUI();
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::CreateBoxLayout()
{
    //// layouts
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setMargin( MARGIN );

    // Layout for comboboxes and sliders
    QVBoxLayout* vComboBoxLayout = new QVBoxLayout;
    vComboBoxLayout->setSpacing( SPACING );
    QHBoxLayout* hButtonLayout = new QHBoxLayout;

    // Display layout
    QHBoxLayout* displayLayout = new QHBoxLayout;
    displayLayout->setSpacing( SPACING );

    // Atom layout
    QHBoxLayout* atomLayout = new QHBoxLayout;
    atomLayout->setSpacing( SPACING );

    // Bond layout
    QHBoxLayout* bondLayout = new QHBoxLayout;
    bondLayout->setSpacing( SPACING );

    // Residue layout
    QHBoxLayout* residueLayout = new QHBoxLayout;
    residueLayout->setSpacing( SPACING );

    // Atom detail layout
    QHBoxLayout* atomDetailLayout = new QHBoxLayout;
    atomDetailLayout->setSpacing( SPACING );

    // Bond detail layout
    QHBoxLayout* bondDetailLayout = new QHBoxLayout;
    bondDetailLayout->setSpacing( SPACING );

    //// create ok, apply, cancel buttons
    QPushButton* ok = new QPushButton( tr( "Ok" ) );
    connect( ok, SIGNAL( clicked() ), this, SLOT( AcceptSlot() ) );
    QPushButton* apply = new QPushButton( tr( "Apply" ) );
    connect( apply, SIGNAL( clicked() ), this, SLOT( ApplySlot() ) );
    QPushButton* cancel = new QPushButton( tr( "Cancel" ) );
    connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );

    //// create widgets
    // combo boxes
    displayStyle_ = new QComboBox;
    displayStyle_->setEditable( false );
    atom_ = new QComboBox;
    atom_->setEditable( false );
    bond_ = new QComboBox;
    bond_->setEditable( false );
    residue_ = new QComboBox;
    residue_->setEditable( false );

    // sliders
    atomDetail_ = new QSlider;
    atomDetail_->setOrientation(Qt::Horizontal);
    atomDetail_->setRange(1,100);
    bondDetail_ = new QSlider;
    bondDetail_->setOrientation(Qt::Horizontal);
    bondDetail_->setRange(1,100);

    // labels
    QLabel* displayLabel = new QLabel( tr( "Style" ) );
    QLabel* atomLabel = new QLabel( tr( "Atom" ) );
    QLabel* bondLabel = new QLabel( tr( "Bond" ) );
    QLabel* residueLabel = new QLabel( tr( "Residue" ) );
    QLabel* atomDetailLabel = new QLabel( tr( "Atom Detail" ) );
    QLabel* bondDetailLabel = new QLabel( tr( "Bond Detail" ) );

    //// fill layouts bottom up (inner --> outer)
    displayLayout->addWidget( displayLabel, Qt::AlignLeft );
    displayLayout->addWidget( displayStyle_, Qt::AlignLeft );
    atomLayout->addWidget( atomLabel, Qt::AlignLeft );
    atomLayout->addWidget( atom_ , Qt::AlignLeft );
    bondLayout->addWidget( bondLabel, Qt::AlignLeft );
    bondLayout->addWidget( bond_, Qt::AlignLeft );
    residueLayout->addWidget( residueLabel, Qt::AlignLeft );
    residueLayout->addWidget( residue_, Qt::AlignLeft );


    vComboBoxLayout->addItem( displayLayout );
    vComboBoxLayout->addItem( atomLayout );
    vComboBoxLayout->addItem( bondLayout );
    vComboBoxLayout->addItem( residueLayout );
    vComboBoxLayout->addItem( atomDetailLayout );
    vComboBoxLayout->addItem( bondDetailLayout );

    // buttons
    hButtonLayout->addWidget( ok );
    hButtonLayout->addWidget( apply );
    hButtonLayout->addWidget( cancel );

    mainLayout->addItem( vComboBoxLayout );
    mainLayout->addItem( hButtonLayout );

    this->setLayout( mainLayout );
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::CreateGridLayout()
{
    //// layouts
    // Main layout
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->setMargin( MARGIN );

    // Layout for comboboxes and sliders
    QGridLayout* vComboBoxLayout = new QGridLayout();
    vComboBoxLayout->setSpacing( SPACING );
    QVBoxLayout* detailLayout = new QVBoxLayout;
    QHBoxLayout* hButtonLayout = new QHBoxLayout;

    // Atom detail layout
    QHBoxLayout* atomDetailLayout = new QHBoxLayout;
    atomDetailLayout->setSpacing( SPACING );

    // Bond detail layout
    QHBoxLayout* bondDetailLayout = new QHBoxLayout;
    bondDetailLayout->setSpacing( SPACING );

    //// create ok, apply, cancel buttons
    QPushButton* ok = new QPushButton( tr( "Ok" ) );
    connect( ok, SIGNAL( clicked() ), this, SLOT( AcceptSlot() ) );
    QPushButton* apply = new QPushButton( tr( "Apply" ) );
    connect( apply, SIGNAL( clicked() ), this, SLOT( ApplySlot() ) );
    QPushButton* cancel = new QPushButton( tr( "Cancel" ) );
    connect( cancel, SIGNAL( clicked() ), this, SLOT( reject() ) );

    //// create widgets
    // combo boxes
    displayStyle_ = new QComboBox;
    displayStyle_->setEditable( false );
    atom_ = new QComboBox;
    atom_->setEditable( false );
    bond_ = new QComboBox;
    bond_->setEditable( false );
    residue_ = new QComboBox;
    residue_->setEditable( false );

    dipoleMoment_ = new QComboBox;
    dipoleMoment_->setEditable( false );

    // sliders
    atomScaling_ = new QSlider;
    atomScaling_->setRange( 0, 100 );
    atomScaling_->setOrientation( Qt::Horizontal );
    bondScaling_ = new QSlider;
    bondScaling_->setRange( 0, 100 );
    bondScaling_->setOrientation( Qt::Horizontal );
    atomDetail_ = new QSlider;
    atomDetail_->setOrientation( Qt::Horizontal );
    atomDetail_->setRange( 1,100 );
    bondDetail_ = new QSlider;
    bondDetail_->setOrientation( Qt::Horizontal );
    bondDetail_->setRange( 1,100 );

    // labels
    QLabel* displayLabel = new QLabel( tr( "Molecule" ) );
    QLabel* atomLabel = new QLabel( tr( "Atom" ) );
    QLabel* bondLabel = new QLabel( tr( "Bond" ) );
    QLabel* residueLabel = new QLabel( tr( "Residue" ) );
    QLabel* atomDetailLabel = new QLabel( tr( "Atom Detail" ) );
    QLabel* bondDetailLabel = new QLabel( tr( "Bond Detail" ) );
    QLabel* atomScalingLabel = new QLabel( tr( "Atom Scaling" ) );
    QLabel* bondScalingLabel = new QLabel( tr( "Bond Scaling" ) );
    QLabel* dipoleMomentLabel = new QLabel( tr( "Dipole Moment" ) );

    //// fill layouts bottom up (inner --> outer)
    vComboBoxLayout->addWidget( displayLabel, 0, 0 );
    vComboBoxLayout->addWidget( displayStyle_, 0, 1 );
    vComboBoxLayout->addWidget( atomLabel, 1, 0 );
    vComboBoxLayout->addWidget( atom_ , 1, 1 );
    vComboBoxLayout->addWidget( bondLabel, 2, 0 );
    vComboBoxLayout->addWidget( bond_, 2, 1 );
    vComboBoxLayout->addWidget( residueLabel, 3, 0 );
    vComboBoxLayout->addWidget( residue_, 3, 1 );
    vComboBoxLayout->addWidget( dipoleMomentLabel, 4, 0 );
    vComboBoxLayout->addWidget( dipoleMoment_, 4, 1 );

    atomDetailLayout->addWidget( atomDetailLabel, Qt::AlignLeft );
    atomDetailLayout->addWidget( atomDetail_, Qt::AlignRight );
    bondDetailLayout->addWidget( bondDetailLabel, Qt::AlignLeft );
    bondDetailLayout->addWidget( bondDetail_, Qt::AlignRight );
    detailLayout->addItem( atomDetailLayout );
    detailLayout->addSpacing( VPADDING );
    detailLayout->addItem( bondDetailLayout );

    QHBoxLayout* atomScalingLayout = new QHBoxLayout;
    atomScalingLayout->setSpacing( SPACING );
    atomScalingLayout->addWidget( atomScalingLabel, Qt::AlignLeft );
    atomScalingLayout->addWidget( atomScaling_, Qt::AlignRight );

    QHBoxLayout* bondScalingLayout = new QHBoxLayout;
    bondScalingLayout->setSpacing( SPACING );
    bondScalingLayout->addWidget( bondScalingLabel, Qt::AlignLeft );
    bondScalingLayout->addWidget( bondScaling_, Qt::AlignRight );

    // Color
    QGridLayout* colorLayout = new QGridLayout;
    colorLayout->setSpacing( SPACING );
    QPushButton* colorPushButton = new QPushButton( "Color (SoSphere/SoCylinder)" );
    connect( colorPushButton, SIGNAL( released() ), this, SLOT( SetColorSlot() ) );
    colorLayout->addWidget( colorPushButton, 0, 0 );
    colorLabel_ = new QLabel;
    const int frameStyle = QFrame::Sunken | QFrame::Panel;
    colorLabel_->setFrameStyle( frameStyle );
    colorLayout->addWidget( colorLabel_, 0, 1 );

    QPushButton* loadColorsPushButton = new QPushButton( "Atom Colors..." );
    connect( loadColorsPushButton, SIGNAL( released() ),
             this, SLOT( LoadAtomColorsSlot() ) );
    colorLayout->addWidget( loadColorsPushButton, 1, 0 );
    atomColorsLabel_ = new QLabel( "Default" );
    if( molecule_->GetAtomColorFileName().size() > 0 )
    {
        atomColorsLabel_->setText( molecule_->GetAtomColorFileName().c_str() );
    }
    colorLayout->addWidget( atomColorsLabel_, 1, 1 );

    // buttons
    hButtonLayout->addWidget( ok );
    hButtonLayout->addWidget( apply );
    hButtonLayout->addWidget( cancel );

    mainLayout->addItem( vComboBoxLayout );
    mainLayout->addSpacing( VPADDING );
    mainLayout->addItem( detailLayout );
    mainLayout->addSpacing( VPADDING );
    mainLayout->addItem( atomScalingLayout );
    //mainLayout->addSpacing( VPADDING );
    mainLayout->addItem( bondScalingLayout );
    mainLayout->addSpacing( VPADDING );
     mainLayout->addItem( colorLayout );
    mainLayout->addSpacing( VPADDING );


    // Separator
    QFrame* line = new QFrame;
    line->setFrameShape( QFrame::HLine );
    mainLayout->addWidget( line );

    // checkbox
    rtUpdateCheckBox_ = new QCheckBox( tr( "Real-time update" ) );
    rtUpdateCheckBox_->setCheckState( Qt::Checked );
    mainLayout->addWidget( rtUpdateCheckBox_ );


    mainLayout->addSpacing( VPADDING );
    mainLayout->addItem( hButtonLayout );

    this->setLayout( mainLayout );
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::SetColorSlot()
{
    float r, g, b;
    const int MAX_COMPONENT_VALUE = 255;
    molecule_->GetColor( r, g, b );
    QColor color = QColorDialog::getColor( QColor( int( r * MAX_COMPONENT_VALUE ),
                                                   int( g * MAX_COMPONENT_VALUE ),
                                                   int( b * MAX_COMPONENT_VALUE ) ),
                                           this );
    if ( color.isValid() )
    {
        colorLabel_->setText( color.name() );
        colorLabel_->setPalette( QPalette( color ) );
        colorLabel_->setAutoFillBackground( true );
        color_ = color;
    }
    if( rtUpdateCheckBox_->checkState() == Qt::Checked ) RealTimeUpdateSlot();
}


//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::Init()
{
    displayStyle_->addItem( tr( "CPK" ), QVariant( ChemDisplayParam::DISPLAY_CPK ) );
    displayStyle_->addItem( tr( "Ball & Stick" ), QVariant( ChemDisplayParam::DISPLAY_BALLSTICK ) );
    displayStyle_->addItem( tr( "Stick" ) , QVariant( ChemDisplayParam::DISPLAY_STICK ) );
    displayStyle_->addItem( tr( "Wireframe" ) , QVariant( ChemDisplayParam::DISPLAY_WIREFRAME ) );
    displayStyle_->addItem( tr( "Ball & Wire" ) , QVariant( ChemDisplayParam::DISPLAY_BALLWIRE ) );
    connect( displayStyle_, SIGNAL( currentIndexChanged( int ) ), SLOT( RealTimeUpdateSlot() ) );

    atom_->addItem( tr( "Hemisphere" ), QVariant( ChemDisplayParam::ATOMSPHERE_HEMISPHERES ) );
    atom_->addItem( tr( "Full Sphere" ), QVariant( ChemDisplayParam::ATOMSPHERE_FULLSPHERES ) );
    atom_->addItem( tr( "glDrawArrays - Hemisphere" ), QVariant( ChemDisplayParam::ATOMSPHERE_ARRAY_HEMISPHERES ) );
    atom_->addItem( tr( "glDrawArrays - Sphere" ), QVariant( ChemDisplayParam::ATOMSPHERE_FULLSPHERES ) );
    atom_->addItem( tr( "gluSphere" ), QVariant( ChemDisplayParam::ATOMSPHERE_GLUSPHERE ) );
    atom_->addItem( tr( "SoSphere" ), QVariant( ChemDisplayParam::ATOMSPHERE_SOSPHERE ) );
    atom_->addItem( tr( "Billboard" ), QVariant( ChemDisplayParam::ATOMSPHERE_BILLBOARD ) );
    atom_->addItem( tr( "Fast Level of Detail" ), QVariant( ChemDisplayParam::ATOMSPHERE_LOD ) );
    atom_->addItem( tr( "Level of Detail" ), QVariant( ChemDisplayParam::ATOMSPHERE_LOD_GEOMETRY ) );
    connect( atom_, SIGNAL( currentIndexChanged( int ) ), SLOT( RealTimeUpdateSlot() ) );

    bond_->addItem( tr( "Cylinder" ), QVariant( ChemDisplayParam::BONDCYLINDER_NOCAP ) );
    bond_->addItem( tr( "Cylinder, round cap" ), QVariant( ChemDisplayParam::BONDCYLINDER_ROUNDCAP ) );
    bond_->addItem( tr( "Cylinder LOD, round cap" ), QVariant( ChemDisplayParam::BONDCYLINDER_LOD_ROUNDCAP ) );
    bond_->addItem( tr( "glDrawArrays" ), QVariant( ChemDisplayParam::BONDCYLINDER_ARRAY_NOCAP ) );
    bond_->addItem( tr( "gluCylinder" ), QVariant( ChemDisplayParam::BONDCYLINDER_GLUCYLINDER_NOCAP ) );
    bond_->addItem( tr( "SoCylinder" ), QVariant( ChemDisplayParam::BONDCYLINDER_SOCYLINDER_NOCAP ) );
    bond_->addItem( tr( "Semicylinder" ), QVariant( ChemDisplayParam::BONDCYLINDER_SEMI_NOCAP ) );
    bond_->addItem( tr( "Level of Detail" ), QVariant( ChemDisplayParam::BONDCYLINDER_LOD_NOCAP ) );
    connect( bond_, SIGNAL( currentIndexChanged( int ) ), SLOT( RealTimeUpdateSlot() ) );

    residue_->addItem( tr( "Wire" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_CAWIRE ) );
    residue_->addItem( tr( "Stick" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_CASTICK ) );
    residue_->addItem( tr( "Line Ribbon" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_LINERIBBON ) );
    residue_->addItem( tr( "Flat Ribbon" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_FLATRIBBON ) );
    residue_->addItem( tr( "Solid Ribbon" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_SOLIDRIBBON ) );
    residue_->addItem( tr( "Schematic" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_SCHEMATIC ) );
    residue_->addItem( tr( "Hide" ), QVariant( ChemDisplayParam::DISPLAY_RESIDUES_NONE ) );
    connect( residue_, SIGNAL( currentIndexChanged( int ) ), SLOT( RealTimeUpdateSlot() ) );

    dipoleMoment_->addItem( tr( "Hide" ), QVariant( false ) );
    dipoleMoment_->addItem( tr( "Show" ), QVariant( true ) );
    connect( dipoleMoment_, SIGNAL( currentIndexChanged( int ) ), SLOT( RealTimeUpdateSlot() ) );

    atomDetail_->setRange( 1, MAX_COMPLEXITY );
    connect( atomDetail_, SIGNAL( sliderMoved( int ) ), SLOT( RealTimeUpdateSlot() ) );

    bondDetail_->setRange( 1, MAX_COMPLEXITY );
    connect( bondDetail_, SIGNAL( sliderMoved( int ) ), SLOT( RealTimeUpdateSlot() ) );

    atomScaling_->setRange( 0, MAX_ATOM_SCALING );
    connect( atomScaling_, SIGNAL( sliderMoved( int ) ), SLOT( RealTimeUpdateSlot() ) );

    bondScaling_->setRange( 0, MAX_BOND_SCALING );
    connect( bondScaling_, SIGNAL( sliderMoved( int ) ), SLOT( RealTimeUpdateSlot() ) );
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::UpdateUI()
{
    ResourceHandler< bool > rh( updatingGUI_, true, false );

    const int ds = molecule_->GetChemDisplayParam()->displayStyle.getValue();
    const int dsi = displayStyle_->findData( QVariant( ds ) );
    assert( dsi >= 0 && "Invalid ChemDisplayStyle data" );
    displayStyle_->setCurrentIndex( dsi );

    const int a = molecule_->GetChemDisplayParam()->atomSphereDisplayStyle.getValue();
    const int ai = atom_->findData( QVariant( a ) );
    assert( ai >= 0 && "Invalid ChemDisplayStyle data" );
    atom_->setCurrentIndex( ai );

    const int b = molecule_->GetChemDisplayParam()->bondCylinderDisplayStyle.getValue();
    const int bi = bond_->findData( QVariant( b ) );
    assert( bi >= 0 && "Invalid ChemDisplayStyle data" );
    bond_->setCurrentIndex( bi );

    const int r = molecule_->GetChemDisplayParam()->residueDisplayStyle.getValue();
    const int ri = residue_->findData( QVariant( r ) );
    assert( ri >= 0 && "Invalid ChemDisplayStyle data" );
    residue_->setCurrentIndex( ri );

    if( !molecule_->HasDipoleMoment() ) dipoleMoment_->setEnabled( false );
    else
    {
        dipoleMoment_->setEnabled( true );
        const int dpi = dipoleMoment_->findData( molecule_->GetDipoleMomentVisibility() );
        dipoleMoment_->setCurrentIndex( dpi );
    }

    atomDetail_->setValue( int( molecule_->GetChemDisplayParam()
                            ->atomSphereComplexity.getValue() * MAX_COMPLEXITY ) );
    bondDetail_->setValue( int( molecule_->GetChemDisplayParam()
                            ->bondCylinderComplexity.getValue() * MAX_COMPLEXITY ) );

    atomDetail_->setTickInterval( MAX_COMPLEXITY / SLIDER_TICKS );
    atomDetail_->setTickPosition( QSlider::TicksAbove );
    bondDetail_->setTickInterval( MAX_COMPLEXITY / SLIDER_TICKS );
    bondDetail_->setTickPosition( QSlider::TicksAbove );

    atomScaling_->setTickPosition( QSlider::TicksAbove );
    atomScaling_->setValue( int( molecule_->GetChemDisplayParam()->
                            ballStickSphereScaleFactor.getValue() * 100 ) );
    //atomScaling_->setValue( int( molecule_->GetChemDisplayParam()->
    //                          atomRadiiScaleFactor.getValue() * 100 ) );
    atomScaling_->setTickInterval( MAX_ATOM_SCALING / SLIDER_TICKS );

    bondScaling_->setTickPosition( QSlider::TicksAbove );
    bondScaling_->setValue( int( molecule_->GetChemDisplayParam()->
                            bondCylinderRadius.getValue() * 100 ) );
    bondScaling_->setTickInterval( MAX_ATOM_SCALING / SLIDER_TICKS );

    float cr, cg, cb;
    const int MAX_COMPONENT_VALUE = 255;
    molecule_->GetColor( cr, cg, cb );
    color_ = QColor( int( cr * MAX_COMPONENT_VALUE ),
                     int( cg * MAX_COMPONENT_VALUE ),
                     int( cb * MAX_COMPONENT_VALUE ) );
    colorLabel_->setText( color_.name() );
    colorLabel_->setPalette( QPalette( color_ ) );
    colorLabel_->setAutoFillBackground( true );
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::UpdateMolecule()
{
    const int ds = displayStyle_->itemData( displayStyle_->currentIndex() ).toInt();

    molecule_->GetChemDisplayParam()->displayStyle.setValue( ds );

    const int a = atom_->itemData( atom_->currentIndex() ).toInt();
    molecule_->GetChemDisplayParam()->atomSphereDisplayStyle.setValue( a );

    const int b = bond_->itemData( bond_->currentIndex() ).toInt();
    molecule_->GetChemDisplayParam()->bondCylinderDisplayStyle.setValue( b );

    const int r = residue_->itemData( residue_->currentIndex() ).toInt();
    molecule_->GetChemDisplayParam()->residueDisplayStyle.setValue( r );

    const bool dp = dipoleMoment_->itemData( dipoleMoment_->currentIndex() ).toBool();
    molecule_->SetDipoleMomentVisibility( dp );

    const int comp = MAX_COMPLEXITY  == 0 ? 1 : MAX_COMPLEXITY;
    molecule_->GetChemDisplayParam()->atomSphereComplexity.setValue( float( atomDetail_->value() - 1 ) / comp );
    molecule_->GetChemDisplayParam()->bondCylinderComplexity.setValue( float( bondDetail_->value() - 1 ) / comp );

    molecule_->GetChemDisplayParam()->ballStickSphereScaleFactor.setValue( float( atomScaling_->value() )
                                                                           / MAX_ATOM_SCALING);
    
    //molecule_->GetChemDisplayParam()->atomRadiiScaleFactor.setValue( float( atomScaling_->value() )
    //                                                                / MAX_ATOM_SCALING );                                                                     
    
    molecule_->GetChemDisplayParam()->bondCylinderRadius.setValue( float( bondScaling_->value() )
                                                                    / MAX_BOND_SCALING );

    const float CMAX = 255;
    molecule_->SetColor( float( color_.red() ) / CMAX,
                         float( color_.green() ) / CMAX,
                         float( color_.blue() ) / CMAX );

    molecule_->RecomputeBBox();

    //update persistent settings
    QSettings settings;
    settings.setValue( psKeys_.atomDetail.c_str(),
        molecule_->GetChemDisplayParam()->atomSphereComplexity.getValue() );
    settings.setValue( psKeys_.bondDetail.c_str(),
        molecule_->GetChemDisplayParam()->bondCylinderComplexity.getValue() );
    settings.setValue( psKeys_.atomScalingFactor.c_str(),
        molecule_->GetChemDisplayParam()->ballStickSphereScaleFactor.getValue() );
    settings.setValue( psKeys_.bondScalingFactor.c_str(),
        molecule_->GetChemDisplayParam()->bondCylinderRadius.getValue() );
    settings.setValue( psKeys_.displayStyle.c_str(),
        molecule_->GetChemDisplayParam()->displayStyle.getValue() );
}


//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::AcceptSlot()
{
    ApplySlot();
    accept();
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::ApplySlot()
{
    UpdateMolecule();
    mainWin_->Refresh();
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::RealTimeUpdateSlot()
{
    if( updatingGUI_ ) return;
    if( rtUpdateCheckBox_->checkState() != Qt::Checked ) return;
    ApplySlot();
}

//------------------------------------------------------------------------------
void MoleculeRenderingStyleWidget::LoadAtomColorsSlot()
{
    try
    {
        QString f = QFileDialog::getOpenFileName( this, "Select file to read atom colors from" );
        if( f.size() == 0 ) return;
        molecule_->SetAtomColors( f.toStdString() );
        atomColorsLabel_->setText( f );
        mainWin_->Refresh();
    } catch( const std::exception& e )
    {
        QMessageBox::critical( this, "I/O Error", e.what() );
    }
}

