/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: SQLToolbarActions.cxx,v $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "SQLToolbarActions.h"

#include <QApplication>
#include <QStyle>

#include "vtkMySQLDatabase.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSQLiteDatabase.h"
#include "vtkStringArray.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqView.h"

//-----------------------------------------------------------------------------
SQLToolbarActions::SQLToolbarActions(QObject* p) : QActionGroup(p)
{
  QAction* a = new QAction("Load Table From SQL Database", this);
  this->addAction(a);
  QObject::connect(this, SIGNAL(triggered(QAction*)),
                   this, SLOT(onAction(QAction*)));

  this->UsingMySQL = false;
  this->UsingPostgreSQL = false;
  this->UsingSQLite = false;

  this->SetupDatabaseDialog();
}

//-----------------------------------------------------------------------------
SQLToolbarActions::~SQLToolbarActions()
{
    delete this->DatabaseDialog;
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::onAction(QAction* a)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* sm = core->getServerManagerModel();

  /// Check that we are connected to some server (either builtin or remote).
  if (sm->getNumberOfItems<pqServer*>())
    {
    this->DatabaseDialog->show();
    }
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::SetupDatabaseDialog()
{
  this->CreateDialogUi();
  this->SetupSignalsAndSlots();
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::CreateDialogUi()
{
  this->DatabaseDialog = new QWidget();
  this->DatabaseDialog->setObjectName(QString::fromUtf8("DatabaseDialog"));
  this->DatabaseDialog->resize(400, 375);
  this->DatabaseDialog->setWindowTitle(
    QApplication::translate("DatabaseDialog", "Load Table from Database", 0,
                            QApplication::UnicodeUTF8));

  this->SQLiteLabel = new QLabel(this->DatabaseDialog);
  this->SQLiteLabel->setObjectName(QString::fromUtf8("SQLiteLabel"));
  this->SQLiteLabel->setGeometry(QRect(20, 20, 181, 17));
  this->OpenSQLiteDatabaseButton = new QPushButton(this->DatabaseDialog);
  this->OpenSQLiteDatabaseButton->setObjectName(
    QString::fromUtf8("OpenSQLiteDatabaseButton"));
  this->OpenSQLiteDatabaseButton->setGeometry(QRect(220, 20, 161, 32));
  this->OpenSQLiteDatabaseButton->setText(
    QApplication::translate("DatabaseDialog", "Browse", 0,
                            QApplication::UnicodeUTF8));
  this->SQLiteLabel->setText(
    QApplication::translate("DatabaseDialog", "Open local SQLite database", 0,
                            QApplication::UnicodeUTF8));

  this->MySQLRadioButton = new QRadioButton(this->DatabaseDialog);
  this->MySQLRadioButton->setObjectName(QString::fromUtf8("MySQLRadioButton"));
  this->MySQLRadioButton->setGeometry(QRect(20, 90, 180, 21));
  this->MySQLRadioButton->setText(
    QApplication::translate("DatabaseDialog", "MySQL database", 0,
                            QApplication::UnicodeUTF8));
  this->PostgresRadioButton = new QRadioButton(this->DatabaseDialog);
  this->PostgresRadioButton->setObjectName(
    QString::fromUtf8("PostgresRadioButton"));
  this->PostgresRadioButton->setGeometry(QRect(200, 90, 180, 21));
  this->PostgresRadioButton->setText(
    QApplication::translate("DatabaseDialog", "Postgres database", 0,
                            QApplication::UnicodeUTF8));

  this->HostInput = new QLineEdit(this->DatabaseDialog);
  this->HostInput->setObjectName(QString::fromUtf8("HostInput"));
  this->HostInput->setGeometry(QRect(20, 120, 170, 22));
  this->HostLabel = new QLabel(this->DatabaseDialog);
  this->HostLabel->setObjectName(QString::fromUtf8("HostLabel"));
  this->HostLabel->setGeometry(QRect(20, 145, 100, 17));
  this->HostLabel->setText(
    QApplication::translate("DatabaseDialog", "Host", 0,
                            QApplication::UnicodeUTF8));

  this->DBNameInput = new QLineEdit(this->DatabaseDialog);
  this->DBNameInput->setObjectName(QString::fromUtf8("DBNameInput"));
  this->DBNameInput->setGeometry(QRect(200, 120, 170, 22));
  this->DBNameLabel = new QLabel(this->DatabaseDialog);
  this->DBNameLabel->setObjectName(QString::fromUtf8("DBNameLabel"));
  this->DBNameLabel->setGeometry(QRect(200, 145, 100, 17));
  this->DBNameLabel->setText(
    QApplication::translate("DatabaseDialog", "Database", 0,
                            QApplication::UnicodeUTF8));

  this->UserInput = new QLineEdit(this->DatabaseDialog);
  this->UserInput->setObjectName(QString::fromUtf8("UserInput"));
  this->UserInput->setGeometry(QRect(20, 180, 170, 22));
  this->UserLabel = new QLabel(this->DatabaseDialog);
  this->UserLabel->setObjectName(QString::fromUtf8("UserLabel"));
  this->UserLabel->setGeometry(QRect(20, 205, 100, 17));
  this->UserLabel->setText(
    QApplication::translate("DatabaseDialog", "Username", 0,
                            QApplication::UnicodeUTF8));

  this->PasswordInput = new QLineEdit(this->DatabaseDialog);
  this->PasswordInput->setObjectName(QString::fromUtf8("PasswordInput"));
  this->PasswordInput->setGeometry(QRect(200, 180, 170, 22));
  this->PasswordInput->setEchoMode(QLineEdit::Password);
  this->PasswordLabel = new QLabel(this->DatabaseDialog);
  this->PasswordLabel->setObjectName(QString::fromUtf8("PasswordLabel"));
  this->PasswordLabel->setGeometry(QRect(200, 205, 100, 17));
  this->PasswordLabel->setText(
    QApplication::translate("DatabaseDialog", "Password", 0,
                            QApplication::UnicodeUTF8));

  this->ConnectButton = new QPushButton(this->DatabaseDialog);
  this->ConnectButton->setObjectName(QString::fromUtf8("ConnectButton"));
  this->ConnectButton->setGeometry(QRect(220, 230, 161, 32));
  this->ConnectButton->setText(
    QApplication::translate("DatabaseDialog", "Connect", 0,
                            QApplication::UnicodeUTF8));
  this->ConnectButton->setEnabled(false);

  this->TableLabel = new QLabel(this->DatabaseDialog);
  this->TableLabel->setObjectName(QString::fromUtf8("TableLabel"));
  this->TableLabel->setGeometry(QRect(20, 285, 81, 17));
  this->TableLabel->setText(
    QApplication::translate("DatabaseDialog", "Select table:", 0,
                            QApplication::UnicodeUTF8));
  this->TableComboBox = new QComboBox(this->DatabaseDialog);
  this->TableComboBox->setObjectName(QString::fromUtf8("TableComboBox"));
  this->TableComboBox->setGeometry(QRect(130, 285, 251, 26));

  this->LoadTableButton = new QPushButton(this->DatabaseDialog);
  this->LoadTableButton->setObjectName(QString::fromUtf8("LoadTableButton"));
  this->LoadTableButton->setGeometry(QRect(222, 325, 161, 32));
  this->LoadTableButton->setEnabled(false);
  this->LoadTableButton->setText(
    QApplication::translate("this->DatabaseDialog", "Load Selected Table", 0,
                            QApplication::UnicodeUTF8));
  
  this->DatabaseDialog->hide();
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::SetupSignalsAndSlots()
{
  this->connect(this->OpenSQLiteDatabaseButton, SIGNAL(pressed()),
                this, SLOT(SelectSQLiteDatabaseFile()));

  this->connect(this->HostInput, SIGNAL(textChanged(const QString &)),
                this, SLOT(CheckConnectionParameters()));
  this->connect(this->DBNameInput, SIGNAL(textChanged(const QString &)),
                this, SLOT(CheckConnectionParameters()));
  this->connect(this->UserInput, SIGNAL(textChanged(const QString &)),
                this, SLOT(CheckConnectionParameters()));
  this->connect(this->PasswordInput, SIGNAL(textChanged(const QString &)),
                this, SLOT(CheckConnectionParameters()));
  this->connect(this->ConnectButton, SIGNAL(pressed()),
                this, SLOT(ConnectToDatabase()));
  this->connect(this->LoadTableButton, SIGNAL(pressed()),
                this, SLOT(LoadTable()));
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::SelectSQLiteDatabaseFile()
{
  //ask the user to select a database file
  this->SQLiteDatabaseFile = QFileDialog::getOpenFileName(this->DatabaseDialog,
    tr("Select SQLite Database"), "", tr("SQLite Database Files (*.*)"));

  //bail out now if the user pressed cancel
  if(this->SQLiteDatabaseFile == "")
    {
    return;
    }

  vtkSQLiteDatabase *sqliteDB = vtkSQLiteDatabase::New();
  sqliteDB->SetDatabaseFileName(this->SQLiteDatabaseFile.toStdString().c_str());
  if(sqliteDB->Open(""))
    {
    //if we can open it succesfully, populate the list of table names
    vtkStringArray *tableNames = sqliteDB->GetTables();
    for(int i = 0; i < tableNames->GetNumberOfValues(); i++)
      {
      this->TableComboBox->insertItem(i, QString(tableNames->GetValue(i))); 
      }
    if(tableNames->GetNumberOfValues() > 0)
      {
      this->LoadTableButton->setEnabled(true);
      }
    tableNames->Delete();
    }
  sqliteDB->Close();
  sqliteDB->Delete();
  this->UsingMySQL = false;
  this->UsingPostgreSQL = false;
  this->UsingSQLite = true;
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::CheckConnectionParameters()
{
  if(this->HostInput->text() != "" && this->DBNameInput->text() != "" &&
     this->UserInput->text() != "" && this->PasswordInput->text() != "" &&
       (this->MySQLRadioButton->isChecked() ||
        this->PostgresRadioButton->isChecked()))
    {
    this->ConnectButton->setEnabled(true);
    }
  else
    {
    this->ConnectButton->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::ConnectToDatabase()
{
  if(this->MySQLRadioButton->isChecked())
    {
    this->ConnectToMySQLDatabase();
    }
  else if(this->PostgresRadioButton->isChecked())
    {
    this->ConnectToPostgresDatabase();
    }
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::ConnectToMySQLDatabase()
{
  vtkSmartPointer<vtkMySQLDatabase> mySQLDB =
    vtkSmartPointer<vtkMySQLDatabase>::New();
  mySQLDB->SetHostName(this->HostInput->text().toStdString().c_str());
  mySQLDB->SetDatabaseName(this->DBNameInput->text().toStdString().c_str());
  mySQLDB->SetUser(this->UserInput->text().toStdString().c_str());
  mySQLDB->SetPassword(this->PasswordInput->text().toStdString().c_str());
  if(mySQLDB->Open())
    {
    //if we can open it succesfully, populate the list of table names
    vtkStringArray *tableNames = mySQLDB->GetTables();
    for(int i = 0; i < tableNames->GetNumberOfValues(); i++)
      {
      this->TableComboBox->insertItem(i, QString(tableNames->GetValue(i))); 
      }
    if(tableNames->GetNumberOfValues() > 0)
      {
      this->LoadTableButton->setEnabled(true);
      }
    mySQLDB->Close();
    this->UsingMySQL = true;
    this->UsingPostgreSQL = false;
    this->UsingSQLite = false;
    }
  else
    {
    QMessageBox msgBox;
    msgBox.setText(
      "Unable to connect to the database.  Please check your settings and try again.");
    msgBox.exec();
    }
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::ConnectToPostgresDatabase()
{
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::LoadTable()
{
  //get the name of the requested table
  QString tableName = this->TableComboBox->currentText();

  if(this->UsingMySQL)
    {
    this->LoadMySQLTable(tableName);
    }
  if(this->UsingSQLite)
    {
    this->LoadSQLiteTable(tableName);
    }
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::LoadMySQLTable(QString tableName)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* sm = core->getServerManagerModel();
  pqObjectBuilder* builder = core->getObjectBuilder();

  // just create it on the first server connection
  pqServer* s = sm->getItemAtIndex<pqServer*>(0);

  // make this operation undo-able 
  BEGIN_UNDO_SET("Load vtkTable from MySQL");

  // create the new source
  pqPipelineSource *source =
    builder->createSource("sources", "MySQL Table", s);
  
  //set its properties based on the values the user submitted
  vtkSMPropertyHelper(source->getProxy(), "HostName").Set(
    this->HostInput->text().toStdString().c_str());
  vtkSMPropertyHelper(source->getProxy(), "DatabaseName").Set(
    this->DBNameInput->text().toStdString().c_str());
  vtkSMPropertyHelper(source->getProxy(), "TableName").Set(
    tableName.toStdString().c_str());
  vtkSMPropertyHelper(source->getProxy(), "User").Set(
    this->UserInput->text().toStdString().c_str());
  vtkSMPropertyHelper(source->getProxy(), "Password").Set(
    this->PasswordInput->text().toStdString().c_str());

  source->getProxy()->UpdateVTKObjects();
    source->updatePipeline();

  pqDisplayPolicy* displayPolicy =
    pqApplicationCore::instance()->getDisplayPolicy();
  if (!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  //display the new table
  pqView *activeview = pqActiveObjects::instance().activeView();
  pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
    source->getOutputPort(0), activeview, false);
  if (repr && repr->getView())
    {
    pqView* view = repr->getView();
    view->render();
    }
  
  END_UNDO_SET();
  this->DatabaseDialog->hide();
}

//-----------------------------------------------------------------------------
void SQLToolbarActions::LoadSQLiteTable(QString tableName)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* sm = core->getServerManagerModel();
  pqObjectBuilder* builder = core->getObjectBuilder();

  // just create it on the first server connection
  pqServer* s = sm->getItemAtIndex<pqServer*>(0);

  // make this operation undo-able 
  BEGIN_UNDO_SET("Load vtkTable from SQLite");

  // create the new source
  pqPipelineSource *source =
    builder->createSource("sources", "SQLite Table", s);
  
  //set its properties based on the values the user submitted
  vtkSMPropertyHelper(source->getProxy(), "FileName").Set(
    this->SQLiteDatabaseFile.toStdString().c_str());
  vtkSMPropertyHelper(source->getProxy(), "TableName").Set(
    tableName.toStdString().c_str());
  source->getProxy()->UpdateVTKObjects();
  source->updatePipeline();

  pqDisplayPolicy* displayPolicy =
    pqApplicationCore::instance()->getDisplayPolicy();
  if (!displayPolicy)
    {
    qCritical() << "No display policy defined. Cannot create pending displays.";
    return;
    }

  //display the new table
  pqView *activeview = pqActiveObjects::instance().activeView();
  pqDataRepresentation* repr = displayPolicy->createPreferredRepresentation(
    source->getOutputPort(0), activeview, false);
  if (repr && repr->getView())
    {
    pqView* view = repr->getView();
    view->render();
    }
  
  END_UNDO_SET();
  this->DatabaseDialog->hide();
}

