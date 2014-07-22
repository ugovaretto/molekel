#ifndef WORKSPACETREEWIDGET_H_
#define WORKSPACETREEWIDGET_H_
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
#include <QMenu>



// STD
#include <map>

#include "../MolekelData.h"

// Forward declarations.
class MainWindow;
class QTreeWidget;
class QVBoxLayout;
class QTreeWidgetItem;
class MolekelMolecule;
class QPoint;


/// Workspace widget: shows a tree representing all objects
/// in 3d scene.
class WorkspaceTreeWidget : public QWidget
{
    Q_OBJECT
public:
    /// Constructor: create layout, tree widget and add
    /// tree to layout.
    WorkspaceTreeWidget( MainWindow& w, QWidget* parent = 0 );
    /// Add molecule to tree.
    /// @param mol pointer to molecule, not stored internally, only
    /// used to extract information from molecule.
    /// @param i molecule index/key in database, stored inside tree widget's items
    void AddMolecule( const MolekelMolecule* mol, MolekelData::IndexType i );
    /// Remove molecule from tree.
    /// @param i index/key of molecule to be removed, tree item associated
    /// with molecule will be removed from tree.
    void RemoveMolecule( MolekelData::IndexType i );
    /// Updates tree with molecule data adding noodes for
    /// molecular orbitals, grid data and electron density surfaces.
    void UpdateMoleculeItem( MolekelData::IndexType i );
    /// Select molecule.
    void SelectMolecule( MolekelData::IndexType i, bool notify = true );
    /// Unselects all molecules.
    void UnselectMolecules();
    /// Clear tree.
    void Clear();
//Event handling
public slots:
    /// Invoked only when item checked/unchecked, after the molecule has
    /// been added to the tree. While a new molecule is being added a
    /// sequence of itemChanged signals is sent to this method;
    /// signals are discarded if updatingGUI_ is true.
    void ItemChangedSlot( QTreeWidgetItem* current, int column );
    /// Invoked when item highlighted.
    void ItemPressedSlot( QTreeWidgetItem* item, int column );
    /// Invoked when context menu event received.
    void ContextMenuSlot( const QPoint& );
    /// Called when action in popup menu triggered.
    void DeleteItemSlot();
//Data
private:
    /// Reference to main window.
    MainWindow& mw_;
    /// Layout.
    QVBoxLayout* layout_;
    /// Tree widget item.
    QTreeWidget* tree_;
    /// Reference to root node.
    QTreeWidgetItem* root_;
    typedef std::map< MolekelData::IndexType, QTreeWidgetItem* >
        MoleculeItemMap;
    /// Molecule Id --> Tree widget pointer map.
    MoleculeItemMap moleculeItemMap_;
    /// Set to true when new node is being added or removed; this is required
    /// to discard ItemChanged signals received in ItemChangedSlot()
    /// when the molecule is being added to the tree.
    bool updatingGUI_;
    /// Returns molecule containing specific QTreeWidgetItem.
    QTreeWidgetItem* GetMoleculeItem( QTreeWidgetItem* ) const;
    /// Item type stored into tree items to recognize what type of object
    /// has been selected.
    enum  { ITEM_TYPE_MOLECULES,
            ITEM_TYPE_MOLECULE,
            ITEM_TYPE_ORBITALS,
            ITEM_TYPE_ORBITAL,
            ITEM_TYPE_EL_DENS_SURFACE,
            ITEM_TYPE_GRID_DATA_SURFACE,
            ITEM_TYPE_SPIN_DENS_SURFACE,
            ITEM_TYPE_SAS,
            ITEM_TYPE_SES,
            ITEM_TYPE_SES_MSMS
           } ItemType;
    /// Item type key in QVariantMap stored in tree item.
    static const QString ITEM_TYPE;
    /// Item value key in QVariantMao storen in tree item.
    static const QString ITEM_DATA;
    /// Widget item selected with right mouse button.
    QTreeWidgetItem* menuItem_;
    /// Pop-up menu; used to delete items.
    QMenu* popupMenu_;
};


#endif /*WORKSPACETREEWIDGET_H_*/
