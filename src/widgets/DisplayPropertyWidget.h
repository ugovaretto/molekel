#ifndef DISPLAYPROPERTYWIDGET_H_
#define DISPLAYPROPERTYWIDGET_H_
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
#include <QStringList>
#include <QHash>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>

// STD
#include <cassert>

/// Generic Property Widget used to display properties as
/// key:value pairs.
class DisplayPropertyWidget : public QWidget
{
public:
    /// Constructor.
    DisplayPropertyWidget( QStringList propertyNames, QWidget* parent = 0 ) :  table_( 0 )
    {
        QHBoxLayout* mainLayout = new QHBoxLayout;
        table_ = new QTableWidget;
        table_->setColumnCount( 2 );
        table_->setRowCount( propertyNames.size() );
        QStringList sl;
        sl << "Property" << "Value";
        table_->setHorizontalHeaderLabels( sl );
        int row = 0;
        for( QStringList::const_iterator i = propertyNames.begin();
             i != propertyNames.end(); ++i, ++row )
        {
            QTableWidgetItem* propertyItem = new QTableWidgetItem( *i + ':' );
            //QFont f = propertyItem->font();
            //f.setWeight( QFont::Bold );
            //propertyItem->setFont( f );
            table_->setItem( row, 0, propertyItem );
            QTableWidgetItem* valueItem = new QTableWidgetItem( "" );
            table_->setItem( row, 1, valueItem );
            propertyItemMap_[ *i ] = valueItem;
        }
        table_->verticalHeader()->hide();
        table_->setEditTriggers( QAbstractItemView::NoEditTriggers );
        table_->setSelectionMode( QAbstractItemView::NoSelection );
        table_->setShowGrid( false );
        mainLayout->addWidget( table_ );
        this->setLayout( mainLayout );
    }
    /// Sets a property value.
    void SetValue( const QString& property, const QVariant& value )
    {
        assert( propertyItemMap_.contains( property ) && property.toStdString().c_str() );
        propertyItemMap_[ property ]->setText( value.toString() );
    }
    /// Clear property values.
    void ClearValues()
    {
        for( PropertyMapType::iterator i = propertyItemMap_.begin();
             i != propertyItemMap_.end();
             ++i )
        {
            i.value()->setText( "" );
        }
    }
private:
    typedef QHash< QString, QTableWidgetItem* > PropertyMapType;
    /// Reference to MolekelMolecule instance.
    PropertyMapType propertyItemMap_;
    /// Table widget used to display molecule properties.
    QTableWidget* table_;
};

#endif /*DISPLAYPROPERTYWIDGET_H_*/
