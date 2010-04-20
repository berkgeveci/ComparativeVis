#include <vector>

#include "vtkMySQLDatabase.h"
#include "vtkObjectFactory.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include "vtkDatabaseConnection.h"

vtkStandardNewMacro(vtkDatabaseConnection);
vtkCxxRevisionMacro(vtkDatabaseConnection, "$Revision: 1.1 $");


//-----------------------------------------------------------------------------
vtkDatabaseConnection::vtkDatabaseConnection()
{
  this->MySQLDatabase = 0;
  this->PostgreSQLDatabase = 0;
  this->SQLiteDatabase = 0;
  this->SQLQuery = 0;
  this->HostName = "";
  this->DatabaseName = "";
  this->User = "";
  this->Password = "";
}

//-----------------------------------------------------------------------------
vtkDatabaseConnection::~vtkDatabaseConnection()
{
  if(this->SQLQuery)
    {
    this->SQLQuery->Delete();
    }
  if(this->MySQLDatabase)
    {
    if(this->MySQLDatabase->IsOpen())
      {
      this->MySQLDatabase->Close();
      }
    this->MySQLDatabase->Delete();
    }
  if(this->PostgreSQLDatabase)
    {
    if(this->PostgreSQLDatabase->IsOpen())
      {
      this->PostgreSQLDatabase->Close();
      }
    this->PostgreSQLDatabase->Delete();
    }
  if(this->SQLiteDatabase)
    {
    if(this->SQLiteDatabase->IsOpen())
      {
      this->SQLiteDatabase->Close();
      }
    this->SQLiteDatabase->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::SetHostName(const char *hostName)
{
  this->HostName = hostName;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::SetDatabaseName(const char *databaseName)
{
  this->DatabaseName = databaseName;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::SetUser(const char *user)
{
  this->User = user;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::SetPassword(const char *password)
{
  this->Password = password;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::SetFileName(const char *fileName)
{
  this->FileName = fileName;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::UseMySQL()
{
  this->UsingMySQL = true;
  this->UsingPostgreSQL = false;
  this->UsingSQLite = false;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::UsePostgreSQL()
{
  this->UsingMySQL = false;
  this->UsingPostgreSQL = true;
  this->UsingSQLite = false;
}

//-----------------------------------------------------------------------------
void vtkDatabaseConnection::UseSQLite()
{
  this->UsingMySQL = false;
  this->UsingPostgreSQL = false;
  this->UsingSQLite = true;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::IsUsingMySQL()
{
  return this->UsingMySQL;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::IsUsingPostgreSQL()
{
  return this->UsingPostgreSQL;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::IsUsingSQLite()
{
  return this->UsingSQLite;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::ConnectToDatabase()
{
  if(this->UsingMySQL)
    {
    return this->ConnectToMySQLDatabase();
    }
  if(this->UsingPostgreSQL)
    {
    return this->ConnectToPostgreSQLDatabase();
    }
  if(this->UsingSQLite)
    {
    return this->ConnectToSQLiteDatabase();
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::ConnectToMySQLDatabase()
{
  //check required settings before proceeding
  if(this->ReadyToConnectToDatabase() == false)
    {
    return false;
    }
  if(this->MySQLDatabase)
    {
    this->MySQLDatabase->Delete();
    }
  this->MySQLDatabase = vtkMySQLDatabase::New();
  this->MySQLDatabase->SetHostName(this->HostName.c_str());
  this->MySQLDatabase->SetDatabaseName(this->DatabaseName.c_str());
  this->MySQLDatabase->SetUser(this->User.c_str());
  this->MySQLDatabase->SetPassword(this->Password.c_str());

  if(this->MySQLDatabase->Open())
    {
    this->SQLQuery = this->MySQLDatabase->GetQueryInstance();
    return true;
    }
  else
    {
    cerr << "Unable to connect to the database.  Please check your settings " 
          << "and try again." << endl;
    this->MySQLDatabase->Delete();
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::ConnectToPostgreSQLDatabase()
{
  //check required settings
  if(this->ReadyToConnectToDatabase() == false)
    {
    return false;
    }

  this->PostgreSQLDatabase = vtkPostgreSQLDatabase::New();
  this->PostgreSQLDatabase->SetHostName(this->HostName.c_str());
  this->PostgreSQLDatabase->SetDatabaseName(this->DatabaseName.c_str());
  this->PostgreSQLDatabase->SetUser(this->User.c_str());
  this->PostgreSQLDatabase->SetPassword(this->Password.c_str());

  if(this->PostgreSQLDatabase->Open(this->Password.c_str()))
    {
    this->SQLQuery = this->PostgreSQLDatabase->GetQueryInstance();
    return true;
    }
  else
    {
    cerr << "Unable to connect to the database.  Please check your settings " 
          << "and try again." << endl;
    this->PostgreSQLDatabase->Delete();
    }
  return false;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::ConnectToSQLiteDatabase()
{
  //we can't proceed if we don't have a file name
  if(this->FileName == "")
    {
    cerr << "You must call SetFileName() before attempting to connect to a "
         << "SQLite database." << endl;
    return false;
    }
  
  this->SQLiteDatabase = vtkSQLiteDatabase::New();
  this->SQLiteDatabase->SetDatabaseFileName(this->FileName.c_str());
  if(this->SQLiteDatabase->Open(""))
    {
    this->SQLQuery = this->SQLiteDatabase->GetQueryInstance();
    return true;
    }
  else
    {
    cerr << "Unable to connect to the database.  Please check your settings " 
          << "and try again." << endl;
    this->SQLiteDatabase->Delete();
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkMySQLDatabase* vtkDatabaseConnection::GetMySQLDatabase()
{
  return this->MySQLDatabase;
}

//-----------------------------------------------------------------------------
vtkPostgreSQLDatabase* vtkDatabaseConnection::GetPostgreSQLDatabase()
{
  return this->PostgreSQLDatabase;
}

//-----------------------------------------------------------------------------
vtkSQLiteDatabase* vtkDatabaseConnection::GetSQLiteDatabase()
{
  return this->SQLiteDatabase;
}

//-----------------------------------------------------------------------------
vtkSQLQuery* vtkDatabaseConnection::GetSQLQuery()
{
  return this->SQLQuery;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::ReadyToConnectToDatabase()
{
  std::string dbType;
  if(this->UsingMySQL)
    {
    dbType = "MySQL";
    }
  else
    {
    dbType = "PostgreSQL";
    }

  if(this->HostName == "")
    {
    cerr << "You must call SetHostName() before attempting to connect to a "
         << dbType << " database." << endl;
    return false;
    }
  if(this->DatabaseName == "")
    {
    cerr << "You must call SetDatabaseName() before attempting to connect to a "
         << dbType << " database." << endl;
    return false;
    }
  if(this->User == "")
    {
    cerr << "You must call SetUser() before attempting to connect to a "
         << dbType << " database." << endl;
    return false;
    }
  //not all databases require a password, so we won't squawk if that hasn't
  //been set.

  return true;
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::CheckDatabaseConnection()
{
  //make sure we have an open database connection
  if(this->UsingMySQL)
    {
    if(this->MySQLDatabase->IsOpen() == false)
      {
      return false; 
      }
    return true;
    }
  else if(this->UsingPostgreSQL)
    {
    if(this->PostgreSQLDatabase->IsOpen() == false)
      {
      return false; 
      }
    return true;
    }
  else if(this->UsingSQLite)
    {
    if(this->SQLiteDatabase->IsOpen() == false)
      {
      return false; 
      }
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkStringArray* vtkDatabaseConnection::GetTableNamesFromDatabase()
{
  //database connection checks are done prior to this function being called
  if(this->UsingMySQL)
    {
    return this->MySQLDatabase->GetTables();
    }
  else if(this->UsingPostgreSQL)
    {
    return this->PostgreSQLDatabase->GetTables();
    }
  else //SQLite
    {
    return this->SQLiteDatabase->GetTables();
    }
}

vtkStringArray* vtkDatabaseConnection::GetColumnNames(const char *tableName)
{
  //it's the caller's responsibility to ensure an active database connection
  //before calling this function
  if(this->UsingMySQL)
    {
    return this->MySQLDatabase->GetRecord(tableName);
    }
  else if(this->UsingPostgreSQL)
    {
    return this->PostgreSQLDatabase->GetRecord(tableName);
    }
  else //SQLite
    {
    return this->SQLiteDatabase->GetRecord(tableName);
    }
}

//-----------------------------------------------------------------------------
bool vtkDatabaseConnection::ExecuteQuery(const char *query)
{
  if(this->SQLQuery->SetQuery(query) == false)
    {
    return false;
    }
  return this->SQLQuery->Execute();
}


