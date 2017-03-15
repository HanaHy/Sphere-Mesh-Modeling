#!/usr/bin/python

import numpy
import sys
from decimal import *
import argparse
import re

infile = str(sys.argv[1])
outfile = str(sys.argv[2])
mult = int(sys.argv[3])

print outfile

stri = ""

f = open(outfile, 'r+')
read = open(infile, 'r')
for line in read:
  l1 = line[:2]
  if l1 == 'v ':
    l2 = re.findall(r"[-+]?\d*\.\d+|\d+",line)
    stri = stri + ("v %f %f %f\n" % (mult*float(l2[0]), mult*float(l2[1]), mult*float(l2[2])))
  else:
    stri = stri + line

read.close()
f.write(stri)
f.close()
