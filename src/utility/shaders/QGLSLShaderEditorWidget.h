#ifndef QGLSLSHADEREDITORWIDGET_H_
#define QGLSLSHADEREDITORWIDGET_H_
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

#include <GL/glew.h>
#include <GL/gl.h> // need GLuint, GLenum typedefs

#include <exception>
#include <cassert>

#include <QWidget>
#include <QString>
#include <QVector>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpinBox>

class QGridLayout;

/// Widget to view and edit float GLSL parameters.
/// Each time a GLSL shader program handler is passed to an instance of this
/// widget a new UI is generated reading parameter names and values
/// from OpenGL.
/// It is a good idea to put instances of this widget inside a scroll area.
/// e.g.
/// @code
/// ...
/// GLuint shaderProgram;
/// ...
/// try
/// {
///   QGLSLShaderEditorWidget* se = new QGLSLShaderEditorWidget;
///   se->SetGLSLProgram( shaderProgram );
///   sa_ = new QScrollArea; // data member
///   sa_->setWidget( se );
///   mainLayout_->addWidget( sa_ );
///   connect( se, SIGNAL( ValueChanged() ), this, SLOT( Update() ) );
/// }
/// catch( const QGLSLShaderEditorWidget::Exception& e )
/// {
///   QMessageBox::critical( this, "GLSL Error", e.GetMessage() );
/// }
/// @endcode
class QGLSLShaderEditorWidget : public QWidget
{
    Q_OBJECT

signals:
    /// Emitted when a value is changed. Used to refresh the GL window
    void ValueChanged();

private slots:
    /// Called whenever a double value changes.
    void DoubleValueChangedSlot( double );
    /// Called whenever a double value changes.
    void IntValueChangedSlot( int );
    /// Called when color button pressed.
    void ColorSlot();
    /// Called whenever a boolean value changes.
    void StateChangedSlot( int );
public:

    /// Exception derived from std::exception thrown by instances
    /// of this class in case of errors.
    class Exception : std::exception
    {
        /// Error message.
        QString msg_;
     public:
        /// Constructor, assigns value to error message.
        Exception( const QString& msg ) : msg_( msg ) {}
        /// Overridden method.
        const char* what() const throw() { return msg_.toAscii().constData(); }
        /// Convenience method returning a reference to the message QString instance.
        const QString& GetMessage() const { return msg_; }
        /// Required overridden destructor.
        ~Exception() throw() {}
    };
    /// Constructor.
    QGLSLShaderEditorWidget( QWidget* parent = 0 ) : QWidget( parent ) {}
    /// Set GLSL program handler; UI is rebuilt each time this method is called.
    void SetGLSLProgram( unsigned int );
    /// Returns the last GLSL program handler passed to this widget.
    GLuint GetGLSLProgram() const { return shaderProgram_; }
    /// Saves parameters to file; opens a file dialog to let the user choose
    /// the file name.
    /// @parameter dir default directory passed to QFileDialog::getSaveFileName().
    void SaveParameters( const QString& dir = QString() );
    /// Saves parameters to file.
    void SaveParametersToFile( const char* fileName );
    /// Destructor: resets GLSL shader program. @note this may not be required.
    ~QGLSLShaderEditorWidget();

private:

    struct ShaderParamInfo; // forward declaration

    //@{ Add individual UI controls to edit individual shader parameter values
    void AddIntUIControl( ShaderParamInfo& si, int value, QGridLayout* layout, int row, int column );
    void AddBoolUIControl( ShaderParamInfo& si, bool value, QGridLayout* layout, int row, int column );
    void AddFloatUIControl( ShaderParamInfo& si, float value, QGridLayout* layout, int row, int column );
    //@}

    /// Adds a row or UI controls for a specific parameter.
    /// @param layout grid layout the row is added to
    /// @param type GLSL parameter type (GL_FLOAT, GL_FLOAT_VEC2...)
    /// @param name parameter name
    /// @param rowIndex zero based row index
    void AddRow( QGridLayout* layout, GLenum type, const QString& name, int rowIndex );
    /// Check program: check if the program is valid.
    /// @param p GLSL shader program.
    /// @throws QGLSLShaderEditorWidget::Exception if program invalid.
    void CheckProgram( GLuint p ) const;

