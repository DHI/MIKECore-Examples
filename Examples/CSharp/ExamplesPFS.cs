using System;
using System.Globalization;
using DHI.PFS;

namespace DHI.SDK.Examples
{
  public class ExamplesPFS
  {
    /// <summary>
    /// Example of how to read a PFS file
    /// </summary>
    /// <param name="filePath">File and path to the pfsExample.pfs file in the TestData folder</param>
    public static void OpenPFSFile(string filePath)
    {
      // Loading/Reading the file
      PFSFile pfsFile = new PFSFile(filePath, false);

      // Outmost section
      PFSSection target = pfsFile.GetTarget("Run11", 1);

      // Sub-sections
      PFSSection section1 = target.GetSection("Results", 1);
      PFSSection section2 = section1.GetSection("Result", 1);

      // Keywords and paramters
      string bridge = section2.GetKeyword("outid", 1).GetParameter(1).ToStringValue();

      // Get the second key3 keyword from the run11 target
      PFSKeyword kwKey3 = target.GetKeyword("key3", 2);
      // Get the CLOB parameter
      PFSClob clob = kwKey3.GetParameter(1).ToClob();
      // Extract values from the CLOB
      for (int i = 0; i < 5; i++)
      {
        double d = clob.GetDouble();
        Console.Out.WriteLine("clob data: {0}", d);
      }
    }

    /// <summary>
    /// Example of different modifications to a PFS file, storing the result
    /// in a new file name
    /// </summary>
    /// <param name="filePath">File and path to the pfsExample.pfs file in the TestData folder</param>
    /// <param name="newFilePath">File and path to the new modified file</param>
    public static void ModifyPFSFile(string filePath, string newFilePath)
    {
      // Loading/Reading the file
      PFSFile pfsFile = new PFSFile(filePath, false);

      // Outmost section
      PFSSection target = pfsFile.GetTarget("Run11", 1);

      // Sub-section
      PFSSection section1 = target.GetSection("Results", 1);
      // Rename section
      section1.Name = "HDResults";

      // Add another section after the HDResults section. The new section
      // is returned.
      PFSSection sectionRR = section1.InsertNewSection("RRResults", 2);
      // Add a keyword to the new section. The new keyword is returned.
      PFSKeyword kwOutid = sectionRR.InsertNewKeyword("outid", 1);
      // Add a parameter
      kwOutid.InsertNewParameterString("rr out", 1);
      // Add yet another keyword to the new section. The new keyword is returned.
      PFSKeyword kwFile = sectionRR.InsertNewKeyword("file", 2);
      // Add a parameter
      kwFile.InsertNewParameterFileName(@".\outputRR.res11", 1);

      PFSSection section2 = section1.GetSection("Result", 1);
      // Modify first parameter (string) of keyword
      section2.GetKeyword("outid", 1).GetParameter(1).ModifyStringParameter("hd out");

      // Delete the first keyword in the outer-most section
      target.DeleteKeyword("key1", 1);

      // Write back file
      pfsFile.Write(newFilePath);

    }

    /// <summary>
    /// Example on how to create a new PFS file from scratch
    /// </summary>
    /// <param name="newFilePath">File and path to the new modified file</param>
    public static void PFSBuilderExample(string newFilePath)
    {

      PFSBuilder builder = new PFSBuilder();
      builder.AddTarget("Run11");
      // Add keyword and parameters
      builder.AddKeyword("key1");
      builder.AddInt(2);
      builder.AddBool(true);
      // Add keyword and parameters in once call
      builder.AddKeywordValues("key2", 3.3, 4, "someText");

      // Add subsections
      builder.AddSection("Results");
      builder.AddSection("Result");
      builder.AddKeywordValues("outid", "default out");
      // File name parameter
      builder.AddKeyword("file");
      builder.AddFileName(@".\output.res11");
      builder.EndSection();
      builder.EndSection();

      // End target section (Run11)
      builder.EndSection();

      // Write file to disc
      builder.Write(newFilePath);
    }

    /// <summary>
    /// Example on how to use the <see cref="PFSTokenReader"/> for
    /// finding a keyword.
    /// </summary>
    /// <param name="filePath">File and path to the pfsExampleData.pfs file in the TestData folder</param>
      public static void PFSTokenReading(string filePath)
    {
      PFSTokenReader reader = new PFSTokenReader(filePath);

      int tokenCount = 0;
      PFSToken pfstNextToken;
      while ((pfstNextToken = reader.NextToken()) != PFSToken.PfsEOF)
      {
        if (pfstNextToken == PFSToken.KeyWord)
        {
          string kwName = reader.GetTokenString();
          if (StringComparer.OrdinalIgnoreCase.Equals(kwName, "Point"))
          {
            reader.NextToken();
            double x = double.Parse(reader.GetTokenString(), CultureInfo.InvariantCulture);
            reader.NextToken();
            double y = double.Parse(reader.GetTokenString(), CultureInfo.InvariantCulture);
            reader.NextToken();
            double z = double.Parse(reader.GetTokenString(), CultureInfo.InvariantCulture);
            Console.Out.WriteLine("(x, y, z) = ({0}, {1}, {2})", x, y, z);
          }
        }
        tokenCount++;
      }
      reader.Dispose();
    }


  }
}
