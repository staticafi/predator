#! /usr/local/bin/python3

import sys

first = False
for line in sys.stdin:
  if "TRACE" not in line:
    continue
  if "NEW" in line:
    first = True
    continue
  
  line = line.split(' ')[1].split(':')[1:3]
  line = (':').join(line)
  if len(line) == 0:
    continue

  if first is True:
    first = False
    print("error: ", line.strip())
  else:
    print(line.strip())
