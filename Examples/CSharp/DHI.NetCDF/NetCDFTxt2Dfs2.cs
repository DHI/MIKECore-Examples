using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfs123;
using DHI.Generic.MikeZero;

namespace DHI.NetCDF
{
  
  /// <summary>
  /// Example of how to read a grid based NetCDF text file and
  /// create a dfs2 file.
  /// </summary>
  public class NetCDFTxt2Dfs2
  {
		
    /// <summary> Static constructor </summary>
    static NetCDFTxt2Dfs2()
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
    /// Main method, main entry point to program.
    /// </summary>
    public static void Main(string[] args)
		{
      if (args.Length != 2)
      {
        Console.Out.WriteLine("Converting NetCDF text grid file to dfs2");
        Console.Out.WriteLine("Usage:");
        Console.Out.WriteLine("     NetCDFTxt2Dfs2.exe netcdftxtfile.txt \"Projection string\"");
        Console.Out.WriteLine("     NetCDFTxt2Dfs2.exe netcdftxtfile.txt ProjectionStringFile.prj");
        return;
      }

      string projString = args[1];
      if (projString.EndsWith(".prj"))
      {
        if (File.Exists(projString))
          projString = File.ReadAllText(projString);
        else
          throw new Exception("Could not locate projection file: "+projString);
      }

      NetCDFTxt2Dfs2 converter = new NetCDFTxt2Dfs2();
      converter.Process(args[0], projString);
    }

    
    
    private StreamReader _reader;
    private Stack<string> _readStack;

    /// <summary> Map for fast access to each dimension based on its variable name </summary>
    private Dictionary<string, int> _dimensionsMap;
    /// <summary> Map for fast access to each variable based on its name </summary>
    private Dictionary<string, Variable> _variablesMap;
    /// <summary> Dynamic items </summary>
    private List<Variable> _items;
    /// <summary> Time item </summary>
    private Variable _time;
    /// <summary> X axis item </summary>
    private Variable _x;
    /// <summary> Y axis item </summary>
    private Variable _y;

    /// <summary> Dimensions, size of X and Y </summary>
    private readonly int[] _dimensions = new int[2];

    /// <summary> Type of variable </summary>
    enum VariableType
    {
      Time,
      Axis,
      Item
    }

    /// <summary>
    /// Helper class, containing variable/item information
    /// </summary>
    class Variable
    {
      public VariableType Type;
      public Type DataType;
      public string Name;
      public string Id;
      public string Unit;
      public int Code;
      public int Size;
      public Array Array;
    }

    /// <summary>
    /// Process the file, store the provided projection string in the DFS2 file
    /// </summary>
    public void Process(string filename, string projection)
    {
      // Reset working variables
      _readStack = new Stack<string>();
      _dimensionsMap = new Dictionary<string, int>();
      _variablesMap = new Dictionary<string, Variable>();
      _items = new List<Variable>();

      // Read NetCDF txt file
      Read(filename);
      // Make filename with .dfs2 extension
      string dfsFilename = Path.ChangeExtension(filename, ".dfs2");
      // Write dfs2 file
      Write(dfsFilename, projection);
    }


    #region DFS writing routines

    private const int FlipX = 1;
    private const int FlipY = 2;
    private const int FlipXY = 3;

    /// <summary> Write dfs2 file </summary>
    private void Write(string dfsFilename, string projectionString)
    {
      DfsFactory factory = new DfsFactory();
      Dfs2Builder builder = new Dfs2Builder();

      // Data type
      builder.SetDataType(0);

      // Projection and spatial axis
      double lon0; double lat0; double dx; double dy;
      int flip;
      FindAxisProperties(out lon0, out lat0, out dx, out dy, out flip);
      builder.SetGeographicalProjection(factory.CreateProjectionGeoOrigin(projectionString, lon0, lat0, 0));
      builder.SetSpatialAxis(factory.CreateAxisEqD2(eumUnit.eumUdegree, _dimensions[0], 0, dx, _dimensions[1], 0, dy));

      // Time axis
      eumUnit timeUnit;
      DateTime startDateTime;
      double timeStep;
      FindTimeProperties(out timeUnit, out startDateTime, out timeStep);
      builder.SetTemporalAxis(factory.CreateTemporalEqCalendarAxis(timeUnit, startDateTime, 0, timeStep));

      // Add dynamic items
      foreach (Variable item in _items)
      {
        eumQuantity quantity = GetQuantityFromItem(item);
        builder.AddDynamicItem(item.Name, quantity, DfsSimpleType.Float, DataValueType.Instantaneous);
      }

      // Create and get file (no static items there)
      builder.CreateFile(dfsFilename);
      Dfs2File dfs2File = builder.GetFile();

      // Write data to file
      int itemSize = _dimensions[0]*_dimensions[1];
      float[] values = new float[itemSize];
      float[] valuesFlipped = new float[itemSize];
      for (int i = 0; i < _time.Array.Length; i++)
      {
        for (int j = 0; j < _items.Count; j++)
        {
          // Time of time step
          double time = (double) _time.Array.GetValue(i);
          // Values for all time steps
          float[] allfloats = (float[]) _items[j].Array;
          // Copy single time step data from allFloats to values
          Array.Copy(allfloats, i*itemSize, values, 0, itemSize);
          // Flip values, if necessary
          float[] actual;
          if (flip == 0)
            actual = values;
          else
          {
            PerformFlip(flip, values, valuesFlipped);
            actual = valuesFlipped;
          }
          // Save values to file
          dfs2File.WriteItemTimeStepNext(time, actual);
        }
      }
      dfs2File.Close();
    }

