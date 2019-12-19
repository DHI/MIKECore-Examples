using System;
using System.Collections.Generic;
using System.IO;
using DHI.Generic.MikeZero;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfs123;
using DHI.Generic.MikeZero.DFS.dfsu;
using DHI.Generic.MikeZero.DFS.mesh;

namespace DHI.MikeCore.Examples
{
  /// <summary>
  /// Class with example methods related to dfsu files.
  /// </summary>
  public class ExamplesDfsu
  {
    /// <summary> Static constructor </summary>
    static ExamplesDfsu()
    {
#if !XCOPY
      // The setup method will make your application find the MIKE assemblies at runtime.
      // The first call of the setup method takes precedense. Any subsequent calls will be ignored.
      // It must be called BEFORE any method using MIKE libraries is called, i.e. it is not sufficient
      // to call it as the first thing in that method using the MIKE libraries. Often this can be achieved
      // by having this code in the static constructor.
      // If MIKE Core is x-copy deployed with the application, this is not required.
      if (!DHI.Mike.Install.MikeImport.Setup(17, DHI.Mike.Install.MikeProducts.MikeCore))
        throw new Exception("Cannot find a proper MIKE installation");
#endif
    }

    /// <summary>
    /// Introductory example of how to load a dfsu file.
    /// <para>
    /// The method assumes that the OresundHD.dfsu test file
    /// is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of OresundHD.dfsu test file</param>
    public static void ReadingDfsuFile(string filename)
    {
      // This Setup call is required to find the MIKE libraries at runtime.
      // If MIKE Core is x-copy deployed with the application, this is not required.
      if (!DHI.Mike.Install.MikeImport.Setup(17, DHI.Mike.Install.MikeProducts.MikeCore))
        throw new Exception("Cannot find a proper MIKE installation");

      IDfsuFile file = DfsuFile.Open(filename);

      // Read geometry
      int numberOfElements = file.NumberOfElements;        // 3636
      int numberOfNodes = file.NumberOfNodes;              // 2057
      double firstNodeXCoordinate = file.X[0];             // 359978.8
      int[] firstElementNodes = file.ElementTable[0];      // [1, 2, 3]

      // Read dynamic item info
      string firstItemName = file.ItemInfo[0].Name;        // "Surface elevation"
      eumQuantity quantity = file.ItemInfo[0].Quantity;    // eumISurfaceElevation in eumUmeter

      // load data for the first item, 6th timestep
      float[] itemTimeStepData = (float[])file.ReadItemTimeStep(1, 5).Data;
      // Read the value of the third element
      float thirdElementValue = itemTimeStepData[2];       // 0.0014070312

      file.Close();
    }

    /// <summary>
    /// Find element (index) for a specified coordinate
    /// <para>
    /// The method assumes that the OresundHD.dfsu test file
    /// is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of OresundHD.dfsu test file</param>
    public static void FindElementForCoordinate(string filename)
    {
      IDfsuFile file = DfsuFile.Open(filename);
      double[] X = file.X;
      double[] Y = file.Y;

      // Coordinate to search for
      double xc = 346381;
      double yc = 6153637;

      // Loop over all elements - linear search, which may be slow!
      // If to find element for a large number of coordinates and if especially the
      // file has many elements, then a search tree procedure should be used to 
      // optimize the searching performance.
      int elmt = -1;  // result of search
      for (int i = 0; i < file.NumberOfElements; i++)
      {
        // Take out nodes for element
        int[] nodes = file.ElementTable[i];

        // Loop over all faces in element. The coordinate (x,y) is
        // inside an element if the coordinate is "left of" all faces,
        // when travelling faces counter-clockwise

        bool isInside = true;
        for (int j = 0; j < nodes.Length; j++)
        {
          // face start/end node indices
          int a = nodes[j] - 1;
          int b = nodes[(j + 1)%nodes.Length] - 1;

          // Assuming face is A->B and coordinate is C, then "left of" test:
          // (B-A) X (C-A) > 0  
          // where X is the cross product
          double cross = (X[b] - X[a])*(yc - Y[a]) - (Y[b] - Y[a])*(xc - X[a]);
          if (cross < 0)
          {
            // (xc, yc) is "right of", hence not inside, skip to next element
            isInside = false;
            break;
          }
        }
        if (isInside)
        {
          // All "left of" tests succeded, element found!
          elmt = i;
          break;
        }
      }

      if (elmt >= 0)
      {
        Console.Out.WriteLine("Found     element index: = {0}", elmt);
        Console.Out.WriteLine("(xc,yc) = ({0},{1})", xc, yc);
        int[] resNodes = file.ElementTable[elmt];
        for (int j = 0; j < resNodes.Length; j++)
        {
          int node = resNodes[j] - 1;
          Console.Out.WriteLine("(x,y)   = ({0},{1})", X[node], Y[node]);
        }
      }

      file.Close();

    }

