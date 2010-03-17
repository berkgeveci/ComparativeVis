#include <iostream>
using std::cout;
using std::endl;

#include <vtkStringArray.h>
#include <vtkTable.h>

#include "vtkDatabaseConnection.h"
#include "vtkMetadataBrowser.h"

int main(int argc, char **argv)
{
  if(argc < 2)
    {
    cout << argv[0] << " <experimentName> " << endl;
    return 1;
    }

  vtkDatabaseConnection *connection = vtkDatabaseConnection::New();
  connection->UsePostgreSQL();
  connection->SetHostName("elysium");
  connection->SetDatabaseName("example");
  connection->SetUser("postgres");
  connection->SetPassword("2pw4kw");

  //test connection
  if(connection->ConnectToDatabase())
    {
    cout << "Connection successful!" << endl;
    }
  else
    {
    cout << "Connection failed!" << endl;
    connection->Delete();
    return 1;
    }

  //test bad query
  connection->ExecuteQuery("SELECT bogons FROM faketable;");

  vtkMetadataBrowser *browser = vtkMetadataBrowser::New();
  browser->SetDatabaseConnection(connection);

  //test GetListOfExperiments()
  vtkStringArray *experiments = browser->GetListOfExperiments();
  cout << "There are " << experiments->GetNumberOfValues() << " experiments:" << endl;
  for(int i = 0; i < experiments->GetNumberOfValues(); i++)
    {
    cout << experiments->GetValue(i) << endl;
    }
  cout << endl;
  experiments->Delete();

  //test GetExperiment
  std::string experimentName = argv[1];
  std::string query = "SELECT * FROM ";
  query += experimentName;

  vtkTable *experiment = browser->GetDataFromExperiment(experimentName, query);
  cout << "Experiment table '" << experimentName << "' has "
       << experiment->GetNumberOfRows() << " rows and "
       << experiment->GetNumberOfColumns() << " columns." << endl;
  experiment->Delete();

  //test GetAnalyses
  vtkTable *analyses = browser->GetAnalyses(experimentName);
  cout << "Analysis table has " << analyses->GetNumberOfRows() << " rows and "
       << analyses->GetNumberOfColumns() << " columns for experiment '"
       << experimentName << "'" << endl;
  analyses->Delete();

  browser->Delete();
  return 0;
}

