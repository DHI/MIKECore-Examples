# This script uses the zedgraph.dll to draw a graph of a dfs0 item
# 
# The following is required:
# - zedgraph.dll
# - ZgForm.py
# They need only be present in the same folder as the script using them.

import clr

# The SetupLatest method will make your script find the MIKE assemblies at runtime.
# This is required for MIKE Version 2019 (17.0) and onwards. For previous versions, the 
# next three lines must be removed.
clr.AddReference("DHI.Mike.Install");
from DHI.Mike.Install import MikeImport, MikeProducts
MikeImport.SetupLatest(MikeProducts.MikeCore)

clr.AddReference('System.Drawing')
clr.AddReference('System.Windows.Forms')
clr.AddReference('ZedGraph')
clr.AddReference("DHI.Generic.MikeZero.DFS");
clr.AddReference("DHI.Generic.MikeZero.EUM");

from System.Drawing import (Color)
from System.Windows.Forms import (Application)
from ZedGraph import (PointPairList,
                      GraphPane,
                      Fill,
                      SymbolType, AxisType, XDate)
                      
from ZgForm import ZgControl, ZgForm

from DHI.Generic.MikeZero import eumUnit, eumItem, eumQuantity
from DHI.Generic.MikeZero.DFS import *
from DHI.Generic.MikeZero.DFS.dfs123 import *
               
        
# Initialize a Windows Form
Application.EnableVisualStyles()
Application.SetCompatibleTextRenderingDefault(False)

# Create a ZedGraph form and extract the GraphPane
form = ZgForm()

myPane = form.zgControl.GraphPane

# Load data from dfs0 info PointPairList
itemNo = 1;

# Load the file
infile = "..\..\TestData\data_ndr_roese.dfs0";
dfs0File  = DfsFileFactory.DfsGenericOpen(infile);

myPane.Title.Text = "dfs0 file plot"
myPane.XAxis.Title.Text = "time"
myPane.YAxis.Title.Text = dfs0File.ItemInfo[itemNo].Quantity.UnitAbbreviation
myPane.XAxis.Type = AxisType.Date;

timeAxis = dfs0File.FileInfo.TimeAxis
list = PointPairList()
for it in xrange(timeAxis.NumberOfTimeSteps):
    itemData = dfs0File.ReadItemTimeStep(itemNo,it);
    date = XDate(timeAxis.StartDateTime.AddSeconds(DfsExtensions.TimeInSeconds(itemData, timeAxis)))
    list.Add(date.XLDate, itemData.Data[0] )

# Done with the file now
dfs0File.Close();

# Add the curve
myCurve = myPane.AddCurve( dfs0File.ItemInfo[itemNo].Name, list, Color.Black, SymbolType.Diamond )
# Don't display the line (This makes a scatter plot)
myCurve.Line.IsVisible = False
# Hide the symbol outline
myCurve.Symbol.Border.IsVisible = False
# Fill the symbol interior with color
myCurve.Symbol.Fill = Fill( Color.Firebrick )

# Fill the background of the chart rect and pane
myPane.Chart.Fill = Fill( Color.White, Color.LightGoldenrodYellow, 45.0 )
myPane.Fill = Fill( Color.White, Color.SlateGray, 45.0 )

# Run the form (show it)
Application.Run(form)
    