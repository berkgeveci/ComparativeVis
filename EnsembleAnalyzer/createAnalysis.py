#!/usr/bin/env python

import sqlite3
import sys

try:
  conn = sqlite3.connect(sys.argv[1])
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)

c = conn.cursor()

try:
  c.execute("DROP TABLE analyses")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]

try:
  c.execute("CREATE TABLE analyses(analysis INTEGER, experiment INTEGER, name TEXT, description TEXT, FOREIGN KEY(experiment) REFERENCES experiments(experimentid))")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
conn.commit()
c.close()
