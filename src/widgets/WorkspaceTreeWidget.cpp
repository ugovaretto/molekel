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
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QStatusBar> // @todo remove, used only to display debug info
#include <QStringList>
#include <QPoint>

#include "WorkspaceTreeWidget.h"
#include "../MainWindow.h"
#include "../MolekelMolecule.h"
#include "../dialogs/ComputeElDensSurfaceDialog.h"
#include "../utility/RAII.h"


const QString WorkspaceTreeWidget::ITEM_TYPE = "TYPE";
const QString WorkspaceTreeWidget::ITEM_DATA = "DATA";

//------------------------------------------------------------------------------
WorkspaceTreeWidget::WorkspaceTreeWidget( MainWindow& mw, QWidget* parent )
    : QWidget( parent ), mw_( mw ), layout_( 0 ), tree_( 0 ), menuItem_( 0 )
{
    // create layout
    layout_ = new QVBoxLayout( this );

    // create tree and add root node
    tree_ = new QTreeWidget( this );
    tree_->setFocusPolicy(Qt::ClickFocus);
    QStringList sl; sl << tr( "" );
    tree_->setHeaderLabels( sl );
    tree_->setColumnCount( 1 );
    root_ = new QTreeWidgetItem( tree_ );
    root_->setText( 0, tr( "Molecules" ) );
    QVariantMap data;
    data[ ITEM_TYPE ] = int( ITEM_TYPE_MOLECULE );
    data[ ITEM_DATA ] = int();
    root_->setData( 0, Qt::UserRole, data );
    // add tree to layout
    layout_->addWidget( tree_ );

    // add everything to this widget
    this->setLayout( layout_ );

    // connect signals to slots
    connect( tree_, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ),
             this,  SLOT( ItemChangedSlot( QTreeWidgetItem*, int ) ) );
    connect( tree_, SIGNAL( itemPressed( QTreeWidgetItem*, int ) ),
             this,  SLOT( ItemPressedSlot( QTreeWidgetItem*, int ) ) );
    popupMenu_ = new QMenu( 0 );
    popupMenu_->addMenu( mw_.GetStyleMenu() );
    popupMenu_->addMenu( mw_.GetDisplayMenu() );
    popupMenu_->addMenu( mw_.GetSurfacesMenu() );
    popupMenu_->addMenu( mw_.GetAnalysisMenu() );
    QAction* deleteAction = popupMenu_->addAction( "Delete" );
    connect( deleteAction, SIGNAL( triggered() ), this, SLOT( DeleteItemSlot() ) );
   
    
}

//------------------------------------------------------------------------------
void WorkspaceTreeWidget::AddMolecule( const MolekelMolecule* mol, MolekelData::IndexType i )
{
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    // create new item
    QTreeWidgetItem* wi = new QTreeWidgetItem( root_ );
    wi->setText( 0, tr( mol->GetFileName().c_str() ) );
    wi->setCheckState( 0, Qt::Checked );
    // set data (type, index)
    QVariantMap data;
    data[ ITEM_TYPE ] = int( ITEM_TYPE_MOLECULE );
    data[ ITEM_DATA ] = i;
    wi->setData( 0, Qt::UserRole, data );
    // add entry into molecule id --> item map
    moleculeItemMap_[ i ] = wi;
    // expand tree to show new entry
    tree_->setItemExpanded( root_, true );
}

//------------------------------------------------------------------------------
void WorkspaceTreeWidget::RemoveMolecule( MolekelData::IndexType i )
{
    assert( moleculeItemMap_.find( i ) != moleculeItemMap_.end()
            && "Molecule not in tree" );
    UnselectMolecules();
    delete root_->takeChild( root_->indexOfChild( moleculeItemMap_[ i ] ) );
    moleculeItemMap_.erase( i );
}

#include <QMessageBox>
//------------------------------------------------------------------------------
//void WorkspaceTreeWidget::ItemClickedSlot(QTreeWidgetItem* wi, int column)
//{
//	//selectedWidgetItem = wi;
//	//if(widgetToPtrMap.find(wi) != widgetToPtrMap.end())
//	//{
//	//	if(widgetToPtrMap[selectedWidgetItem]->getNodeMask()==0xffffffff)
//	//	{
//	//		treeItemMenu->addAction(hideAct);
//	//		treeItemMenu->removeAction(showAct);
//	//		treeItemMenu->addAction(removeAct);
//	//		editMenu->removeAction(showAct);
//	//		editMenu->addAction(hideAct);
//	//	}
//	//	else
//	//	{
//	//		treeItemMenu->addAction(showAct);
//	//		treeItemMenu->removeAction(hideAct);
//	//		editMenu->removeAction(hideAct);
//	//		editMenu->addAction(showAct);
//	//		treeItemMenu->addAction(removeAct);
//	//	}
//	//}
//	//if(lastMouseButtons==Qt::RightButton)
//	//	treeItemMenu->popup(QCursor::pos());
//
//	lastMouseButtons_ = QApplication::mouseButtons();
//}