    /// <summary>
    /// Example of how to modify the geometry of a dfsu file.
    /// The method will rotate the geometry by 125 degrees.
    /// <para>
    /// The method will work on any dfsu file. The OresundHD.dfsu test file
    /// (preferably a copy of it) can be used as input file.
    /// </para>
    /// </summary>
    /// <param name="filename">Path and name of a dfsu file</param>
    public static void ModifyDfsuFileGeometry(string filename)
    {
      // Open file for editing
      IDfsuFile dfsuFile = DfsuFile.OpenEdit(filename);
      dfsuFile.TimeStepInSeconds /= 2;
      dfsuFile.StartDateTime = new DateTime(2019,6,27,13,50,30);

      // Make a rotation matrix
      double rotation = 125.0 / 180.0 * System.Math.PI;
      double x1 = System.Math.Cos(rotation);
      double y1 = -System.Math.Sin(rotation);
      double x2 = System.Math.Sin(rotation);
      double y2 = System.Math.Cos(rotation);

      // Get the x- and y-coordinates from the file
      double[] x = dfsuFile.X;
      double[] y = dfsuFile.Y;

      // Point to rotate around
      double x0 = x[0];
      double y0 = y[0];

      // Rotate coordinates
      for (int i = 0; i < dfsuFile.NumberOfNodes; i++)
      {
        double xx = x[i] - x0;
        double yy = y[i] - y0;
        x[i] = x1 * xx + y1 * yy + x0;
        y[i] = x2 * xx + y2 * yy + y0;
      }

      // Set the x- and y-coordinates back to the file
      dfsuFile.X = x;
      dfsuFile.Y = y;

      // Close the file
      dfsuFile.Close();
    }

    /// <summary>
    /// Example on how to extract dfs0 data from a 2D dfsu file for certain elements. All items
    /// from dfsu file are extracted.
    /// </summary>
    /// <param name="dfsuFileNamePath">Name, including path, of 2D dfsu file</param>
    /// <param name="elmtsIndices">Indices of elements to extract data from</param>
    /// <param name="useStream">Use stream when writing dfs0 files - then more than 400 files can be created simultaneously</param>
    public static void ExtractDfs0FromDfsu(string dfsuFileNamePath, IList<int> elmtsIndices, bool useStream)
    {
      // If not using stream approach, at most 400 elements at a time can be processed. 
      // There is a limit on how many files you can have open at the same time using 
      // the standard approach. It will fail in a nasty way, if the maximum number of 
      // file handles are exceeded. This is not an issue when using .NET streams.
      if (!useStream && elmtsIndices.Count > 400)
        throw new ArgumentException("At most 400 elements at a time");

      // Open source dfsu file
      IDfsuFile source;
      Stream stream = null;
      if (useStream)
      {
        stream = new FileStream(dfsuFileNamePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        source = DfsuFile.Open(stream);
      }
      else
        source = DfsuFile.Open(dfsuFileNamePath);

      // Figure out "basic" dfs0 file name
      string dfsuFilename = Path.GetFileNameWithoutExtension(dfsuFileNamePath);
      string path = Path.GetDirectoryName(dfsuFileNamePath);
      string dfs0BaseFilename = Path.Combine(path, "test_" + dfsuFilename+"-");

      // Factory for creating dfs objects
      DfsFactory factory = new DfsFactory();

      // Create a dfs0 file for each element in elmtsIndices
      DfsFile[] dfs0Files   = new DfsFile[elmtsIndices.Count];
      Stream[]  dfs0Streams = new Stream [elmtsIndices.Count];
      double timeSpan = source.TimeStepInSeconds * source.NumberOfTimeSteps;
      for (int k = 0; k < elmtsIndices.Count; k++)
      {
        // Index of element to create dfs0 for
        int elmtsIndex = elmtsIndices[k];

        // Calculate element center coordinates, to be stored in dfs0 items.
        // Stored as float in dfs0, hence possible loss of precision...
        float x=0, y=0, z=0; 
        int[] nodeNumbers = source.ElementTable[elmtsIndex];
        for (int i = 0; i < nodeNumbers.Length; i++)
        {
          int nodeIndex = nodeNumbers[i]-1; // from number to index
          x += (float)source.X[nodeIndex];
          y += (float)source.Y[nodeIndex];
          z += source.Z[nodeIndex];
        }
        x /= nodeNumbers.Length;
        y /= nodeNumbers.Length;
        z /= nodeNumbers.Length;

        // Start building dfs0 file header
        DfsBuilder builder = DfsBuilder.Create("fileTitle", "appTitle", 1);
        builder.SetDataType(1); // standard dfs0 value
        builder.SetGeographicalProjection(source.Projection);
        builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec, source.StartDateTime, 0, source.TimeStepInSeconds));

        // Add all dynamic items from dfsu file to dfs0 file
        for (int j = 0; j < source.ItemInfo.Count; j++)
        {
          IDfsSimpleDynamicItemInfo sourceItem = source.ItemInfo[j];
          DfsDynamicItemBuilder itemBuilder = builder.CreateDynamicItemBuilder();
          itemBuilder.Set(sourceItem.Name, sourceItem.Quantity, sourceItem.DataType);
          itemBuilder.SetAxis(factory.CreateAxisEqD0());
          itemBuilder.SetValueType(sourceItem.ValueType);
          itemBuilder.SetReferenceCoordinates(x, y, z); // optional
          builder.AddDynamicItem(itemBuilder.GetDynamicItemInfo());
        }


        // Create and get file, store them in dfs0s array
        string dfs0Filename = dfs0BaseFilename + (elmtsIndex).ToString("000000") + ".dfs0";
        if (useStream)
        {
          // Create file using C# streams - necessary to provie number of time steps and timespan of data
          builder.SetNumberOfTimeSteps(source.NumberOfTimeSteps);
          builder.SetTimeInfo(0, timeSpan);
          Stream dfs0FileStream = new FileStream(dfs0Filename, FileMode.Create, FileAccess.Write, FileShare.ReadWrite);
          builder.CreateStream(dfs0FileStream);
          dfs0Streams[k] = dfs0FileStream;
        }
        else
        {
          // Create file in the ordinary way. Will include statistics (of delete values etc).
          builder.CreateFile(dfs0Filename);
        }
        dfs0Files[k] = builder.GetFile();
      }

