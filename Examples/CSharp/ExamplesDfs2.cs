using System;
using System.Linq;
using DHI.Generic.MikeZero;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfs123;
using DHI.Projections;

// ReSharper disable RedundantAssignment
#pragma warning disable 168 // disable warning for 'never used' variables in Visual Studio

namespace DHI.SDK.Examples
{
  /// <summary>
  /// Class with example methods related to dfs2 files.
  /// </summary>
  public class ExamplesDfs2
  {
    /// <summary> Static constructor </summary>
    static ExamplesDfs2()
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
    /// Introductory example of how to load a dfs2 file.
    /// <para>
    /// The method assumes that the OresundHD.dfs2 test file
    /// is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of OresundHD.dfs2 test file</param>
    public static void ReadingDfs2File(string filename)
    {
      // Open the file as a dfs2 file
      Dfs2File dfs2File = DfsFileFactory.Dfs2FileOpen(filename);

      // Spatial axis for this file is a 2D equidistant axis
      IDfsAxisEqD2 axisEqD2 = ((IDfsAxisEqD2)dfs2File.SpatialAxis);
      double dx = axisEqD2.Dx;                                           // 900
      double dy = axisEqD2.Dy;                                           // 900

      // Header information is contained in the IDfsFileInfo
      IDfsFileInfo fileInfo = dfs2File.FileInfo;
      int steps = fileInfo.TimeAxis.NumberOfTimeSteps;                   // 13
      string projectionString = fileInfo.Projection.WKTString;           // "UTM-33"

      // Information on each of the dynamic items, here the first one
      IDfsSimpleDynamicItemInfo dynamicItemInfo = dfs2File.ItemInfo[0];
      string nameOfFirstDynamicItem = dynamicItemInfo.Name;              // "H Water Depth m"
      DfsSimpleType typeOfFirstDynamicItem = dynamicItemInfo.DataType;   // Float

      // Read data of first item, third time step (items start by 1, timesteps by 0),
      // assuming data is of type float.
      IDfsItemData2D<float> data2D = (IDfsItemData2D<float>)dfs2File.ReadItemTimeStep(1, 2);
      // Get the value at (i,j) = (3,4) of the item and timestep
      float value = data2D[3, 4];                                        // 11.3634329

      // This iterates through all the timesteps and items in the file
      // For performance reasons it is important to iterate over time steps
      // first and items second.
      for (int i = 0; i < steps; i++)
      {
        for (int j = 1; j <= dfs2File.ItemInfo.Count; j++)
        {
          data2D = (IDfsItemData2D<float>)dfs2File.ReadItemTimeStep(j, i);
          value = data2D[3, 4];
        }
      }
    }


    /// <summary>
    /// Example of how to update item information in a dfs2 file.
    /// <para>
    /// The method assumes that the Landuse.dfs2 test file
    /// (or preferably a copy of it) is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">Path and name of Landuse.dfs2 test file</param>
    public static void ModifyDfs2ItemInfo(string filename)
    {
      // Open the file for editing
      IDfs2File file = DfsFileFactory.Dfs2FileOpenEdit(filename);

      // Original name is "Landuse" (7 characters), "GroundUse" is truncated to "GroundU"
      file.ItemInfo[0].Name = "GroundUse";
      // Provide a new quantity (updating the item and unit of the quantity directly does not work!)
      file.ItemInfo[0].Quantity = new eumQuantity(eumItem.eumIAreaFraction, eumUnit.eumUPerCent);

      // done
      file.Close();
    }

    /// <summary>
    /// Example of how to update item axis in a dfs2 file.
    /// <para>
    /// The method assumes that the OresundHD.dfs2 test file
    /// (or preferably a copy of it) is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">Path and name of OresundHD.dfs2 test file</param>
    public static void ModifyDfs2ItemAxis(string filename)
    {
      Dfs2File file = DfsFileFactory.Dfs2FileOpenEdit(filename);
      
      IDfsAxisEqD2 axisEqD2 = ((IDfsAxisEqD2)file.SpatialAxis);
      axisEqD2.X0 = 55;
      axisEqD2.Dx = 905;
      axisEqD2.Y0 = -55;
      axisEqD2.Dy = 915;
      
      file.Close();

    }

    
    /// <summary>
    /// Example of how to modify data of a certain item and time
    /// step in a dfs2 file.
    /// <para>
    /// The method assumes that the Landuse.dfs2 test file
    /// (or preferably a copy of it) is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">Path and name of Landuse.dfs2 test file</param>
    public static void ModifyDfs2FileData(string filename)
    {
      // Open the file for editing
      IDfs2File file = DfsFileFactory.Dfs2FileOpenEdit(filename);

      // Load and modify data from the first item and timestep
      IDfsItemData2D data2D = file.ReadItemTimeStepNext();
      data2D[21, 61] = 7f;
      data2D[21, 62] = 6f;
      data2D[21, 63] = 5f;
      data2D[21, 64] = 4f;
      data2D[21, 65] = 3f;

      // Write modified data back
      file.WriteItemTimeStep(1, 0, data2D.Time, data2D.Data);

      // done
      file.Close();
    }

