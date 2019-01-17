# Writes times of DFS file to console (csv format)

import sys
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
from System import Array, DateTime
from DHI.Generic.MikeZero import eumUnit, eumItem, eumQuantity
from DHI.Generic.MikeZero.DFS import *

if (len(sys.argv) == 1):
    print "Usage: Writes times of DFS file to console (csv format)"
    print "    ipy.exe PrintDfsTimes.py dfsFileName.dfs"
    sys.exit()

filename = sys.argv[1]

dfsFile = DfsFileFactory.DfsGenericOpen(filename);

numtimes = dfsFile.FileInfo.TimeAxis.NumberOfTimeSteps

print "Number of time steps: %i" % (numtimes)

prevTime = 0

startTime = DateTime.MinValue;
if (DfsExtensions.IsCalendar(dfsFile.FileInfo.TimeAxis)):
    startTime = dfsFile.FileInfo.TimeAxis.StartDateTime;

for i in range(numtimes):
    itemData = dfsFile.ReadItemTimeStep(1,i)
    newTime  = DfsExtensions.TimeInSeconds(itemData,dfsFile.FileInfo.TimeAxis)
    dt       = newTime-prevTime
    prevTime = newTime
    if (startTime == DateTime.MinValue):
        print "%5i;%18.10f;%18.10f" % (i, newTime, dt)
    else:
        print "%5i;%18.10f;%18.10f;  %s" % (i, newTime, dt, startTime.AddSeconds(newTime).ToString("yyyy-MM-dd HH:mm:ss"))
    
