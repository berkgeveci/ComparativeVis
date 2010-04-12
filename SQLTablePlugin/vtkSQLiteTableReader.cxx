/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSQLiteTableReader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSQLiteQuery.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtkSQLiteTableReader.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSQLiteTableReader, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkSQLiteTableReader);

//----------------------------------------------------------------------------
vtkSQLiteTableReader::vtkSQLiteTableReader()
{
  vtkTable *output = vtkTable::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallelism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  this->DatabaseFileName = "";
  this->Database = vtkSQLiteDatabase::New();
}

//----------------------------------------------------------------------------
vtkSQLiteTableReader::~vtkSQLiteTableReader()
{
  if(this->Database->IsOpen())
    {
    this->Database->Close();
    }

  this->Database->Delete();
  this->Database = 0;
}

//----------------------------------------------------------------------------
void vtkSQLiteTableReader::SetInput(vtkSQLiteDatabase *db)
{
  this->Database = db;
}

//----------------------------------------------------------------------------
void vtkSQLiteTableReader::SetFileName(const char *filename)
{
  this->DatabaseFileName = filename;
  this->OpenDatabaseConnection();
  if(this->TableName != "")
    {
    this->CheckIfTableExists();
    }
}

//----------------------------------------------------------------------------
bool vtkSQLiteTableReader::OpenDatabaseConnection()
{
  if(this->DatabaseFileName == "")
    {
    vtkErrorMacro(<<"No filename specified!");
    return false;
    }
  this->Database->SetDatabaseFileName(this->DatabaseFileName.c_str());

  if(this->Database->IsOpen())
    {
    this->Database->Close();
    }

  if(!this->Database->Open(""))
    {
    vtkErrorMacro(<<"Error opening database!");
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSQLiteTableReader::CloseDatabaseConnection()
{
  this->Database->Close();
}

//----------------------------------------------------------------------------
vtkStringArray* vtkSQLiteTableReader::GetTables()
{
  return this->Database->GetTables();
}

//----------------------------------------------------------------------------
bool vtkSQLiteTableReader::SetTableName(const char *name)
{
  vtkstd::string nameStr = name;
  this->TableName = nameStr; 
  if(this->Database->IsOpen())
    {
    if(this->CheckIfTableExists())
      {
      return true;
      }
    else
      {
      this->TableName = "";
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSQLiteTableReader::CheckIfTableExists()
{
  if(!this->Database->IsOpen())
    {
    vtkErrorMacro(<<"CheckIfTableExists() called with no open database!");
    return false;
    }
  if(this->TableName == "")
    {
    vtkErrorMacro(<<"CheckIfTableExists() called but no table name specified.");
    return false;
    }

  vtkStringArray *tableNames = this->GetTables();

  if(tableNames->LookupValue(this->TableName) == -1)
    {
    vtkErrorMacro(<<"Table " << this->TableName
                  << " does not exist in the database!");
    this->TableName = "";
    //tableNames->Delete();
    return false;
    }

  //tableNames->Delete();
  return true;
}

//----------------------------------------------------------------------------
int vtkSQLiteTableReader::RequestData(vtkInformation *,
                                      vtkInformationVector **,
                                      vtkInformationVector *outputVector)
{
  //Make sure we have all the information we need to provide a vtkTable
  if(!this->Database->IsOpen())
    {
    vtkErrorMacro(<<"No open database connection");
    return 1;
    }
  if(this->TableName == "")
    {
    vtkErrorMacro(<<"No table selected");
    return 1;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Return all data in the first piece ...
  if(outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
    {
    return 1;
    }

  vtkTable* const output = vtkTable::SafeDownCast(
      outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //perform a query to get the names and types of the columns 
  vtkstd::string queryStr = "pragma table_info(";
  queryStr += this->TableName;
  queryStr += ")";
  vtkSQLiteQuery *query =
    static_cast<vtkSQLiteQuery*>(this->Database->GetQueryInstance());
  query->SetQuery(queryStr.c_str());
  if(!query->Execute())
    {
    vtkErrorMacro(<<"Error performing 'pragma' query");
    }
  
  //use the results of the query to create columns of the proper name & type
  std::vector<std::string> columnTypes;
  while(query->NextRow())
    {
    std::string columnName = query->DataValue(1).ToString();
    std::string columnType = query->DataValue(2).ToString();
    columnTypes.push_back(columnType);
    if(columnType == "INTEGER")
      {
      vtkSmartPointer<vtkIntArray> column =
        vtkSmartPointer<vtkIntArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      }
    else if(columnType == "REAL")
      {
      vtkSmartPointer<vtkDoubleArray> column =
        vtkSmartPointer<vtkDoubleArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      }
    else
      {
      vtkSmartPointer<vtkStringArray> column =
        vtkSmartPointer<vtkStringArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      }
    }

  //do a query to get the contents of the SQLite table
  queryStr = "SELECT * FROM ";
  queryStr += this->TableName;
  query->SetQuery(queryStr.c_str());
  if(!query->Execute())
    {
    vtkErrorMacro(<<"Error performing 'select all' query");
    }
  
  //use the results of the query to populate the columns
  while(query->NextRow())
    {
    for(int col = 0; col < query->GetNumberOfFields(); ++ col)
      {
      if(columnTypes[col] == "INTEGER")
        {
        vtkIntArray *column =
          static_cast<vtkIntArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToInt());
        }
      else if(columnTypes[col] == "REAL")
        {
        vtkDoubleArray *column =
          static_cast<vtkDoubleArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToDouble());
        }
      else
        {
        vtkStringArray *column =
          static_cast<vtkStringArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToString());
        }
      }
    }

  query->Delete();
  return 1;
}

//----------------------------------------------------------------------------
void vtkSQLiteTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
     
