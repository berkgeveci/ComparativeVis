#include <vector>

#include "vtkMySQLDatabase.h"
#include "vtkObjectFactory.h"
#include "vtkPostgreSQLDatabase.h"
#include "vtkRowQueryToTable.h"
#include "vtkSQLiteDatabase.h"
#include "vtkSQLQuery.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

#include "vtkSmartPointer.h"

#include "vtkDatabaseConnection.h"
#include "vtkMetadataBrowser.h"
#include "vtkTableToSQLiteWriter.h" 

vtkStandardNewMacro(vtkMetadataBrowser);
vtkCxxRevisionMacro(vtkMetadataBrowser, "$Revision: 1.1 $");

vtkCxxSetObjectMacro(vtkMetadataBrowser, DatabaseConnection, vtkDatabaseConnection);

//-----------------------------------------------------------------------------
vtkMetadataBrowser::vtkMetadataBrowser()
{
  this->DatabaseConnection = 0;
}

//-----------------------------------------------------------------------------
vtkMetadataBrowser::~vtkMetadataBrowser()
{
  if(this->DatabaseConnection)
    {
    this->DatabaseConnection->Delete();
    }
}

//-----------------------------------------------------------------------------
vtkDatabaseConnection* vtkMetadataBrowser::GetDatabaseConnection()
{
  return this->DatabaseConnection;
}

//-----------------------------------------------------------------------------
vtkStringArray* vtkMetadataBrowser::GetListOfExperiments() 
{
  //initialize the vector that we will be returning
  vtkStringArray* experiments = vtkStringArray::New();

  //make sure we're ready to query the database
  if(this->DatabaseConnection == 0 ||
     this->DatabaseConnection->CheckDatabaseConnection() == false)
    {
    cerr << "Before calling GetListOfExperiments() you must 1) specify what "
         << "type of database you're using, and 2) connect to it by calling "
         << "ConnectToDatabase()" << endl;
    return experiments;
    }

  //get a list of tables from the database by querying the 'experiments' table
  bool ok = this->DatabaseConnection->ExecuteQuery(
    "SELECT name FROM experiments;");
  if(!ok)
    {
    return experiments;
    }

  vtkVariantArray* va = vtkVariantArray::New();
  while (this->DatabaseConnection->GetSQLQuery()->NextRow( va ) )
    {
    experiments->InsertNextValue(va->GetValue(0).ToString());
    }

  va->Delete();
  return experiments;
}

//-----------------------------------------------------------------------------
vtkTable* vtkMetadataBrowser::GetAnalyses(std::string experimentName)
{
  //instantiate return value
  vtkTable* analyses = vtkTable::New();

  //check if we're ready to query the database
  if(this->DatabaseConnection == 0 ||
     this->DatabaseConnection->CheckDatabaseConnection() == false)
    {
    cerr << "You must connect to a database before calling GetAnalyses()"
         << endl;
    return analyses;
    }

 //create a vector of columns from the analyses table
  std::vector<vtkVariantArray *> columns;
  vtkStringArray *columnNames;
  if(this->DatabaseConnection->IsUsingMySQL())
    {
    columnNames =
      this->DatabaseConnection->GetMySQLDatabase()->GetRecord("analyses");
    }
  else if(this->DatabaseConnection->IsUsingSQLite())
    {
    columnNames = this->DatabaseConnection->GetSQLiteDatabase()->
      GetRecord("analyses");
    }
  else
    { //postgres
    columnNames = this->DatabaseConnection->GetPostgreSQLDatabase()->
      GetRecord("analyses");
    }
  for(int col = 0; col < columnNames->GetNumberOfValues(); col++)
    {
    //set the columns' names based on the database
    columns.push_back(vtkVariantArray::New());
    (columns[col])->SetName(columnNames->GetValue(col));
    }
  columnNames->Delete();

  //get the experimentid for the requested experiment
  std::string query = "SELECT experimentid FROM experiments WHERE name='"; 
  query += experimentName;
  query += "';";
  bool ok = this->DatabaseConnection->ExecuteQuery(query.c_str());
  if(!ok)
    {
    return analyses;
    }

  vtkVariantArray* va = vtkVariantArray::New();
  vtkSQLQuery *sqlQuery = this->DatabaseConnection->GetSQLQuery();
  sqlQuery->NextRow( va );
  std::string experimentId = va->GetValue(0).ToString();

  //now that we have the experimentId, we can get all the analyses performed on
  //this experiment
  query = "SELECT * from analyses WHERE experiment='";
  query += experimentId;
  query += "';";
  ok = this->DatabaseConnection->ExecuteQuery(query.c_str());
  if(!ok)
    {
    va->Delete();
    return analyses;
    }
 
  //construct a table from the returned rows
  while(sqlQuery->NextRow())
    {
    for(int col = 0; col < sqlQuery->GetNumberOfFields(); ++col)
      {
      columns[col]->InsertNextValue(sqlQuery->DataValue(col));
      }
    }

  //combine the columns into a vtkTable and cleanup
  for(unsigned int col = 0; col < columns.size(); col++)
    {
    analyses->AddColumn(columns[col]);
    (columns[col])->Delete();
    }
  va->Delete();
  return analyses;
}