    /// QWidget container; used to store QWidget derived controls together with
    /// parameter types inside containers.
    class UIControlWrapper
    {
        /// Reference to widget.
        QWidget* w_;
        /// OpenGL type associated with widget.
        GLenum type_;
     public:
        /// Standard constructor; required to create default constructed collections.
        UIControlWrapper() : w_( 0 ), type_( GLenum() ) {}
        /// Actual constructor used from client code.
        UIControlWrapper( QWidget* w, GLenum t ) : w_( w ), type_( t ) {}
        /// Set float value. Succeeds iff the control is a QDoubleSpinBox and
        /// type == GL_FLOAT.
        void SetFloatValue( float v )
        {
            assert( type_ == GL_FLOAT );
            assert( qobject_cast< QDoubleSpinBox* >( w_ ) );
            qobject_cast< QDoubleSpinBox* >( w_ )->setValue( v );
        }
        /// Set boolean value. Succeeds iff the control is a QCheckBox and
        /// type == GL_BOOL.
        void SetBooleanValue( bool v )
        {
            //assert( type_ == GL_BOOL );
            assert( qobject_cast< QCheckBox* >( w_ ) );
            qobject_cast< QCheckBox* >( w_ )->setCheckState( v  ? Qt::Checked : Qt::Unchecked );
        }
        /// Set int value. Succeeds iff the control is a QSpinBox and
        /// type == GL_INT.
        void SetIntValue( int v )
        {
            assert( type_ == GL_INT );
            assert( qobject_cast< QSpinBox* >( w_ ) );
            qobject_cast< QSpinBox* >( w_ )->setValue( v );
        }
        /// Return float value. Succeeds iff the control is a QDoubleSpinBox and
        /// type == GL_FLOAT.
        float GetFloatValue() const
        {
            assert( type_ == GL_FLOAT );
            assert( qobject_cast< const QDoubleSpinBox* >( w_ ) );
            return float( qobject_cast< const QDoubleSpinBox* >( w_ )->value() );
        }
        /// Return boolean value. Succeeds iff the control is a QCheckBox and
        /// type == GL_BOOL.
        bool GetBooleanValue() const
        {
            //assert( type_ == GL_BOOL );
            assert( qobject_cast< const QCheckBox* >( w_ ) );
            return qobject_cast< const QCheckBox* >( w_ )->checkState() == Qt::Checked;
        }
        /// Return int value. Succeeds iff the control is a QSpinBox and
        /// type == GL_INT.
        int GetIntValue() const
        {
            assert( type_ == GL_INT );
            assert( qobject_cast< const QSpinBox* >( w_ ) );
            return qobject_cast< const QSpinBox* >( w_ )->value();
        }
    };

    typedef UIControlWrapper CW;

private:

    ///Utility class used to record information about each parameter.
    struct ShaderParamInfo
    {
        /// OpenGL type.
        GLenum type;
        /// GLSL parameter name.
        QString name;
        /// Sequence of UI controls used to edit the GLSL parameter.
        QVector< UIControlWrapper > uiControls; // for supporting multiple control types
                                   // use a GUIControlWrapper containing
                                   // - parameter type info
                                   // - pointer to GUI widget
                                   // - multiple Get<type info>Value() (one per type)

    };

    typedef QVector< ShaderParamInfo > ShaderValues;

    ///Current GLSL shader program.
    GLuint shaderProgram_;
    ///Vector of currently dispayed parameters; this list is used to change parameter
    ///values: each time a parameter is added to this vector its index is stored in the QWidget
    ///added to the UI.
    ///Whenever a value changes a slot is invoked; the slot selects the parameter to change by reading
    ///the parameter index from the sender QWidget.
    ShaderValues shaderValues_;
};

#endif /*QGLSLSHADEREDITORWIDGET_H_*/