      // For performance, use predefined itemdata objects when reading data from dfsu
      IDfsItemData<float>[] dfsuItemDatas = new IDfsItemData<float>[source.ItemInfo.Count];
      for (int j = 0; j < source.ItemInfo.Count; j++)
      {
        dfsuItemDatas[j] = (IDfsItemData<float>)source.ItemInfo[j].CreateEmptyItemData();
      }

      // Read data from dfsu and store in dfs0
      float[] dfs0Data = new float[1];
      for (int i = 0; i < source.NumberOfTimeSteps; i++)
      {
        for (int j = 0; j < source.ItemInfo.Count; j++)
        {
          // Read data from dfsu
          IDfsItemData<float> dfsuItemData = dfsuItemDatas[j];
          bool ok = source.ReadItemTimeStep(dfsuItemData, i);
          float[] floats = dfsuItemData.Data;

          // write data to dfs0's
          for (int k = 0; k < elmtsIndices.Count; k++)
          {
            int elmtsIndex = elmtsIndices[k];
            dfs0Data[0] = floats[elmtsIndex];
            dfs0Files[k].WriteItemTimeStepNext(0, dfs0Data);
          }
        }
      }

      // Close dfsu files
      source.Close();
      if (stream != null)
        stream.Close();
      // Close all dfs0 files
      for (int k = 0; k < elmtsIndices.Count; k++)
      {
        dfs0Files[k].Close();
        if (dfs0Streams[k] != null) 
          dfs0Streams[k].Close();
      }
    }


    /// <summary>
    /// Example of how to create a Dfsu file from scratch. This method
    /// creates a copy of the OresundHD.dfsu test file.
    /// <para>
    /// Data for static and dynamic item is taken from a source dfs file,
    /// which here is the OresundHD.dfsu test file. The data could come
    /// from any other source. 
    /// </para>
    /// </summary>
    /// <param name="sourceFilename">Path and name of the OresundHD.dfsu test file</param>
    /// <param name="filename">Path and name of the new file to create</param>
    /// <param name="zInMeters">Flag specifying whether the z values are in meters or feet </param>
    public static void CreateDfsuFile(string sourceFilename, string filename, bool zInMeters)
    {
      IDfsuFile source = DfsuFile.Open(sourceFilename);

      DfsuBuilder builder = DfsuBuilder.Create(DfsuFileType.Dfsu2D);

      // Setup header and geometry, copy from source file
      builder.SetNodes(source.X, source.Y, source.Z, source.Code);
      builder.SetElements(source.ElementTable);
      builder.SetProjection(source.Projection);
      builder.SetTimeInfo(source.StartDateTime, source.TimeStepInSeconds);
      if (zInMeters)
        builder.SetZUnit(eumUnit.eumUmeter);
      else
        builder.SetZUnit(eumUnit.eumUfeet);

      // Add dynamic items, copying from source
      foreach (DfsuDynamicItemInfo itemInfo in source.ItemInfo)
      {
        builder.AddDynamicItem(itemInfo.Name, itemInfo.Quantity);
      }

      DfsuFile file = builder.CreateFile(filename);

      // Add data for all item-timesteps, copying from source
      IDfsItemData sourceData;
      while (null != (sourceData = source.ReadItemTimeStepNext()))
      {
        file.WriteItemTimeStepNext(sourceData.Time, sourceData.Data);
      }

      source.Close();
      file.Close();
    }

