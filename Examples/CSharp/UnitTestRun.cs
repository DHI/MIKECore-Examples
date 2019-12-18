using NUnit.Framework;

namespace DHI.MikeCore.Examples
{
  /// <summary>
  /// Helper class, running some of the examples as unit tests
  /// </summary>
  public class UnitTestRun
  {

    [Test]
    public void RunExamplesPFSTest()
    {
      ExamplesPFS.OpenPFSFile(UnitTestHelper.TestDataRoot + "pfsExample.pfs");
      ExamplesPFS.PFSBuilderExample(UnitTestHelper.TestDataRoot + "test_pfsBuilderExample.pfs");
    }

    [Test]
    public void RunExamplesDfs2Test()
    {
      ExamplesDfs2.GetjkIndexForGeoCoordinate(UnitTestHelper.TestDataRoot + "OresundHD.dfs2");
      ExamplesDfs2.MaxVelocityField(UnitTestHelper.TestDataRoot + "OresundHD.dfs2", UnitTestHelper.TestDataRoot + "test_OresundHD-maxVel.dfs2");
    }
  }

  /// <summary>
  /// Helper class, running some of the examples as unit tests
  /// </summary>
  [TestFixture]
  public class ChartMapTests
  {
    [Test]
    public void MeshTest()
    {
      ChartExamples.MeshTest(true);
    }

    [Test]
    public void DfsuTest()
    {
      ChartExamples.DfsuTest(true);
    }
 
  }


}
