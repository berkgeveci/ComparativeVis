#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"

#include "vtkBatchAnalyzer.h"

vtkStandardNewMacro(vtkBatchAnalyzer);
vtkCxxRevisionMacro(vtkBatchAnalyzer, "$Revision: 1.1 $");

//-----------------------------------------------------------------------------
vtkBatchAnalyzer::vtkBatchAnalyzer()
{
  this->Visualization = "";
  this->Parameters = "";
  this->ScriptPath = SCRIPT_PATH;
  this->DataPath = DATA_PATH;
  this->ResultsPath = RESULTS_PATH;
  this->Process = new QProcess();
}

//-----------------------------------------------------------------------------
vtkBatchAnalyzer::~vtkBatchAnalyzer()
{
  delete this->Process;
}

//-----------------------------------------------------------------------------
QStringList vtkBatchAnalyzer::GetListOfVisualizations(std::string filter)
{
  //get directory listing from the ScriptPath
  QDir dir(this->ScriptPath.c_str());
  if(filter != "")
    {
    QStringList filters;
    filters << filter.c_str();
    dir.setNameFilters(filters);
    }
  QStringList visualizations = dir.entryList();

  //don't include . and .. in the return value
  int idx = visualizations.indexOf(".");
  if(idx != -1)
    {
    visualizations.removeAt(idx);
    }
  idx = visualizations.indexOf("..");
  if(idx != -1)
    {
    visualizations.removeAt(idx);
    }

  return visualizations;
}

//-----------------------------------------------------------------------------
bool vtkBatchAnalyzer::ReadyToVisualize()
{
  if(this->Visualization != "")
    {
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::VisualizeDataset(std::string input, std::string output)
{
  QString qInput(input.c_str());
  QString qOutput(output.c_str());

  if(this-ReadyToVisualize() == false)
    {
    return;
    }
  QString fullPathToScript(this->ScriptPath.c_str());
  fullPathToScript += QString("/");
  fullPathToScript += this->Visualization.c_str();
  
  QString inputFileName(this->DataPath.c_str());
  inputFileName.append(QString("/"));
  inputFileName.append(qInput);

  QString outputFileName(this->ResultsPath.c_str());
  outputFileName.append(QString("/"));
  outputFileName.append(qOutput);
  
  QStringList arguments;

  arguments << inputFileName;
  if(this->Parameters != "")
    {
    arguments = arguments + QString(this->Parameters.c_str()).split(" "); 
    }
  arguments << outputFileName;
  cout << fullPathToScript.toStdString();
  for(int i = 0; i < arguments.size(); i++)
    {
    cout << " " << arguments[i].toStdString();
    }
  cout << endl;
  this->Process->start(fullPathToScript, arguments);
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::VisualizeCollection(QStringList collection)
{

  for(int i = 0; i < collection.size(); ++i)
    {
    //generate an output name based on the input name
    const char *input = collection[i].toStdString().c_str();
    QFileInfo inputFileInfo(input);
    QString outputFilePath = inputFileInfo.baseName() + "_out"
      + inputFileInfo.completeSuffix();
    const char *output = outputFilePath.toStdString().c_str();
    //run the visualization
    this->VisualizeDataset(input, output);
    }
}
  
//-----------------------------------------------------------------------------
const char* vtkBatchAnalyzer::GetScriptPath()
{
  return this->ScriptPath.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkBatchAnalyzer::GetDataPath()
{
  return this->ScriptPath.c_str();
}

//-----------------------------------------------------------------------------
const char* vtkBatchAnalyzer::GetResultsPath()
{
  return this->ScriptPath.c_str();
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::SetScriptPath(const char * scriptPath)
{
  this->ScriptPath = scriptPath;
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::SetDataPath(const char * dataPath)
{
  this->DataPath = dataPath;
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::SetResultsPath(const char * resultsPath)
{
  this->ResultsPath = resultsPath;
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::SetVisualization(const char *script)
{
  this->Visualization = script;
}

//-----------------------------------------------------------------------------
void vtkBatchAnalyzer::SetParameters(const char *parameters)
{
  this->Parameters = parameters;
}

