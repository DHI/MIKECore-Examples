using System;
using System.Collections.Generic;
using DHI.Generic.MikeZero;
using DHI.Generic.MikeZero.DFS;

namespace DHI.MikeCore.Examples
{


  /// <summary>
  /// Class with example methods for modifying different parts of 
  /// different dfs files
  /// </summary>
  public class ExamplesMisc
  {
    /// <summary> Static constructor </summary>
    static ExamplesMisc()
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
    /// Example of how to copy a Dfs file.
    /// <para>
    /// This example is intended to show how to generically copy a file. In
    /// case a copy with modified data is required, this could be used as a base
    /// for the copy. 
    /// </para>
    /// </summary>
    /// <param name="sourceFilename">Path and name of the source dfs file</param>
    /// <param name="filename">Path and name of the new file to create</param>
    public static void CopyDfsFile(string sourceFilename, string filename)
    {
      IDfsFile source = DfsFileFactory.DfsGenericOpen(sourceFilename);
      IDfsFileInfo fileInfo = source.FileInfo;

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

      // Transfer compression keys - if any.
      if (fileInfo.IsFileCompressed)
      {
        int[] xkey;
        int[] ykey;
        int[] zkey;
        fileInfo.GetEncodeKey(out xkey, out ykey, out zkey);
        builder.SetEncodingKey(xkey, ykey, zkey);
      }

      // Copy custom blocks - if any
      foreach (IDfsCustomBlock customBlock in fileInfo.CustomBlocks)
      {
        builder.AddCustomBlock(customBlock);
      }

      // Copy dynamic items
      foreach (var itemInfo in source.ItemInfo)
      {
        builder.AddDynamicItem(itemInfo);
      }

      // Create file
      builder.CreateFile(filename);

      // Copy static items
      IDfsStaticItem sourceStaticItem;
      while (null != (sourceStaticItem = source.ReadStaticItemNext()))
      {
        builder.AddStaticItem(sourceStaticItem);
      }

      // Get the file
      DfsFile file = builder.GetFile();

      // Copy dynamic item data
      IDfsItemData sourceData;
      while (null != (sourceData = source.ReadItemTimeStepNext()))
      {
        file.WriteItemTimeStepNext(sourceData.Time, sourceData.Data);
      }

      source.Close();
      file.Close();
    }

    public static readonly string UsageMergeDfsFileItems = @"
    -dfsMerge: Merge two or more dfs files into one

        DHI.MikeCore.Util -dfsMerge [outMergeFile] [file1] [file2]
        DHI.MikeCore.Util -dfsMerge [outMergeFile] [file1] [file2] [file3] ...

        Merge any number of dfs files. The merger is on dynamic item basis, 
        i.e. add all dynamic items of a number of dfs files to a new dfs file.
        Static data is copied from [file1].

        It is assumed that all files has the same time stepping layout. It will merge
        as many time steps as the file with the least number of timesteps.

        If merging one of the specific types of dfs files, dfs0 or dfs1 or dfs2 or dfs3, 
        the structure of the files must be identical, i.e. the sizes of the axis must equal. 
        Otherwise, the outcome will not be a valid dfs0/1/2/3 file.
";



