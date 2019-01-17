// Run this from the command line using cs-script by 
//     css testscript.cs


//css_reference DHI.Generic.MikeZero.EUM;
using System;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfs123;
using DHI.Generic.MikeZero;

namespace ConsoleApplication
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

    static void Main(string[] args)
    {
      string filename = @"Landuse2.dfs2";
      
      // Copy existing file to new name, in order to not mess up existing file
      System.IO.File.Copy(@"..\..\TestData\Landuse.dfs2", filename, true);
      System.IO.File.SetAttributes(filename, System.IO.FileAttributes.Normal); // remove read-only flag
      
      // Open the file for editing
      IDfs2File file = DfsFileFactory.Dfs2FileOpenEdit(filename);
      
      // Original name is "Landuse" (7 characters), "GroundUse" is truncated to "GroundU"
      file.ItemInfo[0].Name = "GroundUse";
      // Provide a new quantity (updating the item and unit of the quantity directly is not allowed!)
      file.ItemInfo[0].Quantity = new eumQuantity(eumItem.eumIAreaFraction, eumUnit.eumUPerCent);
      
      // done
      file.Close();
      
      Console.Out.WriteLine("Updated "+filename+" - remember to delete the file again");
      
    }
  }
}
