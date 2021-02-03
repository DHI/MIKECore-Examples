#include "pch.h"
#include <iostream>
#include "eum.h"
#include "dfsio.h"
#include "util.h"
#include <CppUnitTest.h>

/******************************************
 * Example of how to generally read data from a dfs file, 
 * especially dfs0, dfs1 and dfs2 files.
 ******************************************/
#include <CppUnitTest.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// a global variable so it only has to be setup once
extern LPCTSTR TestDataFolder;
namespace UnitestForC_MikeCore
{
  TEST_CLASS(Dfs01_tests)
  {
  public:
    /// reads the dfs0 file "data_ndr_roese.dfs0"

    TEST_METHOD(ReadDfs0File)
    {
      LPCTSTR fileName = "data_ndr_roese.dfs0";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      readDfs(inputFullPath);
      Assert::AreEqual(1, 1);
      delete[] inputFullPath;
    }

    /// <summary>
    /// Creates a dfs0 file, with an equidistant time axis and one dynamic item.
    /// <para>
    /// It uses the generic <see cref="DfsBuilder"/>, since currently no specialized 
    /// builder exists for the dfs0 files.
    /// </para>
    /// </summary>
    /// <param name="filename">Name of new file</param>
    /// <param name="calendarAxis">boolean specifying whether the temporal axis should be a calendar axis or a time axis</param>
    TEST_METHOD(CreateDfs0)
    {

      LPCTSTR OutfileName = "test_dfs0_created.dfs0";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);

      LPHEAD pdfsWr; // header for writing
      LPFILE fpWr;   // filepointer for writing


      FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
      LPCTSTR title = "";
      LPCTSTR appTitle = "";
      StatType statT = StatType::F_NO_STAT;
      long num_items = 2;
      long rc = dfsHeaderCreate(ft, title, appTitle, 0, num_items, statT, &pdfsWr);

      rc = dfsSetEqCalendarAxis(pdfsWr, "1993-12-02", "00:00:00", (long)1400, 0.0, 86400.0, 0);

      rc = dfsSetDeleteValFloat(pdfsWr, 1e-35f);
      rc = dfsSetDeleteValDouble(pdfsWr, -1e-255);
      rc = dfsSetDeleteValByte(pdfsWr, 0);
      rc = dfsSetDeleteValInt(pdfsWr, 2147483647);
      rc = dfsSetDeleteValUnsignedInt(pdfsWr, 2147483647);

      rc = dfsSetDataType(pdfsWr, (long)0);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, "UTM-33", 12.438741600559766, 55.225707842436385, 326.99999999999955);

      // Set up dynamic items
      SimpleType st1 = SimpleType::UFS_FLOAT;
      LPITEM item1 = dfsItemD(pdfsWr, 1);

      rc = dfsSetItemInfo(pdfsWr, item1, 100078, "Surface Elevation [m]", 1000, st1);
      rc = dfsSetItemAxisEqD0(item1, 1000);
      rc = dfsSetItemAxisOrientation(item1, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item1, 0, 0.0, 0.0);

      LPITEM item2 = dfsItemD(pdfsWr, 2);

      rc = dfsSetItemInfo(pdfsWr, item2, 100002, "Point 1: Velocity [m/s]", 2000, st1);
      rc = dfsSetItemAxisEqD0(item2, 2000);
      rc = dfsSetItemAxisOrientation(item2, 0.0, 0.0, 0.0);
      rc = dfsSetItemRefCoords(item2, 0, 0.0, 0.0);

      float customblock_data_ptr[] = {-1e-30, -1e-30, -1e-30, -1e-30, -1e-30, -1e-30, -1e-30};
      rc = dfsAddCustomBlock(pdfsWr, st1, "M21_Misc", 7, customblock_data_ptr);
      rc = dfsFileCreateEx(outputFullPath, pdfsWr, &fpWr, true);

      // No need to add static items containing bathymetri data, use data from source

