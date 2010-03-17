/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSQLTableReader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSQLTableReader - Read an SQL table as a vtkTable
// .SECTION Description
// vtkSQLTableReader connects to an SQL database and reads a table,
// outputting it as a vtkTable.

#ifndef __vtkSQLTableReader_h
#define __vtkSQLTableReader_h

#include <vtkstd/vector>
#include <vtkstd/string>
#include "vtkTableReader.h"

class vtkSQLDatabase;
class vtkStringArray;

class vtkSQLTableReader : public vtkTableReader
{
public:
  vtkTypeRevisionMacro(vtkSQLTableReader,vtkTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the database associated with this reader
  virtual void SetInput(vtkSQLDatabase *db) = 0;

  // Description:
  // Open a connection to the database.
  virtual int OpenDatabaseConnection() = 0;

  // Description:
  // Close the existing database connection.
  virtual void CloseDatabaseConnection() = 0;

  // Description:
  // Get the table names from the currently opened database.
  virtual vtkStringArray* GetTables() = 0;

protected:
   vtkSQLTableReader();
  ~vtkSQLTableReader();
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) = 0;

private:
  vtkSQLTableReader(const vtkSQLTableReader&);  // Not implemented.
  void operator=(const vtkSQLTableReader&);  // Not implemented.
};

#endif
