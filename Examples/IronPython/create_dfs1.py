# Script that creates a dfs1 file with a moving sinusoidal wave.
#
# This scripts is the IronPython counterpart of the create_dfs1 Matlab script

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

# Create a new dfs1 file
filename = 'test_created.dfs1';

# Create an empty dfs1 file object
factory = DfsFactory();
builder = Dfs1Builder.Create('Matlab dfs1 file','Matlab DFS',0);
builder.SetDataType(0);

# Create a temporal definition
builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec,System.DateTime(2002,2,25,13,45,32),0,60));

# Create a spatial defition
builder.SetSpatialAxis(factory.CreateAxisEqD1(eumUnit.eumUmeter,11,0,100));
builder.SetGeographicalProjection(factory.CreateProjectionGeoOrigin('UTM-33',12,54,2.6));

# Add two items
builder.AddDynamicItem('Surface height',eumQuantity(eumItem.eumISurfaceElevation,eumUnit.eumUmeter),DfsSimpleType.Float,DataValueType.Instantaneous);
builder.AddDynamicItem('current',eumQuantity(eumItem.eumICurrentSpeed,eumUnit.eumUmeterPerSec),DfsSimpleType.Float,DataValueType.Instantaneous);

# Create the file - make it ready for data
builder.CreateFile(filename);
dfs = builder.GetFile();

# read coordinates
xv = array.array('f',range(0,1001,100)) # 1001 to assure that 1000 is included
# Put some date in the file
for i in range(0,10): 
  data1 = Array.CreateInstance(System.Single, len(xv))
  data2 = Array.CreateInstance(System.Single, len(xv))
  xi = 0;
  for x in xv:
    data1[xi] =   -cos(x*(2*pi/1000)-i*2*pi/10); 
    data2[xi] = 10*cos(x*(2*pi/1000)-i*2*pi/10+pi/2); 
    xi = xi+1

  dfs.WriteItemTimeStepNext(0, data1); 
  dfs.WriteItemTimeStepNext(0, data2); 


dfs.Close();

print "\nFile created: {0}\n".format(filename)
