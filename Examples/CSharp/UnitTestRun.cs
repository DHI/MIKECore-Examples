using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfsu;
using NUnit.Framework;

namespace DHI.SDK.Examples
{
  /// <summary>
  /// Helper class, running some of the examples as unit tests
  /// </summary>
  public class UnitTestRun
  {

    [Test]
    public void RunMaxVelocityFieldTest()
    {
      ExamplesDfs2.MaxVelocityField(UnitTestHelper.TestDataRoot+ "OresundHD.dfs2", UnitTestHelper.TestDataRoot+ "out_OresundHD-maxVel.dfs2");
    }

  }
}
