#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkIndent.h"
#include "vtkTable.h"

#include "vtkMySQLDatabase.h"

int main(int argc, char **argv)
{
  if(argc < 5)
    {
    cerr << argv[0] << " <host> <db> <user> <passwd>" << endl;
    return 1;
    }
  vtkSmartPointer<vtkMySQLDatabase> db =
    vtkSmartPointer<vtkMySQLDatabase>::New();
  db->SetHostName(argv[1]);
  db->SetDatabaseName(argv[2]);
  db->SetUser(argv[3]);
  db->SetPassword(argv[4]);
  db->Open();
  if(db->IsOpen())
    {
    vtkStringArray *tableNames = db->GetTables();
    for(int i = 0; i < tableNames->GetNumberOfValues(); i++)
      {
      cout << tableNames->GetValue(i) << endl;
      }
    }
  else
    {
    cout << "unable to open a connection..." << endl;
    }
  return 0;
}

