using System;

namespace DHI.MikeCore.Examples
{
  /// <summary>
  /// Class for running one of the examples, used when building an appliation (.exe file)
  /// </summary>
  class Program
  {
    /// <summary> Static constructor </summary>
    static Program()
    {
      // The setup method will make your application find the MIKE assemblies at runtime.
      // The first call of the setup method takes precedence. Any subsequent calls will be ignored.
      // It must be called BEFORE any method using MIKE libraries is called, i.e. it is not sufficient
      // to call it as the first thing in that method using the MIKE libraries. Often this can be achieved
      // by having this code in the static constructor.
      // If MIKE Core is x-copy deployed with the application, this is not required.
      if (!DHI.Mike.Install.MikeImport.Setup(18, DHI.Mike.Install.MikeProducts.MikeCore))
        throw new Exception("Cannot find a proper MIKE installation");
    }

    /// <summary>
    /// Simple main program that runs one of the examples.
    /// <para>
    /// This example starts and runs the <see cref="ExamplesDfsu.ExtractDfsu2DLayerFrom3D"/>.
    /// </para>
    /// <para>
    /// Use the ExampleRunBuild.bat file to build the program.
    /// The bat file may need modifications if using another example.
    /// </para>
    /// <para>
    /// You may also want to add /platform:x86 or /platform:x64 to the .bat file, 
    /// in case you want to be sure to build a specific version.
    /// </para>
    /// </summary>
    public static void Main(string[] args)
    {
      if (args.Length != 3)
      {
        Console.Out.WriteLine("Extracting layer from 3D dfsu file to a 2D dfsu file");
        Console.Out.WriteLine("Usage:");
        Console.Out.WriteLine("    ExampleRun dfsu3Dfile.dfsu newdfsu2DFile.dfsu layernumber");
        Console.Out.WriteLine("where:");
        Console.Out.WriteLine("    layernumber: number of layer to extract");
        Console.Out.WriteLine("                 positive: counting from bottom and up, i.e. 1 is bottom layer");
        Console.Out.WriteLine("                 negative: counting from top and down, i.e. -1 is top layer");
        return;
      }

      ExamplesDfsu.ExtractDfsu2DLayerFrom3D(args[0], args[1], int.Parse(args[2]));
    }
  }
}