using System;
using System.IO;
using NUnit.Framework;

namespace DHI.MikeCore.Examples
{
  public class UnitTestHelper
  {

    /// <summary>
    /// Relative path to test data. Must end with a \
    /// </summary>
    public static string TestDataRootRelative = @"..\..\..\..\..\TestData\";

    /// <summary>
    /// Path to test data
    /// </summary>
    public static string TestDataRoot
    {
      get { return (new Uri(Path.Combine(TestContext.CurrentContext.TestDirectory, TestDataRootRelative)).LocalPath); }
    }

  }
}
