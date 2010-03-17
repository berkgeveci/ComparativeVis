/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: SQLToolbarActions.h,v $

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
#ifndef __SQLToolbarActions_h
#define __SQLToolbarActions_h

#include <QActionGroup>
#include <QtGui>

/// This toolbar allows a user to read form an SQL database
class SQLToolbarActions : public QActionGroup
{
  Q_OBJECT
public:
  SQLToolbarActions(QObject* p);
  ~SQLToolbarActions();
  void SetupDatabaseDialog();

public slots:
  /// Callback for each action triggerred.
  void onAction(QAction* a);

  /// Setup the user interface for this plugin
  void CreateDialogUi();

  /// Make our Qt widgets do more than just look pretty
  void SetupSignalsAndSlots();

  /// Select to a SQLite database file
  void SelectSQLiteDatabaseFile();

  /// Check if the user supplied enough information to connect to a database
  void CheckConnectionParameters();

  /// Connect to a MySQL or Postgres database
  void ConnectToDatabase();
 
  /// Connect to a MySQL database
  void ConnectToMySQLDatabase();

  /// Connect to a Postgres database
  void ConnectToPostgresDatabase();

  /// Load the selected table into ParaView
  void LoadTable();

  /// Load the selected MySQL table into ParaView
  void LoadMySQLTable(QString tableName);
  
  /// Load the selected SQLitetable into ParaView
  void LoadSQLiteTable(QString tableName);

protected:

  bool UsingSQLite;
  bool UsingMySQL;
  bool UsingPostgreSQL;
  //GUI elements
  QWidget *DatabaseDialog;
  QRadioButton *MySQLRadioButton;
  QRadioButton *PostgresRadioButton;
  QPushButton *OpenSQLiteDatabaseButton;
  QLineEdit *HostInput;
  QLabel *HostLabel;
  QLineEdit *DBNameInput;
  QLabel *DBNameLabel;
  QLineEdit *UserInput;
  QLabel *UserLabel;
  QLineEdit *PasswordInput;
  QLabel *PasswordLabel;
  QLabel *SQLiteLabel;
  QPushButton *ConnectButton;
  QLabel *TableLabel;
  QComboBox *TableComboBox;
  QPushButton *LoadTableButton;
  QString SQLiteDatabaseFile;
};

#endif

