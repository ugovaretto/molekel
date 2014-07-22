#ifndef SASDIALOG_H_
#define SASDIALOG_H_
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
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QThread>

#include "../MainWindow.h"
#include "../widgets/SasWidget.h"
#include "../MolekelMolecule.h"

/// GUI control to generate SAS and Van der Waals surfaces.
/// The dialog box interacts with SasWidget.
class SasDialog : public QDialog
{
    Q_OBJECT

public slots:

    /// Called when computation thread started.
    void ThreadStartedSlot()
    {
        removeButton_->setEnabled( false );
        mw_->DisplayStatusMessage( "Generating data..." );
        //generateButton_->setText( "Stop" );
    }
    /// Called when computation thread done.
    void ThreadFinishedSlot()
    {
        if( mol_->HasSAS() ) removeButton_->setEnabled( true );
        else removeButton_->setEnabled( false );
        generateButton_->setText( "Compute" );
        if( mol_->SASComputationStopped() )
        {
            mw_->DisplayStatusMessage( "Stopped" );
        }
        sasWidget_->SetRenderingStyle( mol_->GetSASRenderingStyle() ) ;
        mw_->Refresh();

    }
    /// Generate SAS surface and color code with MEP if MEP checkbox is checked.
    /// Actual computation is performed in thread started from within this method.
    void GenerateSlot()
    {
        // disable multithreading: issues on linux
        ThreadStartedSlot();
        ComputeSAS();
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
    /// Remove SAS surface. This slot is never invoked during computation
    /// since the "Remove" button is disabled when the computation thread
    /// starts.
    void RemoveSlot()
    {
        mol_->RemoveSAS();
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
        mol_->SetSASRenderingStyle( rs );
        mw_->Refresh();
    }

public:
    /// Constructor.
    SasDialog( MainWindow* mw, MolekelMolecule* m, QWidget* parent = 0 ) : QDialog( parent ), mw_( mw ), mol_( m )
    {
        QVBoxLayout* mainLayout = new QVBoxLayout;
        sasWidget_ = new SasWidget( mol_ );
        mainLayout->addWidget( sasWidget_ );
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
        if( mol_->HasSAS() ) removeButton_->setEnabled( true );
        else removeButton_->setEnabled( false );
        connect( sasWidget_, SIGNAL( RenderingStyleChanged( MolekelMolecule::RenderingStyle ) ),
                 this, SLOT( RenderingStyleChangedSlot( MolekelMolecule::RenderingStyle ) ) );
        thread_ = new DataGenerationThread( this );
        connect( thread_, SIGNAL( started() ), this, SLOT( ThreadStartedSlot() ) );
        connect( thread_, SIGNAL( finished() ), this, SLOT( ThreadFinishedSlot() ) );
        sasWidget_->SetRenderingStyle( mol_->GetSASRenderingStyle() ) ;
    }

    /// Destructor: deletes thread.
    ~SasDialog() { delete thread_; }

    /// Performs computation of SAS.
    void ComputeSAS()
    {
        /// @warning progress report currently disabled: updating the status bar from a separate
        /// thread causes Qt to (sometimes) crash.
        mol_->AddSAS( sasWidget_->GetRadius(), sasWidget_->GetStep(),
                      MainWindow::ProgressCallback, mw_ );

        if( mol_->HasSAS() )
        {
            if( sasWidget_->GetRenderingStyle() != MolekelMolecule::INVALID_RENDERING_STYLE )
            {
                 mol_->SetSASRenderingStyle( sasWidget_->GetRenderingStyle() );
            }
            if( sasWidget_->ComputeMep() )
            {
                mol_->MapMEPOnSAS( sasWidget_->GetStep(), 0, !mol_->CanComputeMEP() );
                mw_->SetMEPScalarBarLUT( mol_->GetSASLUT() );
            }
        }
    }
private:
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
        mol_->StopSASComputation();
    }

    /// Reference to MainWindow instance; used to invoke Refresh().
    MainWindow* mw_;
    /// Reference to Molecule instance whose SAS is being computed.
    MolekelMolecule* mol_;
    /// Owned SAS widget hosting the UI control used for interaction.
    SasWidget* sasWidget_;
    /// Generate button: start computation upon release.
    QPushButton* generateButton_;
    /// Remove button: remove surface upon release.
    QPushButton* removeButton_;
    /// Close button: close dialog upon release.
    QPushButton* closeButton_;

    /// Thread object used to compute SAS in an interruptible
    /// separate thread.
    class DataGenerationThread : public QThread
    {
    public:
       DataGenerationThread( SasDialog* parent )
                              : QThread( parent ) {}
       void run()
       {
            SasDialog* pd = dynamic_cast< SasDialog* >( parent() );
            assert( pd );
            pd->ComputeSAS();
       }
    };
    /// Thread where actual computation is performed; thread is created in
    /// constructor and deleted in destructor.
    DataGenerationThread* thread_;
};


#endif /*SASDIALOG_H_*/