    /// <summary>
    /// Update DFS2 bathymetry, lowering bathymetry with 5.61 meters everywhere,
    /// taking land value into account.
    /// <para>
    /// The method assumes that the OresundBathy900.dfs2 test file
    /// (or preferably a copy of it) is the input file.
    /// </para>
    /// </summary>
    /// <param name="bathyFilename">Path and name of OresundBathy900.dfs2 test file</param>
    public static void ModifyDfs2Bathymetry(string bathyFilename)
    {
      // Open file
      Dfs2File dfs2 = DfsFileFactory.Dfs2FileOpenEdit(bathyFilename);

      // Second custom block (index 1) contains the M21_MISC values, 
      // where the 4th (index 3) is the land value
      float landValue = (float)dfs2.FileInfo.CustomBlocks[1][3];

      // Read bathymetry data
      IDfsItemData2D<float> bathyData = (IDfsItemData2D<float>)dfs2.ReadItemTimeStepNext();

      // Modify bathymetry data
      for (int i = 0; i < bathyData.Data.Length; i++)
      {
        if (bathyData.Data[i] != landValue)
        {
          bathyData.Data[i] -= 5.61f;
        }
      }

      // Write back bathymetry data
      dfs2.WriteItemTimeStep(1, 0, 0, bathyData.Data);
      dfs2.Close();
    }


    /// <summary>
    /// Example of how to create a Dfs2 file from scratch. This method
    /// creates a copy of the OresundHD.dfs2 test file.
    /// <para>
    /// Data for static and dynamic item is taken from a source dfs file,
    /// which here is the OresundHD.dfs2 test file. The data could come
    /// from any other source. 
    /// </para>
    /// </summary>
    /// <param name="sourceFilename">Path and name of the OresundHD.dfs2 test file</param>
    /// <param name="filename">Path and name of the new file to create</param>
    public static void CreateDfs2File(string sourceFilename, string filename)
    {
      IDfs2File source = DfsFileFactory.Dfs2FileOpen(sourceFilename);

      DfsFactory factory = new DfsFactory();
      Dfs2Builder builder = Dfs2Builder.Create("", @"C:\Program Files\DHI\2010\bin\nmodel.exe", 0);

      // Set up the header
      builder.SetDataType(1);
      builder.SetGeographicalProjection(factory.CreateProjectionGeoOrigin("UTM-33", 12.438741600559766, 55.225707842436385, 326.99999999999955));
      builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec, new DateTime(1993, 12, 02, 0, 0, 0), 0, 86400));
      builder.SetSpatialAxis(factory.CreateAxisEqD2(eumUnit.eumUmeter, 71, 0, 900, 91, 0, 900));
      builder.DeleteValueFloat = -1e-30f;

      // Add custom block 
      // M21_Misc : {orientation (should match projection), drying depth, -900=has projection, land value, 0, 0, 0}
      builder.AddCustomBlock(factory.CreateCustomBlock("M21_Misc", new float[] { 327f, 0.2f, -900f, 10f, 0f, 0f, 0f }));

      // Set up dynamic items
      builder.AddDynamicItem("H Water Depth m", eumQuantity.Create(eumItem.eumIWaterLevel, eumUnit.eumUmeter), DfsSimpleType.Float, DataValueType.Instantaneous);
      builder.AddDynamicItem("P Flux m^3/s/m", eumQuantity.Create(eumItem.eumIFlowFlux, eumUnit.eumUm3PerSecPerM), DfsSimpleType.Float, DataValueType.Instantaneous);
      builder.AddDynamicItem("Q Flux m^3/s/m", eumQuantity.Create(eumItem.eumIFlowFlux, eumUnit.eumUm3PerSecPerM), DfsSimpleType.Float, DataValueType.Instantaneous);

