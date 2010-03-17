/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSQLiteTableReader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQLiteTableReader - Read an SQLite table as a vtkTable
// .SECTION Description
// vtkSQLiteTableReader connects to an SQLite database and reads a table,
// outputting it as a vtkTable.

#ifndef __vtkSQLiteTableReader_h
#define __vtkSQLiteTableReader_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include "vtkTableReader.h"

class vtkSQLiteDatabase;
class vtkStringArray;

class vtkSQLiteTableReader : public vtkTableReader
{
public:
  static vtkSQLiteTableReader *New();
  vtkTypeRevisionMacro(vtkSQLiteTableReader,vtkTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the database associated with this reader
  void SetInput(vtkSQLiteDatabase *db);

  // Description:
  // Set the filename of the SQLite database
  void SetFileName(const char *filename);
 
  // Description:
  // Open a connection to a database.  Will not work unless SetFileName()
  // has been called previously.
  bool OpenDatabaseConnection();

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

  vtkSQLiteDatabase *GetDatabase() { return this->Database; }

protected:
   vtkSQLiteTableReader();
  ~vtkSQLiteTableReader();
  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  vtkSQLiteDatabase *Database;
  //BTX
  vtkstd::string DatabaseFileName;
  vtkstd::string TableName;
  //ETX
private:
  vtkSQLiteTableReader(const vtkSQLiteTableReader&);  // Not implemented.
  void operator=(const vtkSQLiteTableReader&);  // Not implemented.
};

#endif