//-----------------------------------------------------------------------------
vtkTable* vtkMetadataBrowser::GetAnalyses(const char *experimentName)
{
  std::string experiment = experimentName;
  return this->GetAnalyses(experiment);
}

//-----------------------------------------------------------------------------
vtkTable* vtkMetadataBrowser::GetDataFromExperiment(std::string experimentName,
                                                    std::string query)
{
  //instantiate return value
  vtkTable* experiment = vtkTable::New();

  //check if we're ready to query the database
  if(this->DatabaseConnection == 0 ||
     this->DatabaseConnection->CheckDatabaseConnection() == false)
    {
    cerr << "You must connect to a database before calling "
         << "GetDataFromExperiment()" << endl;
    return experiment;
    }

  //don't require supplied queries to contain "FROM <experimentName>"
  //since that information is supplied separately
  if(query.find("from") == std::string::npos &&
     query.find("FROM") == std::string::npos)
    {
    query = query + " FROM " + experimentName;
    }

  //select from the experiment table
  bool ok = this->DatabaseConnection->ExecuteQuery(query.c_str());
  if(!ok)
    {
    return experiment;
    }
 
  //construct a table from the returned rows
  vtkSQLQuery *sqlQuery = this->DatabaseConnection->GetSQLQuery();
  vtkSmartPointer<vtkRowQueryToTable> rqtt = 
    vtkSmartPointer<vtkRowQueryToTable>::New();
  rqtt->SetQuery(sqlQuery);
  rqtt->Update();
  experiment->ShallowCopy(rqtt->GetOutput());
  // while(sqlQuery->NextRow())
  //   {
  //   for(int col = 0; col < sqlQuery->GetNumberOfFields(); ++ col)
  //     {
  //     columns[col]->InsertNextValue(sqlQuery->DataValue(col));
  //     }
  //   }
  // 
  // //combine the columns into a vtkTable and cleanup
  // for(unsigned int col = 0; col < columns.size(); col++)
  //   {
  //   experiment->AddColumn(columns[col]);
  //   (columns[col])->Delete();
  //   }
  return experiment;
}

//-----------------------------------------------------------------------------
vtkTable* vtkMetadataBrowser::GetDataFromExperiment(const char *experimentName,
                                                    const char *query)
{
  std::string experiment = experimentName;
  std::string queryStr = query;
  return this->GetDataFromExperiment(experiment, queryStr);
}

//-----------------------------------------------------------------------------
vtkTable* 
vtkMetadataBrowser::GetMetaDataFromExperiment(const char *experimentName)
{
  const char *query = "SELECT *";
  return this->GetDataFromExperiment(experimentName, query);
}

//-----------------------------------------------------------------------------
vtkTable* 
vtkMetadataBrowser::GetMetaDataFromExperiment(std::string experimentName)
{
  std::string query = "SELECT *";
  return this->GetDataFromExperiment(experimentName, query);
}

//-----------------------------------------------------------------------------
int vtkMetadataBrowser::GetExperimentId(const char *experimentName)
{
  std::string query = "SELECT experimentid FROM experiments WHERE name = '";
  query += experimentName;
  query += "';";
  bool ok = this->DatabaseConnection->ExecuteQuery(query.c_str());
  if(!ok)
    {
    return -1;
    }
  vtkVariantArray* va = vtkVariantArray::New();
  vtkSQLQuery *sqlQuery = this->DatabaseConnection->GetSQLQuery();
  sqlQuery->NextRow( va );
  int experimentId = va->GetValue(0).ToInt();
  va->Delete();
  return experimentId;
}

//-----------------------------------------------------------------------------
void vtkMetadataBrowser::AddAnalysis(const char *tableName, vtkTable *analysis)
{
  if(this->GetDatabaseConnection()->IsUsingSQLite() == false)
    {
    cerr << "AddAnalysis is only implemented for SQLite so far." << endl;
    return;
    }

  vtkSmartPointer<vtkTableToSQLiteWriter> analysisWriter = 
    vtkSmartPointer<vtkTableToSQLiteWriter>::New();
  
  analysisWriter->SetDatabase(this->DatabaseConnection->GetSQLiteDatabase());
  analysisWriter->SetInput(analysis);
  analysisWriter->SetTableName(tableName);
  analysisWriter->Update();
}

