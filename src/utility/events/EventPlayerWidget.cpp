//
// Copyright (c) 2006, 2007, 2008, 2009 - Ugo Varetto
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

#include <QCoreApplication>
#include <QEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QList>
#include <QSettings>
#include <QDir>
#include <QCheckBox>

#include <fstream>

#include "EventPlayerWidget.h"
#include "EventReader.h"
#include "../qtfileutils.h"

//------------------------------------------------------------------------------
void EventPlayerWidget::Play()
{
	if( filePath_->text().isEmpty() )
    {
    	QMessageBox::critical( this, "I/O Error", "Select input file" + filePath_->text() );
        return;
    }

    std::ifstream is( filePath_->text().toStdString().c_str() );
    if( !is.is_open() )
    {
        QMessageBox::critical( this, "I/O Error", "Cannot read from file" + filePath_->text() );
        return;
    }

    UpdateSettings();

    EventReader< std::istream > er( &is );
    QList< EventInfo > evt = er.Read();
    is.close();
    eventPlayer_.SetEvents( evt );
    eventPlayer_.AsynchPlay();
}

//------------------------------------------------------------------------------
void EventPlayerWidget::Stop()
{
    eventPlayer_.Pause();
}

//------------------------------------------------------------------------------
void EventPlayerWidget::SelectFile()
{
    QSettings settings;
    const QString dir = settings.value( settingsKey_, "" ).toString();
    const QString file = GetOpenFileName( this, "Select input file",
                                          dir.isEmpty() ? QCoreApplication::applicationDirPath() +
                                          "/../" : dir );
    if( file.isEmpty() ) return;
    filePath_->setText( file );
    UpdateSettings();
}

//------------------------------------------------------------------------------
void EventPlayerWidget::Advanced()
{
    if( advancedDlg_.isVisible() ) return;
    advancedDlg_.show();
}

//------------------------------------------------------------------------------
void EventPlayerWidget::UpdateSettings() const
{
    if( !settingsKey_.isEmpty() )
    {
        QSettings settings;
        settings.setValue( settingsKey_, DirPath( filePath_->text() ) );
    }
}

