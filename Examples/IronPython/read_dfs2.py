# Script that opens a dfs2 file and prints out some information in the file.
#
# This scripts is the IronPython counterpart of the read_dfs2 Matlab script


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
from DHI.Generic.MikeZero.DFS.dfs123 import *

print "";

filename = "..\..\TestData\OresundHD.dfs2";

# Load the file
dfs2File = DfsFileFactory.Dfs2FileOpen(filename);

# Print out some info on the spatial axis
axis = dfs2File.SpatialAxis;
print "Size of grid    : {0} x {1}".format(axis.XCount, axis.YCount);
print "Projection      : {0}".format(dfs2File.FileInfo.Projection.WKTString);

# Print out some info of the first item
dynamicItemInfo = dfs2File.ItemInfo[0];
print "Item 1 name     : {0}".format(dynamicItemInfo.Name);
print "Item 1 datatype : {0}".format(dynamicItemInfo.DataType);

# This iterates through the first 5 time steps and print out the value in the grid
# at index (3,4) for the first item
for i in range(0,5):
  data2D = dfs2File.ReadItemTimeStep(1, i);
  value = data2D[3, 4];
  print "Value in time step {0} = {1}".format(i, value);
