#ifndef SESDIALOG_H_
#define SESDIALOG_H_
//
// Molekel - Molecular Visualization Program
// Copyright (C) 2006, 2007, 2008 Swiss National Supercomputing Centre (CSCS)
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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QThread>
#include <QMessageBox>
#include <QFile>

#include "../MainWindow.h"
#include "../widgets/SesWidget.h"
#include "../MolekelMolecule.h"
#include "../utility/Timer.h"

/// GUI control to generate Solvent Excluded (Connolly) Surface.
/// The dialog box interacts with SesWidget.
/// Multithreading is currently disabled.
class SesDialog : public QDialog
{
    Q_OBJECT

public slots:


    /// MSMS path changed.
    void MSMSExecutablePathChangedSlot( const QString& path )
    {
        mw_->SetMSMSExecutablePath( path );
    }
    /// Called when computation thread started.
    void ThreadStartedSlot()
    {
        removeButton_->setEnabled( false );
        mw_->DisplayStatusMessage( "Generating data..." );
        // disable: no multithreading yet
        //generateButton_->setText( "Stop" );
        timer_.Start();
    }
    /// Called when computation thread done.
    void ThreadFinishedSlot()
    {
        timer_.Stop();
        if( !useMSMS_ && mol_->HasSES() ||
            useMSMS_ && mol_->HasSESMS() ) removeButton_->setEnabled( true );
        else removeButton_->setEnabled( false );
        generateButton_->setText( "Compute" );
        if( !useMSMS_ && mol_->SESComputationStopped() ||
            useMSMS_ && mol_->SESMSComputationStopped() )
        {
            mw_->DisplayStatusMessage( "Stopped" );
        }
        else
        {
            mw_->DisplayStatusMessage( QString( "Generated SES - elapsed time %1s" )
                                              .arg( timer_.GetElapsedTime() ) );
        }
        if( !useMSMS_ ) sesWidget_->SetRenderingStyle( mol_->GetSESRenderingStyle() );
        else sesWidget_->SetRenderingStyle( mol_->GetSESMSRenderingStyle() );
        mw_->Refresh();

    }
    /// Generate SAS surface and color code with MEP if MEP checkbox is checked.
    /// Actual computation is performed in thread started from within this method.
    void GenerateSlot()
    {
        // no multithreading for now
        ThreadStartedSlot(); // short-circuit to methods as if thread actually executed
        ComputeSES();
        ThreadFinishedSlot();
        return;

        if( thread_->isRunning() )
        {
            StopComputation();
            thread_->wait();
            return;
        }
        thread_->start();
    }
    /// Remove SES surface. This slot is never invoked during computation
    /// since the "Remove" button is disabled when the computation thread
    /// starts.
    void RemoveSlot()
    {
        if( useMSMS_ ) mol_->RemoveSESMS();
        else( mol_->RemoveSES() );
        mw_->Refresh();
        removeButton_->setEnabled( false );
    }
    /// Close dialog.
    void CloseSlot()
    {
        done( QDialog::Rejected );
    }
    /// Change surface rendering style: this does not require any computation and
    /// the changes are applied immediately.
    void RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle rs )
    {
        if( thread_->isRunning() ) thread_->wait();
        if( !sesWidget_->UseMSMS() ) mol_->SetSESRenderingStyle( rs );
        else mol_->SetSESMSRenderingStyle( rs );
        mw_->Refresh();
    }

