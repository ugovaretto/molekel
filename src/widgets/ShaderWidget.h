#ifndef SHADERWIDGET_H_
#define SHADERWIDGET_H_
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


#include <cassert>

#include <string>
#include <exception>

#include <QWidget>
#include <QFileDialog>
#include <QLabel>
#include <QGridLayout>
#include <QPushButton>
#include <QString>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegExp>


#include "../MainWindow.h"
#include "../MolekelMolecule.h"
#include "../utility/shaders/QGLSLShaderEditorWidget.h"
#include "../utility/qtfileutils.h"

//
// --------------------------------------------
//|  _______________     ____________________  |
//| |_Vertex Shader_|   |____________________| |
//|  _________________   ____________________  |
//| |_Fragment Shader_| |____________________| |
//|  ____________        ____________________  |
//| |_Parameters_|      |____________________| |
//|                                            |
//|    _______       ________        ______    |
//|   |_Apply_|     |_Remove_|      |_Save_|   |
//|  ________________________________________  |
//| |                                      |^| |
//| |                                      | | |
//| |                                      | | |
//| |                                      | | |
//| |______________________________________|v| |
//|                                            |
// --------------------------------------------
//
/// Shader widget: adds a QGLSLShaderEditoWidget into a scroll area and
/// displays edit box to enter vertex, fragment and parameters file paths.
/// Supports loading/saving of shader and parameter files.
/// Parameter files are created when saving to files the parameters displayed
/// in the scroll area.
class ShaderWidget : public QWidget
{
	Q_OBJECT

private slots:

    /// Redraws content of MainWindow's 3D view.
    void Update()
    {
        mw_->Refresh();
    }
    /// Called when 'Vertex Shader File' button pressed.
	void VertSlot()
	{
        try {
    		QString f = GetOpenFileName( this,
    									 "Select GLSL vertex shader file",
    									 mw_->GetShadersDir() );
    		if( f.isEmpty() ) return;
    		vertEdit_->setText( f );
    		mw_->SetShadersDir( DirPath( f ) );
    		mw_->Refresh();
        }
        catch( const std::exception& e )
        {
            QMessageBox::critical( this, "Error Loading Shader", e.what() );
            RemoveSlot();
        }
        catch( ... )
        {
            QMessageBox::critical( this, "Error", "Error Loading Shader" );
            RemoveSlot();
        }
	}

    /// Called when 'Fragment Shader File' button pressed.
	void FragSlot()
	{
        try
        {
    		QString f = GetOpenFileName( this,
    									 "Select GLSL fragment shader file",
    									 mw_->GetShadersDir() );
    		if( f.isEmpty() ) return;
    		fragEdit_->setText( f );
    		mw_->SetShadersDir( DirPath( f ) );
    		mw_->Refresh();
        }
        catch( const std::exception& e )
        {
            QMessageBox::critical( this, "Error Loading Shader", e.what() );
            RemoveSlot();
        }
        catch( ... )
        {
            QMessageBox::critical( this, "Error", "Error Loading Shader" );
            RemoveSlot();
        }

	}

    /// Called when 'Parameters File Button' pressed.
	void ParamsSlot()
	{
        QString f;
        try
        {
    		f = GetOpenFileName( this,
    			  			     "Select shader parameter file",
    							 mw_->GetShadersDir() );
    		if( f.isEmpty() ) return;
            paramsEdit_->setText( f );
            mw_->SetShadersDir( DirPath( f ) );
            mw_->Refresh();
        }
        catch( const std::exception& e )
        {
            QMessageBox::critical( this, "Error Loading Shader", e.what() );
        }
        catch( ... )
        {
            QMessageBox::critical( this, "Error", "Error Loading Shader" );
        }

        if( !vertEdit_->text().size() && f.size() )
        {
            QString s( f );
            s.replace( QRegExp( "\\.[^\\.]+$" ), ".vert" );
            vertEdit_->setText( s );
        }

        if( !fragEdit_->text().size() && f.size() )
        {
            QString s( f );
            s.replace( QRegExp( "\\.[^\\.]+$" ), ".frag" );
            fragEdit_->setText( s );
        }
	}

    /// Called when 'Remove' button pressed.
    void RemoveSlot()
    {
        try
        {
            mol_->DeleteShaderProgram( surfType_ );
            fragEdit_->setText( "" );
            vertEdit_->setText( "" );
            paramsEdit_->setText( "" );
            AddShaderEditor();
            mw_->Refresh();
        }
        catch( const std::exception& e )
        {
            QMessageBox::critical( this, "Error Deleting Shader", e.what() );
        }
        catch( ... )
        {
            QMessageBox::critical( this, "Error", "Error Deleting Shader" );
        }
    }

    /// Called when 'Save Parameters...' button pressed.
    void SaveSlot()
    {
        if( sa_ )
        {
            QGLSLShaderEditorWidget* w = dynamic_cast< QGLSLShaderEditorWidget* >( sa_->widget() );
            if( !w ) return;
            w->SaveParameters();
        }
    }