//------------------------------------------------------------------------------
void WorkspaceTreeWidget::ItemChangedSlot( QTreeWidgetItem* item, int column )
{
    if( updatingGUI_ ) return;
    // find molecule containing item
    QTreeWidgetItem* current = GetMoleculeItem( item );
    if( !current ) return;
    const MolekelData::IndexType molid =
        current->data( 0, Qt::UserRole ).toMap()[ ITEM_DATA ].toUInt();
    // find data type and change visibility accordingly.
    bool visibility =  item->checkState( 0 ) == Qt::Unchecked ? false : true;
    QVariantMap data = item->data( 0, Qt::UserRole ).toMap();
    MolekelMolecule* mol = mw_.GetMolecule( molid );
    assert( mol && "NULL molecule" );
    switch( data[ ITEM_TYPE ].toInt() )
    {
        case ITEM_TYPE_MOLECULE:
            mol->SetVisible( visibility );
            break;
        case ITEM_TYPE_ORBITAL:
            // orbital id is stored in item data
            mol->SetOrbitalSurfaceVisibility( data[ ITEM_DATA ].toInt(), visibility );
            break;
        case ITEM_TYPE_EL_DENS_SURFACE:
            mol->SetElDensSurfaceVisibility( visibility );
            break;
        case ITEM_TYPE_SPIN_DENS_SURFACE:
            mol->SetSpinDensitySurfaceVisibility( visibility );
            break;
        case ITEM_TYPE_GRID_DATA_SURFACE:
            if( mol->GetFormat() != "t41" ) mol->SetGridDataSurfaceVisibility( visibility );
            else mol->SetGridDataSurfaceVisibility( visibility, data[ ITEM_DATA ].toString().toStdString() );
            break;
        case ITEM_TYPE_SAS:
            mol->SetSASVisibility( visibility );
            break;
        case ITEM_TYPE_SES:
            mol->SetSESVisibility( visibility );
            break;
        case ITEM_TYPE_SES_MSMS:
            mol->SetSESMSVisibility( visibility );
            break;
        default:
            break;
    }
    mw_.Refresh();
}

//------------------------------------------------------------------------------
void WorkspaceTreeWidget::ItemPressedSlot( QTreeWidgetItem* item, int column )
{
    if( item == root_ ) return;
    const Qt::MouseButtons lastMouseButtons = QApplication::mouseButtons();
    menuItem_ = 0;
    //if( lastMouseButtons == Qt::LeftButton )
    {   
        mw_.UnselectAll();
        QTreeWidgetItem* current = GetMoleculeItem( item );
        mw_.SelectMolecule( current->data( 0, Qt::UserRole ).toMap()[ ITEM_DATA ].toUInt(), 0, false, false );
    }
    if( lastMouseButtons == Qt::RightButton )
    {
        menuItem_ = item;
        popupMenu_->popup( QCursor::pos() );        
    }
    else menuItem_ = 0;
    mw_.Refresh();
}



//------------------------------------------------------------------------------
void WorkspaceTreeWidget::ContextMenuSlot( const QPoint& p )
{
    // Need to create custom QTreeWidget to support this.
    // Following code should never be executed.
    QMessageBox::about( this, "Info", "Not implemented" );
//    QTreeWidgetItem* item = tree_->itemAt( p );
//    if( !item ) return;
//    MolekelMolecule* mol = mw_.GetMolecule( GetMoleculeItem( item )->data( 0, Qt::UserRole ).toUInt() );
//    assert( mol );
//    ComputeElDensSurfaceDialog dlg( mol, &mw_ );
//    dlg.exec();
}

//------------------------------------------------------------------------------
void WorkspaceTreeWidget::SelectMolecule( MolekelData::IndexType id, bool notify )
{
    assert( moleculeItemMap_.find( id ) != moleculeItemMap_.end() && "Invalid index" );
    UnselectMolecules();
    tree_->setItemSelected( moleculeItemMap_[ id ], true );
}

//------------------------------------------------------------------------------
void WorkspaceTreeWidget::UnselectMolecules()
{
    for( MoleculeItemMap::iterator i = moleculeItemMap_.begin();
         i != moleculeItemMap_.end();
         ++i )
    {
        tree_->setItemSelected( i->second, false );
    }
    tree_->setItemSelected( root_, false );
}

