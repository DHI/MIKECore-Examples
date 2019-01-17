# Writes information on static items to console

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
    print "Usage: Writes information on static items to console"
    print "    ipy.exe PrintDfsStaticItems.py dfsFileName.dfs"
    sys.exit()

filename = sys.argv[1]

dfsFile = DfsFileFactory.DfsGenericOpen(filename);

i = 0
while True:
    staticItem = dfsFile.ReadStaticItemNext();
    if (staticItem == None):
        break;
    i = i+1
    print "====== Item %3d ======"  % (i)
    print "Name : %s"  % (staticItem.Name)
    print "Item : %s"  % (staticItem.Quantity.Item)
    print "Unit : %s"  % (staticItem.Quantity.Unit)
    print "Count: %5d" % (staticItem.ElementCount)
    print "Axis : %s"  % (staticItem.SpatialAxis.AxisType)
    print "Data : %s"  % (staticItem.DataType)
