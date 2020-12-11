#include "pch.h"
#include <iostream>
#include "eum.h"
#include "dfsio.h"
#include "util.h"

/******************************************
 * Example of how to read data from a dfsu file.
 ******************************************/
void readDfsu(LPCTSTR filename)
{
  LONG rc;

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

  if (dfs_data_type != 2001)
  {
    printf("This tool currently only supports standard 2D (horizontal) dfsu files\n");
    exit(-1);
  }


  /******************************************
   * Geographic information
   ******************************************/
  GeoInfoType   geo_info_type;
  double      lon0, lat0;                      // Not used for dfsu files
  double      orientation;                     // Not used for dfsu files
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
   * Time axis information - almost all dfsu files are F_CAL_EQ_AXIS
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
  LPCTSTR       item_name;                     // Name of item
  LPCTSTR       item_unit;                     // Item EUM unit string
  SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double
  float        **item_timestep_dataf;          // Time step data for all items - assuming float

  int num_items = dfsGetNoOfItems(pdfs);
  // Buffer arrays, used when reading data. dfsu always stores floats
  item_timestep_dataf = new float*[num_items];     

  for (int i_item = 1; i_item <= num_items; i_item++)
  {
    // Name, quantity type and unit, and datatype
    rc = dfsGetItemInfo_(dfsItemD(pdfs, i_item), &item_type, &item_name, &item_unit, &item_datatype);
    CheckRc(rc, "Error reading dynamic item info");
    int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, i_item));
    printf("Dynamic Item: %s, %i\n", item_name, item_num_elmts);

    // Create buffer for when reading data.
    item_timestep_dataf[i_item - 1] = new float[item_num_elmts];
  }


  /****************************************
   * Geometry sizes - read from custom block "MIKE_FM"
   ****************************************/
  int num_nodes = -1;
  int num_elmts = -1;
  int dimension = -1;
  int max_num_layers = 0;
  int num_sigma_layers = 0;
  LPBLOCK customblock_ptr;
  rc = dfsGetCustomBlockRef(pdfs, &customblock_ptr);
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

    if (0 == strcmp(name, "MIKE_FM") && csdata_type == UFS_INT)
    {
      int* intData = (int*)customblock_data_ptr;
      num_nodes      = intData[0];
      num_elmts      = intData[1];
      dimension      = intData[2];
      max_num_layers = intData[3];
      if (size < 5)
        num_sigma_layers = max_num_layers;
      else
        num_sigma_layers = intData[4];
      break;
    }
  }
  if (num_nodes < 0)
  {
    printf("Error in Geometry definition: Could not find custom block \"MIKE_FM\"\n");
    exit(-1);
  }
  if (dimension != 2 || max_num_layers > 0 || num_sigma_layers > 0)
  {
    printf("This tool currently only supports standard 2D (horizontal) dfsu files\n");
    exit(-1);
  }

  /***********************************
   * Geometry information
   ***********************************/
   // Read DFSU geometry from static items in DFSU file
  int*    node_ids       = (int*)   readStaticItem(fp, pdfs, "Node id", UFS_INT);
  double* node_x         = (double*)readStaticItem(fp, pdfs, "X-coord", UFS_DOUBLE);
  double* node_y         = (double*)readStaticItem(fp, pdfs, "Y-coord", UFS_DOUBLE);
  float*  node_z         = (float*) readStaticItem(fp, pdfs, "Z-coord", UFS_FLOAT);
  int*    node_codes     = (int*)   readStaticItem(fp, pdfs, "Code", UFS_INT);

  int*    elmt_ids       = (int*)   readStaticItem(fp, pdfs, "Element id", UFS_INT);
  int*    elmt_types     = (int*)   readStaticItem(fp, pdfs, "Element type", UFS_INT);
  int*    elmt_num_nodes = (int*)   readStaticItem(fp, pdfs, "No of nodes", UFS_INT);
  int*    elmt_conn      = (int*)   readStaticItem(fp, pdfs, "Connectivity", UFS_INT);

  /***********************************
   * Print out element polygons to file that can be plotted in gnuplot 
   ***********************************/
  // Create filename for gnuplot element output
  int fnLen = lstrlen(filename);
  char filename_gnuplot[MAX_PATH];
  strcpy(filename_gnuplot, filename);
  strcpy(&filename_gnuplot[fnLen], ".gplot.txt");

  FILE *fgp_ptr = fopen(filename_gnuplot, "w");
  fprintf(fgp_ptr, "# The content of this file can be plotted in gnuplot with one or more of the following commands:\n");
  fprintf(fgp_ptr, "# set size ratio -1\n");
  fprintf(fgp_ptr, "# plot \"filename.dfsu.gplot.txt\" with lines lc rgb 'black' title \"mesh\"\n");
  fprintf(fgp_ptr, "# \n");
  fprintf(fgp_ptr, "# The file contains coordinates for a closed polygon, for each element in the 2D dfsu file\n");
  fprintf(fgp_ptr, "# X Y\n");
  fprintf(fgp_ptr, "# \n");

  int curr_elmt_conn_index = 0;
  int nodeIndex;
  // Loop over all elements
  for (int i=0; i < num_elmts; i++)
  {
    fprintf(fgp_ptr, "# Element %6d, id = %6d\n", i+1, elmt_ids[i]);
    int num_nodes_in_elmt = elmt_num_nodes[i];
    // Loop over all nodes in element, print out node coordinate for all nodes in the element
    for (int j=0; j<num_nodes_in_elmt; j++)
    {
      // Lookup nodes of element in connectivity table (elmt_conn).
      // The elmt_conn is 1-based, so subtract 1 to get zero-based indices
      nodeIndex = elmt_conn[curr_elmt_conn_index + j] - 1;
      fprintf(fgp_ptr, "%f %f\n", node_x[nodeIndex], node_y[nodeIndex]);
    }
    // Print out the first element-node coordinate again, to close the polygon
    nodeIndex = elmt_conn[curr_elmt_conn_index] - 1;
    fprintf(fgp_ptr, "%f %f\n", node_x[nodeIndex], node_y[nodeIndex]);
    // Empty line to tell gnuplot that a new polygon is coming
    fprintf(fgp_ptr, "\n");
    // Prepare for next element
    curr_elmt_conn_index+=num_nodes_in_elmt;
  }

  fclose(fgp_ptr);


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
  delete node_ids;
  delete node_x;
  delete node_y;
  delete node_z;
  delete node_codes;
  delete elmt_ids;
  delete elmt_types;
  delete elmt_num_nodes;
  delete elmt_conn;

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
