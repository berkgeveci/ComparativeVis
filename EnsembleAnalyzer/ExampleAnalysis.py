#! /usr/bin/env python

import sys
  
if len(sys.argv) < 4:
  print sys.argv[0] + " <database file> <experiment name> <contour value>"
  sys.exit(1)

import os.path
from libvtkDatabaseConnectionPython import *
from libvtkMetadataBrowserPython import *
from paraview.simple import *
from vtk import *
#import stats

def describe_outputs():

  filename_column = vtkStringArray()
  filename_column.SetName("contour_filename")
  output_table = vtkTable()
  output_table.AddColumn(filename_column)

  #mean_column = vtkFloatArray()
  #mean_column.SetName("mean")
  #output_table.AddColumn(mean_column)

  #std_dev_column = vtkFloatArray()
  #std_dev_column.SetName("std_deviation")
  #output_table.AddColumn(std_dev_column)

  return output_table


def process_member(fname, contourValue, output_table):
  outputName = fname[0:fname.rfind(".")] + "_contours.vtk"
  reader = XMLRectilinearGridReader(FileName=fname)
  contour = Contour(PointMergeMethod = "Uniform Binning", Input = reader)
  contour.Isosurfaces = [contourValue]
  writer = XMLPolyDataWriter(contour, FileName=outputName)
  writer.UpdatePipeline()

  output_table.GetColumnByName("contour_filename").InsertNextValue( outputName )
  #output_table.GetColumnByName("mean").InsertNextValue( stats.mean(reader, "Temperature") )
  #output_table.GetColumnByName("std_deviation").InsertNextValue( stats.std_deviation(reader, "Temperature") )

if __name__ == "__main__":
  #connect to database
  connection = vtkDatabaseConnection()
  connection.UseSQLite()
  connection.SetFileName(sys.argv[1])
  if connection.ConnectToDatabase() == False:
    sys.exit(1)

  #assume database and files are in the same directory
  dataDir = os.path.dirname(sys.argv[1])
  if dataDir == "":
    dataDir = "."

  contourValue = float(sys.argv[3])

  #create a browser to extract information from the database
  br = vtkMetadataBrowser()
  br.SetDatabaseConnection(connection)
  metadata = br.GetMetaDataFromExperiment(sys.argv[2])

  #initialize a new analysis table
  analysis = describe_outputs()

  #generate contours for all the files in the experiment
  fnames = metadata.GetColumnByName("Handle")
  for i in range(fnames.GetNumberOfValues()):
    fname = fnames.GetValue(i)
    output_fname = process_member(dataDir + "/" + fname, contourValue, analysis)

  #record this analysis in the database
  br.AddAnalysis("test_contour", analysis)