    public static readonly string UsageExtractDfsu2DLayerFrom3D = @"
    -dfsuExtractLayer: Extract layer from 3D dfsu

        DHI.MikeCore.Util -dfsuExtractLayer [filenameDfsu3] [outputFilenameDfsu2] [layerNumber]

        Extract a single layer from a 3D dfsu file, and write it to a 2D dfsu file.

        [LayerNumber] is the layer to extract
          - Positive values count from bottom up i.e. 1 is bottom layer, 2 is 
            second layer from bottom etc.
          - Negative values count from top down, i.e. -1 is toplayer, -2 is 
            second layer from top etc.

        If a layer value does not exist for a certain 2D element, delete value is written
        to the 2D resut file. This is relevant for Sigma-Z type of files.
";

    /// <summary>
    /// Extract a single layer from a 3D dfsu file, and write it to a 2D dfsu file.
    /// <para>
    /// If a layer value does not exist for a certain 2D element, delete value is written
    /// to the 2D resut file. This is relevant for Sigma-Z type of files.
    /// </para>
    /// </summary>
    /// <param name="filenameDfsu3">Name of 3D dfsu source file</param>
    /// <param name="filenameDfsu2">Name of 2D dfsu result file</param>
    /// <param name="layerNumber">Layer to extract. 
    ///   <para>
    ///     Positive values count from bottom up i.e. 1 is bottom layer, 2 is second layer from bottom etc.  
    ///   </para>
    ///   <para>
    ///     Negative values count from top down, i.e. -1 is toplayer, -2 is second layer from top etc.
    ///   </para>
    /// </param>
    public static void ExtractDfsu2DLayerFrom3D(string filenameDfsu3, string filenameDfsu2, int layerNumber)
    {
      // This Setup call is required to find the MIKE libraries at runtime.
      // If MIKE Core is x-copy deployed with the application, this is not required.
      if (!DHI.Mike.Install.MikeImport.Setup(17, DHI.Mike.Install.MikeProducts.MikeCore))
        throw new Exception("Cannot find a proper MIKE installation");

      IDfsuFile dfsu3File = DfsFileFactory.DfsuFileOpen(filenameDfsu3);

      // Check that dfsu3 file is a 3D dfsu file.
      switch (dfsu3File.DfsuFileType)
      {
        case DfsuFileType.Dfsu2D:
        case DfsuFileType.DfsuVerticalColumn:
        case DfsuFileType.DfsuVerticalProfileSigma:
        case DfsuFileType.DfsuVerticalProfileSigmaZ:
          throw new InvalidOperationException("Input file is not a 3D dfsu file");
      }

      // Calculate offset from toplayer element. Offset is between 0 (top layer) and
      // dfsu3File.NumberOfLayers-1 (bottom layer)
      int topLayerOffset;
      if (layerNumber > 0 && layerNumber <= dfsu3File.NumberOfLayers)
      {
        topLayerOffset = dfsu3File.NumberOfLayers - layerNumber;
      }
      else if (layerNumber < 0 && -layerNumber <= dfsu3File.NumberOfLayers)
      {
        topLayerOffset = -layerNumber - 1;
      }
      else
      {
        throw new ArgumentException("Layer number is out of range");
      }

      double[] xv = dfsu3File.X;
      double[] yv = dfsu3File.Y;
      float[] zv = dfsu3File.Z;
      int[] cv = dfsu3File.Code;

      // --------------------------------------------------
      // Create 2D mesh from 3D mesh

      // List of new 2D nodes
      int node2DCount = 0;
      List<double> xv2 = new List<double>();
      List<double> yv2 = new List<double>();
      List<float> zv2 = new List<float>();
      List<int> cv2 = new List<int>();

      // Renumbering array, from 3D node numbers to 2D node numbers
      // i.e. if a 3D element refers to node number k, the 2D element node number is renumber[k]
      int[] renumber = new int[dfsu3File.NumberOfNodes];

      // Coordinates of last created node
      double xr2 = -1e-10;
      double yr2 = -1e-10;

      // Create 2D nodes, by skipping nodes with equal x,y coordinates
      for (int i = 0; i < dfsu3File.NumberOfNodes; i++)
      {
        // If 3D x,y coordinates are equal to the last created 2D node,
        // map this node to the last created 2D node, otherwise
        // create new 2D node and map to that one
        if (xv[i] != xr2 || yv[i] != yr2)
        {
          // Create new node
          node2DCount++;
          xr2 = xv[i];
          yr2 = yv[i];
          float zr2 = zv[i];
          int cr2 = cv[i];
          xv2.Add(xr2);
          yv2.Add(yr2);
          zv2.Add(zr2);
          cv2.Add(cr2);
        }
        // Map this 3D node to the last created 2D node.
        renumber[i] = node2DCount;
      }

      // Find indices of top layer elements
      IList<int> topLayer = dfsu3File.FindTopLayerElements();

      // Create element table for 2D dfsu file
      int[][] elmttable2 = new int[topLayer.Count][];
      for (int i = 0; i < topLayer.Count; i++)
      {
        // 3D element nodes
        int[] elmt3 = dfsu3File.ElementTable[topLayer[i]];
        // 2D element nodes, only half as big, so copy over the first half
        int[] elmt2 = new int[elmt3.Length / 2];
        for (int j = 0; j < elmt2.Length; j++)
        {
          elmt2[j] = renumber[elmt3[j]];
        }
        elmttable2[i] = elmt2;
      }

      // --------------------------------------------------
      // Create 2D dfsu file
      DfsuBuilder builder = DfsuBuilder.Create(DfsuFileType.Dfsu2D);

      // Setup header and geometry
      builder.SetNodes(xv2.ToArray(), yv2.ToArray(), zv2.ToArray(), cv2.ToArray());
      builder.SetElements(elmttable2);
      builder.SetProjection(dfsu3File.Projection);
      builder.SetTimeInfo(dfsu3File.StartDateTime, dfsu3File.TimeStepInSeconds);
      if (dfsu3File.ZUnit == eumUnit.eumUUnitUndefined)
        builder.SetZUnit(eumUnit.eumUmeter);
      else
        builder.SetZUnit(dfsu3File.ZUnit);

      // Add dynamic items, copying from source, though not the first one, if it
      // contains the z-variation on the nodes
      for (int i = 0; i < dfsu3File.ItemInfo.Count; i++)
      {
        IDfsSimpleDynamicItemInfo itemInfo = dfsu3File.ItemInfo[i];
        if (itemInfo.ElementCount == dfsu3File.NumberOfElements)
          builder.AddDynamicItem(itemInfo.Name, itemInfo.Quantity);
      }

      // Create file
      DfsuFile dfsu2File = builder.CreateFile(filenameDfsu2);

      // --------------------------------------------------
      // Process data

      // Check if the layer number exists for 2D element, i.e. if that element 
      // in 2D has that number of columnes in the 3D (relevant for sigma-z files)
      // If elementExists[i] is false, write delete value to file
      bool[] elementExists = new bool[topLayer.Count];
      int numLayersInColumn = topLayer[0] + 1;
      elementExists[0] = (numLayersInColumn - topLayerOffset) > 0;
      for (int i = 1; i < topLayer.Count; i++)
      {
        numLayersInColumn = (topLayer[i] - topLayer[i - 1]);
        elementExists[i] = (numLayersInColumn - topLayerOffset) > 0;
      }

      // For performance, use predefined itemdata objects when reading data from dfsu 3D file
      IDfsItemData<float>[] dfsu3ItemDatas = new IDfsItemData<float>[dfsu3File.ItemInfo.Count];
      for (int j = 0; j < dfsu3File.ItemInfo.Count; j++)
      {
        dfsu3ItemDatas[j] = (IDfsItemData<float>)dfsu3File.ItemInfo[j].CreateEmptyItemData();
      }

      // Float data to write to dfsu 2D file
      float[] data2 = new float[dfsu2File.NumberOfElements];
      float deleteValueFloat = dfsu2File.DeleteValueFloat;

      for (int i = 0; i < dfsu3File.NumberOfTimeSteps; i++)
      {
        for (int j = 0; j < dfsu3File.ItemInfo.Count; j++)
        {
          // Read data from 3D dfsu
          IDfsItemData<float> data3Item = dfsu3ItemDatas[j];
          bool ok = dfsu3File.ReadItemTimeStep(data3Item, i);
          // 3D data
          float[] data3 = data3Item.Data;

          // Skip any items not having size = NumberOfElments (the z-variation on the nodes)
          if (data3.Length != dfsu3File.NumberOfElements)
            continue;

          // Loop over all 2D elements
          for (int k = 0; k < topLayer.Count; k++)
          {
            // Extract layer data from 3D column into 2D element value
            if (elementExists[k])
              data2[k] = data3[topLayer[k] - topLayerOffset];
            else
              data2[k] = deleteValueFloat;
          }

          dfsu2File.WriteItemTimeStepNext(data3Item.Time, data2);
        }
      }

      dfsu3File.Close();
      dfsu2File.Close();
    }


