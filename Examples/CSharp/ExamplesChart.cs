using System.Drawing;
using System.IO;
using DHI.Chart.Map;
using DHI.Generic.MikeZero.DFS;
using DHI.Generic.MikeZero.DFS.dfsu;
using DHI.Generic.MikeZero.DFS.mesh;
using DHI.Projections;

namespace DHI.MikeCore.Examples
{
  /// <summary>
  /// Example of how to use <see cref="DHI.Chart.Map"/> for plotting various types of data
  /// </summary>
  public class ChartExamples
  {
    /// <summary>
    /// Example of how to plot a mesh file. A mesh contains bottom levels in each node.
    /// </summary>
    public static void MeshTest(bool makeBmp)
    {
      DHI.Chart.Map.Chart.Init();

      // Load mesh data
      string pathName = Path.Combine(UnitTestHelper.TestDataRoot, @"oresund.mesh");
      MeshFile meshFile = MeshFile.ReadMesh(pathName);

      // FemGridData is data for the bottom levels, a value in each element node, used for coloring
      FemGridData data = new FemGridData();
      data.CreateNodesAndElements(meshFile.NumberOfNodes, meshFile.NumberOfElements, meshFile.X, meshFile.Y, meshFile.Z.ToFloatArray(), meshFile.ElementTable);

      // Create chart
      DHI.Chart.Map.Chart chart = new DHI.Chart.Map.Chart();

      // Add overlay that plots the bottom levels
      FemGridOverlay overlay = new FemGridOverlay();
      overlay.SetGridData(data);
      overlay.EnableNiceValue = true;
      overlay.CreateAutoScaledRainbowPalette();
      overlay.EnableIsoline = true;
      overlay.ColoringType = MapOverlay.EColoringType.ContinuousColoring;
      overlay.SetFeathering(true, 0.5f);
      overlay.EnableIsolineLabel = true;
      chart.AddOverlay(overlay);

      // Grab map projection of meshfile
      MapProjection mapProj = new MapProjection(meshFile.ProjectionString);
      double lonOrigin, latOrigin, eastOrigin, northOrigin;
      mapProj.GetOrigin(out lonOrigin, out latOrigin);
      mapProj.Geo2Proj(lonOrigin, latOrigin, out eastOrigin, out northOrigin);
      double convergence = mapProj.GetConvergence(lonOrigin, latOrigin);

      // Overlay adding geographical lines
      // Mesh is drawn in map projection coordinates
      GeoGridOverlay ggOverlay = new GeoGridOverlay();
      ggOverlay.ReferenceProjection = meshFile.ProjectionString;
      ggOverlay.DisplayProjection = meshFile.ProjectionString;
      // Origin for mesh data is the origin of the projection
      ggOverlay.SetGeoOrigin(lonOrigin, latOrigin);
      ggOverlay.SetOrigin(eastOrigin, northOrigin);
      // Mesh overlay draws in map projection coordinates, so north-Y rotation is the convergence (which is zero)
      ggOverlay.SetNYCRotation(convergence);
      ggOverlay.EnableLonLatGrid = true;
      ggOverlay.EnableMapProjGrid = true;
      // DataValid must be set after changing in GeoGridOverlay
      ggOverlay.DataValid = true;
      chart.AddOverlay(ggOverlay);

      // Select rectangle to plot, by default the full data area
      MzRectangle wrect = new MzRectangle();
      data.GetDataArea(wrect);

      // Here you may limit the area to be plotted
      //wrect.X0 = wrect.X0 + 20000;
      //wrect.Y0 = wrect.Y0 + 20000;
      //wrect.X1 = wrect.X0 + 30000;
      //wrect.Y1 = wrect.Y0 + 30000;

      chart.SetDataArea(wrect);
      chart.SetView(wrect);

      //chart.GetActiveCoordinateSystem().SetDrawAxis(true, true);
      //chart.GetActiveCoordinateSystem().EnableBorder(true);
      //chart.DrawGrid = true;
      //chart.DrawTitle = true;

      // Draw to bitmap
      double ratio = wrect.Width / (double)wrect.Height;
      Bitmap bmp = new Bitmap(((int)(ratio * 1024)) + 10, 1024);
      chart.Draw(bmp);

      chart.Dispose();

      if (makeBmp)
      {
        string pngFilepath = Path.Combine(UnitTestHelper.TestDataRoot, @"oresundMesh.png");
        bmp.Save(pngFilepath);

        System.Diagnostics.Process.Start(pngFilepath);
      }

    }

