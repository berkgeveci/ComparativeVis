#include <iostream>
using std::cout;
using std::endl;

#include <QProcess>
#include <QStringList>

#include "vtkBatchAnalyzer.h"

int main(int argc, char **argv)
{
/*
  if(argc < 2)
    {
    cout << argv[0] << " <dataset> " << endl;
    return 1;
    }
*/

  vtkBatchAnalyzer *analyzer = vtkBatchAnalyzer::New();
  analyzer->SetScriptPath("/projects/MetadataBrowser/python");
  analyzer->SetDataPath("/projects/MetadataBrowser/data");
  analyzer->SetResultsPath("/projects/MetadataBrowser/results");

  QStringList visualizations = analyzer->GetListOfVisualizations("*.py");
  cout << "Here are the available visualizations: " << endl;
  for(int i = 0; i < visualizations.size(); i++)
    {
    cout << visualizations[i].toStdString() << endl;
    }
  
  analyzer->SetVisualization("contour.py");
  analyzer->SetParameters("200 20");

  analyzer->VisualizeDataset("sref_rsm_t03z_pgrb212_ctl1_f66.vtk", "test_analyzer.vtk");
  analyzer->Process->waitForFinished();
  analyzer->Delete();
  return 0;
}