      // Create file
      builder.CreateFile(filename);

      // Add static items containing bathymetri data, use data from source
      IDfsStaticItem sourceStaticItem = source.ReadStaticItemNext();
      builder.AddStaticItem("Static item", eumQuantity.UnDefined, sourceStaticItem.Data);

      // Get the file
      Dfs2File file = builder.GetFile();

      // Loop over all time steps
      for (int i = 0; i < source.FileInfo.TimeAxis.NumberOfTimeSteps; i++)
      {
        // Loop over all items
        for (int j = 0; j < source.ItemInfo.Count; j++)
        {
          // Add data for all item-timesteps, copying data from source file.

          // Read data from source file
          IDfsItemData2D<float> sourceData = (IDfsItemData2D<float>)source.ReadItemTimeStepNext();

          // Create empty item data, and copy over data from source
          // The IDfsItemData2D can handle 2D indexing, on the form data2D[k,l].
          // An ordinary array, float[], can also be used, though indexing from 2D to 1D must be 
          // handled by user code i.e. using data1D[k + l*xCount] compared to data2D[k,l]
          IDfsItemData2D<float> itemData2D = (IDfsItemData2D<float>)file.CreateEmptyItemData(j+1);
          for (int k = 0; k < 71; k++)
          {
            for (int l = 0; l < 91; l++)
            {
              itemData2D[k, l] = sourceData[k, l];
            }
          }
          // the itemData2D.Data is a float[], so any float[] of the correct size is valid here.
          file.WriteItemTimeStep(j + 1, i, sourceData.Time, itemData2D.Data);
        }
      }

      source.Close();
      file.Close();
    }

    public static readonly string MaxVelocityFieldUsage = @"
    -MaxVelocityField: Create maximum velocity field for a dfs2 file

        DHI.MikeCore.Util -MaxVelocityField [sourceFilename] [outputFilename]

        From a dfs2 file containing items (H-P-Q), (P-Q-Speed) or  (u-v-Speed), 
        find maximum velocity for each cell and store in [outputFilename]