    /// <summary>
    /// Example of plotting dfsu file data, water levels with a velocity vector overlay.
    /// </summary>
    public static void DfsuTest(bool makeBmp)
    {
      DHI.Chart.Map.Chart.Init();

      // File to load
      string pathName = Path.Combine(UnitTestHelper.TestDataRoot, @"OresundHD.dfsu");
      DfsuFile dfsu = DfsFileFactory.DfsuFileOpen(pathName);

      // FemVolGridData is data for the water levels, a value in the center of each element, used for coloring
      FemVolGridData data = new FemVolGridData();
      data.CreateNodesAndElements(dfsu.NumberOfNodes, dfsu.NumberOfElements, dfsu.X, dfsu.Y, dfsu.Z, dfsu.ElementTable);

      // FemVolVectorData is data for the vector overlay
      FemVolVectorData uvData = new FemVolVectorData();
      uvData.CreateNodesAndElements(dfsu.NumberOfNodes, dfsu.NumberOfElements, dfsu.X, dfsu.Y, dfsu.Z, dfsu.ElementTable);
      uvData.CreateUV();
      uvData.CreateStructuredElements(50, 100);

      // Create chart
      DHI.Chart.Map.Chart chart = new DHI.Chart.Map.Chart(1024, 1024);

      // Add overlay that plots the water levels
      FemVolGridOverlay overlay = new FemVolGridOverlay();
      overlay.SetGridData(data);
      overlay.EnableNiceValue = true;
      overlay.CreateAutoScaledRainbowPalette();
      overlay.EnableIsoline = true;
      overlay.ColoringType = MapOverlay.EColoringType.ContinuousColoring;
      overlay.SetFeathering(true, 0.5f);
      overlay.EnableIsolineLabel = true;
      overlay.SetLineColor(MapOverlay.ELineType.Isoline, Color.LawnGreen);
      //overlay.DrawElementNumbers = true;
      chart.AddOverlay(overlay);

      // Add overlay that plots the vectors
      FemVolVectorOverlay uvOverlay = new FemVolVectorOverlay();
      uvOverlay.SetVectorData(uvData);
      uvOverlay.SetScale(VectorOverlay.EScaleType.UserDefinedScale, 2000.0);
      uvOverlay.EnableAutomaticScaling = false;
      uvOverlay.VectorStyle = FemVolVectorOverlay.EVectorStyle.StructuredVectors;
      uvOverlay.LineColor = Color.BlueViolet;
      uvOverlay.SetLineThickness(VectorOverlay.EVectorType.NormalVector, 0.4f);
      chart.AddOverlay(uvOverlay);

      // Select rectangle to plot, by default the full data area
      MzRectangle wrect = new MzRectangle();
      data.GetDataArea(wrect);

      // Here you may limit the area to be plotted
      //wrect.X0 = wrect.X0 + 20000;
      //wrect.Y0 = wrect.Y0 + 20000;
      //wrect.X1 = wrect.X0 + 30000;
      //wrect.Y1 = wrect.Y0 + 30000;

      chart.SetDataArea(wrect);
      chart.SetView(wrect);

      string pngFilepath = null;
      // Loop over the first 4 time steps
      for (int i = 0; i < 4; i++)
      {
        // Set water level data
        data.SetElementValues(dfsu.ReadItemTimeStep(1, i).Data as float[]);
        // Set u-v vector data
        uvData.SetUV(dfsu.ReadItemTimeStep(2, i).Data as float[], dfsu.ReadItemTimeStep(3, i).Data as float[]);
        // When running with autoscaled palette, an update is required.
        overlay.UpdatePalette();
        // Draw to bitmap
        Bitmap bmp = chart.DrawBitmap();

        // Save bitmap to disc
        if (makeBmp)
        {
          pngFilepath = Path.Combine(UnitTestHelper.TestDataRoot, string.Format(@"test_oresundHD.1.{0}.png", i));
          bmp.Save(pngFilepath);
        }
      }

      chart.Dispose();

      if (makeBmp && pngFilepath != null) // Show last bitmap saved
        System.Diagnostics.Process.Start(pngFilepath);

    }


  }
}
