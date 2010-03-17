#!/usr/bin/env python
import os
import re
import sqlite3
import sys

if __name__ == "__main__":
  #grab all the .vtk filenames out of the directory and pull the two time
  #values out of the filename.
  files = os.listdir(sys.argv[1])
  rows = []
  regexp = re.compile(r"sref_([a-zA-Z]+)_t(\d+)z_pgrb212_([a-zA-Z0-9]+)_f(\d+)\.vti")
  for f in files:
    #only grab files that end with .vti
    if f.endswith(".vti"):
      match = regexp.search(f)
      if match:
        rows.append((match.group(1), match.group(3), match.group(2),
                    match.group(4), match.group(0)))
      else:
        print "no match on " + f
  try:
    conn = sqlite3.connect('bin/weather.db')
    c = conn.cursor()
    c.execute("DROP TABLE weather")
    c.execute("CREATE TABLE weather(Model TEXT, Perturbation TEXT, Initialization_Time INTEGER, Forecast_Hour INTEGER, Handle TEXT)")
  except sqlite3.Error, e:
    print "Error 1 occurred:", e.args[0]
    sys.exit(1)
  try:
    for r in rows:
      c.execute("INSERT INTO weather(Model, Perturbation, Initialization_Time, Forecast_Hour, Handle) VALUES(?,?,?,?,?)", r)
      conn.commit()
  except sqlite3.Error, e:
    print "Error 2 occurred:", e.args[0]
    sys.exit(1)

  try:
    c.execute("SELECT * FROM weather")
  except sqlite3.Error, e:
    print "Error 1 occurred:", e.args[0]
    sys.exit(1)
  for row in c:
    print row
  c.close()

