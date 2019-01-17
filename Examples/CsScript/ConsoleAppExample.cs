using System;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfs123;

namespace ConsoleApplication
{
  class Program
  {
    /// <summary> Static constructor </summary>
    static Program()
    {
      // The setup method will make your application find the MIKE assemblies at runtime.
      // The first call of the setup method takes precedense. Any subsequent calls will be ignored.
      // It must be called BEFORE any method using MIKE libraries is called, i.e. it is not sufficient
      // to call it as the first thing in that method using the MIKE libraries. Often this can be achieved
      // by having this code in the static constructor.
      // If MIKE Core is x-copy deployed with the application, this is not required.
      if (!DHI.Mike.Install.MikeImport.Setup(17, DHI.Mike.Install.MikeProducts.MikeCore))
        throw new Exception("Cannot find a proper MIKE installation");
    }

    static void Main(string[] args)
    {
      string filename = @"..\..\TestData\OresundHD.dfs2";

      // Load the file
      Dfs2File dfs2File = DfsFileFactory.Dfs2FileOpen(filename);

      // Print out some info on the spatial axis
      IDfsAxisEqD2 axis = (IDfsAxisEqD2)dfs2File.SpatialAxis;
      Console.Out.WriteLine("Size of grid    : {0} x {1}", axis.XCount, axis.YCount);
      Console.Out.WriteLine("Projection      : "+dfs2File.FileInfo.Projection.WKTString);
      
      // Print out some info of the first item
      IDfsSimpleDynamicItemInfo dynamicItemInfo = dfs2File.ItemInfo[0];
      Console.Out.WriteLine("Item 1 name     : " + dynamicItemInfo.Name);
      Console.Out.WriteLine("Item 1 datatype : " + dynamicItemInfo.DataType);
      
      // This iterates through the first 5 time steps and print out the value in the grid
      // at index (3,4) for the first item
      for (int i = 0; i < 5; i++)
      {
        IDfsItemData2D<float> data2D = (IDfsItemData2D<float>)dfs2File.ReadItemTimeStep(1, i);
        float value = data2D[3, 4];
        Console.Out.WriteLine("Value in time step {0} = {1}", i, value);
      }
    }
  }
}
