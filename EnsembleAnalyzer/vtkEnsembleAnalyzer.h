#ifndef VTKMETADATABROWSER_H_
#define VTKMETADATABROWSER_H_

#include <QtGui>
#include "ui_vtkEnsembleAnalyzer.h"

class vtkBatchAnalyzer;
class vtkDatabaseConnection;
class vtkMetadataBrowser;
class vtkStringArray;

class vtkEnsembleAnalyzer : public QMainWindow, public Ui::EnsembleAnalyzer
{
Q_OBJECT;
public:
	vtkEnsembleAnalyzer();
	~vtkEnsembleAnalyzer();
	void Initialize();
	void SetupMainWindow();

public slots:
  void PerformQuery();
  void AddSearchTerm();
  void RemoveSearchTerm(QObject *rowToRemove);
  void RunScript();
  void AppendOutputToDisplay();
  void AppendErrorToDisplay();
  void SelectSQLiteDatabaseFile();
  void SetDatabaseType();
  void CheckConnectionParameters();
  void ConnectToDatabase();
  void ShowMainWindow();
  void ChangeInputDirectory();
  void ChangeScriptDirectory();
  void ChangeOutputDirectory();
  void HandleColumnSpecified();


protected:
  void PerformQuery(std::string query);
	void closeEvent(QCloseEvent *event);
  void AlterPenultimateRow();
  void RunScriptOnFile(QString filename);
  void GenerateInputFileList();
  void CreateDialogUi();
  void PopulateExperimentNames();
  void UpdateColumnNames();
  void AskUserForHandleColumn();

private:
  vtkDatabaseConnection *DatabaseConnection;
  vtkMetadataBrowser *MetadataBrowser;
  vtkBatchAnalyzer *BatchAnalyzer;
  std::string TableName;
  vtkStringArray *ColumnNames;
  int HandleColumn;

	QMenu *FileMenu;
	QAction *ExitAction;
  QSignalMapper *Mapper;
  QTextEdit *OutputDisplay;
  QStringList InputFileList;
  bool InitializeFileList;

  //GUI elements for the database connection dialog
  QWidget *DatabaseDialog;
  QComboBox *DatabaseType;
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

  //GUI elements for the handle column selector dialog
   QWidget *HandleColumnDialog;
  QLabel *HandleColumnLabel;
  QComboBox *HandleColumnSelector;
  QPushButton *HandleColumnButton;
};
#endif

