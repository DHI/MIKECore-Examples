using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using DHI.SDK.Examples;

namespace DHI.MikeCore.Util
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

    public static readonly string usage =
      @"
Usage:
    DHI.MikeCore.Util -[tool] [arguments]

Tools:
";
      //+ DfsDiff.UsageCreateDiffFile
      //+ ExamplesMisc.UsageMergeDfsFileItems
      //+ ExamplesDfs2.UsageMaxVelocityField
      //+ ExamplesDfs2.UsageResample
      //+ ExamplesDfsu.UsageExtractDfsu2DLayerFrom3D
      //+ ExamplesDfsu.UsageExtractSubareaDfsu2D;

    static void PrintUsage()
    {
      Console.Out.WriteLine(usage);

      List<string> usages = new List<string>();
      usages.Add(DfsDiff.UsageCreateDiffFile);
      usages.Add(ExamplesMisc.UsageMergeDfsFileItems);
      usages.Add(ExamplesDfs2.UsageMaxVelocityField);
      usages.Add(ExamplesDfs2.UsageResample);
      usages.Add(ExamplesDfsu.UsageExtractDfsu2DLayerFrom3D);
      usages.Add(ExamplesDfsu.UsageExtractSubareaDfsu2D);
      foreach (string usage in usages)
      {
        int positionOfNewLine = usage.IndexOf("\r\n",2);

        if (positionOfNewLine >= 0)
        {
          string shortUsage = usage.Substring(2, positionOfNewLine-2);
          Console.Out.WriteLine(shortUsage);
        }
      }
      Console.Out.WriteLine("");
      Console.Out.WriteLine("Usage of each tool:");
      foreach (string usage in usages)
      {
        Console.Out.WriteLine(usage);
      }

    }

    static void PrintToolError(string error)
    {
      Console.Out.WriteLine("");
      if (!string.IsNullOrEmpty(error))
      {
        Console.Out.WriteLine("ERROR: {0}", error);
      }

      if (!string.IsNullOrEmpty(_currentToolUsage))
      {
        Console.Out.WriteLine("");
        Console.Out.WriteLine("Usage of this tool");
        Console.Out.WriteLine(_currentToolUsage);
        Console.Out.WriteLine("");
        Console.Out.WriteLine("Run DHI.MikeCore.Util without arguments to see usage of all tools");
      }
    }

    private static string _currentToolUsage;

    static void Main(string[] args)
    {
      if (args.Length == 0)
      {
        PrintUsage();
        return;
      }

      string arg0 = args[0].ToLower();

      // In honor of Microsoft Word and its super users, i.e. those using Word as their text-file-editor
      // Word dislikes the '-' or '\u002D' hyphen, and is more than happy to replace it with '–' ('\u2013')
      // '-' ('\u002D') is the standard hyphen character. These are other unicode hyphen characters
      string hyphenChars = "\u05be\u1806\u2010\u2011\u2012\u2013\u2014\u2015\u2053\u2212\uFE58\uFE63\uFF0D";
      // Check if first character is one of the special hyphen characters, replace with standard hyphen.
      if (hyphenChars.IndexOf(arg0[0]) >= 0)
        arg0 = "-" + arg0.Substring(1);

      if (arg0.StartsWith("-debug"))
      {
        arg0 = arg0.Substring(6);
        System.Diagnostics.Debugger.Launch();
      }

      // Remember to lowercase all case cases
      switch (arg0)
      {
        case "-dfsdiff":
          _currentToolUsage = DfsDiff.UsageCreateDiffFile;
          CheckArguments(arg0, args, 2, 3);
          DfsDiff.CreateDiffFile(args[1], args[2], ArgNull(args,3));
          break;
        case "-dfsmerge":
          _currentToolUsage = ExamplesMisc.UsageMergeDfsFileItems;
          CheckArguments(arg0, args, 2, 99);
          ExamplesMisc.MergeDfsFileItems(args[1], ArgList(args, 2));
          break;
        case "-resample":
          _currentToolUsage = ExamplesDfs2.UsageResample;
          CheckArguments(arg0, args, 4);
          ExamplesDfs2.Resample(args[1], args[2], ArgInt(args, 3), ArgInt(args, 4));
          break;
        case "-maxvelocityfield":
          _currentToolUsage = ExamplesDfs2.UsageMaxVelocityField;
          CheckArguments(arg0, args, 2);
          ExamplesDfs2.MaxVelocityField(args[1], args[2]);
          break;
        case "-dfsuextractlayer":
          _currentToolUsage = ExamplesDfsu.UsageExtractDfsu2DLayerFrom3D;
          CheckArguments(arg0, args, 3);
          ExamplesDfsu.ExtractDfsu2DLayerFrom3D(args[1], args[2], ArgInt(args, 3));
          break;
        case "-dfsuextractsubarea":
          _currentToolUsage = ExamplesDfsu.UsageExtractSubareaDfsu2D;
          CheckArguments(arg0, args, 6);
          ExamplesDfsu.ExtractSubareaDfsu2D(args[1], args[2], ArgDouble(args, 3), ArgDouble(args, 4), ArgDouble(args, 5), ArgDouble(args, 6));
          break;
        default:
          Console.Out.WriteLine("");
          Console.Out.WriteLine("ERROR: Coult not recognize tool name: {0}", arg0);
          Console.Out.WriteLine("");
          Console.Out.WriteLine("Run DHI.MikeCore.Util without arguments to see usage of all tools");
          Environment.Exit(-2);
          break;
      }
    }

    private static List<string> ArgList(string[] args, int startIndex)
    {
      List<string> list = new List<string>(args.Length - startIndex);
      for (int i = startIndex; i < args.Length; i++) list.Add(args[i]);
      return list;
    }

    /// <summary> Return argument number, or null </summary>
    private static string ArgNull(string[] args, int argi)
    {
      if (argi < args.Length)
        return args[argi];
      return null;
    }

    /// <summary> Check number of arguments </summary>
    private static void CheckArguments(string tool, string[] args, int argNumTest)
    {
      int toolArgLength = args.Length - 1;
      // First args is -[tool], so disregard that one
      if (toolArgLength != argNumTest)
      {
        string err = string.Format("{0} requires {1} arguments.", tool, argNumTest);
        PrintToolError(err);
        Environment.Exit(-1);
      }
    }

    /// <summary> Check number of arguments </summary>
    private static void CheckArguments(string tool, string[] args, int argNumFromTest, int argNumToTest)
    {
      int toolArgLength = args.Length - 1;
      // First args is -[tool], so disregard that one
      if (argNumFromTest > toolArgLength || toolArgLength > argNumToTest)
      {
        string err = string.Format("{0} requires {1}-{2} arguments.", tool, argNumFromTest, argNumToTest);
        PrintToolError(err);
        Environment.Exit(-1);
      }
    }

    /// <summary> Parse and return integer at <paramref name="argIndex"/> </summary>
    private static int ArgInt(string[] args, int argIndex)
    {
      if (int.TryParse(args[argIndex], out int result))
        return result;
      string err = String.Format("Argument {0} is not an integer.", argIndex);
      PrintToolError(err);
      Environment.Exit(-1);
      return -1;
    }

    /// <summary> Parse and return integer at <paramref name="argIndex"/> </summary>
    private static double ArgDouble(string[] args, int argIndex)
    {
      if (double.TryParse(args[argIndex], NumberStyles.Any, CultureInfo.InvariantCulture, out double result))
        return result;
      string err = String.Format("Argument {0} is not a floating point number.", argIndex);
      PrintToolError(err);
      Environment.Exit(-1);
      return -1;
    }
  }
}