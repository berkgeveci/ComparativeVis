#!/usr/bin/env python

import sqlite3

conn = sqlite3.connect('bin/weather.db')
c = conn.cursor()
try:
  c.execute("DROP TABLE experiments")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
try:
  c.execute("CREATE TABLE experiments(experimentid INTEGER PRIMARY KEY ASC, name TEXT, description TEXT)")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
try:
  c.execute("INSERT INTO experiments(name, description) VALUES('weather', 'example weather experiment')")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
try:
  c.execute("SELECT * FROM experiments")
except sqlite3.Error, e:
  print "Error occurred:", e.args[0]
  sys.exit(1)
conn.commit()
for row in c:
  print row
c.close()
  