    /// <summary>
    /// Create dfsu and mesh file from dfs2 file.
    /// <para>
    /// Note 1: Boundary code is set to land value at
    ///         all boundaries of mesh and dfsu file.
    ///         These must be updated to something "better" 
    ///         if to use as input in another simulation.
    /// </para>
    /// <para>
    /// Note 2: P and Q values are not rotated with the
    ///         grid, but should be so, if used in the
    ///         projected coordinate system. It must take 
    ///         the 327 degrees rotation into account.
    /// </para>
    /// </summary>
    /// <param name="dfs2Filename">Name of input dfs2 file, e.g. the OresundHD.dfs2</param>
    /// <param name="meshFilename">Name of output mesh file</param>
    /// <param name="dfsuFilename">Name of output dfsu file</param>
    public static void CreateDfsuFromDfs2(string dfs2Filename, string meshFilename, string dfsuFilename)
    {
 
      // Open file
      Dfs2File dfs2 = DfsFileFactory.Dfs2FileOpen(dfs2Filename);

      // Read bathymetry from first static item
      IDfsStaticItem bathymetryItem = dfs2.ReadStaticItemNext();
      float[] bathymetry = (float[])bathymetryItem.Data;

      // Extract spatial axis
      IDfsAxisEqD2 spatialAxis = (IDfsAxisEqD2)dfs2.SpatialAxis;
      // Some convenience variables
      double dx = spatialAxis.Dx;
      double dy = spatialAxis.Dy;
      double x0 = spatialAxis.X0;
      double y0 = spatialAxis.Y0;
      int xCount = spatialAxis.XCount;
      int yCount = spatialAxis.YCount;

      // First custom block (index 0) contains the M21_MISC values, 
      // where the 4th (index 3) is the land value
      float landValue = (float)dfs2.FileInfo.CustomBlocks[0][3];

      //-----------------------------------------
      // Find out which elements in the dfs2 grid that is not a land value 
      // and include all those elements and their surrounding nodes in mesh

      // Arrays indicating if element and node in grid is used or not in mesh
      bool[,] elmts = new bool[xCount, yCount];
      int[,] nodes = new int[xCount + 1, yCount + 1];

      // Loop over all elements in 2D grid
      for (int l = 0; l < yCount; l++)
      {
        for (int k = 0; k < xCount; k++)
        {
          // If bathymetry is not land value, use element.
          if (bathymetry[k + l * xCount] != landValue)
          {
            // element [l,k] is used, and also the 4 nodes around it
            elmts[k  , l  ] = true;
            nodes[k  , l  ] = 1;
            nodes[k+1, l  ] = 1;
            nodes[k  , l+1] = 1;
            nodes[k+1, l+1] = 1;
          }
        }
      }

      //-----------------------------------------
      // Create new mest nodes

      // Cartography object can convert grid (x,y) to projection (east,north)
      IDfsProjection proj = dfs2.FileInfo.Projection;
      DHI.Projections.Cartography cart = new DHI.Projections.Cartography(proj.WKTString, proj.Longitude, proj.Latitude, proj.Orientation);

      // New mesh nodes
      List<double> X = new List<double>();
      List<double> Y = new List<double>();
      List<float> Zf = new List<float>();   // float values for dfsu file
      List<double> Zd = new List<double>(); // double values for mesh file
      List<int> Code = new List<int>();

      // Loop over all nodes
      int nodesCount = 0;
      for (int l = 0; l < yCount + 1; l++)
      {
        for (int k = 0; k < xCount + 1; k++)
        {
          // Check if node is included in mesh
          if (nodes[k, l] > 0)
          {
            // Convert from mesh (x,y) to projection (east,north)
            double east, north;
            cart.Xy2Proj((k - 0.5) * dx + x0, (l - 0.5) * dy + y0, out east, out north);

            // Average Z on node from neighbouring grid cell values, cell value is used 
            // unless they are outside grid or has land values
            double z = 0;
            int zCount = 0;
            if (k > 0 && l > 0           && bathymetry[k-1 + (l-1) * xCount] != landValue)
            { zCount++;                z += bathymetry[k-1 + (l-1) * xCount]; }
            if (k < xCount && l > 0      && bathymetry[k   + (l-1) * xCount] != landValue)
            { zCount++;                z += bathymetry[k   + (l-1) * xCount]; }
            if (k > 0 && l < yCount      && bathymetry[k-1 + (l  ) * xCount] != landValue)
            { zCount++;                z += bathymetry[k-1 + (l  ) * xCount]; }
            if (k < xCount && l < yCount && bathymetry[k   + (l  ) * xCount] != landValue)
            { zCount++;                z += bathymetry[k   + (l  ) * xCount]; }

            if (zCount > 0)
              z /= zCount;
            else
              z = landValue;

            // Store new node number and add node
            nodesCount++;
            nodes[k, l] = nodesCount; // this is the node number to use in the element table
            X.Add(east);
            Y.Add(north);
            Zf.Add((float)z);
            Zd.Add(z);
            Code.Add(zCount == 4 ? 0 : 1); // Land boundary if zCount < 4
          }
        }
      }

      // New mesh elements
      List<int[]> elmttable2 = new List<int[]>();

      for (int l = 0; l < yCount; l++)
      {
        for (int k = 0; k < xCount; k++)
        {
          // Check if element is included in mesh
          if (elmts[k, l])
          {
            // For this element, add the four surrounding nodes, 
            // counter-clockwise order
            int[] newNodes = new int[4];
            newNodes[0] = nodes[k  , l  ];
            newNodes[1] = nodes[k+1, l  ];
            newNodes[2] = nodes[k+1, l+1];
            newNodes[3] = nodes[k  , l+1];
            elmttable2.Add(newNodes);
          }
        }
      }

      //-----------------------------------------
      // Create mesh
      {
        // Create 2D dfsu file
        MeshBuilder builder = new MeshBuilder();

        // Setup header and geometry
        builder.SetNodes(X.ToArray(), Y.ToArray(), Zd.ToArray(), Code.ToArray());
        builder.SetElements(elmttable2.ToArray());
        builder.SetProjection(dfs2.FileInfo.Projection);

        // Create new file
        MeshFile mesh = builder.CreateMesh();
        mesh.Write(meshFilename);

      }

      //-----------------------------------------
      // Create dfsu file
      {
        // dfs2 time axis
        IDfsEqCalendarAxis timeAxis = (IDfsEqCalendarAxis)dfs2.FileInfo.TimeAxis;

        // Create 2D dfsu file
        DfsuBuilder builder = DfsuBuilder.Create(DfsuFileType.Dfsu2D);

        // Setup header and geometry
        builder.SetNodes(X.ToArray(), Y.ToArray(), Zf.ToArray(), Code.ToArray());
        builder.SetElements(elmttable2.ToArray());
        builder.SetProjection(dfs2.FileInfo.Projection);
        builder.SetTimeInfo(timeAxis.StartDateTime, timeAxis.TimeStepInSeconds());
        builder.SetZUnit(eumUnit.eumUmeter);

        // Add dynamic items, copying from dfs2 file
        for (int i = 0; i < dfs2.ItemInfo.Count; i++)
        {
          IDfsSimpleDynamicItemInfo itemInfo = dfs2.ItemInfo[i];
          builder.AddDynamicItem(itemInfo.Name, itemInfo.Quantity);
        }

        // Create new file
        DfsuFile dfsu = builder.CreateFile(dfsuFilename);

        // Add dfs2 data to dfsu file
        float[] dfsuData = new float[dfsu.NumberOfElements];
        for (int i = 0; i < dfs2.FileInfo.TimeAxis.NumberOfTimeSteps; i++)
        {
          for (int j = 0; j < dfs2.ItemInfo.Count; j++)
          {
            // Read dfs2 grid data
            IDfsItemData2D<float> itemData = (IDfsItemData2D<float>)dfs2.ReadItemTimeStep(j + 1, i);
            // Extract 2D grid data to dfsu data array
            int lk = 0;
            for (int l = 0; l < yCount; l++)
            {
              for (int k = 0; k < xCount; k++)
              {
                if (elmts[k, l])
                {
                  dfsuData[lk++] = itemData[k, l];
                }
              }
            }
            // write data
            dfsu.WriteItemTimeStepNext(itemData.Time, dfsuData);
          }
        }
        dfsu.Close();
      }

      dfs2.Close();
    }