public:
    /// Constructor.
    SesDialog( MainWindow* mw, MolekelMolecule* m, QWidget* parent = 0 ) :
        QDialog( parent ), mw_( mw ), mol_( m )
    {
        QVBoxLayout* mainLayout = new QVBoxLayout;
        sesWidget_ = new SesWidget( mol_, mw_->GetMSMSExecutablePath() );
        mainLayout->addWidget( sesWidget_ );
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        generateButton_ = new QPushButton( "Compute" );
        removeButton_ = new QPushButton( "Remove" );
        closeButton_ = new QPushButton( "Close" );
        connect( generateButton_, SIGNAL( released() ), this, SLOT( GenerateSlot() ) );
        connect( removeButton_,   SIGNAL( released() ), this, SLOT( RemoveSlot()   ) );
        connect( closeButton_,   SIGNAL( released() ), this, SLOT( CloseSlot()   ) );
        buttonLayout->addWidget( generateButton_ );
        buttonLayout->addWidget( removeButton_ );
        buttonLayout->addWidget( closeButton_ );
        mainLayout->addItem( buttonLayout );
        this->setLayout( mainLayout );
        if( mol_->HasSES() || mol_->HasSESMS() ) removeButton_->setEnabled( true );
        else removeButton_->setEnabled( false );
        useMSMS_ = mol_->HasSESMS();
        connect( sesWidget_, SIGNAL( RenderingStyleChanged( MolekelMolecule::RenderingStyle ) ),
                 this, SLOT( RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle ) ) );
        connect( sesWidget_, SIGNAL( MSMSExecutablePathChanged( const QString& ) ),
                 this, SLOT( MSMSExecutablePathChangedSlot( const QString& ) ) );
        thread_ = new DataGenerationThread( this );
        connect( thread_, SIGNAL( started() ), this, SLOT( ThreadStartedSlot() ) );
        connect( thread_, SIGNAL( finished() ), this, SLOT( ThreadFinishedSlot() ) );
        sesWidget_->SetRenderingStyle( mol_->GetSESMSRenderingStyle() ) ;
    }

    /// Destructor: deletes thread.
    ~SesDialog() { delete thread_; }

    /// Performs computation of SAS.
    void ComputeSES()
    {
        if( !sesWidget_->UseMSMS() ) // internal SES computation
        {
            useMSMS_ = false;
            mol_->AddSES( sesWidget_->GetRadius(), sesWidget_->GetStep() );

            if( mol_->HasSES() )
            {
                if( sesWidget_->GetRenderingStyle() != MolekelMolecule::INVALID_RENDERING_STYLE )
                {
                     mol_->SetSESRenderingStyle( sesWidget_->GetRenderingStyle() );
                }
                if( sesWidget_->ComputeMep() )
                {
                    mol_->MapMEPOnSES( sesWidget_->GetStep(), 0 );
                    mw_->SetMEPScalarBarLUT( mol_->GetSESLUT() );
                }
            }
        }
        else // use MSMS
        {

            if( !CheckExecutable( mw_->GetMSMSExecutablePath() ) )
            {
                QMessageBox::critical( this, "SES Generation Error",
                                       "Cannot execute file " +
                                        mw_->GetMSMSExecutablePath() );
                mol_->StopSESMSComputation();
                return;
            }
            useMSMS_ = true;
            try
            {
                mol_->AddSESMS( sesWidget_->GetRadius(),
                                sesWidget_->GetStep(),
                                mw_->GetMSMSExecutablePath().toStdString() );
                if( mol_->HasSESMS() )
                {
                    if( sesWidget_->GetRenderingStyle() != MolekelMolecule::INVALID_RENDERING_STYLE )
                    {
                         mol_->SetSESMSRenderingStyle( sesWidget_->GetRenderingStyle() );
                    }
                    if( sesWidget_->ComputeMep() )
                    {
                        mol_->MapMEPOnSESMS( sesWidget_->GetStep(), 0, !mol_->CanComputeMEP() );
                        mw_->SetMEPScalarBarLUT( mol_->GetSESMSLUT() );
                    }
                }
            }
            catch( const std::exception& e )
            {
                QMessageBox::critical( this, "SES Generation Error", e.what() );
            }
        }
    }

private:

    /// Check file permissions to verify that file is executable by current user.
    bool CheckExecutable( const QString& execPath )
    {
        if( QFile::exists( execPath ) &&
            ( QFile::permissions( execPath ) & QFile::ExeUser ) ) return true;

        return false;
    }

    /// Overridden method: waits for thread completion and exits.
    void done( int r )
    {
        if( thread_->isRunning() )
        {
            StopComputation();
            thread_->wait();
        }
        QDialog::done( r );
    }

    /// Stop data generation.
    void StopComputation()
    {
        mol_->StopSESComputation();
    }

    /// Timer.
    StopWatch timer_;
    /// Reference to MainWindow instance; used to invoke Refresh().
    MainWindow* mw_;
    /// Reference to Molecule instance whose SAS is being computed.
    MolekelMolecule* mol_;
    /// Owned SES widget hosting the UI control used for interaction.
    SesWidget* sesWidget_;
    /// Generate button: start computation upon release.
    QPushButton* generateButton_;
    /// Remove button: remove surface upon release.
    QPushButton* removeButton_;
    /// Close button: close dialog upon release.
    QPushButton* closeButton_;

    /// Thread object used to compute SES in an interruptible
    /// separate thread.
    class DataGenerationThread : public QThread
    {
    public:
       DataGenerationThread( SesDialog* parent )
                              : QThread( parent ) {}
       void run()
       {
            SesDialog* pd = dynamic_cast< SesDialog* >( parent() );
            assert( pd );
            pd->ComputeSES();
       }
    };
    /// Thread where actual computation is performed; thread is created in
    /// constructor and deleted in destructor.
    DataGenerationThread* thread_;

    ///
    bool useMSMS_;
};


#endif /*SESDIALOG_H_*/
