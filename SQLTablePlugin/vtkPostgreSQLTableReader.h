/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPostgreSQLTableReader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPostgreSQLTableReader - Read a PostgreSQL table as a vtkTable
// .SECTION Description
// vtkPostgreSQLTableReader connects to an PostgreSQL database and reads a table,
// outputting it as a vtkTable.

#ifndef __vtkPostgreSQLTableReader_h
#define __vtkPostgreSQLTableReader_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include "vtkTableReader.h"

class vtkPostgreSQLDatabase;
class vtkStringArray;

class vtkPostgreSQLTableReader : public vtkTableReader
{
public:
  static vtkPostgreSQLTableReader *New();
  vtkTypeRevisionMacro(vtkPostgreSQLTableReader,vtkTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the database associated with this reader
  void SetInput(vtkPostgreSQLDatabase *db);

  // Description:
  // Set the host name of the PostgreSQL database
  void SetHostName(const char *hostname);
 
  // Description:
  // Set the database name of the PostgreSQL database
  void SetDatabaseName(const char *databasename);

  // Description:
  // Set the user name of the PostgreSQL database
  void SetUser(const char *user);

  // Description:
  // Set the password of the PostgreSQL database
  void SetPassword(const char *password);

  // Description:
  // Check necessary parameters and attempt to open a connection to a PostgreSQL
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

  vtkPostgreSQLDatabase *GetDatabase() { return this->Database; }

protected:
   vtkPostgreSQLTableReader();
  ~vtkPostgreSQLTableReader();
  // Description:
  // Open a connection to a database.  This should only be called after
  // host, database, user, and password are set.
  void OpenDatabaseConnection();
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);
  vtkPostgreSQLDatabase *Database;
  //BTX
  vtkstd::string HostName;
  vtkstd::string DatabaseName;
  vtkstd::string User;
  vtkstd::string Password;
  vtkstd::string TableName;
  //ETX
private:
  vtkPostgreSQLTableReader(const vtkPostgreSQLTableReader&);  // Not implemented.
  void operator=(const vtkPostgreSQLTableReader&);  // Not implemented.
};

#endif
