/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkMySQLTableReader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMySQLTableReader - Read a MySQL table as a vtkTable
// .SECTION Description
// vtkMySQLTableReader connects to an MySQL database and reads a table,
// outputting it as a vtkTable.

#ifndef __vtkMySQLTableReader_h
#define __vtkMySQLTableReader_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include "vtkTableReader.h"

class vtkMySQLDatabase;
class vtkStringArray;

class vtkMySQLTableReader : public vtkTableReader
{
public:
  static vtkMySQLTableReader *New();
  vtkTypeRevisionMacro(vtkMySQLTableReader,vtkTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the database associated with this reader
  void SetInput(vtkMySQLDatabase *db);

  // Description:
  // Set the host name of the MySQL database
  void SetHostName(const char *hostname);
 
  // Description:
  // Set the database name of the MySQL database
  void SetDatabaseName(const char *databasename);

  // Description:
  // Set the user name of the MySQL database
  void SetUser(const char *user);

  // Description:
  // Set the password of the MySQL database
  void SetPassword(const char *password);

  // Description:
  // Check necessary parameters and attempt to open a connection to a MySQL
  // database.
  void TryToConnectToDatabase();

  // Description: 
  // Close the existing database connection.
  void CloseDatabaseConnection();

  //BTX
  // Description:
  // Get the table names from the currently opened database.
  vtkStringArray* GetTables();
  //ETX

  // Description:
  // Set the name of the table that you'd like to convert to a vtkTable
  // Returns false if the specified table does not exist in the database. 
  bool SetTableName(const char *name);

  // Description:
  // Check if the currently specified table name exists in the database.
  bool CheckIfTableExists();

  vtkMySQLDatabase *GetDatabase() { return this->Database; }

protected:
   vtkMySQLTableReader();
  ~vtkMySQLTableReader();
  // Description:
  // Open a connection to a database.  This should only be called after
  // host, database, user, and password are set.
  void OpenDatabaseConnection();
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  vtkMySQLDatabase *Database;
  //BTX
  vtkstd::string HostName;
  vtkstd::string DatabaseName;
  vtkstd::string User;
  vtkstd::string Password;
  vtkstd::string TableName;
  //ETX
private:
  vtkMySQLTableReader(const vtkMySQLTableReader&);  // Not implemented.
  void operator=(const vtkMySQLTableReader&);  // Not implemented.
};

#endif
