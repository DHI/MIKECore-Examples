#include "pch.h"
#include "eum.h"
#include "dfsio.h"
#include "util.h"
#include <CppUnitTest.h>

/******************************************
 * Example of how to generally read data from a dfs file, 
 * especially dfs0, dfs1 and dfs2 files.
 ******************************************/

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// a global variable so it only has to be setup once
namespace UnitestForC_MikeCore
{
  TEST_CLASS(Dfs_tests)
  {
  public:

    /// reads the dfs0 file "data_ndr_roese.dfs0"
    TEST_METHOD(ReadDfs0File)
    {
      LPCTSTR fileName = "data_ndr_roese.dfs0";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      readDfs(inputFullPath);
    }

    /** Read DFS file, going generically through all types of data in the DFS file. */
    void readDfs(LPCTSTR filename)
    {
      int rc;

      // Test that EUM library is available
      LPCTSTR baseUnitStr;
      LONG baseUnit;
      eumGetBaseUnit(1002, &baseUnit, &baseUnitStr);
      LOG("Base Unit: %s", baseUnitStr);

      // Get filename from arguments
      LOG("filename = %s", filename);

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
        LOG("Coordinate system: %s", projection_id);
        break;
      case F_UNDEFINED_GEOINFO: // Only for very old dfs files.
        LOG("Coordinate system: %s", projection_id);
        break;
      default:
        LOG("Error in projection info");
        exit(-1);
      }

      /****************************************
       * Time axis information
       ****************************************/
      TimeAxisType  time_axis_type;                // Type of time axis
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
        LOG("Time axis: Equidistant time: no_of_timesteps = %ld, tstep = %f", num_timesteps, tstep);
        break;
      case F_TM_NEQ_AXIS: // Non-equidistant time axis
        rc = dfsGetNeqTimeAxis(pdfs, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        CheckRc(rc, "Error reading Non-equidistant time axis");
        LOG("Time axis: Non-equidistant time: no_of_timesteps = %ld", num_timesteps);
        break;
      case F_CAL_EQ_AXIS:  // Equidistant calendar axis
        is_time_equidistant = true;
        rc = dfsGetEqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
        CheckRc(rc, "Error reading Equidistant calendar axis");
        LOG("Time axis: Equidistant calendar: no_of_timesteps = %ld, start = %s %s, tstep = %f", num_timesteps, start_date, start_time, tstep);
        break;
      case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
        rc = dfsGetNeqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        CheckRc(rc, "Error reading Non-equidistant calendar axis");
        LOG("Time axis: Non-equidistant calendar: no_of_timesteps = %ld, start = %s %s", num_timesteps, start_date, start_time);
        break;
      default:
        LOG("Error in time definition\n");
        exit(-1);
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
      // Dynamic item spatial axis
      SpaceAxisType item_axis_type;
      LONG          naxis_unit;                    // Axis EUM unit id
      LPCTSTR       taxis_unit;                    // Axis EUM unit string
      LONG          n_item, m_item;                // Axis dimension sizes
      float         x0, y0;                        // Axis start coordinate
      float         dx, dy;                        // Axis coordinate delta
      Coords       *C1 = NULL, *tC = NULL;         // Axis coordinates
      float        **item_timestep_dataf;          // Time step data for all items - assuming float

      // Number of dynamic items
      int num_items = dfsGetNoOfItems(pdfs);
      // Buffer arrays, used when reading data. dfsu always stores floats
      item_timestep_dataf = new float*[num_items];

      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        // Name, quantity type and unit, and datatype
        rc = dfsGetItemInfo(dfsItemD(pdfs, i_item), &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        CheckRc(rc, "Error reading dynamic item info");
        // Number of elements/values in dynamic item.
        int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, i_item));

        // Create buffer for when reading data.
        item_timestep_dataf[i_item - 1] = new float[item_num_elmts];

        /*********************************
         * Dynamic item axis
         *********************************/
        char axis_str[100];
        snprintf(axis_str, 100, "-");
        // This switch statement does not handle all item axis types.
        switch (item_axis_type = dfsGetItemAxisType(dfsItemD(pdfs, i_item)))
        {
        case F_EQ_AXIS_D0:
          // Zero dimensional axis, a point item with one value in space,.
          // Used by dfs0 files, usually time series.
          rc = dfsGetItemAxisEqD0(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(axis_str, 100, ", EQ-D0");
          break;
        case F_EQ_AXIS_D1:
          // 1D equidistant axis, containing n_item values
          // Defined by n_item, x0 and dx. 
          // Used by dfs1 files
          rc = dfsGetItemAxisEqD1(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &x0, &dx);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(axis_str, 100, ", EQ-D1, %li", n_item);
          break;
        case F_EQ_AXIS_D2:
          // 2D equidistant axis, containing n_item  x m_item values
          // Defined similar to F_EQ_AXIS_D1, just in two dimensions.
          // Used by dfs2 files
          rc = dfsGetItemAxisEqD2(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(axis_str, 100, ", EQ-D2, %li x %li", n_item, m_item);
          break;
        case F_NEQ_AXIS_D1:
          // 1D non-equidistant axis, containing n_item values.
          // Defined by a number of (x,y,z) coordinates. 
          // Used by dfs1 files, and some special dfs file types (res1d/res11)
          rc = dfsGetItemAxisNeqD1(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &C1);
          CheckRc(rc, "Error reading dynamic item axis");
          snprintf(axis_str, 100, ", NEQ-D1, %li", n_item);
          break;
        default:
          snprintf(axis_str, 100, ", Spatial axis not yet implemented: %d\n", item_axis_type);
          exit(-1);
        }

        LOG("Dynamic Item: %s, %i%s", item_name, item_num_elmts, axis_str);
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
        LOG("Static Item: %s, %i", item_name, dfsGetItemElements(staticItem));

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
          LOG(", EQ-D2, %li x %li", n_item, m_item);
          break;
        case F_EQ_AXIS_D1:
          rc = dfsGetItemAxisEqD1(staticItem, &naxis_unit, &taxis_unit, &n_item, &x0, &dx);
          CheckRc(rc, "Error reading static item axis");
          data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
          rc = dfsStaticGetData(pvec, data_topo);
          CheckRc(rc, "Error reading static item data");
          LOG(", EQ-D1, %li", n_item);
          break;
        case F_NEQ_AXIS_D1:
          rc = dfsGetItemAxisNeqD1(staticItem, &naxis_unit, &taxis_unit, &n_item, &tC);
          CheckRc(rc, "Error reading static item axis");
          data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
          rc = dfsStaticGetData(pvec, data_topo);
          CheckRc(rc, "Error reading static item data");
          LOG(", NEQ-D1, %li", n_item);
          break;
        default:
          LOG("Static item spatial axis not yet implemented: %d\n", item_axis_type);
          exit(-1);
        }
        free(data_topo);
        rc = dfsStaticDestroy(&pvec);
        CheckRc(rc, "Error destroying static item");
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
        char buff[256];
        int buffpos = 0;
        buffpos += snprintf(buff+buffpos, 256-buffpos, "time = %lf", time);
        // Print out first item value for all items
        for (int i_item = 1; i_item <= num_items; i_item++)
        {
          if (buffpos < 230)
            buffpos += snprintf(buff + buffpos, 256 - buffpos, ", %f", item_timestep_dataf[i_item - 1][0]);
        }
        LOG(buff);
        current_tstep++;
      }

      // Clean up
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete[] item_timestep_dataf[i_item - 1];
      }
      delete[] item_timestep_dataf;

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfs);
      CheckRc(rc, "Error destroying header");

    }


    /**
     * Creates a dfs0 file, with an equidistant time axis and one dynamic item.
     */
    TEST_METHOD(CreateDfs0)
    {

      LPCTSTR OutfileName = "test_dfs0_Ccreate.dfs0";
      char outputFullPath[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataPath(), OutfileName);

      LPHEAD pdfsWr; // header for writing
      LPFILE fpWr;   // filepointer for writing

      FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
      LPCTSTR fileTitle = "";
      LPCTSTR appTitle  = "";
      long appVerNo = 0;
      long num_items = 2;
      StatType statT = StatType::F_NO_STAT;
      long rc = dfsHeaderCreate(ft, fileTitle, appTitle, appVerNo, num_items, statT, &pdfsWr);

      rc = dfsSetDataType(pdfsWr, (long)0);
      rc = dfsSetDeleteValFloat(pdfsWr, 1e-35f);
      rc = dfsSetDeleteValDouble(pdfsWr, -1e-255);

      rc = dfsSetEqCalendarAxis(pdfsWr, "1993-12-02", "00:00:00", (long)1400, 0.0, 86400.0, 0);

      rc = dfsSetGeoInfoUTMProj(pdfsWr, "UTM-33", 12.438741600559766, 55.225707842436385, 326.99999999999955);

      // Set up dynamic items
      SimpleType st1 = SimpleType::UFS_FLOAT;

      LPITEM item1 = dfsItemD(pdfsWr, 1);
      rc = dfsSetItemInfo(pdfsWr, item1, 100078, "Surface Elevation [m]", 1000, st1);
      rc = dfsSetItemAxisEqD0(item1, 1000);

      LPITEM item2 = dfsItemD(pdfsWr, 2);
      rc = dfsSetItemInfo(pdfsWr, item2, 100002, "Point 1: Velocity [m/s]", 2000, st1);
      rc = dfsSetItemAxisEqD0(item2, 2000);

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
      rc = dfsHeaderDestroy(&pdfsWr);
    }

    /// Writes a copy of a dfs0 file by reading all its internal components
    TEST_METHOD(WriteDfs0FileFromSource)
    {
      LPCTSTR fileName = "data_ndr_roese.dfs0";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      LPCTSTR OutfileName = "test_data_ndr_roese_Ccreated.dfs0";
      char outputFullPath[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataPath(), OutfileName);

      CreateDfs0FromSource(inputFullPath, outputFullPath);
    }

    void CreateDfs0FromSource(LPCTSTR inputFullPath,  LPCTSTR outputFullPath)
    {
      // dfsDebugOn(true);
      LPFILE      fpIn;
      LPHEAD      pdfsIn;
      long rc = dfsFileRead(inputFullPath, &pdfsIn, &fpIn);
      long num_items = dfsGetNoOfItems(pdfsIn);

      LPHEAD pdfsWr;
      LPFILE fpWr;
      CopyDfsHeader(pdfsIn, &pdfsWr, num_items);
      long num_timesteps = CopyDfsTimeAxis(pdfsIn, pdfsWr);

      long dataTypeIn = dfsGetDataType(pdfsIn);
      dfsSetDataType(pdfsWr, dataTypeIn);

      float deleteF;
      double deleteD;
      char deleteByte;
      int deleteInt;
      unsigned int deleteUint;
      GetDfsDeleteVals(pdfsIn, &deleteF, &deleteD, &deleteByte, &deleteInt, &deleteUint);
      SetDfsDeleteVals(pdfsWr, deleteF, deleteD, deleteByte, deleteInt, deleteUint);

      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      rc = GetDfsGeoInfo(pdfsIn, &projection_id, &lon0, &lat0, &orientation);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, projection_id, lon0, lat0, orientation);

      CopyDfsCustomBlocks(pdfsIn, pdfsWr);

      CopyDfsDynamicItemInfo(pdfsIn, pdfsWr, num_items);

      rc = dfsFileCreate(outputFullPath, pdfsWr, &fpWr);

      CopyDfsStaticItems(pdfsIn, fpIn, pdfsWr, fpWr);

      // Buffer for reading dynamic item data
      // As void pointer values, since the type is not known (double vs float)
      void** item_timestep_dataf = (void**)malloc(num_items*sizeof(void*));
      for (int i_item = 1; i_item <= num_items; i_item++)
        item_timestep_dataf[i_item - 1] = malloc(dfsGetItemBytes(dfsItemD(pdfsIn, i_item)));

      CopyDfsTemporalData(pdfsIn, fpIn, pdfsWr, fpWr, item_timestep_dataf, num_timesteps, num_items);

      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      rc = dfsHeaderDestroy(&pdfsWr);
      // Close file and destroy header
      rc = dfsFileClose(pdfsIn, &fpIn);
      rc = dfsHeaderDestroy(&pdfsIn);

      for (int i_item = 1; i_item <= num_items; i_item++)
        free(item_timestep_dataf[i_item - 1]);
      free(item_timestep_dataf);
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

  };
}


