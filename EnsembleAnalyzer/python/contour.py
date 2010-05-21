#!/usr/bin/env python

import os.path
import sys

if len(sys.argv) < 4:
  print "Usage: contour.py <input1> [<input2>...] <contour value> <slice value> <output>"
  sys.exit(1)

from paraview.simple import *

#parse command line arguments
contourValue = float(sys.argv[len(sys.argv) - 3])
sliceValue = float(sys.argv[len(sys.argv) - 2])
outputName = sys.argv[len(sys.argv) - 1]
readers = []
contours = []
slices = []
transforms = []
print "Generating contour lines for value %f at slice %f" \
  % (contourValue, sliceValue)
sys.stdout.flush()
numInputs = 0

#generate a contour for each data set
for i in range(1,len(sys.argv) - 3):
  print "processing %s" % sys.argv[i]
  sys.stdout.flush()
  readers.append(XMLRectilinearGridReader(FileName=sys.argv[i]))
  slices.append(Slice( SliceType="Plane" ))
  slices[i-1].SliceType.Origin = [92.0, 64.0, sliceValue]
  slices[i-1].SliceType.Normal = [0.0, 0.0, 1.0]
  slices[i-1].SliceType = "Plane"

  transforms.append(Transform(Input = slices[i-1]))
  transforms[i-1].Transform.Scale = [5.535, 5.535, 1]

  contours.append(
    Contour(PointMergeMethod = "Uniform Binning", Input = transforms[i-1]))
    #Contour(PointMergeMethod = "Uniform Binning", Input = slices[i-1]))
  contours[i-1].Isosurfaces = [contourValue]
  numInputs += 1

#combine and color the contours
print "combining all %d results" % numInputs
sys.stdout.flush()
groupDatasets = GroupDatasets(Input=contours)
Show()
groupRep = GetDisplayProperties(groupDatasets)
lt = servermanager.rendering.PVLookupTable()
groupRep.LookupTable = lt
lt.RGBPoints = [0.0, 0, 0, 1, numInputs, 1, 0, 0]
lt.ColorSpace = 1
groupRep.GetProperty("ColorArrayName").SMProperty.SetElement(0, "vtkCompositeIndex")
groupRep.UpdateVTKObjects()
Render()

#load up the geopolitical map as a backdrop to the traces
scriptPath = os.path.dirname( os.path.realpath( __file__ ) )
PNGReader(FileName='%s/mapInvert.png' % scriptPath)
rep = Show()
rep.ColorArrayName = 'PNGImage'
rep.ColorAttributeType = 'POINT_DATA'
rep.LookupTable = []
rep.MapScalars = 0
Render()

sys.stdout.flush()
Render()
Show()
view = Render()

#position the camera nicely
view.ViewSize = [800, 600]
view.CameraFocalPoint = [430, 223.98818969726562, 10.0]
view.CameraPosition = [430, 223.98818969726562, 1353.733090815165]
camera = view.GetActiveCamera()
camera.Zoom(2.0)
Render()

#save a screenshot to disk
print "Output will be saved as " + outputName 
view.WriteImage(outputName, 1)

print "Contour complete"
sys.stdout.flush()

