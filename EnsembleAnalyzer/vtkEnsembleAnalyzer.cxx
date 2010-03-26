#include <iostream>
#include <string>
using std::cerr;
using std::cout;
using std::endl;

#include <QAction>
#include "vtkBatchAnalyzer.h"
#include "vtkDatabaseConnection.h"
#include "vtkEnsembleAnalyzer.h"
#include "vtkMetadataBrowser.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

//-----------------------------------------------------------------------------
vtkEnsembleAnalyzer::vtkEnsembleAnalyzer()
{
  this->DatabaseConnection = vtkDatabaseConnection::New();
  this->MetadataBrowser = vtkMetadataBrowser::New();
  this->MetadataBrowser->SetDatabaseConnection(this->DatabaseConnection);
  this->BatchAnalyzer = vtkBatchAnalyzer::New();
  this->ColumnNames = 0;
  this->HandleColumn = -1;
  this->TableName = "";
  this->OutputDisplay = 0;
  this->setObjectName("vtkEnsembleAnalyzer");
  this->InitializeFileList = true;
  this->Initialize();
}

//-----------------------------------------------------------------------------
vtkEnsembleAnalyzer::~vtkEnsembleAnalyzer()
{
  this->MetadataBrowser->Delete();
  this->MetadataBrowser = 0;
  this->BatchAnalyzer->Delete();
  this->BatchAnalyzer = 0;
  if(this->ColumnNames)
    {
    this->ColumnNames->Delete();
    this->ColumnNames = 0;
    }
  if(this->OutputDisplay)
    {
    delete this->OutputDisplay;
    }
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::Initialize()
{
  //setup main GUI elements from the interface file
  this->setupUi(this);
  this->show();

  //Present a modal dialog to the user asking them to connect to a database
  //and select a table.
  this->CreateDialogUi();
  this->DatabaseDialog->show();
}

// Set up the components of the main interface and their connections 
//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::SetupMainWindow()
{
  //initialize output window for feedback from the visualizations
  this->OutputDisplay = new QTextEdit();
  this->OutputDisplay->resize(400,600);
  this->OutputDisplay->hide();

  //change QTableWidget's default behavior
  this->Table->setSelectionBehavior(QAbstractItemView::SelectRows);

  //setup the default input, output, and script directories
  this->BatchAnalyzer->SetDataPath(DATA_PATH);
  QDir dataDir(DATA_PATH);
  this->InputButtonLabel->setText(dataDir.dirName());

  this->BatchAnalyzer->SetResultsPath(RESULTS_PATH);
  QDir outputDir(RESULTS_PATH);
  this->OutputButtonLabel->setText(outputDir.dirName());

  this->BatchAnalyzer->SetScriptPath(SCRIPT_PATH);
  QDir scriptDir(SCRIPT_PATH);
  QStringList filters;
  filters << "*.py";
  scriptDir.setNameFilters(filters);
  for(int i = 0; i < scriptDir.count(); i++)
    {
    this->ScriptSelector->insertItem(i, scriptDir[i]);
    }
  this->ScriptButtonLabel->setText(scriptDir.dirName());

  //setup signals and slots
	connect(this->QueryButton, SIGNAL(pressed()), this, SLOT(PerformQuery()));
	connect(this->AddSearchTermButton, SIGNAL(pressed()), this, 
          SLOT(AddSearchTerm()));
  connect(this->ChangeInputButton, SIGNAL(pressed()), this,
          SLOT(ChangeInputDirectory()));
  connect(this->ChangeScriptButton, SIGNAL(pressed()), this,
          SLOT(ChangeScriptDirectory()));
  connect(this->ChangeOutputButton, SIGNAL(pressed()), this,
          SLOT(ChangeOutputDirectory()));
 
	connect(this->RunScriptButton, SIGNAL(pressed()),
          this, SLOT(RunScript()));
  this->RunScriptButton->setEnabled(false);

  this->connect(this->BatchAnalyzer->Process,
                SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(RunScript()));
  connect(this->BatchAnalyzer->Process, SIGNAL(readyReadStandardOutput()),
          this, SLOT(AppendOutputToDisplay()));
  connect(this->BatchAnalyzer->Process, SIGNAL(readyReadStandardError()),
          this, SLOT(AppendErrorToDisplay()));

  //set up the signal mapper
  this->Mapper = new QSignalMapper(this);
  connect(this->Mapper, SIGNAL(mapped(QObject *)), this,
          SLOT(RemoveSearchTerm(QObject *)));

  //create an initial search term input row
  this->AddSearchTerm();
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::AddSearchTerm()
{
  //change what was the last row to match all the others
  this->AlterPenultimateRow();

  QComboBox *parameterCombo = new QComboBox();
  for(unsigned int i = 0; i < this->ColumnNames->GetNumberOfValues(); i++)
    {
    parameterCombo->insertItem((int)i, tr(this->ColumnNames->GetValue(i)));
    }
  
  //will need a callback on parameterCombo if we want to change the options
  //in compareCombo based on what's currently selected in parameterCombo.

  QComboBox *compareCombo = new QComboBox();
  compareCombo->insertItem(0, "<");
  compareCombo->insertItem(1, "<=");
  compareCombo->insertItem(2, "=");
  compareCombo->insertItem(3, "!=");
  compareCombo->insertItem(4, ">=");
  compareCombo->insertItem(5, ">");

  QLineEdit *inputField = new QLineEdit();
  inputField->setMinimumWidth(32);
  inputField->setMinimumWidth(150);
  
  QPushButton *removeRowButton = new QPushButton("-");
  connect(removeRowButton, SIGNAL(pressed()), this->Mapper, SLOT(map()));
  removeRowButton->setStatusTip("Remove this search term from the query");

  QGridLayout *rowLayout = new QGridLayout();
  rowLayout->addWidget(parameterCombo, 0, 0);
  rowLayout->addWidget(compareCombo, 0, 1);
  rowLayout->addWidget(inputField, 0, 2);
  rowLayout->addWidget(removeRowButton, 0, 3);
  this->Mapper->setMapping(removeRowButton, rowLayout);
  this->SearchTermsLayout->addLayout(rowLayout,
                                     this->SearchTermsLayout->rowCount(), 0);
}

//add the logical and/or field to the 2nd to last row
//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::AlterPenultimateRow()
{
  int numRows = this->SearchTermsLayout->count();
  if(numRows > 0)
    {
    //find the 2nd to last row
    QGridLayout *penultimateRow = qobject_cast<QGridLayout *>
      (this->SearchTermsLayout->itemAt(numRows-1)->layout());

    //temporarily remove the - button
    QPushButton *removeRowButton = qobject_cast<QPushButton *>
      (penultimateRow->itemAt(3)->widget());

    //add a logical and/or field to this row
    QComboBox *logicCombo = new QComboBox();
    logicCombo->insertItem(0, "AND");
    logicCombo->insertItem(0, "OR");
    logicCombo->setStatusTip(
      "How should this search term logically relate to the next search term in the query?");
    penultimateRow->addWidget(logicCombo, 0, 3);

    //re-add the - button at the end of the row
    penultimateRow->addWidget(removeRowButton, 0, 4);
    }
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::PerformQuery()
{
  //construct the query from user specified values
  QString query = "SELECT * FROM ";
  query.append(this->TableName.c_str());
  query.append(" WHERE ");
  int numRows = this->SearchTermsLayout->count();
  for(int currRow = 0; currRow < numRows; currRow++)
    {
    QGridLayout *row = qobject_cast<QGridLayout *>
      (this->SearchTermsLayout->itemAt(currRow)->layout());
    QComboBox *parameterCombo = qobject_cast<QComboBox *>
      (row->itemAt(0)->widget());
    QComboBox *compareCombo = qobject_cast<QComboBox *>
      (row->itemAt(1)->widget());
    QLineEdit *inputField = qobject_cast<QLineEdit *>
      (row->itemAt(2)->widget());
    query.append(parameterCombo->currentText());
    query.append(" ");
    query.append(compareCombo->currentText());
    query.append(" ");
    //text needs to be surrounded by quotes in SQL...
    bool ok;
    inputField->text().toFloat(&ok);
    if(!ok)
      {
      QString inputVal = "'";
      inputVal.append(inputField->text());
      inputVal.append("'");
      query.append(inputVal);
      }
    else
      {
      query.append(inputField->text());
      }
    query.append(" ");
    if(currRow == numRows - 1)
      {
      query.append(";");
      }
    else
      {
      QComboBox *logicCombo = qobject_cast<QComboBox *>
        (row->itemAt(3)->widget());
      query.append(logicCombo->currentText());
      query.append(" ");
      }
    }
  
  //execute the user's request
  this->PerformQuery(query.toStdString());
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::PerformQuery(std::string query)
{
  //clear the table
  this->Table->clear();
  this->Table->setRowCount(0);

  // Perform the query
  vtkTable *table =
    this->MetadataBrowser->GetDataFromExperiment(this->TableName, query);
 
  if(table->GetNumberOfRows() == 0)
    {
    QMessageBox msgBox;
    msgBox.setText(
      "No results found.  Adjust your search terms and perform another query.");
    msgBox.exec();
    return;
    }
  
  //if the table has at least one result, then we're ready to visualize
  this->RunScriptButton->setEnabled(true);
  this->InitializeFileList = true;

  //copy data & structure from vtkTable to QTable
  //first grab the column names
  if(this->Table->columnCount() == 0)
    {
    this->Table->setColumnCount(table->GetNumberOfColumns());
    QStringList headers;
    //this->UpdateColumnNames();
    for(int i = 0; i< this->ColumnNames->GetNumberOfValues(); i++)
      {
      headers << this->ColumnNames->GetValue(i).c_str();
      }
    this->Table->setHorizontalHeaderLabels(headers);
    }

  //then copy cell data
  for(int rowNum = 0; rowNum < table->GetNumberOfRows(); rowNum++)
    {
    this->Table->insertRow(rowNum);
    for(int colNum = 0; colNum < table->GetNumberOfColumns(); colNum++)
      {
      QTableWidgetItem *newItem = new QTableWidgetItem(
        QString(table->GetValue(rowNum, colNum).ToString()));
      this->Table->setItem(rowNum, colNum, newItem);
      }
    }
  table->Delete();

  //select all the rows in the results table
  this->Table->selectAll();
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::UpdateColumnNames()
{
  if(this->TableName == "" ||
     this->DatabaseConnection->CheckDatabaseConnection() == false)
    {
    return;
    }
  if(this->ColumnNames)
    {
    this->ColumnNames->Delete();
    }
  this->ColumnNames =
    this->DatabaseConnection->GetColumnNames(this->TableName.c_str());

  //figure out which one is the 'file handle' column
  for(int i = 0; i < this->ColumnNames->GetNumberOfValues(); i++)
    {
    QString curStr(this->ColumnNames->GetValue(i).c_str());
    if(curStr.toLower() == "file" || curStr.toLower() == "filename" ||
       curStr.toLower() == "handle")
      {
      this->HandleColumn = i;
      this->HandleColumnSpecified();
      return;
      }
    }

  //ask the user if we can't automatically figure out which column it is
  this->AskUserForHandleColumn();
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::AskUserForHandleColumn()
{
  this->HandleColumnDialog = new QWidget();
  this->HandleColumnDialog->setObjectName("this->HandleColumnDialog");
  this->HandleColumnDialog->setWindowTitle("Select handle column");
  this->HandleColumnDialog->resize(400, 300);
  //might have to change window style too...

  this->HandleColumnLabel = new QLabel(this->HandleColumnDialog);
  this->HandleColumnLabel->setObjectName("HandleColumnLabel");
  this->HandleColumnLabel->setGeometry(QRect(20, 50, 361, 31));
  this->HandleColumnLabel->setText(
    "Which column specifies where the data files are located?");

  this->HandleColumnSelector = new QComboBox(this->HandleColumnDialog);
  this->HandleColumnSelector->setObjectName("this->HandleColumnSelector");
  this->HandleColumnSelector->setGeometry(QRect(90, 120, 201, 26));
  for(unsigned int i = 0; i < this->ColumnNames->GetNumberOfValues(); i++)
    {
    this->HandleColumnSelector->insertItem((int)i, tr(this->ColumnNames->GetValue(i)));
    }

  this->HandleColumnButton = new QPushButton(this->HandleColumnDialog);
  this->HandleColumnButton->setObjectName("HandleColumnButton");
  this->HandleColumnButton->setGeometry(QRect(130, 220, 113, 32));
  this->HandleColumnButton->setText("OK");

  this->connect(this->HandleColumnButton, SIGNAL(pressed()),
                this, SLOT(HandleColumnSpecified()));

  this->HandleColumnDialog->show();
  this->HandleColumnDialog->setFocus();
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::HandleColumnSpecified()
{
  if(this->HandleColumn == -1)
    {
    this->HandleColumn = this->HandleColumnSelector->currentIndex();
    this->HandleColumnDialog->hide();
    delete this->HandleColumnDialog;
    }

  //finish setting up the main window
  this->SetupMainWindow();
  std::string query = "SELECT * FROM " + this->TableName;
  this->PerformQuery(query);
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::RemoveSearchTerm(QObject *rowToRemove)
{
  QGridLayout *row = static_cast<QGridLayout *>(rowToRemove); 
  QLayoutItem *child;
  QWidget *widget;
  while ((child = row->takeAt(0)) != 0)
    {
    if( (widget = child->widget()) != 0)
      {
      if(widget->inherits("QPushButton"))
        {
        this->Mapper->removeMappings(widget);
        }
      delete widget;
      }
    //delete child;
    }
  this->SearchTermsLayout->removeItem(row);
  row->setParent(NULL);
  delete row;
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::GenerateInputFileList()
{
  QList<QTableWidgetItem *> selectedItems = this->Table->selectedItems();
  this->InputFileList.clear();

  for (int i = 0; i < selectedItems.size(); ++i)
    {
    if(selectedItems.at(i)->column() == this->HandleColumn)
    //if(selectedItems.at(i)->column() == this->Table->columnCount() - 1)
      {
      QString nextFile = selectedItems.at(i)->data(Qt::DisplayRole).toString();
      const char *cStr = nextFile.trimmed().toStdString().c_str(); 
      this->InputFileList.append(QString(cStr));
      //this->InputFileList.append(nextFile.trimmed());
      }
    }

  this->InitializeFileList = false;
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::RunScript()
{
  //if the user just pressed the "run output" button, we need to generate
  //a list of files from the selected rows of the table.
  if(this->InitializeFileList)
    {
    this->GenerateInputFileList();
    
    //set the visualization to run
    this->BatchAnalyzer->SetVisualization(
      this->ScriptSelector->currentText().toStdString().c_str());

    //and the parameters
    this->BatchAnalyzer->SetParameters(
      this->ScriptParameters->text().toStdString().c_str());
    
    //disable the GUI elements involved in running a output
    this->RunScriptButton->setEnabled(false);
    this->ScriptSelector->setEnabled(false);
    this->ScriptParameters->setEnabled(false);

    //and show the output output widget
    this->OutputDisplay->clear();
    this->OutputDisplay->show();
    }

  //if the input file list is empty then we're done running python for now.
  if(this->InputFileList.empty())
    {
    this->InitializeFileList = true;
    this->RunScriptButton->setEnabled(true);
    this->ScriptSelector->setEnabled(true);
    this->ScriptParameters->setEnabled(true);
    return;
    }

  this->RunScriptOnFile(this->InputFileList.takeFirst());
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::RunScriptOnFile(QString inputFileName)
{
  //automatically generate an output file name based on input, script, and
  //parameters
  QFileInfo inputFileInfo(inputFileName);
  QFileInfo scriptFileInfo(this->ScriptSelector->currentText());
  QString params = this->ScriptParameters->text().replace(" ", "_");
  QString outputFileName = inputFileInfo.baseName() + "_" +
    scriptFileInfo.baseName() + "_" + params + "." +
    inputFileInfo.completeSuffix();
  std::string input = inputFileName.toStdString();
  std::string output = outputFileName.toStdString();
  //const char *input = inputFileName.toStdString().c_str();
  //const char *output = outputFileName.toStdString().c_str();

  //run the visualization.
  this->BatchAnalyzer->VisualizeDataset(input, output);

  //load results into database
  QString query =
    "INSERT INTO analyses (experiment, input, operation, parameters, results) VALUES('";
  query.append(this->MetadataBrowser->GetExperimentId(this->TableName.c_str()));
  query.append("','");
  query.append(inputFileName);
  query.append("','");
  query.append(this->ScriptSelector->currentText());
  query.append("','");
  query.append(this->ScriptParameters->text());
  query.append("','");
  query.append(outputFileName);
  query.append("')");

  this->DatabaseConnection->ExecuteQuery(query.toStdString().c_str());
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::AppendOutputToDisplay()
{
  this->OutputDisplay->append(
    QString(this->BatchAnalyzer->Process->readAllStandardOutput())); 
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::AppendErrorToDisplay()
{
  this->OutputDisplay->append(
    QString(this->BatchAnalyzer->Process->readAllStandardError())); 
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::closeEvent(QCloseEvent *event)
{
  event->accept();
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::CreateDialogUi()
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

  this->DatabaseType = new QComboBox(this->DatabaseDialog);
  this->DatabaseType->setObjectName(QString::fromUtf8("DatabaseType"));
  this->DatabaseType->setGeometry(QRect(20, 90, 180, 21));
  this->DatabaseType->insertItem(0, QString("MySQL"));
  this->DatabaseType->insertItem(1, QString("PostgreSQL"));
  this->DatabaseType->insertItem(2, QString("SQLite"));

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
  
  //connect signals & slots here too
  this->connect(this->OpenSQLiteDatabaseButton, SIGNAL(pressed()),
                this, SLOT(SelectSQLiteDatabaseFile()));

  this->connect(this->DatabaseType, SIGNAL(currentIndexChanged(int)),
                this, SLOT(SetDatabaseType()));
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
                this, SLOT(ShowMainWindow()));
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::SelectSQLiteDatabaseFile()
{
  this->DatabaseType->setCurrentIndex(2);

  //ask the user to select a database file
  this->SQLiteDatabaseFile = QFileDialog::getOpenFileName(this->DatabaseDialog,
    tr("Select SQLite Database"), "", tr("SQLite Database Files (*.*)"));

  //bail out now if the user pressed cancel
  if(this->SQLiteDatabaseFile == "")
    {
    return;
    }

  this->DatabaseConnection->UseSQLite();
  this->DatabaseConnection->SetFileName(
    this->SQLiteDatabaseFile.toStdString().c_str());

  if(this->DatabaseConnection->ConnectToDatabase())
    {
    //if we can open it succesfully, populate the list of table names
    this->PopulateExperimentNames();
    }
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::PopulateExperimentNames()
{
  this->TableComboBox->clear();
  vtkStringArray* experiments = this->MetadataBrowser->GetListOfExperiments();
  for(int i = 0; i < experiments->GetNumberOfValues(); i++)
    {
    this->TableComboBox->insertItem(i, QString(experiments->GetValue(i))); 
    }
  if(experiments->GetNumberOfValues() > 0)
    {
    this->LoadTableButton->setEnabled(true);
    }
  experiments->Delete();
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::SetDatabaseType()
{
  if(this->DatabaseType->currentText() == "MySQL")
    {
    this->DatabaseConnection->UseMySQL();
    }
  else if(this->DatabaseType->currentText() == "PostgreSQL")
    {
    this->DatabaseConnection->UsePostgreSQL();
    }
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::CheckConnectionParameters()
{
  if(this->HostInput->text() != "" && this->DBNameInput->text() != "" &&
     this->UserInput->text() != "" && this->PasswordInput->text() != "")
    {
    this->ConnectButton->setEnabled(true);
    }
  else
    {
    this->ConnectButton->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::ConnectToDatabase()
{
  this->DatabaseConnection->SetHostName(
    this->HostInput->text().toStdString().c_str());
  this->DatabaseConnection->SetDatabaseName(
    this->DBNameInput->text().toStdString().c_str());
  this->DatabaseConnection->SetUser(
    this->UserInput->text().toStdString().c_str());
  this->DatabaseConnection->SetPassword(
    this->PasswordInput->text().toStdString().c_str());

  if(this->DatabaseConnection->ConnectToDatabase())
    {
    this->MetadataBrowser->SetDatabaseConnection(this->DatabaseConnection);
    this->PopulateExperimentNames();
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
void vtkEnsembleAnalyzer::ShowMainWindow()
{
  //get information about the selected table
  this->TableName = this->TableComboBox->currentText().toStdString();
  this->UpdateColumnNames();

  //get rid of database dialog
  delete this->DatabaseDialog;
  //we setup and display the rest of the GUI once we know which table column
  //contains filenames
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::ChangeInputDirectory()
{
  QString inputDir = QFileDialog::getExistingDirectory(
    this->CentralWidget, tr("Select input directory"), "");

  if(inputDir  == "")
    {
    return;
    }
  this->BatchAnalyzer->SetDataPath(inputDir.toStdString().c_str());
  QDir dir(inputDir);
  this->InputButtonLabel->setText(dir.dirName());
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::ChangeScriptDirectory()
{
  QString scriptDir = QFileDialog::getExistingDirectory(
    this->CentralWidget, tr("Select script directory"), "");

  if(scriptDir  == "")
    {
    return;
    }
  this->BatchAnalyzer->SetScriptPath(scriptDir.toStdString().c_str());
  QDir dir(scriptDir);
  this->ScriptButtonLabel->setText(dir.dirName());

  //change the values in the script selector
  this->ScriptSelector->clear();
  QStringList filters;
  filters << "*.py";
  dir.setNameFilters(filters);
  for(int i = 0; i < dir.count(); i++)
    {
    this->ScriptSelector->insertItem(i, dir[i]);
    }
}

//-----------------------------------------------------------------------------
void vtkEnsembleAnalyzer::ChangeOutputDirectory()
{
  QString outputDir = QFileDialog::getExistingDirectory(
    this->CentralWidget, tr("Select output directory"), "");

  if(outputDir  == "")
    {
    return;
    }
  this->BatchAnalyzer->SetResultsPath(outputDir.toStdString().c_str());
  QDir dir(outputDir);
  this->OutputButtonLabel->setText(dir.dirName());
}