    /// <summary>
    /// Example of how to merge two or more dfs files. The merger is on dynamic item basis, 
    /// i.e. add all dynamic items of a number of dfs files to a new dfs file.
    /// <para>
    /// It is assumed that all files has the same time stepping layout. It will merge
    /// as many time steps as the file with the least number of timesteps.
    /// </para>
    /// <para>
    /// If merging one of the specific types of dfs files, dfs0 or dfs1 or dfs2 or dfs3, 
    /// the structure of the files must be identical, i.e. the sizes of the axis must equal. 
    /// Otherwise, the outcome will not be a valid dfs0/1/2/3 file.
    /// </para>
    /// </summary>
    /// <param name="targetFilename">Path and name of the new file to create</param>
    /// <param name="sourcesFilenames">Path and name of the source dfs files</param>
    public static void MergeDfsFileItems(string targetFilename, IList<string> sourcesFilenames)
    {
      // List of sources to be merged - in case of more than one, just extend this.
      List<IDfsFile> sources = new List<IDfsFile>();
      for (int i = 0; i < sourcesFilenames.Count; i++)
      {
        sources.Add(DfsFileFactory.DfsGenericOpen(sourcesFilenames[i]));
      }
      
      // Use the first file as skeleton for header and static items.
      IDfsFile source = sources[0];
      IDfsFileInfo fileInfo = source.FileInfo;

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

      // Transfer compression keys - if any.
      if (fileInfo.IsFileCompressed)
      {
        int[] xkey;
        int[] ykey;
        int[] zkey;
        fileInfo.GetEncodeKey(out xkey, out ykey, out zkey);
        builder.SetEncodingKey(xkey, ykey, zkey);
      }

      // Copy custom blocks - if any
      foreach (IDfsCustomBlock customBlock in fileInfo.CustomBlocks)
      {
        builder.AddCustomBlock(customBlock);
      }

      int minNumTimesteps = int.MaxValue;

      // Copy dynamic items for all source files
      for (int j = 0; j < sources.Count; j++)
      {
        if (sources[j].FileInfo.TimeAxis.NumberOfTimeSteps < minNumTimesteps)
          minNumTimesteps = sources[j].FileInfo.TimeAxis.NumberOfTimeSteps;

        foreach (var itemInfo in sources[j].ItemInfo)
        {
          builder.AddDynamicItem(itemInfo);
        }
      }

      // Create file
      builder.CreateFile(targetFilename);

      // Copy static items - add only from main file
      IDfsStaticItem sourceStaticItem;
      while (null != (sourceStaticItem = source.ReadStaticItemNext()))
      {
        builder.AddStaticItem(sourceStaticItem);
      }

      // Get the file
      DfsFile file = builder.GetFile();

      // Copy dynamic item data
      IDfsItemData sourceData;
      for (int i = 0; i < minNumTimesteps; i++)
      {
        for (int j = 0; j < sources.Count; j++)
        {
          IDfsFile sourcej = sources[j];
          // Copy all items for this source
          for (int k = 0; k < sourcej.ItemInfo.Count; k++)
          {
            sourceData = sourcej.ReadItemTimeStepNext();
            file.WriteItemTimeStepNext(sourceData.Time, sourceData.Data);
          }
        }
      }

      foreach (IDfsFile sourcej in sources)
      {
        sourcej.Close();
      }
      file.Close();
    }


    /// <summary>
    /// Example of how to append data from one file to another. It is assumed that:
    /// <list type="bullet">
    /// <item>The files has identical dynamic and static items</item>
    /// <item>The last time step of the target file is equal to the first
    ///       timestep of the sourceFile, and therefor the first time step
    ///       from the source file is not added to the target file</item>
    /// </list>
    /// <para>
    /// This example uses the generic DFS functionality, and will work for any type
    /// of DFS file.
    /// </para>
    /// </summary>
    public static void AppendToFile(string targetFile, string sourceFile)
    {
      // Open target for appending and source for reading
      IDfsFile target = DfsFileFactory.DfsGenericOpenAppend(targetFile);
      IDfsFile source = DfsFileFactory.DfsGenericOpen(sourceFile);

      // Time of last time step of file, in the time unit of the time axis.
      // This is sufficient as long as TimeAxis.StartTimeOffset equals in 
      // source and target file (it is zero for most files)
      double targetEndTime = target.FileInfo.TimeAxis.TimeSpan();

      // Do not add initial time step 0 of source to target file, 
      // so go directly to time step 1 in source
      source.FindTimeStep(1);

      // Copy over data
      IDfsItemData sourceData2;
      while (null != (sourceData2 = source.ReadItemTimeStepNext()))
      {
        target.WriteItemTimeStepNext(targetEndTime + sourceData2.Time, sourceData2.Data);
      }

      // Close the files
      target.Close();
      source.Close();
    }