      rc = dfsWriteItemTimeStep(pdfsWr, fpWr, 0, new float[1] {   0 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {   0 });  // water level
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 100 });  // water depth
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {   1 });  // water level
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 101 });  // water depth
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {   2 });  // water level
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 102 });  // water depth
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {   3 });  // etc...
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 103 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {   4 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 104 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {   5 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 105 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {  10 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 110 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {  11 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 111 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {  12 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 112 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] {  13 });
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr,0, new float[1] { 113 });

      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      dfsHeaderDestroy(&pdfsWr);
      delete[] outputFullPath;
    }

    /// Writes a copy of a dfs0 file by reading all its internal components
    TEST_METHOD(WriteDfs0FileFromSource)
    {
      LPCTSTR fileName = "data_ndr_roese.dfs0";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      LPCTSTR OutfileName = "test_data_ndr_roese_created.dfs0";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);
      CreateDfs0FromSource(inputFullPath, outputFullPath);
      delete[] inputFullPath;
      delete[] outputFullPath;
    }

    void CreateDfs0FromSource(LPCTSTR inputFullPath,  LPCTSTR outputFullPath)
    {
      // dfsDebugOn(true);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      CreateDfs0DFromSource(outputFullPath, pdfs, fp);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
    }

    void CreateDfs0DFromSource(LPCTSTR outputFullPath, LPHEAD  pdfsIn, LPFILE fpIn)
    {
      long rc;
      LPHEAD pdfsWr;
      LPFILE fpWr;

      long num_items = dfsGetNoOfItems(pdfsIn);
      float** item_timestep_dataf = new float*[num_items];
      CopyHeader(pdfsIn, &pdfsWr, num_items);
      long num_timesteps;
      CopyTimeAxis(pdfsIn, pdfsWr, &num_timesteps);

      float deleteF;
      double deleteD;
      char deleteByte;
      int deleteInt;
      unsigned int deleteUint;
      GetDfsDeleteVals(pdfsIn, &deleteF, &deleteD, &deleteByte, &deleteInt, &deleteUint);
      SetDfsDeleteVals(pdfsWr, deleteF, deleteD, deleteByte, deleteInt, deleteUint);
      dataTypeIn = dfsGetDataType(pdfsIn);
      dfsSetDataType(pdfsWr, dataTypeIn);
      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      GetDfsGeoInfo(pdfsIn, &projection_id, &lon0, &lat0, &orientation);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, projection_id, lon0, lat0, orientation);

      /***********************************
       * Dynamic item information
       ***********************************/

      CopyDynamicItemInfo(pdfsIn, pdfsWr, item_timestep_dataf, num_items);

      /****************************************
       * Geometry sizes - read from custom block "MIKE_FM"
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



    void GetDfsDeleteVals(LPHEAD pdfsIn, float* deleteF, double* deleteD, char* deleteByte, int* deleteInt, unsigned int* deleteUint)
    {
      *deleteF = dfsGetDeleteValFloat(pdfsIn);
      *deleteD = dfsGetDeleteValDouble(pdfsIn);
      *deleteByte = dfsGetDeleteValByte(pdfsIn);
      *deleteInt = dfsGetDeleteValInt(pdfsIn);
      *deleteUint = dfsGetDeleteValUnsignedInt(pdfsIn);
    }
    void SetDfsDeleteVals(LPHEAD pdfsWr, float deleteF, double deleteD, char deleteByte, int deleteInt, unsigned int deleteUint)
    {
      long rc = dfsSetDeleteValFloat(pdfsWr, deleteF);
      rc = dfsSetDeleteValDouble(pdfsWr, deleteD);
      rc = dfsSetDeleteValByte(pdfsWr, deleteByte);
      rc = dfsSetDeleteValInt(pdfsWr, deleteInt);
      rc = dfsSetDeleteValUnsignedInt(pdfsWr, deleteUint);
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


    void readDfs(LPCTSTR filename)
    {
      int rc;

      // Test that EUM library is available
      LPCTSTR baseUnitStr;
      LONG baseUnit;
      eumGetBaseUnit(1002, &baseUnit, &baseUnitStr);
      char buff[100];
      snprintf(buff, sizeof(buff), "Base Unit: %s\n", baseUnitStr);
      Logger::WriteMessage(buff); //unit test output
      printf(buff); // std output

      // Get filename from arguments
      snprintf(buff, sizeof(buff), "filename = %s\n", filename);
      Logger::WriteMessage(buff); //test output
      printf(buff); // std output

      // Open file for reading
      LPFILE      fp;
      LPHEAD      pdfs;
      rc = dfsFileRead(filename, &pdfs, &fp);
      CheckRc(rc, "Error opening file");

      // Get some general information on the file
      int app_ver_no = dfsGetAppVersionNo(pdfs);
      float float_delete = dfsGetDeleteValFloat(pdfs);
      double double_delete = dfsGetDeleteValDouble(pdfs);
      int dfs_data_type = dfsGetDataType(pdfs);

      /******************************************
       * Geographic information
       ******************************************/
      GeoInfoType   geo_info_type;
      double      lon0, lat0;                      // Origin of lower left cell, usually center of cell, in geographical lon/lat
      double      orientation;                     // Orientation of model coordinates, the rotation from true north to the model coordinate y-axis in degrees, measured positive clockwise
      LPCTSTR     projection_id;                   // Projection string, either WKT or an abbreviation
      switch (geo_info_type = dfsGetGeoInfoType(pdfs))
      {
      case F_UTM_PROJECTION: // Projection is defined. It needs not be UTM!
        rc = dfsGetGeoInfoUTMProj(pdfs, &projection_id, &lon0, &lat0, &orientation);
        CheckRc(rc, "Error reading projection");
        snprintf(buff, sizeof(buff), "Coordinate system: %s\n", projection_id);
        Logger::WriteMessage(buff);
        printf(buff);
        break;
      case F_UNDEFINED_GEOINFO: // Only for very old dfs files.
        snprintf(buff, sizeof(buff), "Coordinate system: %s\n", projection_id);
        Logger::WriteMessage(buff);
        printf("Coordinate system: Undefined\n");
        break;
      default:
        printf("Error in projection info\n");
        exit(-1);
      }


      /****************************************
       * Time axis information
       ****************************************/
      TimeAxisType  time_axis_type;
      LPCTSTR     start_date, start_time;          // Start date and time for the calendar axes.
      double      tstart = 0;                      // Start time for the first time step in the file. 
      double      tstep = 0;                       // Time step size of equidistant axes
      double      tspan = 0;                       // Time span of non-equidistant axes
      LONG        num_timesteps = 0;               // Number of time steps in file
      LONG        index;                           // Index of first time step. Currently not used, always zero.
      LONG        ntime_unit;                      // Time unit in time axis, EUM unit id
      LPCTSTR     ttime_Unit;                      // Time unit in time axis, EUM unit string
      BOOL        is_time_equidistant = false;
      switch (time_axis_type = dfsGetTimeAxisType(pdfs))
      {
      case F_TM_EQ_AXIS: // Equidistant time axis
        is_time_equidistant = true;
        rc = dfsGetEqTimeAxis(pdfs, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
        CheckRc(rc, "Error reading Equidistant time axis");
        snprintf(buff, sizeof(buff), "Time axis: Equidistant time: no_of_timesteps = %ld, tstep = %f\n", num_timesteps, tstep);
        Logger::WriteMessage(buff);
        printf(buff);
        break;
      case F_TM_NEQ_AXIS: // Non-equidistant time axis
        rc = dfsGetNeqTimeAxis(pdfs, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        CheckRc(rc, "Error reading Non-equidistant time axis");
        snprintf(buff, sizeof(buff), "Time axis: Non-equidistant time: no_of_timesteps = %ld\n", num_timesteps);
        Logger::WriteMessage(buff);
        printf(buff);
        break;
      case F_CAL_EQ_AXIS:  // Equidistant calendar axis
        is_time_equidistant = true;
        rc = dfsGetEqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
        CheckRc(rc, "Error reading Equidistant calendar axis");
        snprintf(buff, sizeof(buff), "Time axis: Equidistant calendar: no_of_timesteps = %ld, start = %s %s, tstep = %f\n", num_timesteps, start_date, start_time, tstep);
        Logger::WriteMessage(buff);
        printf(buff);
        break;
      case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
        rc = dfsGetNeqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        CheckRc(rc, "Error reading Non-equidistant calendar axis");
        snprintf(buff, sizeof(buff), "Time axis: Non-equidistant calendar: no_of_timesteps = %ld, start = %s %s\n", num_timesteps, start_date, start_time);
        Logger::WriteMessage(buff);
        printf(buff);
        break;
      default:
        printf("Error in time definition\n");
        exit(-1);
        break;
      }


      /***********************************
       * Dynamic item information
       ***********************************/
      LONG          item_type;                     // Item EUM type id
      LPCTSTR       item_type_str;                 // Name of item type
      LPCTSTR       item_name;                     // Name of item
      LONG          item_unit;                     // Item EUM unit id
      LPCTSTR       item_unit_str;                 // Item EUM unit string
      SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double
      SpaceAxisType item_axis_type;
      LONG          naxis_unit;                    // Axis EUM unit id
      LPCTSTR       taxis_unit;                    // Axis EUM unit string
      LONG          n_item, m_item;                // Axis dimension sizes
      float         x0, y0;                        // Axis start coordinate
      float         dx, dy;                        // Axis coordinate delta
      Coords       *C1 = NULL, *tC = NULL;         // Axis coordinates
      float        **item_timestep_dataf;          // Time step data for all items - assuming float

      int num_items = dfsGetNoOfItems(pdfs);
      // Buffer arrays, used when reading data. dfsu always stores floats
      item_timestep_dataf = new float*[num_items];

      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        // Name, quantity type and unit, and datatype
        rc = dfsGetItemInfo(dfsItemD(pdfs, i_item), &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        CheckRc(rc, "Error reading dynamic item info");
        int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, i_item));
        snprintf(buff, sizeof(buff), "Dynamic Item: %s, %i", item_name, item_num_elmts);
        Logger::WriteMessage(buff);
        printf(buff);

        // Create buffer for when reading data.
        item_timestep_dataf[i_item - 1] =  new float[item_num_elmts];

        /*********************************
         * Dynamic item axis
         *********************************/
         // This switch statement does not handle all item axis types.
        switch (item_axis_type = dfsGetItemAxisType(dfsItemD(pdfs, i_item)))
        {
        case F_EQ_AXIS_D0:
          // Zero dimensional axis, a point item with one value in space,.
          // Used by dfs0 files, usually time series.
          rc = dfsGetItemAxisEqD0(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit);
          CheckRc(rc, "Error reading dynamic item axis");
          Logger::WriteMessage(", EQ-D0");
          printf(", EQ-D0");
          break;
        case F_EQ_AXIS_D1:
          // 1D equidistant axis, containing n_item values
          // Defined by n_item, x0 and dx. 
          // Used by dfs1 files
          rc = dfsGetItemAxisEqD1(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &x0, &dx);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(buff, sizeof(buff), ", EQ-D1, %li", n_item);
          Logger::WriteMessage(buff);
          printf(buff);
          break;
        case F_EQ_AXIS_D2:
          // 2D equidistant axis, containing n_item  x m_item values
          // Defined similar to F_EQ_AXIS_D1, just in two dimensions.
          // Used by dfs2 files
          rc = dfsGetItemAxisEqD2(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(buff, sizeof(buff), ", EQ-D2, %li x %li", n_item, m_item);
          Logger::WriteMessage(buff);
          printf(buff);
          break;
        case F_NEQ_AXIS_D1:
          // 1D non-equidistant axis, containing n_item values.
          // Defined by a number of (x,y,z) coordinates. 
          // Used by dfs1 files, and some special dfs file types (res1d/res11)
          rc = dfsGetItemAxisNeqD1(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &C1);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(buff, sizeof(buff), ", NEQ-D1, %li", n_item);
          Logger::WriteMessage(buff);
          printf(buff);
          break;
        default:
          snprintf(buff, sizeof(buff), ", Spatial axis not yet implemented: %d\n", item_axis_type);
          Logger::WriteMessage(buff);
          printf(buff);
          exit(-1);
        }
        printf("\n");
      }

      /********************************
       * Static items
       ********************************/
      LPVECTOR    pvec;
      LONG        error;
      // Loop over all static items
      while (pvec = dfsStaticRead(fp, &error))
      {
        LPITEM staticItem;
        staticItem = dfsItemS(pvec);
        rc = dfsGetItemInfo(staticItem, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        CheckRc(rc, "Error reading static item info");
        printf("Static Item: %s, %i", item_name, dfsGetItemElements(staticItem));

        float* data_topo = NULL;
        SpaceAxisType static_axis_type;
        // A static item can have all the same axes as the dynamic item can.
        // For most files using static items (dfs2, dfs3) the static items are of same type a
        // the dynamic item.
        switch (static_axis_type = dfsGetItemAxisType(staticItem))
        {
        case F_EQ_AXIS_D2:
          rc = dfsGetItemAxisEqD2(staticItem, &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
          CheckRc(rc, "Error reading static item axis");
          data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
          rc = dfsStaticGetData(pvec, data_topo);
          CheckRc(rc, "Error reading static item data");
          printf(", EQ-D2, %li x %li", n_item, m_item);
          break;
        case F_EQ_AXIS_D1:
          rc = dfsGetItemAxisEqD1(staticItem, &naxis_unit, &taxis_unit, &n_item, &x0, &dx);
          CheckRc(rc, "Error reading static item axis");
          data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
          rc = dfsStaticGetData(pvec, data_topo);
          CheckRc(rc, "Error reading static item data");
          printf(", EQ-D1, %li", n_item);
          break;
        case F_NEQ_AXIS_D1:
          rc = dfsGetItemAxisNeqD1(staticItem, &naxis_unit, &taxis_unit, &n_item, &tC);
          CheckRc(rc, "Error reading static item axis");
          data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
          rc = dfsStaticGetData(pvec, data_topo);
          CheckRc(rc, "Error reading static item data");
          printf(", NEQ-D1, %li", n_item);
          break;
        default:
          printf("Static item spatial axis not yet implemented: %d\n", item_axis_type);
          exit(-1);
        }
        free(data_topo);
        rc = dfsStaticDestroy(&pvec);
        CheckRc(rc, "Error destroying static item");
        printf("\n");
      }


      /*****************************
       * Time loop
       *****************************/
       // Data are stored in the file in item major order
       // i.e. for each time step, all items are stored in order.
       // To read specific time steps or items, you reposition the file pointer using:
       //   dfsFindTimeStep(pdfs, fp, timestepIndex);
       //   dfsFindItemDynamic(pdfs, fp, timestepIndex, itemNumber);
       // The first will position the file pointer at the first item of that timestep
       // The second will position the file pointer at the specified timestep and item
      long current_tstep = 0;
      double      time;
      // Loop over the first 10 time steps
      int tstep_end = num_timesteps > 10 ? 10 : num_timesteps;
      while (current_tstep < tstep_end)
      {
        // Loop over all items
        for (int i_item = 1; i_item <= num_items; i_item++)
        {
          // Read item-timestep where the file pointer points to,
          // and move the filepointer to the next item-timestep
          rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf[i_item - 1]);
          CheckRc(rc, "Error reading dynamic item data");
          // If the temporal axis is equidistant, the time variable is the timestep index value.
          // If temporal axis is non-equidistant, this is the time from start of the file
          if (is_time_equidistant)
            time *= tstep;
        }

        // Print out time of time step, relative to start time and in time unit of axis
        printf("time = %lf", time);
        // Print out first item value for all items
        for (int i_item = 1; i_item <= num_items; i_item++)
        {
          printf(", %f", item_timestep_dataf[i_item - 1][0]);
        }
        printf("\n");
        current_tstep++;
      }

      // Clean up
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfs);
      CheckRc(rc, "Error destroying header");

    }

    /// file deleteValues
    long           dataTypeIn;
    float          deleteF;
    double         deleteD;
    char           deleteByte;
    int            deleteInt;
    unsigned int   deleteUint;
  };
}


