#include "pch.h"
#include "eum.h"
#include "dfsio.h"
#include "Util.h"
#include <CppUnitTest.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// a global variable so it only has to be setup once
namespace UnitestForC_MikeCore
{

  TEST_CLASS(Dfs2_tests)
  {
  public:
    ///Reads and extract data from the dfs2 file "OresundHD.dfs2"
    TEST_METHOD(ReadDfs2Test)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      LPHEAD      pdfs;  // Header pointer
      LPFILE      fp;    // File pointer
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      TimeAxisType taxis_type;   // Type of time axis
      long num_timesteps;        // Number of time steps
      LPCTSTR start_date;        // Start date and time for the calendar axes.
      LPCTSTR start_time;        // Start date and time for the calendar axes.
      double tstart;             // Start time for the first time step in the file. Usually zero
      double tstep;              // Time step size of equidistant axes
      double tspan;              // Time span of non - equidistant axes
      long neum_unit;            // Time unit in time axis, EUM unit id
      long index;                // Index of first time step. Currently not used, always zero.
      GetDfsTimeAxis(pdfs, &taxis_type, &num_timesteps, &start_date, &start_time, &tstart, &tstep, &tspan, &neum_unit, &index);
      long expectedNbTimeSteps = 13;
      Assert::AreEqual(expectedNbTimeSteps, num_timesteps);
      Assert::AreEqual(86400.0, tstep);
      long expeumUnit = 1400;
      Assert::AreEqual(expeumUnit, neum_unit);

      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      rc = GetDfsGeoInfo(pdfs, &projection_id, &lon0, &lat0, &orientation);
      Assert::AreEqual("UTM-33", projection_id);

      long num_items = dfsGetNoOfItems(pdfs);
      Assert::AreEqual((long)3, num_items);
      // Check first item
      LPITEM item = dfsItemD(pdfs, 1);
      // Item info
      LONG          item_type;              // Item EUM type id
      LPCTSTR       item_type_str;          // Name of item type
      LPCTSTR       item_name;              // Name of item
      LONG          item_unit;              // Item EUM unit id
      LPCTSTR       item_unit_str;          // Item EUM unit string
      SimpleType    item_datatype;          // Simple type stored in item, usually float but can be double
      rc = dfsGetItemInfo(item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
      CheckRc(rc, "Error reading dynamic item info");
      Assert::AreEqual("H Water Depth m", item_name);
      Assert::AreEqual("meter", item_unit_str);
      Assert::AreEqual((long)1000, item_unit);
      Assert::AreEqual((int)UFS_FLOAT, (int)item_datatype);
      LONG    naxis_unit;                   // Axis EUM unit id
      LPCTSTR taxis_unit;                   // Axis EUM unit string
      LONG    n_item, m_item;               // Axis dimension sizes
      float   x0, y0;                       // Axis start coordinate
      float   dx, dy;                       // Axis coordinate delta
      SpaceAxisType axis_type = dfsGetItemAxisType(item);
      Assert::AreEqual((int)SpaceAxisType::F_EQ_AXIS_D2, (int)axis_type);
      rc = dfsGetItemAxisEqD2(item, &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
      Assert::AreEqual(900.0f, dx);
      Assert::AreEqual(900.0f, dy);
      Assert::AreEqual((long)71, n_item);
      Assert::AreEqual((long)91, m_item);

      // Read static data
      LPVECTOR static_data = dfsStaticRead(fp, &rc);
      // Static item info pointer
      LPITEM   static_item = dfsItemS(static_data);
      rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
      CheckRc(rc, "Error reading static item info");
      // Static item axis - must be equidistant D2 axis
      axis_type = dfsGetItemAxisType(static_item);
      Assert::AreEqual((int)SpaceAxisType::F_EQ_AXIS_D2, (int)axis_type);
      rc = dfsGetItemAxisEqD2(static_item, &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
      Assert::AreEqual(900.0f, dx);
      Assert::AreEqual(900.0f, dy);
      Assert::AreEqual((long)71, n_item);
      Assert::AreEqual((long)91, m_item);
      dfsStaticDestroy(&static_data);

      int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, 1));
      float* item_timestep_dataf = new float[item_num_elmts];
      double time;
      int ix = 3;
      int iy = 4;
      int index34 = n_item * iy + ix;
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 1, item 1
      LOG("1. data(3,4) = %g", item_timestep_dataf[index34]);
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 1, item 2
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 1, item 3
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 2, item 1
      LOG("2. data(3,4) = %g", item_timestep_dataf[index34]);
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 2, item 2
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 2, item 3
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 3, item 1
      LOG("3. data(3,4) = %g", item_timestep_dataf[index34]);

      float value = item_timestep_dataf[index34]; // value for time step 3, item 1
      Assert::AreEqual(11.3634329f, value);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
      delete[] item_timestep_dataf;
    }

    // Write a dfs2 file from scratch, but using a template and data from source file
    TEST_METHOD(CreateDfs2File)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      LPCTSTR OutfileName = "test_OresundHD_Ccreate.dfs2";
      char outputFullPath[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataPath(), OutfileName);

      LPHEAD      pdfs;// DFS header for reading
      LPFILE      fp;  // DFS file pointer for reading
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      LPHEAD pdfsWr;   // DFS header for writing
      LPFILE fpWr;     // DFS file pointer for writing

      // Create header. For equidistant time axes, use F_EQTIME_FIXEDSPACE_ALLITEMS
      FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
      LPCTSTR fileTitle = dfsGetFileTitle(pdfs);
      LPCTSTR appTitle = dfsGetAppTitle(pdfs);
      rc = dfsHeaderCreate(ft, fileTitle, appTitle, 0, 3, StatType::F_NO_STAT, &pdfsWr);

      // Datatype value of 1 is specific for a MIKE 21 HPQ result file.
      // For valid datatype values, check:
      // http://docs.mikepoweredbydhi.com/core_libraries/dfs/dfs-file-system/#dfs-data-type-tags-currently-used
      rc = dfsSetDataType(pdfsWr, 1);

      rc = dfsSetEqCalendarAxis(pdfsWr, "1993-12-02", "00:00:00", 1400, 0.0, 86400.0, 0);

      rc = dfsSetDeleteValFloat(pdfsWr, 1e-35f);
      rc = dfsSetDeleteValDouble(pdfsWr, -1e-255);

      rc = dfsSetGeoInfoUTMProj(pdfsWr, "UTM-33", 12.438741600559766, 55.225707842436385, 326.99999999999955);

      // Set up dynamic items
      SimpleType st1 = SimpleType::UFS_FLOAT;

      LPITEM item1 = dfsItemD(pdfsWr, 1);
      int item_num_elmts = 6461;
      rc = dfsSetItemInfo(pdfsWr, item1, 100000, "H Water Depth m", 1000, st1);
      rc = dfsSetItemAxisEqD2(item1, 1000, 71, 91, 0, 0, 900, 900);
      rc = dfsSetItemAxisOrientation(item1, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item1, 0, 0.0, 0.0);

      LPITEM item2 = dfsItemD(pdfsWr, 2);
      rc = dfsSetItemInfo(pdfsWr, item2, 100080, "P Flux m^3/s/m", 4700, st1);
      rc = dfsSetItemAxisEqD2(item2, 1000, 71, 91, 0, 0, 900, 900);
      rc = dfsSetItemAxisOrientation(item2, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item2, 0, 0.0, 0.0);

      LPITEM item3 = dfsItemD(pdfsWr, 3);
      rc = dfsSetItemInfo(pdfsWr, item3, 100080, "Q Flux m^3/s/m", 4700, st1);
      rc = dfsSetItemAxisEqD2(item3, 1000, 71, 91, 0, 0, 900, 900);
      rc = dfsSetItemAxisOrientation(item3, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item3, 0, 0.0, 0.0);

      // Add M21_Misc, which is specific for MIKE 21 HPQ files
      // M21_Misc : {orientation (should match projection), drying depth, -900=has projection, land value, 0, 0, 0}
      float customblock_data_ptr[] = { 327.0f, 0.2f, -900.0f, 10.0f, 0.0f, 0.0f, 0.0f };
      rc = dfsAddCustomBlock(pdfsWr, st1, "M21_Misc", 7, customblock_data_ptr);

      // Create file
      rc = dfsFileCreateEx(outputFullPath, pdfsWr, &fpWr, true);

      // Add static items containing bathymetri data, use data from source
      CopyDfsStaticItems(pdfs, fp, pdfsWr, fpWr);

      // Create buffer for when reading data.
      float** item_timestep_dataf = new float*[3];
      item_timestep_dataf[0] = new float[item_num_elmts];
      item_timestep_dataf[1] = new float[item_num_elmts];
      item_timestep_dataf[2] = new float[item_num_elmts];

      // Copy first 10 time steps
      long num_timesteps = 10;
      long num_items = dfsGetNoOfItems(pdfs);
      CopyDfsTemporalData(pdfs, fp, pdfsWr, fpWr, (void**)item_timestep_dataf, num_timesteps, num_items);

      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      rc = dfsHeaderDestroy(&pdfsWr);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      rc = dfsHeaderDestroy(&pdfs);

      // Clean up
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete[] item_timestep_dataf[i_item - 1];
      }
      delete[] item_timestep_dataf;
    }

    /// Writes a copy of a dfs2 file.
    TEST_METHOD(CreateDfs2FileFromSourceTest)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      LPCTSTR OutfileName = "test_OresundHD_CcreateFromSource.dfs2";
      char outputFullPath[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataPath(), OutfileName);
      CreateDfs2DFromSource(inputFullPath, outputFullPath);
    }


    void CreateDfs2DFromSource(LPCTSTR inputFullPath, LPCTSTR outputFullPath)
    {
      // dfsDebugOn(true);
      LPHEAD pdfsIn;
      LPFILE fpIn;
      long rc = dfsFileRead(inputFullPath, &pdfsIn, &fpIn);

      long num_items = dfsGetNoOfItems(pdfsIn);

      LPHEAD pdfsWr;
      LPFILE fpWr;
      CopyDfsHeader(pdfsIn, &pdfsWr, num_items);

      long num_timesteps = CopyDfsTimeAxis(pdfsIn, pdfsWr);

      DeleteValues delVals;
      GetDfsDeleteVals(pdfsIn, &delVals);
      SetDfsDeleteVals(pdfsWr, delVals);

      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      rc = GetDfsGeoInfo(pdfsIn, &projection_id, &lon0, &lat0, &orientation);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, projection_id, lon0, lat0, orientation);

      CopyDfsCustomBlocks(pdfsIn, pdfsWr);

      CopyDfsDynamicItemInfo(pdfsIn, pdfsWr, num_items);

      rc = dfsFileCreate(outputFullPath, pdfsWr, &fpWr);

      CopyDfsStaticItems(pdfsIn, fpIn, pdfsWr, fpWr);

      // Buffer for reading dynamic item data
      float** item_timestep_dataf = new float*[num_items];
      for (int i_item = 1; i_item <= num_items; i_item++)
        item_timestep_dataf[i_item - 1] = new float[dfsGetItemElements(dfsItemD(pdfsIn, i_item))];

      CopyDfsTemporalData(pdfsIn, fpIn, pdfsWr, fpWr, item_timestep_dataf, num_timesteps, num_items);

      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      rc = dfsHeaderDestroy(&pdfsWr);
      // Close file and destroy header
      rc = dfsFileClose(pdfsIn, &fpIn);
      rc = dfsHeaderDestroy(&pdfsIn);
      // Clean up
      for (int i_item = 1; i_item <= num_items; i_item++)
        delete[] item_timestep_dataf[i_item - 1];
      delete[] item_timestep_dataf;
    }


  };
}