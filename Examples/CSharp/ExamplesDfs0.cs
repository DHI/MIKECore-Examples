using System;
using System.IO;
using DHI.Generic.MikeZero;
using DHI.Generic.MikeZero.DFS;

namespace DHI.MikeCore.Examples
{
  /// <summary>
  /// Class with example methods related to dfs0 files.
  /// </summary>
  public class ExamplesDfs0
  {
    /// <summary> Static constructor </summary>
    static ExamplesDfs0()
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
    /// Introductory example of how to load a dfs0 file.
    /// <para>
    /// The method assumes that the Rain_stepaccumulated.dfs0 test file
    /// is the input file.
    /// </para>
    /// </summary>
    /// <param name="filename">path and name of Rain_stepaccumulated.dfs0 test file</param>
    public static double ReadDfs0File(string filename)
    {
      // Open the file as a generic dfs file
      IDfsFile dfs0File = DfsFileFactory.DfsGenericOpen(filename);

      // Header information is contained in the IDfsFileInfo
      IDfsFileInfo fileInfo = dfs0File.FileInfo;
      int steps = fileInfo.TimeAxis.NumberOfTimeSteps;                   // 19

      // Information on each of the dynamic items, here the first one
      IDfsSimpleDynamicItemInfo dynamicItemInfo = dfs0File.ItemInfo[0];
      string nameOfFirstDynamicItem = dynamicItemInfo.Name;              // "Rain"
      DfsSimpleType typeOfFirstDynamicItem = dynamicItemInfo.DataType;   // Double
      ValueType valueType = dynamicItemInfo.ValueType;                   // StepAccumulated

      // Read data of first item, third time step (items start by 1, timesteps by 0),
      IDfsItemData datag = dfs0File.ReadItemTimeStep(1, 2);
      double value1 = System.Convert.ToDouble(datag.Data.GetValue(0));     // 0.36
      // Assuming this is a double value, the item data object can be converted to the correct type
      IDfsItemData<double> data = (IDfsItemData<double>)datag;
      double value2 = data.Data[0];                                        // 0.36

      // This iterates through all timesteps and items in the file
      // For performance reasons it is important to iterate over time steps
      // first and items second.
      double sum = 0;
      for (int i = 0; i < steps; i++)
      {
        for (int j = 1; j <= dfs0File.ItemInfo.Count; j++)
        {
          data = (IDfsItemData<double>)dfs0File.ReadItemTimeStep(j, i);
          double value = data.Data[0];
          sum += value;
        }
      }

      return sum;
    }

    /// <summary>
    /// Find maximum value and time of maximum for a specified item in dfs0 file
    /// </summary>
    /// <param name="filename">Path and name of file, e.g. data_ndr_roese.dfs0 test file</param>
    /// <param name="itemNumber">Item number to find maximum for</param>
    public static double FindMaxValue(string filename, int itemNumber)
    {
      // Open file, using stream class
      Stream stream = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
      IDfsFile file = DfsFileFactory.DfsGenericOpen(stream);
      //IDfsFile file = DfsFileFactory.DfsGenericOpen(filename);

      // Extract Start date-time of file - assuming file is equidistant-calendar axis
      IDfsEqCalendarAxis timeAxis = (IDfsEqCalendarAxis)file.FileInfo.TimeAxis;
      DateTime startDateTime = timeAxis.StartDateTime;

      // Empty item data, reused when calling ReadItemTimeStep
      IDfsItemData<float> itemData = (IDfsItemData<float>)file.CreateEmptyItemData(itemNumber);

      // max value and time variables
      double maxValue = double.MinValue;
      double maxTimeSeconds = -1;
      DateTime maxDateTime = DateTime.MinValue;
      // Loop over all times in file
      for (int i = 0; i < file.FileInfo.TimeAxis.NumberOfTimeSteps; i++)
      {
        // Read time step for item, and extract value
        file.ReadItemTimeStep(itemData, i);
        double value = itemData.Data[0];
        // Check if value is larger than maxValue
        if (value > maxValue)
        {
          maxValue = value;
          maxTimeSeconds = itemData.TimeInSeconds(timeAxis);
          maxDateTime = itemData.TimeAsDateTime(timeAxis);
        }
      }
      // Report results
      Console.Out.WriteLine("Max Value      : {0} {1}", maxValue, file.ItemInfo[itemNumber-1].Quantity.UnitAbbreviation);
      Console.Out.WriteLine("Max Value time : {0}", maxDateTime.ToString("yyyy-MM-dd HH:mm:ss"));
      return maxValue;
    }