    /// <summary>
    /// Updates information in the header - <see cref="IDfsFileInfo"/>.
    /// <para>
    /// The method assumes that the OresundHD.dfs2 test file
    /// (or preferably a copy of it) is the input file.
    /// </para>
    /// <para>
    /// Strings are padded with zeros, when too short, and truncated when too long.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of OresundHD.dfs2 test file</param>
    public static void FileInfoModify(string filename)
    {
      IDfsFile dfsFile = DfsFileFactory.DfsGenericOpenEdit(filename);

      IDfsFileInfo fileInfo = dfsFile.FileInfo;

      // Modify values
      fileInfo.FileTitle = "ups";
      fileInfo.ApplicationTitle = "Short title";
      fileInfo.ApplicationVersion = 12;
      fileInfo.DataType = 10101;

      fileInfo.DeleteValueFloat = -5.5e-25f;
      fileInfo.DeleteValueByte = 7;
      fileInfo.DeleteValueDouble = -7.7e-114;
      fileInfo.DeleteValueInt = -123456;
      fileInfo.DeleteValueUnsignedInt = 123456;

      dfsFile.Close();
    }
    
    /// <summary>
    /// Example on how to modify a custom block.
    /// <para>
    /// The method assumes that a dfs2 file with the "M21_Misc" custom block, alike 
    /// the OresundHD.dfs2 test file, is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of dfs2 test file</param>
    public static void CustomBlockModify(string filename)
    {
      IDfsFile dfsFile = DfsFileFactory.DfsGenericOpenEdit(filename);

      IDfsFileInfo fileInfo = dfsFile.FileInfo;
      IDfsCustomBlock customBlock = fileInfo.CustomBlocks[0];
      customBlock[3] = 25;

      dfsFile.Close();
    }

    /// <summary>
    /// Updates the temporal axis of a file with an <see cref="IDfsEqCalendarAxis"/>
    /// type time axis.
    /// <para>
    /// The method will work on a file like the OresundHD.dfs2 test file, which has
    /// an <see cref="IDfsEqCalendarAxis"/> type time axis.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of test file</param>
    public static void TemporalAxisModify(string filename)
    {
      IDfsFile dfsFile = DfsFileFactory.DfsGenericOpenEdit(filename);
      IDfsEqCalendarAxis timeAxis = (IDfsEqCalendarAxis)dfsFile.FileInfo.TimeAxis;

      // Update values
      timeAxis.FirstTimeStepIndex = 3;
      timeAxis.StartTimeOffset = 6;
      timeAxis.StartDateTime = new DateTime(2009, 2, 2, 21, 43, 00);
      timeAxis.TimeUnit = eumUnit.eumUminute;
      timeAxis.TimeStep = 1;

      dfsFile.Close();
    }

    /// <summary>
    /// Updates the item info.
    /// <para>
    /// The method assumes that the OresundHD.dfs2 test file
    /// (or preferably a copy of it) is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of OresundHD.dfs2 test file</param>
    public static void DynamicItemInfoModify(string filename)
    {
      IDfsFile dfsFile = DfsFileFactory.DfsGenericOpenEdit(filename);

      IDfsDynamicItemInfo itemInfo = dfsFile.ItemInfo[2];

      // Update the values
      // old name: "Q Flux m^3/s/m". New name: "ShortD        " (padded with spaces)
      // old quantity: (eumItem.eumIFlowFlux, eumUnit.eumUm3PerSecPerM)
      // old ValueType: Instantaneous
      itemInfo.Name = "ShortD";
      itemInfo.Quantity = eumQuantity.Create(eumItem.eumIDischarge, eumUnit.eumUm3PerSec);
      itemInfo.ValueType = DataValueType.MeanStepBackward;

      // Old reference coordinates and orientation is -1.00000002e-35f
      itemInfo.SetReferenceCoordinates(1, 2, 3);
      itemInfo.SetOrientation(4, 5, 6);

      dfsFile.Close();
    }

  }
}
