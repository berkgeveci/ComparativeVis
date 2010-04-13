/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkMySQLTableReader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <QString>

#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkMySQLDatabase.h"
#include "vtkMySQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtkMySQLTableReader.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMySQLTableReader, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkMySQLTableReader);

//----------------------------------------------------------------------------
vtkMySQLTableReader::vtkMySQLTableReader()
{
  vtkTable *output = vtkTable::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallelism.
  // Filters will know it is empty.
  output->ReleaseData();
  output->Delete();

  this->HostName = "";
  this->DatabaseName = "";
  this->User = "";
  this->Password = "";
  this->Database = vtkMySQLDatabase::New();
}

//----------------------------------------------------------------------------
vtkMySQLTableReader::~vtkMySQLTableReader()
{
  if(this->Database->IsOpen())
    {
    this->Database->Close();
    }

  this->Database->Delete();
  this->Database = 0;
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::SetInput(vtkMySQLDatabase *db)
{
  this->Database = db;
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::SetHostName(const char *hostname)
{
  this->HostName = hostname;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::SetDatabaseName(const char *databasename)
{
  this->DatabaseName = databasename;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::SetUser(const char *user)
{
  this->User = user;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::SetPassword(const char *password)
{
  this->Password = password;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::TryToConnectToDatabase()
{
  if(this->HostName != "" && this->DatabaseName != "" &&
     this->User != "" && this->Password != "")
    {
    this->OpenDatabaseConnection();
    }
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::OpenDatabaseConnection()
{
  this->Database->SetHostName(this->HostName.c_str());
  this->Database->SetUser(this->User.c_str());
  this->Database->SetPassword(this->Password.c_str());
  this->Database->SetDatabaseName(this->DatabaseName.c_str());

  if(this->Database->IsOpen())
    {
    this->Database->Close();
    }

  if(!this->Database->Open())
    {
    vtkErrorMacro(<<"Error opening database!");
    }
}

//----------------------------------------------------------------------------
void vtkMySQLTableReader::CloseDatabaseConnection()
{
  this->Database->Close();
}

//----------------------------------------------------------------------------
vtkStringArray* vtkMySQLTableReader::GetTables()
{
  return this->Database->GetTables();
}

//----------------------------------------------------------------------------
bool vtkMySQLTableReader::SetTableName(const char *name)
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
bool vtkMySQLTableReader::CheckIfTableExists()
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
    tableNames->Delete();
    return false;
    }

  tableNames->Delete();
  return true;
}

//----------------------------------------------------------------------------
int vtkMySQLTableReader::RequestData(vtkInformation *,
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
  vtkstd::string queryStr = "SHOW COLUMNS FROM ";
  queryStr += this->TableName;
  vtkMySQLQuery *query =
    static_cast<vtkMySQLQuery*>(this->Database->GetQueryInstance());
  query->SetQuery(queryStr.c_str());
  if(!query->Execute())
    {
    vtkErrorMacro(<<"Error performing 'show columns' query");
    }
 
  //use the results of the query to create columns of the proper name & type
  std::vector<std::string> columnTypes;
  while(query->NextRow())
    {
    std::string columnName = query->DataValue(0).ToString();
    QString columnType = QString(query->DataValue(1).ToString().c_str());
    if(columnType.toLower().contains("int"))
      {
      vtkSmartPointer<vtkIntArray> column =
        vtkSmartPointer<vtkIntArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.push_back("int");
      }
    else if(columnType.toLower().contains("float") || 
      columnType.toLower().contains("double") || 
      columnType.toLower().contains("real") || 
      columnType.toLower().contains("decimal") || 
      columnType.toLower().contains("numeric"))
      {
      vtkSmartPointer<vtkDoubleArray> column =
        vtkSmartPointer<vtkDoubleArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.push_back("double");
      }
    else
      {
      vtkSmartPointer<vtkStringArray> column =
        vtkSmartPointer<vtkStringArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.push_back("string");
      }
    }

  //do a query to get the contents of the MySQL table
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
      if(columnTypes[col] == "int")
        {
        vtkIntArray *column =
          static_cast<vtkIntArray*>(output->GetColumn(col));
        column->InsertNextValue(query->DataValue(col).ToInt());
        }
      else if(columnTypes[col] == "double")
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
void vtkMySQLTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
     
