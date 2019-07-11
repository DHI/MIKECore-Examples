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
Usage
    DHI.MikeCore.Util -[tool] [arguments]

Tools:
"
        + ExamplesDfs2.MaxVelocityFieldUsage
        + ExamplesDfs2.ResampleUsage
        + ExamplesDfsu.ExtractDfsu2DLayerFrom3DUsage
        + ExamplesDfsu.ExtractSubareaDfsu2DUsage
      ;

    static void PrintUsage()
    {
      Console.Out.WriteLine(usage);
    }



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
        case "-resample":
          CheckArguments(arg0, args, 4);
          ExamplesDfs2.Resample(args[1], args[2], ArgInt(args, 3), ArgInt(args, 4));
          break;
        case "-maxvelocityfield":
          CheckArguments(arg0, args, 2);
          ExamplesDfs2.MaxVelocityField(args[1], args[2]);
          break;
        case "-dfsuextractlayer":
          CheckArguments(arg0, args, 3);
          ExamplesDfsu.ExtractDfsu2DLayerFrom3D(args[1], args[2], ArgInt(args, 3));
          break;
        case "-dfsuextractsubarea":
          CheckArguments(arg0, args, 6);
          ExamplesDfsu.ExtractSubareaDfsu2D(args[1], args[2], ArgDouble(args, 3), ArgDouble(args, 4), ArgDouble(args, 5), ArgDouble(args, 6));
          break;
      }
    }

    /// <summary> Check number of arguments </summary>
    private static void CheckArguments(string tool, string[] args, int argTest)
    {
      // First args is -[tool], so disregard that one
      if (args.Length != argTest+1)
      {
        Console.Out.WriteLine("{0} requires {1} arguments.", tool, argTest);
        PrintUsage();
        Environment.Exit(-1);
      }
    }

    /// <summary> Parse and return integer at <paramref name="argIndex"/> </summary>
    private static int ArgInt(string[] args, int argIndex)
    {
      if (int.TryParse(args[argIndex], out int result))
        return result;
      Console.Out.WriteLine("Argument {0} is not an integer.", argIndex);
      PrintUsage();
      Environment.Exit(-1);
      return -1;
    }

    /// <summary> Parse and return integer at <paramref name="argIndex"/> </summary>
    private static double ArgDouble(string[] args, int argIndex)
    {
      if (double.TryParse(args[argIndex], NumberStyles.Any, CultureInfo.InvariantCulture, out double result))
        return result;
      Console.Out.WriteLine("Argument {0} is not a floating point number.", argIndex);
      PrintUsage();
      Environment.Exit(-1);
      return -1;
    }
  }
}
