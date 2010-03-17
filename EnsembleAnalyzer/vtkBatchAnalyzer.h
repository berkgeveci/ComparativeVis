#ifndef __vtkBatchAnalyzer_h
#define __vtkBatchAnalyzer_h

#include <string>
#include "vtkObject.h"

class QProcess;
class QStringList;

class VTK_EXPORT vtkBatchAnalyzer : public vtkObject
{
public:
  static vtkBatchAnalyzer *New();
  vtkTypeRevisionMacro(vtkBatchAnalyzer, vtkObject);

  bool ReadyToVisualize();
  QStringList GetListOfVisualizations(std::string filter="");
  void VisualizeDataset(std::string input, std::string output);
  //void VisualizeDataset(const char *input, const char *output);
  void VisualizeCollection(QStringList collection);
  
  //accessors
  void SetVisualization(const char *filepath);
  void SetParameters(const char *parameters);
  void SetScriptPath(const char *scriptPath);
  void SetDataPath(const char *dataPath);
  void SetResultsPath(const char *resultsPath);
  const char *GetScriptPath();
  const char *GetDataPath();
  const char *GetResultsPath();
  QProcess *Process;

protected:
	vtkBatchAnalyzer();
	~vtkBatchAnalyzer();

private:
  std::string Visualization; 
  std::string Parameters; 
  std::string ScriptPath;
  std::string DataPath;
  std::string ResultsPath;
};
#endif

