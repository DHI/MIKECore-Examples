using System;
using DHI.Generic.MikeZero.DFS;

namespace DHI.SDK.Examples
{
  /// <summary>
  /// Class for making a diff-file from two other files that have the same structure
  /// (same items and time steps), but actual numbers vary. 
  /// </summary>
  class DfsDiff
  {
    /// <summary> Static constructor </summary>
    static DfsDiff()
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
      if (args.Length < 2)
      {
        Console.Out.WriteLine("Must be called with two in structure identical dfs files");
        Console.Out.WriteLine("    Dfs0Diff.exe file1.dfs0 file2.dfs0 [diff.dfs0]");
        return;
      }

      string file1 = args[0];
      string file2 = args[1];
      string filediff;
      if (args.Length == 2)
        filediff = System.IO.Path.ChangeExtension(file1, ".diff" + System.IO.Path.GetExtension(file1));
      else
        filediff = args[2];

      CreateDiffFile(file1, file2, filediff);
    }

    /// <summary>
    /// Create a new file, being the difference of two files.
    /// <para>
    /// The two input files must be equal in structure, e.g. coming
    /// from the same simulation but giving different results.
    /// Header and static data must be identical, only difference
    /// must be in values of the dynamic data.
    /// </para>
    /// </summary>
    public static void CreateDiffFile(string file1, string file2, string filediff)
    {
      IDfsFile dfs1 = DfsFileFactory.DfsGenericOpen(file1);
      IDfsFile dfs2 = DfsFileFactory.DfsGenericOpen(file2);

      // Validate that it has the same number of items.
      if (dfs1.ItemInfo.Count != dfs2.ItemInfo.Count)
        throw new Exception("Number of dynamic items does not match");
      int numItems = dfs1.ItemInfo.Count;

      // In case number of time steps does not match, take the smallest.
      int numTimes = dfs1.FileInfo.TimeAxis.NumberOfTimeSteps;
      if (numTimes > dfs2.FileInfo.TimeAxis.NumberOfTimeSteps)
      {
        numTimes = dfs2.FileInfo.TimeAxis.NumberOfTimeSteps;
        Console.Out.WriteLine("Number of time steps does not match, using the smallest number");
      }

      // For recording max difference for every item
      double[] maxDiff = new double[dfs1.ItemInfo.Count];
      // Index in time (index) of maximum and first difference. -1 if no difference
      int[] maxDiffTime = new int[dfs1.ItemInfo.Count];
      int[] firstDiffTime = new int[dfs1.ItemInfo.Count];
      for (int i = 0; i < dfs1.ItemInfo.Count; i++)
      {
        maxDiffTime[i] = -1;
        firstDiffTime[i] = -1;
      }

      // Copy over info from the first file, assuming the second file contains the same data.
      IDfsFileInfo fileInfo = dfs1.FileInfo;

      DfsBuilder builder = DfsBuilder.Create(fileInfo.FileTitle, fileInfo.ApplicationTitle, fileInfo.ApplicationVersion);

      // Set up the header
      builder.SetDataType(fileInfo.DataType);
      builder.SetGeographicalProjection(fileInfo.Projection);
      builder.SetTemporalAxis(fileInfo.TimeAxis);
      builder.SetItemStatisticsType(fileInfo.StatsType);
      builder.DeleteValueByte = fileInfo.DeleteValueByte;
      builder.DeleteValueDouble = fileInfo.DeleteValueDouble;
      builder.DeleteValueFloat = fileInfo.DeleteValueFloat;
      builder.DeleteValueInt = fileInfo.DeleteValueInt;
      builder.DeleteValueUnsignedInt = fileInfo.DeleteValueUnsignedInt;

      // Transfer compression keys.
      if (fileInfo.IsFileCompressed)
      {
        int[] xkey;
        int[] ykey;
        int[] zkey;
        fileInfo.GetEncodeKey(out xkey, out ykey, out zkey);
        builder.SetEncodingKey(xkey, ykey, zkey);
      }

      // Copy custom blocks
      foreach (IDfsCustomBlock customBlock in fileInfo.CustomBlocks)
      {
        builder.AddCustomBlock(customBlock);
      }

      // Copy dynamic item definitions
      bool[] floatItems = new bool[dfs1.ItemInfo.Count];
      for (int i = 0; i < dfs1.ItemInfo.Count; i++)
      {
        var itemInfo = dfs1.ItemInfo[i];

        // Validate item sizes
        var itemInfo2 = dfs2.ItemInfo[i];
        if (itemInfo.ElementCount != itemInfo2.ElementCount)
          throw new Exception("Dynamic items must have same size, item number " + (i + 1) +
                              " has different sizes in the two files");
        // Validate the data type, only supporting floats and doubles.
        if (itemInfo.DataType == DfsSimpleType.Float)
          floatItems[i] = true;
        else if (itemInfo.DataType != DfsSimpleType.Double)
          throw new Exception("Dynamic item must be double or float, item number " + (i + 1) + " is of type " +
                              (itemInfo.DataType));

        builder.AddDynamicItem(itemInfo);
      }

      // Create file
      builder.CreateFile(filediff);

      // Copy over static items from file 1, assuming the static items of file 2 are identical
      IDfsStaticItem si1;
      while (null != (si1 = dfs1.ReadStaticItemNext()))
      {
        builder.AddStaticItem(si1);
      }

      // Get the file
      DfsFile diff = builder.GetFile();

      // Write dynamic data to the file, being the difference between the two
      for (int i = 0; i < numTimes; i++)
      {
        for (int j = 0; j < numItems; j++)
        {
          if (floatItems[j])
          {
            IDfsItemData<float> data1 = dfs1.ReadItemTimeStepNext() as IDfsItemData<float>;
            IDfsItemData<float> data2 = dfs2.ReadItemTimeStepNext() as IDfsItemData<float>;
            for (int k = 0; k < data1.Data.Length; k++)
            {
              float valuediff = data1.Data[k] - data2.Data[k];
              data1.Data[k] = valuediff;
              float absValueDiff = Math.Abs(valuediff);
              if (absValueDiff > maxDiff[j])
              {
                maxDiff[j]     = absValueDiff;
                maxDiffTime[j] = i;
                if (firstDiffTime[j] == -1)
                  firstDiffTime[j] = i;
              }
            }
            diff.WriteItemTimeStepNext(data1.Time, data1.Data);
          }
          else
          {
            IDfsItemData<double> data1 = dfs1.ReadItemTimeStepNext() as IDfsItemData<double>;
            IDfsItemData<double> data2 = dfs2.ReadItemTimeStepNext() as IDfsItemData<double>;
            for (int k = 0; k < data1.Data.Length; k++)
            {
              double valuediff = data1.Data[k] - data2.Data[k];
              data1.Data[k] = valuediff;
              double absValueDiff = Math.Abs(valuediff);
              if (absValueDiff > maxDiff[j])
              {
                maxDiff[j]     = absValueDiff;
                maxDiffTime[j] = i;
                if (firstDiffTime[j] == -1)
                  firstDiffTime[j] = i;
              }
            }
            diff.WriteItemTimeStepNext(data1.Time, data1.Data);
          }
        }
      }

      System.Console.WriteLine("Difference statistics:");
      for (int i = 0; i < maxDiffTime.Length; i++)
      {
        if (maxDiffTime[i] < 0)
        {
          Console.WriteLine("{0,-30}: no difference", dfs1.ItemInfo[i].Name);
        }
        else
        {
          Console.WriteLine("{0,-30}: Max difference at timestep {1,3}: {2}. First difference at timestep {3}", dfs1.ItemInfo[i].Name, maxDiffTime[i], maxDiff[i], firstDiffTime[i]);
        }
      }

      dfs1.Close();
      dfs2.Close();
      diff.Close();

    }
  }
}
