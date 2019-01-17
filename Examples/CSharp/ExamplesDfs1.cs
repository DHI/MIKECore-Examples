using System;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfs123;

namespace DHI.SDK.Examples
{
  /// <summary>
  /// Class with example methods related to dfs1 files.
  /// </summary>
  public class ExamplesDfs1
  {
    /// <summary> Static constructor </summary>
    static ExamplesDfs1()
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

    /// <summary>
    /// Introductory example of how to load a dfs1 file.
    /// <para>
    /// The method assumes that the wln.dfs1 test file
    /// is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of wln.dfs1 test file</param>
    public static void ReadingDfs1File(string filename)
    {
      // Open the file as a dfs1 file
      Dfs1File dfs1File = DfsFileFactory.Dfs1FileOpen(filename);

      // Spatial axis for this file is a 2D equidistant axis
      IDfsAxisEqD1 axisEqD1 = ((IDfsAxisEqD1)dfs1File.SpatialAxis);
      double dx = axisEqD1.Dx;                                           // 900

      // Header information is contained in the IDfsFileInfo
      IDfsFileInfo fileInfo = dfs1File.FileInfo;
      int steps = fileInfo.TimeAxis.NumberOfTimeSteps;                   // 577
      
      // Information on each of the dynamic items, here the first one
      IDfsSimpleDynamicItemInfo dynamicItemInfo = dfs1File.ItemInfo[0];
      string nameOfFirstDynamicItem = dynamicItemInfo.Name;              // "WL-N (m)"
      DfsSimpleType typeOfFirstDynamicItem = dynamicItemInfo.DataType;   // Float

      // Read data of first item, third time step (items start by 1, timesteps by 0),
      // assuming data is of type float.
      IDfsItemData<float> data = (IDfsItemData<float>)dfs1File.ReadItemTimeStep(1, 2);

    }

  }
}