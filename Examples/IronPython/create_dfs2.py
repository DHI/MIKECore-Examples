# Script that creates a dfs2 file with a moving sinusoidal wave.
#
# This scripts is the IronPython counterpart of the create_dfs2 Matlab script

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

# create relative grid coordinates and create a 2D coordinate set using meshgrid
xv = array.array('f',range(0,1001,100)) # 1001 to assure that 1000 is included
yv = array.array('f',range(0,1001,200)) # 1001 to assure that 1000 is included
xysize = len(xv)*len(yv)

# Creates a new dfs2 file.
filename = 'test_created.dfs2';

# Create an empty dfs2 file object
factory = DfsFactory();
builder = Dfs2Builder.Create('Matlab dfs2 file','Matlab DFS',0);

# Set up the header
builder.SetDataType(1);
builder.SetGeographicalProjection(factory.CreateProjectionGeoOrigin('UTM-33', 12.4387, 55.2257, 327));
builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec,System.DateTime(1993,12,02,0,0,0),0,86400));
builder.SetSpatialAxis(factory.CreateAxisEqD2(eumUnit.eumUmeter,11,0,100,6,0,200));
builder.DeleteValueFloat = -1e-30;

# Add custom block 
# M21_Misc : {orientation (should match projection), drying depth, -900=has projection, land value, 0, 0, 0}
cbdata = Array.CreateInstance(System.Single, 7)
cbdata[0] = 327
cbdata[1] = 0.2
cbdata[2] = -900
cbdata[3] = 10
cbdata[4] = 0
cbdata[5] = 0
cbdata[6] = 0
builder.AddCustomBlock(factory.CreateCustomBlock('M21_Misc', cbdata));

# Add two items
builder.AddDynamicItem('H Water Depth m', eumQuantity.Create(eumItem.eumIWaterLevel, eumUnit.eumUmeter), DfsSimpleType.Float, DataValueType.Instantaneous);
builder.AddDynamicItem('P Flux m^3/s/m', eumQuantity.Create(eumItem.eumIFlowFlux, eumUnit.eumUm3PerSecPerM), DfsSimpleType.Float, DataValueType.Instantaneous);
builder.AddDynamicItem('Q Flux m^3/s/m', eumQuantity.Create(eumItem.eumIFlowFlux, eumUnit.eumUm3PerSecPerM), DfsSimpleType.Float, DataValueType.Instantaneous);

# Create the file ready for data
builder.CreateFile(filename);

# Add one static item, containing bathymetry data
data = Array.CreateInstance(System.Single, xysize)
xyi = 0
for y in yv:
  for x in xv:
    data[xyi] = -cos(2*x*(pi/1000)*sin(y*(pi/1000)))
    xyi = xyi+1
builder.AddStaticItem('Static item', eumQuantity.UnDefined, data);

dfs = builder.GetFile();

# create coordinates
xv = array.array('f',range(0,1001,100)) # 1001 to assure that 1000 is included
yv = array.array('f',range(0,1001,200)) # 1001 to assure that 1000 is included
xysize = len(xv)*len(yv)
# Put some data in the file
for i in range(0,24):
  data1 = Array.CreateInstance(System.Single, xysize)
  data2 = Array.CreateInstance(System.Single, xysize)
  data3 = Array.CreateInstance(System.Single, xysize)
  xyi = 0;
  for y in yv:
    for x in xv:
      data1[xyi] = (-cos(2*x*(pi/1000)-i*2*pi/25)*sin(y*(pi/1000)))
      data2[xyi] = (10*cos(x*(2*pi/1000) + pi/2 -i*2*pi/25)*sin(y*(pi/1000))) 
      data3[xyi] = (10*sin(x*(2*pi/1000) + pi/2 -i*2*pi/25)*cos(y*(pi/1000)))
      xyi = xyi+1
  
  dfs.WriteItemTimeStepNext(0, data1); 
  dfs.WriteItemTimeStepNext(0, data2); 
  dfs.WriteItemTimeStepNext(0, data3); 

dfs.Close();

print "\nFile created: {0}\n".format(filename)
