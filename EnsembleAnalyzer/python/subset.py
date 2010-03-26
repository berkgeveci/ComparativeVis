#!/usr/bin/env python

import os.path
import sys

if len(sys.argv) < 6:
  print "Usage: subset.py <input> <x> <y> <z> <output>"
  sys.exit(1)

from paraview.simple import *

#parse command line arguments
inputName = sys.argv[1]
x = sys.argv[2]
y  = sys.argv[3]
z = sys.argv[4]
outputName = sys.argv[5]

print "Extracting subset of %s from (0, 0, 0) to (%s, %s, %s)" \
  % (inputName, x, y, z)
sys.stdout.flush()

reader = XMLImageDataReader(FileName=[inputName])

extract = ExtractSubset()
extract.VOI = [0, int(x), 0, int(y), 0, int(z)]

print "Writing results to %s" % outputName
sys.stdout.flush()
writer = XMLImageDataWriter(FileName=outputName, Input=extract)
#writer = DataSetWriter(FileName=outputName, Input=extract)
writer.UpdatePipeline()
print "subset complete"
sys.stdout.flush()

del writer
del extract
del reader
