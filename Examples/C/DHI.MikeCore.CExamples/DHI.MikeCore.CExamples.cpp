#define _AFXDLL 1

#include "pch.h"
#include <iostream>
#include "eum.h"
#include <dfsio.h>

/******************************************
 * Example of how to generally read data from a dfs file, 
 * especially dfs0, dfs1 and dfs2 files.
 ******************************************/
int main(unsigned int argc, char **argv)
{
    // Check arguments, must be a file name argument
    // TODO: Should check that the file is there 
    if (argc < 2)
    {
      printf("missing arguments\n");
      printf("Usage:  %s filename\n", argv[0]);
      exit(-1);
    }

    // Test that EUM library is available
    LPCTSTR baseUnitStr;
    LONG baseUnit;
    eumGetBaseUnit(1002, &baseUnit, &baseUnitStr);
    printf("Base Unit: %s\n", baseUnitStr);


    // Get filename from arguments
    LPCTSTR filename = LPCTSTR(argv[1]);
    printf("filename = %s\n", filename);

    // Open file for reading
    LPFILE      Fp;
    LPHEAD      pdfs;
    ufsErrors rc = (ufsErrors)dfsFileRead(filename, &pdfs, &Fp);
    if (rc != F_NO_ERROR)
    {
      // Check the error code in 
      printf("Error opening file: %i\n", rc);
      exit(-1);
    }

    // Get some general information on the file
    int appVerNo = dfsGetAppVersionNo(pdfs);
    float floatDelete = dfsGetDeleteValFloat(pdfs);
    double doubleDelete = dfsGetDeleteValDouble(pdfs);

    /******************************************
     * Geographic information
     ******************************************/
    GeoInfoType   GetGeoInfoType;
    double      lon0, lat0;                      // Origin of lower left cell, usually center of cell, in geographical lon/lat
    double      orientation;                     // Orientation of model coordinates, the rotation from true north to the model coordinate y-axis in degrees, measured positive clockwise
    LPCTSTR     projection_id;                   // Projection string, either WKT or an abbreviation
    switch (GetGeoInfoType = dfsGetGeoInfoType(pdfs))
    {
    case F_UTM_PROJECTION: // Projection is defined. It needs not be UTM!
      dfsGetGeoInfoUTMProj(pdfs, &projection_id, &lon0, &lat0, &orientation);
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
    TimeAxisType  GetTimeAxisType;
    LPCTSTR     start_date, start_time;          // Start date and time for the calendar axes.
    double      tstart = 0;                      // Start time for the first time step in the file. 
    double      tstep = 0;                       // Time step size of equidistant axes
    double      tspan = 0;                       // Time span of non-equidistant axes
    LONG        no_of_timesteps = 0;             // Number of time steps in file
    LONG        index;                           // Index of first time step. Currently not used, always zero.
    LONG        nTimeUnit;                       // Time unit in time axis, EUM unit id
    LPCTSTR     tTimeUnit;                       // Time unit in time axis, EUM unit string
    switch (GetTimeAxisType = dfsGetTimeAxisType(pdfs))
    {
    case F_TM_EQ_AXIS: // Equidistant time axis
      dfsGetEqTimeAxis(pdfs, &nTimeUnit, &tTimeUnit, &tstart, &tstep, &no_of_timesteps, &index);
      printf("Time axis: Equidistant time: no_of_timesteps = %d\n", no_of_timesteps);
      break;
    case F_TM_NEQ_AXIS: // Non-equidistant time axis
      dfsGetNeqTimeAxis(pdfs, &nTimeUnit, &tTimeUnit, &tstart, &tspan, &no_of_timesteps, &index);
      printf("Time axis: Non-equidistant time: no_of_timesteps = %d\n", no_of_timesteps);
      break;
    case F_CAL_EQ_AXIS:  // Equidistant calendar axis
      dfsGetEqCalendarAxis(pdfs, &start_date, &start_time, &nTimeUnit, &tTimeUnit, &tstart, &tstep, &no_of_timesteps, &index);
      printf("Time axis: Equidistant calendar: no_of_timesteps = %d, start = %s %s\n", no_of_timesteps, start_date, start_time);
      break;
    case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
      dfsGetNeqCalendarAxis(pdfs, &start_date, &start_time, &nTimeUnit, &tTimeUnit, &tstart, &tspan, &no_of_timesteps, &index);
      printf("Time axis: Non-equidistant calendar: no_of_timesteps = %d, start = %s %s\n", no_of_timesteps, start_date, start_time);
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
    float         x_ref, y_ref, z_ref;           // Reference coordinates. Usually zero
    SpaceAxisType GetItemAxisType;             
    LONG          nAxisUnit;                     // Axis EUM unit id
    LPCTSTR       tAxisUnit;                     // Axis EUM unit string
    LONG          n_item, m_item;                // Axis dimension sizes
    float         x0, y0;                        // Axis start coordinate
    float         dx, dy;                        // Axis coordinate delta
    Coords       *C1 = NULL, *tC = NULL;         // Axis coordinates
    float        **item_timestep_dataf;          // Time step data for all items - assuming float
    int nItem = dfsGetNoOfItems(pdfs);
    item_timestep_dataf = new float*[nItem];     // Assuming float data here. Some dfs0 files store double data, and then this must be updated
    for (int iItem = 1; iItem <= nItem; iItem++)
    {
      // Name, quantity type and unit, and datatype
      dfsGetItemInfo_(dfsItemD(pdfs, iItem), &item_type, &item_name, &item_unit, &item_datatype);
      // Item reference coordinates are usually zero.
      dfsGetItemRefCoords(dfsItemD(pdfs, iItem), &x_ref, &y_ref, &z_ref);
      printf("Dynamic Item: %s, %i", item_name, dfsGetItemElements(dfsItemD(pdfs, iItem)));
      item_timestep_dataf[iItem-1] = new float[dfsGetItemElements(dfsItemD(pdfs, iItem))];

      /*********************************
       * Dynamic item axis 
       *********************************/
      // This switch statement does not handle all item axis types.
      switch (GetItemAxisType = dfsGetItemAxisType(dfsItemD(pdfs, iItem)))
      {
      case F_EQ_AXIS_D0: 
        // Zero dimensional axis, a point item with one value in space,.
        // Used by dfs0 files, usually time series.
        dfsGetItemAxisEqD0(dfsItemD(pdfs, iItem), &nAxisUnit, &tAxisUnit);
        printf(", EQ-D0");
        break;
      case F_EQ_AXIS_D1:
        // 1D equidistant axis, containing n_item values
        // Defined by n_item, x0 and dx. 
        // Used by dfs1 files
        dfsGetItemAxisEqD1(dfsItemD(pdfs, iItem), &nAxisUnit, &tAxisUnit, &n_item, &x0, &dx);
        printf(", EQ-D1, %i", n_item);
        break;
      case F_EQ_AXIS_D2:
        // 2D equidistant axis, containing n_item  x m_item values
        // Defined similar to F_EQ_AXIS_D1, just in two dimensions.
        // Used by dfs2 files
        dfsGetItemAxisEqD2(dfsItemD(pdfs, iItem), &nAxisUnit, &tAxisUnit, &n_item, &m_item, &x0, &y0, &dx, &dy);
        printf(", EQ-D2, %i x %i", n_item, m_item);
        break;
      case F_NEQ_AXIS_D1:
        // 1D non-equidistant axis, containing n_item values.
        // Defined by a number of (x,y,z) coordinates. 
        // Used by dfs1 files, and some special dfs file types (res1d/res11)
        dfsGetItemAxisNeqD1(dfsItemD(pdfs, iItem), &nAxisUnit, &tAxisUnit, &n_item, &C1);
        printf(", NEQ-D1, %i", n_item);
        break;
      default:
        printf(", Spatial axis not yet implemented: %s\n", GetItemAxisType);
        exit(-1);
      }
      printf("\n", item_name);
    }

    /********************************
     * Static items
     ********************************/
    LPVECTOR    pvec;
    LONG        error;
    // Loop over all static items
    while (pvec = dfsStaticRead(Fp, &error))
    {
      LPITEM staticItem;
      staticItem = dfsItemS(pvec);
      dfsGetItemInfo_(staticItem, &item_type, &item_name, &item_unit, &item_datatype);
      printf("Static Item: %s, %i", item_name, dfsGetItemElements(staticItem));

      float* data_topo = NULL;
      SpaceAxisType GetStaticAxisType;
      // A static item can have all the same axes as the dynamic item can.
      // For most files using static items (dfs2, dfs3) the static items are of same type a
      // the dynamic item.
      switch (GetStaticAxisType = dfsGetItemAxisType(staticItem))
      {
      case F_EQ_AXIS_D2:
        dfsGetItemAxisEqD2(staticItem, &nAxisUnit, &tAxisUnit, &n_item, &m_item, &x0, &y0, &dx, &dy);
        data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
        dfsStaticGetData(pvec, data_topo);
        printf(", EQ-D2, %i x %i", n_item, m_item);
        break;
      case F_EQ_AXIS_D1:
        dfsGetItemAxisEqD1(staticItem, &nAxisUnit, &tAxisUnit, &n_item, &x0, &dx);
        data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
        dfsStaticGetData(pvec, data_topo);
        printf(", EQ-D1, %i", n_item);
        break;
      case F_NEQ_AXIS_D1:
        dfsGetItemAxisNeqD1(staticItem, &nAxisUnit, &tAxisUnit, &n_item, &tC);
        data_topo = (float*)malloc(dfsGetItemBytes(staticItem));
        dfsStaticGetData(pvec, data_topo);
        printf(", NEQ-D1, %i", n_item);
        break;
      default:
        printf("Static item spatial axis not yet implemented: %s\n", GetItemAxisType);
        exit(-1);
      }
      free(data_topo);
      dfsStaticDestroy(&pvec);
      printf("\n", item_name);
    }


    /*****************************
     * Time loop
     *****************************/
    long current_tstep = 0;
    double      time;
    // Loop over all time steps
    while (current_tstep < no_of_timesteps)
    {
      // Loop over all items
      for (int iItem = 1; iItem <= nItem; iItem++)
      {
        dfsReadItemTimeStep(pdfs, Fp, &time, item_timestep_dataf[iItem-1]);
      }
      // Print out time of time step, relative to start time and in time unit of axis
      // If temporal axis is non-equidistant, this is the only way to get the time of the time step.
      // For the equidistant temporal axes, the timestep times can be derived from the temporal axes data.
      printf("time = %lf", time);
      // Print out first item value for all items
      for (int iItem = 1; iItem <= nItem; iItem++)
      {
        printf(", %lf", item_timestep_dataf[iItem-1][0]);
      }
      printf("\n", time);
      current_tstep++;
    }

    // Clean up
    for (int iItem = 1; iItem <= nItem; iItem++)
    {
      delete item_timestep_dataf[iItem - 1];
    }
    delete item_timestep_dataf;

    // Close file and destroy header
    dfsFileClose(pdfs, &Fp);
    dfsHeaderDestroy(&pdfs);

}