";

    /// <summary>
    /// Create maximum velocity field for a dfs2 file
    /// <para>
    /// FFrom a dfs2 file containing items (H-P-Q), (P-Q-Speed) or  (u-v-Speed), 
    /// find maximum velocity for each cell and store in [outputFilename] 
    /// </para>
    /// </summary>
    public static void MaxVelocityField(string sourceFilename, string outfilename)
    {
      // Open source file
      IDfs2File source = DfsFileFactory.Dfs2FileOpen(sourceFilename);

      // Create output file
      Dfs2Builder builder = Dfs2Builder.Create("Max Velocity", @"MIKE SDK", 0);

      // Set up the header
      builder.SetDataType(1);
      builder.SetGeographicalProjection(source.FileInfo.Projection);
      builder.SetTemporalAxis(source.FileInfo.TimeAxis);
      builder.SetSpatialAxis(source.SpatialAxis);
      builder.DeleteValueFloat = -1e-30f;

      // Add custom block 
      foreach (IDfsCustomBlock customBlock in source.FileInfo.CustomBlocks)
      {
        builder.AddCustomBlock(customBlock);
      }

      // Set up dynamic items
      builder.AddDynamicItem("Maximum Speed"  , eumQuantity.Create(eumItem.eumIFlowVelocity, eumUnit.eumUmeterPerSec), DfsSimpleType.Float, DataValueType.Instantaneous);
      builder.AddDynamicItem("u-velocity"     , eumQuantity.Create(eumItem.eumIFlowVelocity, eumUnit.eumUmeterPerSec), DfsSimpleType.Float, DataValueType.Instantaneous);
      builder.AddDynamicItem("v-velocity"     , eumQuantity.Create(eumItem.eumIFlowVelocity, eumUnit.eumUmeterPerSec), DfsSimpleType.Float, DataValueType.Instantaneous);
      //builder.AddDynamicItem("H Water Depth m", eumQuantity.Create(eumItem.eumIWaterLevel, eumUnit.eumUmeter), DfsSimpleType.Float, DataValueType.Instantaneous);

      // Create file
      builder.CreateFile(outfilename);

      // Add static items containing bathymetri data, use data from source

      IDfsStaticItem sourceStaticItem;
      while(null != (sourceStaticItem = source.ReadStaticItemNext()))
        builder.AddStaticItem(sourceStaticItem.Name, sourceStaticItem.Quantity, sourceStaticItem.Data);

      // Get the file
      Dfs2File file = builder.GetFile();

      // Arrays storing max-speed values
      int numberOfCells = file.SpatialAxis.SizeOfData;
      float[] maxSpeed    = new float[numberOfCells];
      float[] uAtMaxSpeed = new float[numberOfCells];
      float[] vAtMaxSpeed  = new float[numberOfCells];
      // Initialize with delete values
      for (int i = 0; i < numberOfCells; i++)
      {
        maxSpeed[i] = source.FileInfo.DeleteValueFloat;
        uAtMaxSpeed[i] = source.FileInfo.DeleteValueFloat;
        vAtMaxSpeed[i] = source.FileInfo.DeleteValueFloat;
      }

      // Create empty ItemData's, for easing reading of source data
      IDfsItemData2D<float>[] datas = new IDfsItemData2D<float>[source.ItemInfo.Count];
      for (int i = 0; i < source.ItemInfo.Count; i++)
      {
        datas[i] = source.CreateEmptyItemData<float>(i + 1);
      }

      // Find HPQ items in file - uses StartsWith, since the string varies slightly with the version of the engine.
      int dIndex = source.ItemInfo.FindIndex(item => item.Name.StartsWith("H Water Depth", StringComparison.OrdinalIgnoreCase));
      int pIndex = source.ItemInfo.FindIndex(item => item.Name.StartsWith("P Flux", StringComparison.OrdinalIgnoreCase));
      int qIndex = source.ItemInfo.FindIndex(item => item.Name.StartsWith("Q Flux", StringComparison.OrdinalIgnoreCase));
      int sIndex = source.ItemInfo.FindIndex(item => item.Name.StartsWith("Current Speed", StringComparison.OrdinalIgnoreCase));
      int uIndex = source.ItemInfo.FindIndex(item => item.Name.StartsWith("U velocity", StringComparison.OrdinalIgnoreCase));
      int vIndex = source.ItemInfo.FindIndex(item => item.Name.StartsWith("V velocity", StringComparison.OrdinalIgnoreCase));
      // Either p and q must be there, or u and v, and either d or s must be there.
      bool haspq = (pIndex >= 0 && qIndex >= 0);
      bool hasuv = (uIndex >= 0 && vIndex >= 0);
      if (!hasuv && !haspq || dIndex < 0 && sIndex < 0)
      {
        throw new Exception("Could not find items. File must have H-P-Q items, P-Q-Speed or U-V-Speed items");
      }
      IDfsItemData2D<float> dItem = dIndex >= 0 ? datas[dIndex] : null;
      IDfsItemData2D<float> pItem = pIndex >= 0 ? datas[pIndex] : null;
      IDfsItemData2D<float> qItem = qIndex >= 0 ? datas[qIndex] : null;
      IDfsItemData2D<float> sItem = sIndex >= 0 ? datas[sIndex] : null;
      IDfsItemData2D<float> uItem = uIndex >= 0 ? datas[uIndex] : null;
      IDfsItemData2D<float> vItem = vIndex >= 0 ? datas[vIndex] : null;

      // Spatial 2D axis
      IDfsAxisEqD2 axis = (IDfsAxisEqD2) source.SpatialAxis;
      double dx = axis.Dx;
      double dy = axis.Dy;

      // Loop over all time steps
      for (int i = 0; i < source.FileInfo.TimeAxis.NumberOfTimeSteps; i++)
      {
        // Read data for all items from source file. That will also update the depth, p and q.
        for (int j = 0; j < source.ItemInfo.Count; j++)
        {
          source.ReadItemTimeStep(datas[j], i);
        }

        // For each cell, find maximum speed and store u, v and depth at that point in time.
        for (int j = 0; j < numberOfCells; j++)
        {
          // Skip delete values
          if (dItem?.Data[j] == source.FileInfo.DeleteValueFloat ||
              sItem?.Data[j] == source.FileInfo.DeleteValueFloat)
            continue;

          double p = pItem.Data[j];
          double q = qItem.Data[j];
          double speed, u, v;
          if (sItem != null)
          {
            // Use speed from result file
            speed = sItem.Data[j];

            if (hasuv) 
            {
              // Use u and v from result file
              u = uItem.Data[j];
              v = vItem.Data[j];
            }
            else // (haspq)
            {
              // Calculate u and v from speed and direction of p and q
              double pqLength = Math.Sqrt(p * p + q * q);
              u = hasuv ? uItem.Data[j] : speed * p / pqLength;
              v = hasuv ? vItem.Data[j] : speed * q / pqLength;
            }
          }
          else // (dItem != null)
          {
            // Current speed is not directly available in source file, calculate from u and v
            if (hasuv)
            {
              u = uItem.Data[j];
              v = vItem.Data[j];
            }
            else
            {
              // u and v is not available, calculate fromdh, p and q.
              double d = dItem.Data[j];
              u = pItem.Data[j] / d;
              v = qItem.Data[j] / d;
            }
            speed = Math.Sqrt(u * u + v * v);
          }
          if (speed > maxSpeed[j])
          {
            maxSpeed[j]    = (float)speed;
            uAtMaxSpeed[j] = (float)u;
            vAtMaxSpeed[j] = (float)v;
          }
        }
      }

      file.WriteItemTimeStepNext(0, maxSpeed);
      file.WriteItemTimeStepNext(0, uAtMaxSpeed);
      file.WriteItemTimeStepNext(0, vAtMaxSpeed);
      //file.WriteItemTimeStepNext(0, maxDepth);

      source.Close();
      file.Close();
    }

    /// <summary>
    /// Example of how to create a M21 Dfs2 Bathymetry from scratch. This method
    /// creates a file matching the OresundBathy900.dfs2 test file.
    /// </summary>
    /// <param name="bathyDataArray">Array of bathymetry data, 1D array with 2D data, size n x m</param>
    /// <param name="filename">Path and name of the new file to create</param>
    public static void CreateM21Bathymetry(float[] bathyDataArray, string filename)
    {
      DfsFactory factory = new DfsFactory();
      Dfs2Builder builder = Dfs2Builder.Create(@"C:\0\Training\Bat1_0.dfs2", @"Grid editor", 1);

      // Set up the header
      builder.SetDataType(0);
      builder.SetGeographicalProjection(factory.CreateProjectionGeoOrigin("UTM-33", 12.438741600559911, 55.2257078424238, 327));
      builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec, new DateTime(2003, 01, 01, 0, 0, 0), 0, 1));
      builder.SetSpatialAxis(factory.CreateAxisEqD2(eumUnit.eumUmeter, 72, 0, 900, 94, 0, 900));
      builder.DeleteValueFloat = -1e-30f;

      builder.AddCustomBlock(factory.CreateCustomBlock("Display Settings", new int[] { 1, 0, 0 }));
      builder.AddCustomBlock(factory.CreateCustomBlock("M21_Misc", new float[] { 327f, 0f, -900f, 10f, 0f, 0f, 0f }));

      // Set up dynamic items
      builder.AddDynamicItem("Bathymetry", eumQuantity.Create(eumItem.eumIWaterLevel, eumUnit.eumUmeter),
                             DfsSimpleType.Float, DataValueType.Instantaneous);

      // Create and get file
      builder.CreateFile(filename);
      Dfs2File file = builder.GetFile();

      // Add bathymetry data
      file.WriteItemTimeStepNext(0, bathyDataArray);

      file.Close();
    }

    public static readonly string ResampleUsage = @"
    -Resample: Resample dfs2 file in x/y space

        DHI.MikeCore.Util -Resample [inputFilename] [outputFilename] [xCount] [yCount]

        Resample the [inputFilename] to contain [xCount] x [yCount] number of cells, and
        store the result in [outputFilename]