    public static readonly string UsageExtractSubareaDfsu2D = @"
    -dfsuExtractSubArea: Extract subarea of dfsu file

        DHI.MikeCore.Util -dfsuExtractSubArea [sourceFilename] [outputFilename] [x1] [y2] [x2] [y2]

        Extract sub-area of dfsu (2D) file to a new dfsu file

        (x1,y1) is coordiante for lower left corner of sub area
        (x2,y2) is coordiante for upper right corner of sub area
";


    /// <summary>
    /// Extract sub-area of dfsu (2D) file to a new dfsu file
    /// </summary>
    /// <param name="sourceFilename">Name of source file, i.e. OresundHD.dfsu test file</param>
    /// <param name="outputFilename">Name of output file</param>
    /// <param name="x1">Lower left x coordinate of sub area</param>
    /// <param name="y1">Lower left y coordinate of sub area</param>
    /// <param name="x2">upper right x coordinate of sub area</param>
    /// <param name="y2">upper right y coordinate of sub area</param>
    public static void ExtractSubareaDfsu2D(string sourceFilename, string outputFilename, double x1, double y1, double x2, double y2)
    {

      DfsuFile dfsu = DfsFileFactory.DfsuFileOpen(sourceFilename);

      // Node coordinates
      double[] X = dfsu.X;
      double[] Y = dfsu.Y;
      float[] Z = dfsu.Z;
      int[] Code = dfsu.Code;

      // Loop over all elements, and all its nodes: If one node is inside
      // region, element (and nodes) are to be included in new mesh
      List<int> elmtsIncluded = new List<int>();
      bool[] nodesIncluded = new bool[dfsu.NumberOfNodes];
      for (int i = 0; i < dfsu.NumberOfElements; i++)
      {
        // Nodes of element
        int[] nodes = dfsu.ElementTable[i];

        // Check if one of the nodes of the element is inside region
        bool elmtIncluded = false;
        for (int j = 0; j < nodes.Length; j++)
        {
          int node = nodes[j] - 1;
          if (x1 <= X[node] && X[node] <= x2 && y1 <= Y[node] && Y[node] <= y2)
            elmtIncluded = true;
        }

        if (elmtIncluded)
        {
          // Add element to list of included elements
          elmtsIncluded.Add(i);
          // Mark all nodes of element as included
          for (int j = 0; j < nodes.Length; j++)
          {
            int node = nodes[j] - 1;
            nodesIncluded[node] = true;
          }
        }
      }

      // array containing numbers of existing nodes in new mesh (indices)
      int[] renumber = new int[dfsu.NumberOfNodes];

      // new mesh nodes
      List<double> X2 = new List<double>();
      List<double> Y2 = new List<double>();
      List<float> Z2 = new List<float>();
      List<int> Code2 = new List<int>();
      List<int> nodeIds = new List<int>();

      int i2 = 0;
      for (int i = 0; i < dfsu.NumberOfNodes; i++)
      {
        if (nodesIncluded[i])
        {
          X2.Add(X[i]);
          Y2.Add(Y[i]);
          Z2.Add(Z[i]);
          Code2.Add(Code[i]);
          nodeIds.Add(dfsu.NodeIds[i]);
          // Node with index i will get index i2 in new mesh
          renumber[i] = i2;
          i2++;
        }
      }

      // New mesh elements
      List<int[]> elmttable2 = new List<int[]>();
      List<int> elmtIds = new List<int>();
      for (int i = 0; i < elmtsIncluded.Count; i++)
      {
        // Add new element
        int elmt = elmtsIncluded[i];
        int[] nodes = dfsu.ElementTable[elmt];
        // newNodes must be renumbered
        int[] newNodes = new int[nodes.Length];
        for (int j = 0; j < nodes.Length; j++)
        {
          // Do the renumbering of nodes from existing mesh to new mesh
          newNodes[j] = renumber[nodes[j] - 1] + 1;
        }
        elmttable2.Add(newNodes);
        elmtIds.Add(dfsu.ElementIds[i]);
      }

      // Create 2D dfsu file
      DfsuBuilder builder = DfsuBuilder.Create(DfsuFileType.Dfsu2D);

      // Setup header and geometry
      builder.SetNodes(X2.ToArray(), Y2.ToArray(), Z2.ToArray(), Code2.ToArray());
      //builder.SetNodeIds(nodeIds.ToArray());
      builder.SetElements(elmttable2.ToArray());
      builder.SetElementIds(elmtIds.ToArray()); // retain original element id's
      builder.SetProjection(dfsu.Projection);
      builder.SetTimeInfo(dfsu.StartDateTime, dfsu.TimeStepInSeconds);
      if (dfsu.ZUnit == eumUnit.eumUUnitUndefined)
        builder.SetZUnit(eumUnit.eumUmeter);
      else
        builder.SetZUnit(dfsu.ZUnit);

      // Add dynamic items, copying from source
      for (int i = 0; i < dfsu.ItemInfo.Count; i++)
      {
        IDfsSimpleDynamicItemInfo itemInfo = dfsu.ItemInfo[i];
        builder.AddDynamicItem(itemInfo.Name, itemInfo.Quantity);
      }

      // Create new file
      DfsuFile dfsuOut = builder.CreateFile(outputFilename);

      // Add new data
      float[] data2 = new float[elmtsIncluded.Count];
      for (int i = 0; i < dfsu.NumberOfTimeSteps; i++)
      {
        for (int j = 0; j < dfsu.ItemInfo.Count; j++)
        {
          // Read data from existing dfsu
          IDfsItemData<float> itemData = (IDfsItemData<float>)dfsu.ReadItemTimeStep(j + 1, i);
          // Extract value for elements in new mesh
          for (int k = 0; k < elmtsIncluded.Count; k++)
          {
            data2[k] = itemData.Data[elmtsIncluded[k]];
          }
          // write data
          dfsuOut.WriteItemTimeStepNext(itemData.Time, data2);
        }
      }
      dfsuOut.Close();
      dfsu.Close();

    }

  }
}