    /// <summary>
    /// Update time series with a constant change factor, adding 10% to all values
    /// </summary>
    /// <param name="dfs0File">Path and name of file, e.g. Rain_instantaneous.dfs0 test file</param>
    /// <param name="dfs0FileNew">Name of new updated file</param>
    public static void UpdateDfs0Data(string dfs0File, string dfs0FileNew)
    {
      // Open source file
      IDfsFile source = DfsFileFactory.DfsGenericOpen(dfs0File);

      // Create a new file with updated rain values
      DfsBuilder builder = DfsBuilder.Create(source.FileInfo.FileTitle+"Updated", "MIKE SDK", 13);

      // Copy header info from source file to new file
      builder.SetDataType(source.FileInfo.DataType);
      builder.SetGeographicalProjection(source.FileInfo.Projection);
      builder.SetTemporalAxis(source.FileInfo.TimeAxis);

      // Copy over first item from source file to new file
      builder.AddDynamicItem(source.ItemInfo[0]);

      // Create the new file
      builder.CreateFile(dfs0FileNew);
      IDfsFile target = builder.GetFile();

      // Loop over all timesteps
      for (int i = 0; i < source.FileInfo.TimeAxis.NumberOfTimeSteps; i++)
      {
        // Read time step for item, and extract value
        IDfsItemData<double> itemData = (IDfsItemData<double>)source.ReadItemTimeStep(1, i);
        double value = itemData.Data[0];
        // Write new value to target, adding 10% to its value
        target.WriteItemTimeStepNext(itemData.Time, new double[] { value * 1.1 });
      }

      source.Close();
      target.Close();
    }



    /// <summary>
    /// Creates a dfs0 file, with an equidistant time axis and one dynamic item.
    /// <para>
    /// It uses the generic <see cref="DfsBuilder"/>, since currently no specialized 
    /// builder exists for the dfs0 files.
    /// </para>
    /// </summary>
    /// <param name="filename">Name of new file</param>
    /// <param name="calendarAxis">boolean specifying whether the temporal axis should be a calendar axis or a time axis</param>
    public static void CreateDfs0File(string filename, bool calendarAxis)
    {
      DfsFactory factory = new DfsFactory();
      DfsBuilder builder = DfsBuilder.Create("TemporalAxisTest", "dfs Timeseries Bridge", 10000);

      // Set up file header
      builder.SetDataType(1);
      builder.SetGeographicalProjection(factory.CreateProjectionUndefined());
      if (calendarAxis)
        builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec, new DateTime(2010, 01, 04, 12, 34, 00), 4, 10));
      else
        builder.SetTemporalAxis(factory.CreateTemporalEqTimeAxis(eumUnit.eumUsec, 3, 10));
      builder.SetItemStatisticsType(StatType.RegularStat);

      // Set up first item
      DfsDynamicItemBuilder item1 = builder.CreateDynamicItemBuilder();
      item1.Set("WaterLevel item", eumQuantity.Create(eumItem.eumIWaterLevel, eumUnit.eumUmeter),
                DfsSimpleType.Float);
      item1.SetValueType(DataValueType.Instantaneous);
      item1.SetAxis(factory.CreateAxisEqD0());
      item1.SetReferenceCoordinates(1f, 2f, 3f);
      builder.AddDynamicItem(item1.GetDynamicItemInfo());

      DfsDynamicItemBuilder item2 = builder.CreateDynamicItemBuilder();
      item2.Set("WaterDepth item", eumQuantity.Create(eumItem.eumIWaterDepth, eumUnit.eumUmeter),
                DfsSimpleType.Float);
      item2.SetValueType(DataValueType.Instantaneous);
      item2.SetAxis(factory.CreateAxisEqD0());
      item2.SetReferenceCoordinates(1f, 2f, 3f);
      builder.AddDynamicItem(item2.GetDynamicItemInfo());

      // Create file
      builder.CreateFile(filename);
      IDfsFile file = builder.GetFile();

