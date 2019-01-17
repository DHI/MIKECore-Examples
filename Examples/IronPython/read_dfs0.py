# A script loading a dfs0 file and calculating the average.

import clr
from math import *
import array

# The SetupLatest method will make your script find the MIKE assemblies at runtime.
# This is required for MIKE Version 2019 (17.0) and onwards. For previous versions, the 
# next three lines must be removed.
clr.AddReference("DHI.Mike.Install");
from DHI.Mike.Install import MikeImport, MikeProducts
MikeImport.SetupLatest(MikeProducts.MikeCore)

clr.AddReference("DHI.Generic.MikeZero.DFS");
clr.AddReference("DHI.Generic.MikeZero.EUM");
clr.AddReference("System");
import System
from System import Array
from System.Diagnostics import Stopwatch
from DHI.Generic.MikeZero import eumUnit, eumItem, eumQuantity
from DHI.Generic.MikeZero.DFS import *
from DHI.Generic.MikeZero.DFS.dfs123 import *

infile = "..\..\TestData\data_ndr_roese.dfs0";

# Load the file
dfs0File  = DfsFileFactory.DfsGenericOpen(infile);

# Read times and data
timer = Stopwatch()
timer.Start()
t = []
data = []
for ii in xrange(dfs0File.ItemInfo.Count):
  data.append([])
for it in xrange(dfs0File.FileInfo.TimeAxis.NumberOfTimeSteps):
  for ii in xrange(dfs0File.ItemInfo.Count):
    itemData = dfs0File.ReadItemTimeStep(ii+1,it);
    if (ii == 0):
      t.append(itemData.Time);
    data[ii].append(itemData.Data[0]);
timer.Stop();

dfs0File.Close();

print "Did app. {0} .NET calls per second".format(dfs0File.FileInfo.TimeAxis.NumberOfTimeSteps*(2*dfs0File.ItemInfo.Count+1)/timer.Elapsed.TotalSeconds);

# Calculate average
av = []
for ii in xrange(dfs0File.ItemInfo.Count):
  sum = 0;
  for it in xrange(dfs0File.FileInfo.TimeAxis.NumberOfTimeSteps):
    sum = sum + data[ii][it]
  av.append(sum/dfs0File.FileInfo.TimeAxis.NumberOfTimeSteps)

# Print out results
for ii in xrange(dfs0File.ItemInfo.Count):
  print "Average of item : {0:30} = {1}".format(dfs0File.ItemInfo[ii].Name,av[ii])