    /// Called when 'Apply' button pressed.
    void ApplyShaderProgramSlot()
	{
        try
        {
		  MolekelMolecule::ShaderProgram sp( vertEdit_->text().toStdString(),
    										 fragEdit_->text().toStdString(),
    										 paramsEdit_->text().toStdString() );
    	  RemoveSlot();
    	  vertEdit_->setText( sp.vertexShaderFileName.c_str() );
    	  fragEdit_->setText( sp.fragmentShaderFileName.c_str() );
    	  paramsEdit_->setText( sp.parametersFile.c_str() );

          if( !vertEdit_->text().isEmpty() ) mw_->SetShadersDir( DirPath( vertEdit_->text() ) );
          else if( !fragEdit_->text().isEmpty() ) mw_->SetShadersDir( DirPath( fragEdit_->text() ) );

		  mol_->SetShaderProgram( sp, surfType_ );
          if( sp.parametersFile.size() )
          {
          	mol_->SetShaderParametersFromFile( paramsEdit_->text().toStdString().c_str(), surfType_ );
          }
          AddShaderEditor();
          mw_->Refresh();
        }
        catch( const std::exception& e )
        {
            QMessageBox::critical( this, "Error Loading Shader", e.what() );

        }
        catch( ... )
        {
            QMessageBox::critical( this, "Error", "Error Loading Shader" );
        }
	}

public:

    typedef MolekelMolecule::SurfaceType SurfaceType;

    /// Construct instance.
    /// @param mw reference to main window.
    /// @param mol molecule to act on.
    /// @param surf surface type (molecule, SES, SAS...)
    /// @parent widget.
    ShaderWidget( MainWindow* mw,
			      MolekelMolecule* mol,
                  SurfaceType surf,
				  QWidget* parent = 0
				 ) : QWidget( parent ), sa_( 0 ), mw_( mw ), mol_( mol ), surfType_( surf )
	{
        assert( mw );
        assert( mol );
        mainLayout_ = new QVBoxLayout;
		QGridLayout* layout = new QGridLayout;
        layout->setSpacing( 10 );

        const MolekelMolecule::ShaderProgram& sp = mol_->GetShaderProgram( surfType_ );

		QPushButton* pb = 0;
		pb = new QPushButton( "Vertex Shader File" );
		connect( pb, SIGNAL( released() ), this, SLOT( VertSlot() ) );
		layout->addWidget( pb, 0, 0 );
		vertEdit_ = new QLineEdit( sp.vertexShaderFileName.c_str() );
		layout->addWidget( vertEdit_, 0, 1, 1, 2 );

		pb = new QPushButton( "Fragment Shader File" );
		connect( pb, SIGNAL( released() ), this, SLOT( FragSlot() ) );
		layout->addWidget( pb, 1, 0 );
		fragEdit_ = new QLineEdit( sp.fragmentShaderFileName.c_str() );
		layout->addWidget( fragEdit_, 1, 1, 1, 2 );

		pb = new QPushButton( "Parameters File" );
		connect( pb, SIGNAL( released() ), this, SLOT( ParamsSlot() ) );
		layout->addWidget( pb, 2, 0 );
		paramsEdit_ = new QLineEdit( sp.parametersFile.c_str() );
		layout->addWidget( paramsEdit_, 2, 1, 1, 2 );

        layout->addWidget( new QLabel( " " ), 3, 0 );

		pb = new QPushButton( "Apply" );
        connect( pb, SIGNAL( released() ), this, SLOT( ApplyShaderProgramSlot() ) );
        layout->addWidget( pb, 4, 0 );
        pb = new QPushButton( "Remove" );
        connect( pb, SIGNAL( released() ), this, SLOT( RemoveSlot() ) );
        layout->addWidget( pb, 4, 1 );
        pb = new QPushButton( "Save Parameters..." );
        connect( pb, SIGNAL( released() ), this, SLOT( SaveSlot() ) );
        layout->addWidget( pb , 4, 2);

        mainLayout_->addItem( layout );
        AddShaderEditor();
        setLayout( mainLayout_ );
    }

private:

    /// Add shader editor widget to scroll area.
	void AddShaderEditor()
    {

    	if( sa_ )
    	{
            // remove widget inside scroll area and scroll area
    		delete sa_->takeWidget();
    		mainLayout_->removeWidget( sa_ );
    		delete sa_;
            sa_ = 0;
    	}

        if( mol_->GetShaderProgramId( surfType_ ) != 0 )
        {
            // add scroll area and widget
        	QGLSLShaderEditorWidget* se = new QGLSLShaderEditorWidget;
        	se->SetGLSLProgram( mol_->GetShaderProgramId( surfType_ ) );
        	sa_ = new QScrollArea;
        	sa_->setWidget( se );
        	mainLayout_->addWidget( sa_ );
        	connect( se, SIGNAL( ValueChanged() ), this, SLOT( Update() ) );
      	}
    }
private:
    /// Scroll area containing an instance of QGLSLShaderEditorWidget.
    QScrollArea* sa_;
    /// Main window layout.
    QVBoxLayout* mainLayout_;
	/// Reference to main window.
    MainWindow* mw_;
    /// Reference to molecule to act on.
	MolekelMolecule* mol_;
    /// Surface type (molecule, SAS, SES...)
    SurfaceType surfType_;
    /// Vertex shader edit box containing the path to the vertex shader file.
	QLineEdit* vertEdit_;
    /// Fragment shader edit box containing the path to the fragment shader file.
	QLineEdit* fragEdit_;
    /// Parameters edit box containing the path to the shader parameters file.
	QLineEdit* paramsEdit_;
};


#endif /*SHADERWIDGET_H_*/
