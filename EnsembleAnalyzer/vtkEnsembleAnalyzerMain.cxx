#include <QtGui/QApplication>
#include "vtkEnsembleAnalyzer.h"

int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  vtkEnsembleAnalyzer *analyzer = new vtkEnsembleAnalyzer();
  //analyzer->show();
  int retval = app.exec();
  delete analyzer;
  return retval;
}

