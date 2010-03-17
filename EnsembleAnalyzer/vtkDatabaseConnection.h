#ifndef __vtkDatabaseConnection_h
#define __vtkDatabaseConnection_h

#include "vtkObject.h"

class vtkMySQLDatabase;
class vtkPostgreSQLDatabase;
class vtkSQLiteDatabase;
class vtkSQLQuery;
class vtkStringArray;

class VTK_EXPORT vtkDatabaseConnection : public vtkObject
{

public:
  static vtkDatabaseConnection *New();
  vtkTypeRevisionMacro(vtkDatabaseConnection, vtkObject);

  void SetHostName(const char *hostName);
  void SetDatabaseName(const char *databaseName);
  void SetUser(const char *user);
  void SetPassword(const char *password);
  void SetFileName(const char *fileName);
  void UseMySQL();
  void UsePostgreSQL();
  void UseSQLite();
  bool ConnectToDatabase();
  bool CheckDatabaseConnection();
  bool ExecuteQuery(const char *query);
  vtkStringArray* GetTableNamesFromDatabase();
  vtkStringArray *GetColumnNames(const char *tableName);
  //accessors
  bool IsUsingMySQL();
  bool IsUsingPostgreSQL();
  bool IsUsingSQLite();
  vtkMySQLDatabase *GetMySQLDatabase();
  vtkPostgreSQLDatabase *GetPostgreSQLDatabase();
  vtkSQLiteDatabase *GetSQLiteDatabase();
  vtkSQLQuery *GetSQLQuery();

protected:
	vtkDatabaseConnection();
	~vtkDatabaseConnection();
  bool ReadyToConnectToDatabase();
  bool ConnectToMySQLDatabase();
  bool ConnectToPostgreSQLDatabase();
  bool ConnectToSQLiteDatabase();

private:
  vtkMySQLDatabase *MySQLDatabase;
  vtkPostgreSQLDatabase *PostgreSQLDatabase;
  vtkSQLiteDatabase *SQLiteDatabase;
  vtkSQLQuery *SQLQuery;

  bool UsingMySQL;
  bool UsingPostgreSQL;
  bool UsingSQLite;

  //BTX
  std::string HostName;
  std::string DatabaseName;
  std::string User;
  std::string Password;
  std::string FileName;
  //ETX
};
#endif