//-------------------------------------------------------------------------------
void WorkspaceTreeWidget::Clear()
{
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    // clear
    tree_->clear();
    moleculeItemMap_.clear();
    // recreate root
    root_ = new QTreeWidgetItem( tree_ );
    root_->setText( 0, tr( "Workspace" ) );
    QVariantMap data;
    data[ ITEM_TYPE ] = int( ITEM_TYPE_MOLECULE );
    data[ ITEM_DATA ] = int();
    root_->setData( 0, Qt::UserRole, data );
}

//-------------------------------------------------------------------------------
QTreeWidgetItem* WorkspaceTreeWidget::GetMoleculeItem( QTreeWidgetItem* item ) const
{
    if( !item ) return 0;
    QVariantMap data = item->data( 0, Qt::UserRole ).toMap();
    switch( data[ ITEM_TYPE ].toInt() )
    {
        case ITEM_TYPE_MOLECULES:
            return 0;
            break;
        case ITEM_TYPE_MOLECULE:
            return item;
            break;
        default:
            return GetMoleculeItem( item->parent() );
            break;
    }
    return 0;
}

//-------------------------------------------------------------------------------
void WorkspaceTreeWidget::UpdateMoleculeItem( MolekelData::IndexType id )
{
    ResourceHandler< bool > rh( updatingGUI_, true, false );
    assert( moleculeItemMap_.find( id ) != moleculeItemMap_.end() && "Invalid index" );
    MolekelMolecule* mol = mw_.GetMolecule( id );
    assert( mol && "NULL molecule" );
    UnselectMolecules();
    QTreeWidgetItem* item = moleculeItemMap_[ id ];
    item->takeChildren();
    if( mol->GetNumberOfOrbitalSurfaces() > 0 )
    {
        QTreeWidgetItem* orbitalsItem = new QTreeWidgetItem( item );
        orbitalsItem->setText( 0, tr( "Orbitals" ) );
        QVariantMap itemData;
        itemData[ ITEM_TYPE ] = int( ITEM_TYPE_ORBITALS );
        itemData[ ITEM_DATA ] = mol->GetNumberOfOrbitalSurfaces();
        orbitalsItem->setData( 0, Qt::UserRole, itemData );
        for( int i = 0; i != mol->GetNumberOfOrbitals(); ++i )
        {
            if( mol->HasOrbitalSurface( i ) )
            {
                QTreeWidgetItem* oi = new QTreeWidgetItem( orbitalsItem );
                oi->setText( 0, tr( "Orbital %1" ).arg( i + 1 ) );
                oi->setCheckState( 0, mol->GetOrbitalSurfaceVisibility( i ) ?
                                   Qt::Checked : Qt::Unchecked );
                QVariantMap data;
                data[ ITEM_TYPE ] = int( ITEM_TYPE_ORBITAL );
                data[ ITEM_DATA ] = i;
                oi->setData( 0, Qt::UserRole, data );
            }
        }
    }
    if( mol->HasElectronDensitySurface() )
    {
        QTreeWidgetItem* eldensItem = new QTreeWidgetItem( item );
        eldensItem->setText( 0, tr( "Electron Density Surface" ) );
        eldensItem->setCheckState( 0, mol->GetElDensSurfaceVisibility() ?
                                   Qt::Checked : Qt::Unchecked );
        QVariantMap itemData;
        itemData[ ITEM_TYPE ] = int( ITEM_TYPE_EL_DENS_SURFACE );
        itemData[ ITEM_DATA ] = int();
        eldensItem->setData( 0, Qt::UserRole, itemData );
    }
    if( mol->HasGridDataSurface() )
    {
        if( mol->GetFormat() != "t41" )
        {
            QTreeWidgetItem* gdItem = new QTreeWidgetItem( item );
            gdItem->setText( 0, tr( "Grid Data Surface" ) );
            gdItem->setCheckState( 0, mol->GetGridDataSurfaceVisibility() ?
                                   Qt::Checked : Qt::Unchecked );
            QVariantMap itemData;
            itemData[ ITEM_TYPE ] = int( ITEM_TYPE_GRID_DATA_SURFACE );
            itemData[ ITEM_DATA ] = int();
            gdItem->setData( 0, Qt::UserRole, itemData );
        }
        else
        {
            typedef MolekelMolecule::SurfaceLabels Labels;
            const Labels labels = mol->GetGridSurfaceLabels();
            Labels::const_iterator i = labels.begin();
            Labels::const_iterator end = labels.end();
            for( ; i != end; ++i )
            {
                QTreeWidgetItem* gdItem = new QTreeWidgetItem( item );
                gdItem->setText( 0, tr( ( *i ).c_str() ) );
                gdItem->setCheckState( 0, mol->GetGridDataSurfaceVisibility( *i ) ?
                                       Qt::Checked : Qt::Unchecked );
                QVariantMap itemData;
                itemData[ ITEM_TYPE ] = int( ITEM_TYPE_GRID_DATA_SURFACE );
                itemData[ ITEM_DATA ] = QString( ( *i ).c_str() );
                gdItem->setData( 0, Qt::UserRole, itemData );
            }
        }
    }
    if( mol->HasSAS() )
    {
        QTreeWidgetItem* gdItem = new QTreeWidgetItem( item );
        gdItem->setText( 0, tr( "Solvent Accessible Surface" ) );
        gdItem->setCheckState( 0, mol->GetSASVisibility() ?
                               Qt::Checked : Qt::Unchecked );
        QVariantMap itemData;
        itemData[ ITEM_TYPE ] = int( ITEM_TYPE_SAS );
        itemData[ ITEM_DATA ] = int();
        gdItem->setData( 0, Qt::UserRole, itemData );
    }
    if( mol->HasSES() )
    {
        QTreeWidgetItem* gdItem = new QTreeWidgetItem( item );
        gdItem->setText( 0, tr( "Solvent Excluded Surface" ) );
        gdItem->setCheckState( 0, mol->GetSESVisibility() ?
                               Qt::Checked : Qt::Unchecked );
        QVariantMap itemData;
        itemData[ ITEM_TYPE ] = int( ITEM_TYPE_SES );
        itemData[ ITEM_DATA ] = int();
        gdItem->setData( 0, Qt::UserRole, itemData );
    }
    if( mol->HasSESMS() )
    {
        QTreeWidgetItem* gdItem = new QTreeWidgetItem( item );
        gdItem->setText( 0, tr( "(MSMS) Solvent Excluded Surface" ) );
        gdItem->setCheckState( 0, mol->GetSESMSVisibility() ?
                               Qt::Checked : Qt::Unchecked );
        QVariantMap itemData;
        itemData[ ITEM_TYPE ] = int( ITEM_TYPE_SES_MSMS );
        itemData[ ITEM_DATA ] = int();
        gdItem->setData( 0, Qt::UserRole, itemData );
    }
}