";

    /// <summary>
    /// Example of how to resample a dfs2 file in x/y space
    /// </summary>
    /// <param name="inputFilename">Path and name of the file to resample</param>
    /// <param name="outputFilename">Path and name of the new file to create</param>
    /// <param name="xCount">Number of cells in x-direction</param>
    /// <param name="yCount">Number of cells in y-direction</param>
    public static void Resample(string inputFilename, string outputFilename, int xCount, int yCount)
    {
      // Load dfs2 file
      Dfs2File dfs2File = DfsFileFactory.Dfs2FileOpen(inputFilename);
      IDfsAxisEqD2 axis = (IDfsAxisEqD2)dfs2File.SpatialAxis;
      
      // Create reprojector
      Dfs2Reprojector reproj = new Dfs2Reprojector(dfs2File, outputFilename);

      // scale change
      double dxScale = (double) axis.XCount / xCount;
      double dyScale = (double) axis.YCount / yCount;

      // Calculate new lon/lat origin - center of lower left cell
      Cartography cart = new Cartography(dfs2File.FileInfo.Projection.WKTString, dfs2File.FileInfo.Projection.Longitude, dfs2File.FileInfo.Projection.Latitude, dfs2File.FileInfo.Projection.Orientation);
      // Change in center of lower left cell
      double dxOrigin = 0.5 * axis.Dx * (dxScale-1);
      double dyOrigin = 0.5 * axis.Dy * (dyScale-1);
      cart.Xy2Geo(dxOrigin, dyOrigin, out double lonOrigin, out double latOrigin);

      // Set new target
      reproj.SetTarget(dfs2File.FileInfo.Projection.WKTString, lonOrigin, latOrigin, dfs2File.FileInfo.Projection.Orientation, xCount, 0, axis.Dx*dxScale, yCount, 0, axis.Dy*dyScale);
      reproj.Interpolate = true;
      // Create new file
      reproj.Process();
    }

    /// <summary>
    /// Example of how to get from a geographical coordinate to an (j,k) index
    /// in the 2D grid. It also shows how to get the closest cell value and how 
    /// to perform bilinear interpolation.
    /// <para>
    /// The method assumes that the OresundHD.dfs2 test file is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">Path and name of OresundHD.dfs2 test file</param>
    public static void GetjkIndexForGeoCoordinate(string filename)
    {
      Dfs2File file = DfsFileFactory.Dfs2FileOpen(filename);

      // The spatial axis is a EqD2 axis
      IDfsAxisEqD2 axis = (IDfsAxisEqD2)file.SpatialAxis;

      // Data for first time step
      IDfsItemData2D<float> data = (IDfsItemData2D<float>)file.ReadItemTimeStep(1, 0);

      // Get the projection and create a cartography object
      IDfsProjection projection = file.FileInfo.Projection;
      DHI.Projections.Cartography cart = new DHI.Projections.Cartography(projection.WKTString, projection.Longitude, projection.Latitude, projection.Orientation);

      // Coordinates just south of Amager
      double lon = 12.59;
      double lat = 55.54;

      // Get the (x,y) grid coordinates
      double x;
      double y;
      cart.Geo2Xy(lon, lat, out x, out y);

      Console.Out.WriteLine("Grid coordinates          (x,y) = ({0:0.000},{1:0.000})", x, y);
      Console.Out.WriteLine("Relative grid coordinates (x,y) = ({0:0.000},{1:0.000})", x / axis.Dx, y / axis.Dy);

      // Calculate the cell indices of the lon-lat coordinate. 
      // The cell extents from its center and +/- 1/2 dx and dy 
      // in each direction
      int j = (int)(x / axis.Dx + 0.5);  // 30
      int k = (int)(y / axis.Dy + 0.5);  // 27

      Console.Out.WriteLine("Value in cell ({0},{1})           = {2}", j, k, data[j, k]);

      // If you want to interpolate between the values, calculate
      // the (j,k) indices of lower left corner and do bilinear interpolation.
      // This procedure does not take delete values into account!!!
      j = (int)(x / axis.Dx);  // 30
      k = (int)(y / axis.Dy);  // 26

      double xFrac = (x % axis.Dx) / axis.Dx;  // fraction of j+1 value
      double yFrac = (y % axis.Dy) / axis.Dy;  // fraction of k+1 value

      double vk = (1 - xFrac) * data[j, k] + xFrac * data[j + 1, k];
      double vkp1 = (1 - xFrac) * data[j, k + 1] + xFrac * data[j + 1, k + 1];
      double v = (1 - yFrac) * vk + yFrac * vkp1;

      Console.Out.WriteLine("Interpolated value              = {0}", v);

      file.Close();

    }

  }
}
#pragma warning restore 168
// ReSharper restore RedundantAssignment
