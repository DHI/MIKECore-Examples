#include "pch.h"
#include <iostream>
#include "eum.h"
#include "dfsio.h"
#include "util.h"

/******************************************
 * Example of how to generally read data from a dfs file, 
 * especially dfs0, dfs1 and dfs2 files.
 ******************************************/
void readDfs(LPCTSTR filename)
{
  int rc;

  // Test that EUM library is available
  LPCTSTR baseUnitStr;
  LONG baseUnit;
  eumGetBaseUnit(1002, &baseUnit, &baseUnitStr);
  printf("Base Unit: %s\n", baseUnitStr);

  // Get filename from arguments
  printf("filename = %s\n", filename);

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
    printf("Coordinate system: %s\n", projection_id);
    break;
  case F_UNDEFINED_GEOINFO: // Only for very old dfs files.
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
    printf("Time axis: Equidistant time: no_of_timesteps = %ld, tstep = %f\n", num_timesteps, tstep);
    break;
  case F_TM_NEQ_AXIS: // Non-equidistant time axis
    rc = dfsGetNeqTimeAxis(pdfs, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
    CheckRc(rc, "Error reading Non-equidistant time axis");
    printf("Time axis: Non-equidistant time: no_of_timesteps = %ld\n", num_timesteps);
    break;
  case F_CAL_EQ_AXIS:  // Equidistant calendar axis
    is_time_equidistant = true;
    rc = dfsGetEqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
    CheckRc(rc, "Error reading Equidistant calendar axis");
    printf("Time axis: Equidistant calendar: no_of_timesteps = %ld, start = %s %s, tstep = %f\n", num_timesteps, start_date, start_time, tstep);
    break;
  case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
    rc = dfsGetNeqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
    CheckRc(rc, "Error reading Non-equidistant calendar axis");
    printf("Time axis: Non-equidistant calendar: no_of_timesteps = %ld, start = %s %s\n", num_timesteps, start_date, start_time);
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
    printf("Dynamic Item: %s, %i", item_name, item_num_elmts);

    // Create buffer for when reading data.
    item_timestep_dataf[i_item - 1] = new float[item_num_elmts];

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
      printf(", EQ-D0");
      break;
    case F_EQ_AXIS_D1:
      // 1D equidistant axis, containing n_item values
      // Defined by n_item, x0 and dx. 
      // Used by dfs1 files
      rc = dfsGetItemAxisEqD1(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &x0, &dx);
      CheckRc(rc, "Error reading dynamic item axis");
      printf(", EQ-D1, %li", n_item);
      break;
    case F_EQ_AXIS_D2:
      // 2D equidistant axis, containing n_item  x m_item values
      // Defined similar to F_EQ_AXIS_D1, just in two dimensions.
      // Used by dfs2 files
      rc = dfsGetItemAxisEqD2(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
      CheckRc(rc, "Error reading dynamic item axis");
      printf(", EQ-D2, %li x %li", n_item, m_item);
      break;
    case F_NEQ_AXIS_D1:
      // 1D non-equidistant axis, containing n_item values.
      // Defined by a number of (x,y,z) coordinates. 
      // Used by dfs1 files, and some special dfs file types (res1d/res11)
      rc = dfsGetItemAxisNeqD1(dfsItemD(pdfs, i_item), &naxis_unit, &taxis_unit, &n_item, &C1);
      CheckRc(rc, "Error reading dynamic item axis");
      printf(", NEQ-D1, %li", n_item);
      break;
    default:
      printf(", Spatial axis not yet implemented: %d\n", item_axis_type);
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
