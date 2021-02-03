#include "pch.h"
#include "eum.h"
#include "dfsio.h"
#include "Util.h"
#include <CppUnitTest.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// a global variable so it only has to be setup once
extern LPCTSTR TestDataFolder;
namespace UnitestForC_MikeCore
{

  TEST_CLASS(Dfs2_tests)
  {
  public:
    ///Reads and extract data from the dfs2 file "OresundHD.dfs2"
    TEST_METHOD(ReadDfs2Test)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      // dfsDebugOn(true);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);
      LPVECTOR    pvec;
      LONG        error;
      // Read static item
      LPITEM static_item;
      pvec = dfsStaticRead(fp, &error);
      static_item = dfsItemS(pvec);
      long item_type, item_unit;
      LPCTSTR item_type_str, item_name, item_unit_str;
      SimpleType item_datatype;
      rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
      CheckRc(rc, "Error reading static item info");
      LONG          naxis_unit;                    // Axis EUM unit id
      LPCTSTR       taxis_unit;                    // Axis EUM unit string
      LONG          n_item, m_item;                // Axis dimension sizes
      float         x0, y0;                        // Axis start coordinate
      float         dx, dy;                        // Axis coordinate delta
      rc = dfsGetItemAxisEqD2(static_item, &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
      Assert::AreEqual(900.0f, dx);
      Assert::AreEqual(900.0f, dy);
      Assert::AreEqual((long)71, n_item);
      Assert::AreEqual((long)91, m_item);
      dfsStaticDestroy(&pvec);

      LPCTSTR start_date;
      long num_timesteps;
      LPCTSTR start_time;
      double tstart;
      double tstep;
      double tspan;
      long neum_unit;
      long index;
      ReadTimeAxis(pdfs, &start_date, &num_timesteps, &start_time, &tstart, &tstep, &tspan, &neum_unit, &index);
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
      auto item = dfsItemD(pdfs, 1);
      rc = dfsGetItemInfo(item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
      CheckRc(rc, "Error reading dynamic item info");
      Assert::AreEqual("H Water Depth m", item_name);
      Assert::AreEqual("meter", item_unit_str);
      Assert::AreEqual((long)1000, item_unit);
      int data_type = (int)item_datatype;
      Assert::AreEqual(1, data_type);
      double      time;
      int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, 1));
      float* item_timestep_dataf = new float[item_num_elmts];
      int ix = 3;
      int iy = 4;
      auto index34 = n_item * iy + ix;
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 1, item 1
      char buff[100];
      snprintf(buff, sizeof(buff), "1. data(3,4) = %g", item_timestep_dataf[index34]);
      Logger::WriteMessage(buff);
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 1, item 2
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 1, item 3
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 2, item 1
      snprintf(buff, sizeof(buff), "2. data(3,4) = %g", item_timestep_dataf[index34]);
      Logger::WriteMessage(buff);
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 2, item 2
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 2, item 3
      rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf); //time step 3, item 1
      snprintf(buff, sizeof(buff), "3. data(3,4) = %g", item_timestep_dataf[index34]);
      Logger::WriteMessage(buff);