    /// <summary>
    /// Flip grid values in X, Y or X-Y direction, storing the
    /// new flipped values in <paramref name="valuesFlipped"/>
    /// </summary>
    private void PerformFlip(int flip, float[] values, float[] valuesFlipped)
    {
      int xDim = _dimensions[0];
      int yDim = _dimensions[1];

      if (flip == FlipX)
        for (int i = 0; i < xDim; i++)
          for (int j = 0; j < yDim; j++)
            valuesFlipped[j*xDim + i] = values[j*xDim + (xDim - i - 1)];
      else if (flip == FlipY)
        for (int i = 0; i < xDim; i++)
          for (int j = 0; j < yDim; j++)
            valuesFlipped[j*xDim + i] = values[(yDim - j - 1)*xDim + i];
      else if (flip == FlipXY)
        for (int i = 0; i < xDim; i++)
          for (int j = 0; j < yDim; j++)
            valuesFlipped[j*xDim + i] = values[(yDim - j - 1)*xDim + (xDim - i - 1)];
    }

    /// <summary> Extract EUM quantity from NetCDF item </summary>
    private eumQuantity GetQuantityFromItem(Variable item)
    {
      eumItem eumitem = eumItem.eumIItemUndefined;
      eumUnit eumunit = eumUnit.eumUUnitUndefined;
      switch (item.Name)
      {
        case "Mean sea-level pressure"  : eumitem = eumItem.eumIPressure; break;
        case "10 metre U wind component": eumitem = eumItem.eumIWindVelocity; break;
        case "10 metre V wind component": eumitem = eumItem.eumIWindVelocity; break;
      }
      switch (item.Unit.ToLower())
      {
        case "m s**-1": eumunit = eumUnit.eumUmeterPerSec; break;
        case "pa"     : eumunit = eumUnit.eumUPascal; break;
      }
      return new eumQuantity(eumitem, eumunit);
    }

    /// <summary> Find and extract axis properties </summary>
    private void FindAxisProperties(out double lon0, out double lat0, out double dx, out double dy, out int flip)
    {
      flip = 0;
      double[] vals = (double[])_x.Array;
      lon0 = vals.Last();
      if (vals[0] < lon0)
        lon0 = vals[0];
      // Assuming equidistant axis here
      dx = vals[1] - vals[0];
      if (dx < 0)
      {
        dx = -dx;
        flip += FlipX;
      }
      
      vals = (double[])_y.Array;
      lat0 = vals.Last();
      if (vals[0] < lat0)
        lat0 = vals[0];
      // Assuming equidistant axis here
      dy = vals[1] - vals[0];
      if (dy < 0)
      {
        dy = -dy;
        flip += FlipY;
      }
    }

