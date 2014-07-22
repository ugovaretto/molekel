#ifndef TIMESTEPWIDGET_H_
#define TIMESTEPWIDGET_H_
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

class QSpinBox;
class MainWindow;
class QString;

//
//-----------------------------
//                 _______ __
// Time Step (ms) |_______|^v| (spinbox)
//-----------------------------
//

/// Used to change the animation time step.
class TimeStepWidget : public QWidget
{
    Q_OBJECT
private slots:
    /// Called when time step changed with arrow buttons.
    void TimeStepIntValueChanged( int );
    /// Called when time step text value changed.
    void TimeStepStringValueChanged( const QString& );
public:
    /// Constructor.
    /// @param mw reference to main window
    /// @param parent widget
    TimeStepWidget( MainWindow* mw, QWidget* parent = 0 );
    //{@ Constants for for animation time step (milliseconds).
    static const int TIMESTEP_MIN = 0;
    static const int TIMESTEP_MAX = 2000;
    static const int TIMESTEP_SINGLE_STEP = 10;
    //@}
private:
    /// Time step.
    QSpinBox* timestepSpinBox_;
    /// Reference to main window
    MainWindow* mw_;
    /// Set to true while GUI is created.
    bool updatingGUI_;
};

#endif /*TIMESTEPWIDGET_H_*/
