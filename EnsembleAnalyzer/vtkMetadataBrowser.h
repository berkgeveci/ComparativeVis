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
  vtkTable* GetDataFromExperiment(const char *experimentName, const char *query);
  vtkTable* GetAnalyses(const char *analysisName);
  void SetDatabaseConnection(vtkDatabaseConnection *dbc);
  vtkDatabaseConnection* GetDatabaseConnection();
  int GetExperimentId(const char *experimentName);
  //BTX
  vtkTable* GetDataFromExperiment(std::string experimentName, std::string query);
  vtkTable* GetAnalyses(std::string analysisName);
  //ETX

protected:
	vtkMetadataBrowser();
	~vtkMetadataBrowser();

private:
  vtkDatabaseConnection *DatabaseConnection;
};
#endif

