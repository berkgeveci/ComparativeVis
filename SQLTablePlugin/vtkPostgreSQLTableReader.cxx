/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPostgreSQLTableReader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <QString>
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkPostgreSQLQuery.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariant.h"
#include "vtkVariantArray.h"

#include "vtkPostgreSQLTableReader.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPostgreSQLTableReader, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkPostgreSQLTableReader);

//----------------------------------------------------------------------------
vtkPostgreSQLTableReader::vtkPostgreSQLTableReader()
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
  this->Database = vtkPostgreSQLDatabase::New();
}

//----------------------------------------------------------------------------
vtkPostgreSQLTableReader::~vtkPostgreSQLTableReader()
{
  if(this->Database->IsOpen())
    {
    this->Database->Close();
    }

  this->Database->Delete();
  this->Database = 0;
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::SetInput(vtkPostgreSQLDatabase *db)
{
  this->Database = db;
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::SetHostName(const char *hostname)
{
  this->HostName = hostname;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::SetDatabaseName(const char *databasename)
{
  this->DatabaseName = databasename;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::SetUser(const char *user)
{
  this->User = user;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::SetPassword(const char *password)
{
  this->Password = password;
  this->TryToConnectToDatabase();
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::TryToConnectToDatabase()
{
  if(this->HostName != "" && this->DatabaseName != "" &&
     this->User != "" && this->Password != "")
    {
    this->OpenDatabaseConnection();
    }
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::OpenDatabaseConnection()
{
  this->Database->SetHostName(this->HostName.c_str());
  this->Database->SetUser(this->User.c_str());
  this->Database->SetPassword(this->Password.c_str());
  this->Database->SetDatabaseName(this->DatabaseName.c_str());

  if(this->Database->IsOpen())
    {
    this->Database->Close();
    }
  if(!this->Database->Open(this->Password.c_str()))
    {
    vtkErrorMacro(<<"Error opening database!");
    }
}

//----------------------------------------------------------------------------
void vtkPostgreSQLTableReader::CloseDatabaseConnection()
{
  this->Database->Close();
}

//----------------------------------------------------------------------------
vtkStringArray* vtkPostgreSQLTableReader::GetTables()
{
  return this->Database->GetTables();
}

//----------------------------------------------------------------------------
bool vtkPostgreSQLTableReader::SetTableName(const char *name)
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
bool vtkPostgreSQLTableReader::CheckIfTableExists()
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
int vtkPostgreSQLTableReader::RequestData(vtkInformation *,
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
  vtkstd::string queryStr = 
    "select column_name, data_type FROM information_schema.columns WHERE table_name = '";
  queryStr += this->TableName;
  queryStr += "';";
  vtkPostgreSQLQuery *query =
    static_cast<vtkPostgreSQLQuery*>(this->Database->GetQueryInstance());
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
    if(columnType.toLower().contains("integer") ||
       columnType.toLower().contains("serial"))
      {
      vtkSmartPointer<vtkIntArray> column =
        vtkSmartPointer<vtkIntArray>::New();
      column->SetName(columnName.c_str());
      output->AddColumn(column);
      columnTypes.push_back("int");
      }
    else if(columnType.toLower().contains("double") || 
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

  //do a query to get the contents of the PostgreSQL table
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
void vtkPostgreSQLTableReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
     
