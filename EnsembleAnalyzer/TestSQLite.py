from vtk import *
from libvtkMetadataBrowserAPIPython import *
from vtk import *
import sys

if __name__ == "__main__":
  if len(sys.argv) < 2:
    print sys.argv[0] + " <experimentName";
    sys.exit(1)
  api = vtkMetadataBrowserAPI();
  api.UseSQLite();
  api.SetFileName("weather.db");

  #test connection
  if(api.ConnectToDatabase()):
    print "Connection successful!"
  else:
    print "Connection failed!"
    sys.exit(1)

  #test bad query
  api.ExecuteQuery("SELECT bogons FROM faketable;");

  #test GetListOfExperiments()
  experiments = api.GetListOfExperiments()
  print str(type(experiments))
  print dir(experiments)
  print "There are %d experiments:" % experiments.GetNumberOfValues()

  for i in range(0, experiments.GetNumberOfValues()):
    print  experiments.GetValue(i)
  experiments.Delete();

  #test GetExperiment
  experiment = api.GetExperiment(sys.argv[1]);
  print "Experiment table '%s' has %d rows and %d columns." \
    % (sys.argv[1], experiment.GetNumberOfRows(), experiment.GetNumberOfColumns())
  experiment.Delete();

  #test GetAnalyses
  analyses = api.GetAnalyses(sys.argv[1]);
  print "Analysis table has %d rows and %d columns for experiment %s" \
    % (analyses.GetNumberOfRows(), analyses.GetNumberOfColumns(), sys.argv[1])
  analyses.Delete();
  api.Delete();

