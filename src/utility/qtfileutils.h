#ifndef QTFILEUTILS_H_
#define QTFILEUTILS_H_
//
// Copyright (c) 2006, 2007, 2008, 2009 - Ugo Varetto and 
// Swiss National Supercomputing Centre (CSCS)
//
// This source code is free; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or (at your option) any later version.
//
// This source code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this source code; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
// 

#include <QString>
#include <QFileDialog>
#include <QRegExp>

/// Replace separators.
inline QString FixSeparators( QString path )
{
	 path.replace( "\\", "/" ); // replace path separator; replaces "\ " too
	 path.replace( "/ ", "\\ " ); // replace "/ " with "\ " to fix spaces
	 return path;
}



/// File Path to Dir Path.
inline QString DirPath( const QString& filePath )
{
    QString path = FixSeparators( filePath );
	return path.remove( QRegExp( "/[^/]+$" ) ); // remove from last path separator to end
}


/// Mimicks the behavior of QFileDialog::getOpenFileName with a non-native
/// dialog.
inline QString GetOpenFileName( QWidget* parent = 0,
                         const QString& caption = QString(),
                         const QString& dir = QString(),
                         const QString& filter = QString(),
                         QString* selectedFilter = 0,
                         QFileDialog::Options options = 0,
                         const QString& defaultFilter = QString() )
{
    QFileDialog fd( parent, caption, dir, filter );
    fd.setObjectName( caption + " Dialog" );
    fd.setAcceptMode( QFileDialog::AcceptOpen );
    if( !defaultFilter.isEmpty() ) fd.selectFilter( defaultFilter );
    QString fileName;
    if( fd.exec() )
    {
        fileName = FixSeparators( fd.selectedFiles().front() );
        if( selectedFilter != 0 ) *selectedFilter = fd.selectedFilter();
    }
    return fileName;
}

/// Mimicks the behavior of QFileDialog::getSaveFileName with a non-native
/// dialog.
inline QString GetSaveFileName( QWidget* parent = 0,
                                const QString& caption = QString(),
                                const QString& dir = QString(),
                                const QString& filter = QString(),
                                QString* selectedFilter = 0,
                                QFileDialog::Options options = 0,
                                const QString& defaultFilter = QString() )
{
    QFileDialog fd( parent, caption, dir, filter );
    fd.setObjectName( caption + " Dialog" );
    fd.setAcceptMode( QFileDialog::AcceptSave );
    if( !defaultFilter.isEmpty() ) fd.selectFilter( defaultFilter );
    QString fileName;
    if( fd.exec() )
    {
        fileName = fd.selectedFiles().front();
        if( selectedFilter != 0 ) *selectedFilter = fd.selectedFilter();
    }
    return fileName;
}

/// Mimicks the behavior of QFileDialog::getSaveFileName with a non-native
/// dialog.
#include <QSpinBox>
#include <QLabel>
inline QString GetSaveImageFileName( unsigned& magFactor,
                                     QWidget* parent = 0,
                                     const QString& caption = QString(),
                                     const QString& dir = QString(),
                                     const QString& filter = QString(),
                                     QString* selectedFilter = 0,
                                     QFileDialog::Options options = 0,
                                     const QString& defaultFilter = QString() )
{
    QFileDialog fd( parent, caption, dir, filter );

    QLayout* layout = fd.layout();
    QSpinBox* magSpinBox = new QSpinBox;
    magSpinBox->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );
    magSpinBox->setMinimum(  1 );
    magSpinBox->setMaximum( 10 );
    magSpinBox->setValue( 1 );
    if( layout )
    {
        //QHBoxLayout* bl = new QHBoxLayout;
        layout->addWidget( new QLabel( "Scaling " ) );
        layout->addWidget( magSpinBox );
        //layout->addItem( bl );
    }
   
    fd.setObjectName( caption + " Dialog" );
    fd.setAcceptMode( QFileDialog::AcceptSave );
    if( !defaultFilter.isEmpty() ) fd.selectFilter( defaultFilter );
    QString fileName;
    if( fd.exec() )
    {
        fileName = fd.selectedFiles().front();
        if( selectedFilter != 0 ) *selectedFilter = fd.selectedFilter();
    }
    magFactor = unsigned( magSpinBox->value() );
    return fileName;
}

/// Mimicks the behavior of QFileDialog::getExistingDirectory() with
/// non-native dialog.
inline QString GetExistingDirectory( QWidget* parent,
                                     const QString& caption,
                                     const QString& dir )
{
     QFileDialog fd( parent, caption, dir );
     fd.setAcceptMode( QFileDialog::AcceptOpen );
     fd.setFileMode( QFileDialog::DirectoryOnly );
     QString dirName;
     if( fd.exec() ) dirName = fd.directory().absolutePath();
     return dirName;
}


#endif /*QTFILEUTILS_H_*/