//-------------------------------------------------------------------------------

namespace
{
    template < class T > void Log( const QString& msg, T data )
    {
        QMessageBox::information( 0, "DELETE", QString( "%1 %2" ).arg( msg ).arg( data ) );
    }
    void Log( const QString& msg )
    {
        QMessageBox::information( 0, "DELETE", msg );
    }
}


void WorkspaceTreeWidget::DeleteItemSlot()
{
    if( menuItem_ == 0 ) return;
    QVariantMap data = menuItem_->data( 0, Qt::UserRole ).toMap();

    // should never be the case since action not triggered for top level item
    if( data[ ITEM_TYPE ].toInt() == ITEM_TYPE_MOLECULES )
    {
        mw_.Clear();
    }
    // molecule
    else if( data[ ITEM_TYPE ].toInt() == ITEM_TYPE_MOLECULE )
    {
        mw_.DeleteMolecule();
    }
    else
    {   
        QTreeWidgetItem* mi = GetMoleculeItem( menuItem_ );
        if( !mi ) return;
        QVariantMap mdata = mi->data( 0, Qt::UserRole ).toMap();
        const MolekelData::IndexType mid = MolekelData::IndexType( mdata[ ITEM_DATA ].toUInt() );
        MolekelMolecule* mol = mw_.GetMolecule( mid );
        if( !mol ) return;
        switch( data[ ITEM_TYPE ].toInt() )
        {
        case ITEM_TYPE_ORBITAL:
            mol->RemoveOrbitalSurface( data[ ITEM_DATA ].toInt() );
            break;
        case ITEM_TYPE_EL_DENS_SURFACE:
            mol->RemoveElectronDensitySurface();
            break;
        case ITEM_TYPE_GRID_DATA_SURFACE:
            mol->RemoveGridDataSurface();
            break;
        case ITEM_TYPE_SAS:
            mol->RemoveSAS();
            break;
        case ITEM_TYPE_SES:
            mol->RemoveSES();
            break;
        case ITEM_TYPE_SES_MSMS:
            mol->RemoveSESMS();
            break;
        case ITEM_TYPE_SPIN_DENS_SURFACE:
            mol->RemoveSpinDensitySurface();
            break;
        case ITEM_TYPE_ORBITALS:
            QMessageBox::information( this, "Delete orbitals", "Please delete one orbital at a time" );
            break;
        default:
            break;
        }
        UpdateMoleculeItem( mid );
    }
    mw_.Refresh();
}