    /// <summary> Extract time properties </summary>
    private void FindTimeProperties(out eumUnit timeUnit, out DateTime startDateTime, out double timestep)
    {
      string unitstr = _time.Unit;
      string[] strings = unitstr.Split(new string[]{"since"},2,StringSplitOptions.RemoveEmptyEntries);
      string timeUnitStr = strings[0].Trim();
      switch (timeUnitStr)
      {
        case "hours":
          timeUnit = eumUnit.eumUhour;
          break;
        default:
          throw new NotSupportedException("Time unit is not known: "+timeUnitStr);
      }
      string startTimeStr = strings[1].Trim();
      startDateTime = DateTime.ParseExact(startTimeStr, "yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
      // Assuming a fixed time step here
      double[] times = (double[]) _time.Array;
      timestep = times[1] - times[0];
    }

    #endregion

    #region NetCDF txt reading routines
    
    private void Read(string filename)
    {
      _reader = new StreamReader(filename);

      string line;

      // Skip first header line
      line = ReadLine();

      line = ReadLine();
      if (line == "dimensions:")
        ReadDimensions();

      line = ReadLine();
      if (line == "variables:")
        ReadVariables();

      line = ReadLine();
      if (line == "data:")
        ReadData();

      _reader.Close();
    }

    private void ReadDimensions()
    {
      while (true)
      {
        string line = ReadLine();
        if (line == null)
          return;

        int dimval;
        
        string key;
        string val;
        if (!SplitKeyVal(line, out key, out val))
        {
          PushBack(line);
          return;
        }
        
        if (val == "UNLIMITED")
          dimval = int.MaxValue;
        else
          dimval = int.Parse(val);
        _dimensionsMap.Add(key, dimval);
      }
    }

    private void ReadVariables()
    {
      ReadVars();
      int itemSize = _dimensions[0]*_dimensions[1];
      foreach (Variable item in _items)
      {
        item.Size = itemSize;
      }
    }

    private void ReadVars()
    {
      while (true) // for each variable
      {
        string line = ReadLine();
        if (line == null) return;

        // Read variable name
        string varDef;
        Type varDataType;
        if (line.StartsWith("double"))
        {
          varDef = RemoveString(line, "double");
          varDataType = typeof (double);
        }
        else if (line.StartsWith("float"))
        {
          varDef = RemoveString(line, "double");
          varDataType = typeof(float);
        }
        else // no more variables
        {
          PushBack(line);
          return;
        }

        // Create variable
        Variable var = new Variable();
        var.DataType = varDataType;
        string axis = null;

        // Find arguments and extract variable name
        int argStart = varDef.IndexOf('(');
        int argLength = varDef.IndexOf(')') - argStart - 1;
        string argsstr = varDef.Substring(argStart + 1, argLength);
        // Extract name of variable without arguments
        var.Id = varDef.Substring(0, argStart);
        // Extract arguments
        string[] args = argsstr.Split(',');
        // Determine the variable type
        var.Type = VariableType.Item;
        if (args.Length != 3)
        {
          if (args[0] == "time")
            var.Type = VariableType.Time;
          else
            var.Type = VariableType.Axis;
        }

        // Read all lines starting with "<varName>:"
        string varlineident = var.Id + ":";
        while (true) // for each parameter
        {
          line = ReadLine();
          // Check if line belongs to this variable
          if (!line.StartsWith(varlineident))
          {
            // Line did not belong here, push it back
            PushBack(line);
            break;
          }
          // Handle parameter
          string parString = RemoveString(line, varlineident);
          // Extract key and value
          string key;
          string val;
          SplitKeyVal(parString, out key, out val);

          // Store the parameter
          switch (key)
          {
            case "":
              break;
            case "long_name":
              var.Name = val.Trim('"');
              break;
            case "units":
              var.Unit = val.Trim('"');
              break;
            case "Code":
              var.Code = int.Parse(val);
              break;
            case "axis":
              axis = val.Trim('"');
              break;
          }
        }

        // Special action for each variable type
        switch (var.Type)
        {
          case VariableType.Time:
            _time = var;
            break;
          case VariableType.Axis:
            if (axis == "X")
            {
              _x = var;
              _dimensions[0] = _dimensionsMap[var.Id];
              _x.Size = _dimensions[0];
            }
            else if (axis == "Y")
            {
              _y = var;
              _dimensions[1] = _dimensionsMap[var.Id];
              _y.Size = _dimensions[1];
            }
            break;
          case VariableType.Item:
            _items.Add(var);
            break;
        }
        // Store in map for fast access
        _variablesMap.Add(var.Id, var);
      }
    }

    /// <summary>
    /// Read and store all data.
    /// Since NetCDF txt file is "item major" (storing all time step values for each item in order)
    /// and DFS is "time major" (storing all item values for each time steps in order), then
    /// we need to read all the data into memory, before writing it again.
    /// </summary>
    public void ReadData()
    {
      while (true)
      {
        string line = ReadLine();
        if (line == null) return;

        int eqIndex = line.IndexOf("=");
        
        if (eqIndex >= 0)
        {
          string varName = line.Substring(0, eqIndex).Trim();
          string rem = line.Substring(eqIndex + 1).Trim();
          // push-back remainder of line, in case there are any values there
          if (!string.IsNullOrEmpty(rem))
            PushBack(rem);

          Variable var = _variablesMap[varName];
          int timeSteps = var.Type == VariableType.Item ? _time.Array.Length : 1;
          if (var.DataType == typeof(double))
          {
            double[] array;
            if (var.Size > 0)
            {
              array = new double[var.Size*timeSteps];
              int readCount = ReadDoubles(array);
              if (readCount != array.Length)
                Console.Out.WriteLine("Size mismatch when reading data: " + var.Id);
            }
            else
            {
              array = ReadDoubles().ToArray();
            }
            var.Array = array;
          }
          else
          {
            float[] array;
            if (var.Size > 0)
            {
              array = new float[var.Size*timeSteps];
              int readCount = ReadFloats(array);
              if (readCount != array.Length)
                Console.Out.WriteLine("Size mismatch when reading data: "+ var.Id);
              var.Array = array;
            }
          }
        }
        else
        {
          PushBack(line);
          return;
        }
      }
    }


    /// <summary> Split character for splitting a "key = value;" string </summary>
    private static readonly char[] _keyvalSplitChars = new char[] { '=' };

    /// <summary>
    /// Split a "key = value;" string into its key and value, return true on success
    /// </summary>
    private bool SplitKeyVal(string str, out string key, out string val)
    {
      
      string[] strs = str.Split(_keyvalSplitChars, 2);
      if (strs.Length == 2)
      {
        key = strs[0].Trim();
        val = strs[1].Trim(' ',';');
        return true;
      }
      key = string.Empty;
      val = null;
      return false;
    }

    /// <summary>
    /// Read doubles from file, stop at first ";"
    /// Returns the total number of doubles read, 
    /// which may not equal the size of the array 
    /// to store the doubles in
    /// </summary>
    private int ReadDoubles(double[] array)
    {
      int i = 0;
      bool cont = true;
      while (cont)
      {
        string line = ReadLine();
        if (line == null)
          return i;

        // Do not read beyond the ';' character
        int endIndex = line.IndexOf(';');
        if (endIndex >= 0)
        {
          line = line.Substring(0, endIndex);
          cont = false;
        }

        // Values are split by the ','
        string[] vals = line.Split(',');

        // Parse values
        foreach (string val in vals)
        {
          if (string.IsNullOrEmpty(val.Trim()))
            continue;
          double d = double.Parse(val, CultureInfo.InvariantCulture);
          if (i < array.Length)
            array[i] = d;
          i++;
        }
      }
      return i;
    }

    /// <summary>
    /// Read doubles from file, stop at first ";"
    /// Use when number of doubles to read is unknown
    /// </summary>
    private List<double> ReadDoubles()
    {
      List<double> array = new List<double>();
      bool cont = true;
      while (cont)
      {
        string line = ReadLine();
        if (line == null)
          return array;

        // Do not read beyond the ';' character
        int endIndex = line.IndexOf(';');
        if (endIndex >= 0)
        {
          line = line.Substring(0, endIndex);
          cont = false;
        }

        // Values are split by the ','
        string[] vals = line.Split(',');

        // Parse values
        foreach (string val in vals)
        {
          if (string.IsNullOrEmpty(val.Trim()))
            continue;
          double d = double.Parse(val, CultureInfo.InvariantCulture);
          array.Add(d);
        }
      }
      return array;
    }

    /// <summary>
    /// Read floats from file, stop at first ";"
    /// Returns the total number of doubles read, 
    /// which may not equal the size of the array 
    /// to store the doubles in
    /// </summary>
    private int ReadFloats(float[] array)
    {
      int i = 0;
      bool cont = true;
      while (cont)
      {
        string line = ReadLine();
        if (line == null)
          return i;

        // Do not read beyond the ';' character
        int endIndex = line.IndexOf(';');
        if (endIndex >= 0)
        {
          line = line.Substring(0, endIndex);
          cont = false;
        }

        // Values are split by the ','
        string[] vals = line.Split(',');

        // Parse values
        foreach (string val in vals)
        {
          if (string.IsNullOrEmpty(val.Trim()))
            continue;
          float d = float.Parse(val, CultureInfo.InvariantCulture);
          if (i < array.Length)
            array[i] = d;
          i++;
        }
      }
      return i;
    }

    /// <summary>
    /// Remove str from beginning of line, and trim again
    /// </summary>
    private string RemoveString(string line, string str)
    {
      return line.Substring(str.Length).Trim();
    }

    /// <summary>
    /// Push-back line to file. It will be the next line
    /// returned by the <see cref="ReadLine"/> method
    /// </summary>
    private void PushBack(string line)
    {
      _readStack.Push(line);
    }

    /// <summary>
    /// Read line from file, removing comments and skipping empty lines.
    /// </summary>
    /// <returns>Next line, null at end-of-file</returns>
    private string ReadLine()
    {
      // First check if lines has been pushed-back
      if (_readStack.Count > 0)
        return _readStack.Pop();

      while (true)
      {
        string line = _reader.ReadLine();
        
        // Check for end-of-file
        if (line == null)
          return null;

        line = line.Trim();

        // Remove comments from lines
        int comment = line.IndexOf("//");
        if (comment >= 0)
          line = line.Substring(0, comment);

        // Remove also lines starting with ":"
        if (line.StartsWith(":"))
          continue;

        // Trim (remove spaces)
        line = line.Trim();

        // Skip empty lines
        if (line == string.Empty)
          continue;

        return line;
      }
    }

    #endregion

  }

}
