#!/usr/bin/env python
import os
import re
import sqlite3
import sys

if __name__ == "__main__":
  try:
    conn = sqlite3.connect('bin/weather.db')
    c = conn.cursor()
  except sqlite3.Error, e:
    print "Error occurred:", e.args[0]
    sys.exit(1)
  try:
    c.execute("SELECT * FROM %s" % sys.argv[1])
  except sqlite3.Error, e:
    print "Error occurred:", e.args[0]
    sys.exit(1)
  for row in c:
    print row
  c.close()