      float value = item_timestep_dataf[index34]; // value for time step 3, item 1
      Assert::AreEqual(11.3634329f, value);
      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
      delete[] inputFullPath;
      delete[] item_timestep_dataf;
    }

    // Write a dfs2 file from scratch, but using a template and data from source file
    TEST_METHOD(CreateDfs2File)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      LPCTSTR OutfileName = "test_dfs2Oresund.dfs2";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      LPHEAD pdfsWr; // header for writing
      LPFILE fpWr;   // filepointer for writing


      FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
      LPCTSTR title = "";
      LPCTSTR appTitle = dfsGetAppTitle(pdfs);
      StatType statT = StatType::F_NO_STAT;
      rc = dfsHeaderCreate(ft, title, appTitle, 0, 3, statT, &pdfsWr);
      long num_items = dfsGetNoOfItems(pdfs);
      float** item_timestep_dataf = new float*[3];
      rc = dfsSetEqCalendarAxis(pdfsWr, "1993-12-02", "00:00:00", (long)1400, 0.0, 86400.0, 0);

      rc = dfsSetDataType(pdfsWr, (long)0);
      rc = dfsSetDeleteValFloat(pdfsWr, 1e-35);
      rc = dfsSetDeleteValDouble(pdfsWr, -1e-255);
      rc = dfsSetDeleteValByte(pdfsWr, 0);
      rc = dfsSetDeleteValInt(pdfsWr, 2147483647);
      rc = dfsSetDeleteValUnsignedInt(pdfsWr, 2147483647);
      rc = dfsSetDataType(pdfsWr, (long)1);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, "UTM-33", 12.438741600559766, 55.225707842436385, 326.99999999999955);

      // Set up dynamic items
      SimpleType st1 = SimpleType::UFS_FLOAT;
      LPITEM item1 = dfsItemD(pdfsWr, 1);
      int item_num_elmts = 6461;
      // Create buffer for when reading data.
      item_timestep_dataf[0] = new float[item_num_elmts];
      rc = dfsSetItemInfo(pdfsWr, item1, 100000, "H Water Depth m", 1000, st1);
      rc = dfsSetItemAxisEqD2(item1, 1000, 71, 91, 0, 0, 900, 900);
      rc = dfsSetItemAxisOrientation(item1, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item1, 0, 0.0, 0.0);

      LPITEM item2 = dfsItemD(pdfsWr, 2);

      item_timestep_dataf[1] = new float[item_num_elmts];
      rc = dfsSetItemInfo(pdfsWr, item2, (long)100080, "P Flux m^3/s/m", (long)4700, st1);
      rc = dfsSetItemAxisEqD2(item2, 1000, 71, 91, 0, 0, 900, 900);
      rc = dfsSetItemAxisOrientation(item2, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item2, 0, 0.0, 0.0);

      LPITEM item3 = dfsItemD(pdfsWr, 3);

      item_timestep_dataf[2] = new float[item_num_elmts];
      rc = dfsSetItemInfo(pdfsWr, item3, (long)100080, "Q Flux m^3/s/m", (long)4700, st1);
      rc = dfsSetItemAxisEqD2(item3, 1000, 71, 91, 0, 0, 900, 900);
      rc = dfsSetItemAxisOrientation(item3, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item3, 0, 0.0, 0.0);

      float customblock_data_ptr[] = { 327.0, 0.2, -900.0, 10.0, 0.0, 0.0, 0.0 };
      rc = dfsAddCustomBlock(pdfsWr, st1, "M21_Misc", 7, customblock_data_ptr);
      rc = dfsFileCreateEx(outputFullPath, pdfsWr, &fpWr, true);

      // Add static items containing bathymetri data, use data from source
      /***********************************
       * Geometry information
       ***********************************/
      CopyDfsStaticInfo(pdfs, fp, pdfsWr, fpWr);
      long num_timesteps = 10;
      CopyTemporalData(pdfs, fp, pdfsWr, fpWr, item_timestep_dataf, num_timesteps, num_items);
      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      dfsHeaderDestroy(&pdfsWr);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;
      delete[] inputFullPath;
      delete[] outputFullPath;
    }

    /// Writes a copy of a dfs2 file from after reading its internal components.
    TEST_METHOD(WriteDfs2FileFromSourceTest)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      LPCTSTR OutfileName = "test_OresundHD_fromSource.dfs2";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);
      CreateDfs2DFromSource(inputFullPath, outputFullPath);
      delete[] inputFullPath;
      delete[] outputFullPath;
    }


    void CreateDfs2DFromSource(LPCTSTR inputFullPath, LPCTSTR outputFullPath)
    {
      // dfsDebugOn(true);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      CreateDfs2DFromSource(outputFullPath, pdfs, fp);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
    }

    void CreateDfs2DFromSource(LPCTSTR outputFullPath, LPHEAD  pdfsIn, LPFILE fpIn)
    {
      LPHEAD pdfsWr;
      LPFILE fpWr;
      /***********************************
       * Dynamic item information
       ***********************************/
      long num_items = dfsGetNoOfItems(pdfsIn);
      float** item_timestep_dataf = new float*[num_items];
      CreateHeader(pdfsIn, &pdfsWr, num_items);
      long num_timesteps;
      CopyTimeAxis(pdfsIn, pdfsWr, &num_timesteps);

      DeleteValues delVals;
      ReadDfsDeleteVals(pdfsIn, &delVals);
      WriteDfsDeleteVals(pdfsWr, delVals);

      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      long rc = GetDfsGeoInfo(pdfsIn, &projection_id, &lon0, &lat0, &orientation);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, projection_id, lon0, lat0, orientation);

      CopyDynamicItemInfo(pdfsIn, pdfsWr, item_timestep_dataf, num_items);

      /****************************************
       * Geometry sizes - read from custom block
       ****************************************/
      CopyDfsCustomBlocks(pdfsIn, pdfsWr);

      rc = dfsFileCreateEx(outputFullPath, pdfsWr, &fpWr, true);

      /***********************************
       * Geometry information
       ***********************************/

      CopyDfsStaticInfo(pdfsIn, fpIn, pdfsWr, fpWr);

      CopyTemporalData(pdfsIn, fpIn, pdfsWr, fpWr, item_timestep_dataf, num_timesteps, num_items);

      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      dfsHeaderDestroy(&pdfsWr);
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;
    }

    void CreateHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr, long num_items)
    {
      FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
      LPCTSTR title = "";
      LPCTSTR appTitle = dfsGetAppTitle(pdfsIn);
      StatType statT = StatType::F_NO_STAT;
      long rc = dfsHeaderCreate(ft, title, appTitle, 0, num_items, statT, pdfsWr);
    }




    void CopyDfsCustomBlocks(LPHEAD pdfsIn, LPHEAD pdfsWr)
    {
      LPBLOCK customblock_ptr;
      long rc = dfsGetCustomBlockRef(pdfsIn, &customblock_ptr);
      CheckRc(rc, "Error reading custom block");
      // Search for "MIKE_FM" custom block containing int data
      while (customblock_ptr)
      {
        SimpleType csdata_type;
        LPCTSTR name;
        LONG size;
        void* customblock_data_ptr;
        rc = dfsGetCustomBlock(customblock_ptr, &csdata_type, &name,
          &size, &customblock_data_ptr, &customblock_ptr);
        CheckRc(rc, "Error reading custom block");

        rc = dfsAddCustomBlock(pdfsWr, csdata_type, name, size, customblock_data_ptr);
      }
    }
  };
}