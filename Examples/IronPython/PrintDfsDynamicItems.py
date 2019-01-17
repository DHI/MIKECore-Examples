# Usage: Writes information on dynamic items to console

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
from System import Array
from DHI.Generic.MikeZero import eumUnit, eumItem, eumQuantity
from DHI.Generic.MikeZero.DFS import *

if (len(sys.argv) == 1):
    print "Usage: Writes information on dynamic items to console"
    print "    ipy.exe PrintDfsDynamicItems.py dfsFileName.dfs"
    sys.exit()

filename = sys.argv[1]

dfsFile = DfsFileFactory.DfsGenericOpen(filename);

numItems = dfsFile.ItemInfo.Count

print "Number of items: %i" % (numItems)

prevTime = 0

for i in range(numItems):
    itemInfo = dfsFile.ItemInfo[i];
    print "%-50s;%-40s;%-20s;%5d;%s;%s" % (itemInfo.Name,itemInfo.Quantity.Item,itemInfo.Quantity.Unit,itemInfo.ElementCount,itemInfo.SpatialAxis.AxisType,itemInfo.ValueType)