      // Write data to file
      file.WriteItemTimeStepNext(0, new float[] {   0f });  // water level
      file.WriteItemTimeStepNext(0, new float[] { 100f });  // water depth
      file.WriteItemTimeStepNext(0, new float[] {   1f });  // water level
      file.WriteItemTimeStepNext(0, new float[] { 101f });  // water depth
      file.WriteItemTimeStepNext(0, new float[] {   2f });  // water level
      file.WriteItemTimeStepNext(0, new float[] { 102f });  // water depth
      file.WriteItemTimeStepNext(0, new float[] {   3f });  // etc...
      file.WriteItemTimeStepNext(0, new float[] { 103f });
      file.WriteItemTimeStepNext(0, new float[] {   4f });
      file.WriteItemTimeStepNext(0, new float[] { 104f });
      file.WriteItemTimeStepNext(0, new float[] {   5f });
      file.WriteItemTimeStepNext(0, new float[] { 105f });
      file.WriteItemTimeStepNext(0, new float[] {  10f });
      file.WriteItemTimeStepNext(0, new float[] { 110f });
      file.WriteItemTimeStepNext(0, new float[] {  11f });
      file.WriteItemTimeStepNext(0, new float[] { 111f });
      file.WriteItemTimeStepNext(0, new float[] {  12f });
      file.WriteItemTimeStepNext(0, new float[] { 112f });
      file.WriteItemTimeStepNext(0, new float[] {  13f });
      file.WriteItemTimeStepNext(0, new float[] { 113f });

      file.Close();
    }

    /// <summary>
    /// Creates a dfs0 file, with an equidistant time axis and one dynamic item.
    /// <para>
    /// It uses the generic <see cref="DfsBuilder"/>, since currently no specialized 
    /// builder exists for the dfs0 files.
    /// </para>
    /// </summary>
    /// <param name="filename">Name of new file</param>
    /// <param name="calendarAxis">boolean specifying whether the temporal axis should be a calendar axis or a time axis</param>
    public static void CreateDfs0FileFromArray(string filename, bool calendarAxis)
    {
      DfsFactory factory = new DfsFactory();
      DfsBuilder builder = DfsBuilder.Create("TemporalAxisTest", "dfs Timeseries Bridge", 10000);

      // Set up file header
      builder.SetDataType(1);
      builder.SetGeographicalProjection(factory.CreateProjectionUndefined());
      if (calendarAxis)
        builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(eumUnit.eumUsec, new DateTime(2010, 01, 04, 12, 34, 00), 4, 10));
      else
        builder.SetTemporalAxis(factory.CreateTemporalEqTimeAxis(eumUnit.eumUsec, 3, 10));
      builder.SetItemStatisticsType(StatType.RegularStat);

      // Set up first item
      DfsDynamicItemBuilder item1 = builder.CreateDynamicItemBuilder();
      item1.Set("WaterLevel item", eumQuantity.Create(eumItem.eumIWaterLevel, eumUnit.eumUmeter),
                DfsSimpleType.Float);
      item1.SetValueType(DataValueType.Instantaneous);
      item1.SetAxis(factory.CreateAxisEqD0());
      item1.SetReferenceCoordinates(1f, 2f, 3f);
      builder.AddDynamicItem(item1.GetDynamicItemInfo());

      DfsDynamicItemBuilder item2 = builder.CreateDynamicItemBuilder();
      item2.Set("WaterDepth item", eumQuantity.Create(eumItem.eumIWaterDepth, eumUnit.eumUmeter),
                DfsSimpleType.Float);
      item2.SetValueType(DataValueType.Instantaneous);
      item2.SetAxis(factory.CreateAxisEqD0());
      item2.SetReferenceCoordinates(1f, 2f, 3f);
      builder.AddDynamicItem(item2.GetDynamicItemInfo());

      // Create file
      builder.CreateFile(filename);
      IDfsFile file = builder.GetFile();

      // Time is not important, since it is equidistant
      double[] times = new double[10];
      double[,] values = new double[10,2];

      // Write data to file
      values[0, 0] = 0f;  // water level
      values[0, 1] = 100f;  // water depth
      values[1, 0] = 1f;  // water level
      values[1, 1] = 101f;  // water depth
      values[2, 0] = 2f;  // water level
      values[2, 1] = 102f;  // water depth
      values[3, 0] = 3f;  // etc...
      values[3, 1] = 103f;
      values[4, 0] = 4f;
      values[4, 1] = 104f;
      values[5, 0] = 5f;
      values[5, 1] = 105f;
      values[6, 0] = 10f;
      values[6, 1] = 110f;
      values[7, 0] = 11f;
      values[7, 1] = 111f;
      values[8, 0] = 12f;
      values[8, 1] = 112f;
      values[9, 0] = 13f;
      values[9, 1] = 113f;

      DHI.Generic.MikeZero.DFS.dfs0.Dfs0Util.WriteDfs0DataDouble(file, times, values);

      file.Close();
    }
  
  
  }
}
