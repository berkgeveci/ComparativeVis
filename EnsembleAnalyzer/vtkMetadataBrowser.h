#ifndef __vtkMetadataBrowser_h
#define __vtkMetadataBrowser_h

#include "vtkObject.h"

class vtkDatabaseConnection;
class vtkStdString;
class vtkStringArray;
class vtkTable;

class VTK_EXPORT vtkMetadataBrowser : public vtkObject
{
public:
  static vtkMetadataBrowser *New();
  vtkTypeRevisionMacro(vtkMetadataBrowser, vtkObject);
 
  //browse database & extract metadata
  vtkStringArray* GetListOfExperiments(); 
  vtkTable* GetMetaDataFromExperiment(const char *experimentName);
  vtkTable* GetDataFromExperiment(const char *experimentName, const char *query);
  vtkTable* GetAnalyses(const char *analysisName);
  void SetDatabaseConnection(vtkDatabaseConnection *dbc);
  vtkDatabaseConnection* GetDatabaseConnection();
  int GetExperimentId(const char *experimentName);
  //BTX
  vtkTable* GetMetaDataFromExperiment(std::string experimentName);
  vtkTable* GetDataFromExperiment(std::string experimentName, std::string query);
  vtkTable* GetAnalyses(std::string analysisName);
  //ETX

  void AddAnalysis(const char *tableName, vtkTable *analysis);

protected:
	vtkMetadataBrowser();
	~vtkMetadataBrowser();

private:
  vtkDatabaseConnection *DatabaseConnection;
};
#endif

