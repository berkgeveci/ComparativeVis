from vtk import *
from libvtkDatabaseConnectionPython import *
from libvtkMetadataBrowserPython import *
from vtk import *
import sys

if __name__ == "__main__":
  if len(sys.argv) < 3:
    print sys.argv[0] + " <path/to/db-file> <experimentName>";
    sys.exit(1)

  db = vtkDatabaseConnection();
  db.UseSQLite();
  db.SetFileName(sys.argv[1]);
  
  #test connection
  if(db.ConnectToDatabase()):
    print "Connection successful!"
  else:
    print "Connection failed!"
    sys.exit(1)

  #test bad query
  db.ExecuteQuery("SELECT bogons FROM faketable;");

  browser = vtkMetadataBrowser();
  browser.SetDatabaseConnection(db);

  #test GetListOfExperiments()
  experiments = browser.GetListOfExperiments()
  print "There are %d experiments:" % experiments.GetNumberOfValues()

  for i in range(0, experiments.GetNumberOfValues()):
    print  experiments.GetValue(i)
  experiments.Delete();

  #test GetExperiment
  experimentName = sys.argv[2]
  query = "SELECT * FROM " + experimentName

  experiment = browser.GetDataFromExperiment(experimentName, query)
  print "Experiment table '%s' has %d rows and %d columns." \
    % (experimentName, experiment.GetNumberOfRows(), experiment.GetNumberOfColumns())
  experiment.Delete()

  #test GetAnalyses
  analyses = browser.GetAnalyses(experimentName);
  print "Analysis table has %d rows and %d columns for experiment %s" \
    % (analyses.GetNumberOfRows(), analyses.GetNumberOfColumns(), experimentName)
  analyses.Delete();
  browser.Delete();

