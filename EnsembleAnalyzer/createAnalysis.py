#!/usr/bin/env python

import sqlite3
import sys

conn = sqlite3.connect('bin/weather.db')
c = conn.cursor()
try:
  c.execute("DROP TABLE analyses")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
try:
  c.execute("CREATE TABLE analyses(experiment INTEGER, input TEXT, operation TEXT, parameters TEXT, results TEXT, FOREIGN KEY(experiment) REFERENCES experiments(experimentid))")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
conn.commit()
c.close()
