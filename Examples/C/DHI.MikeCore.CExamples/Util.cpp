#include "pch.h"
#include "eum.h"
#include <dfsio.h>
#include <CppUnitTestAssert.h>
#include "Util.h"

#include <CppUnitTestLogger.h>


void CheckRc(LONG rc, LPCTSTR errMsg)
{
  if (rc != F_NO_ERROR)
  {
    LOG("ERROR: %s (%li - %s)\n", errMsg, rc, GetRCString(rc));
    exit(-1);
  }
}

const char* GetRCString(LONG rc)
{
  switch (rc)
  {
  case F_NO_ERROR:
    return ("");
  case F_END_OF_FILE:
    return ("End of file was reached");
  case F_FAIL_DATA:
    return ("Failed reading/writing data from/to the file. File is corrupt, not a DFS file or handled incorrectly, or data is invalid");
  case F_FAIL_ILLEGEAL_TSTEP:
    return ("The time step number is out of range");
  case F_FAIL_ILLEGEAL_ITEM:
    return ("The item number is out of range");
  case F_ERR_MALLOC:
    return ("Error allocating memory");
  case F_ERR_READ:
    return ("Error reading file. Common reasons: File has zero size, file is open in write-only mode, disc is corrupt");
  case F_ERR_WRITE:
    return ("Error writing data to disc. Common reasons: Disc is full, filename is invalid, not enough available memory (for write buffers)");
  case F_ERR_OPEN:
    return ("Error opening file. Filename is invalid, or header could not be read (corrupt, or not a DFS file)");
  case F_ERR_CLOSE:
    return ("Error closing file");
  case F_ERR_FLUSH:
    return ("Error flushing data to disc. Disc/quota may be full");
  case F_ERR_SEEK:
    return ("Error seeking in file. File has been truncated or disc is corrupt");
  case F_ERR_ITEMNO:
    return ("An item number is out of range");
  case F_ERR_INDEX:
    return ("An index number is out of range");
  case F_ERR_DTYPE:
    return ("A data type does not match (internal error). File is most likely corrupt");
  case F_ERR_DATA:
    return ("Error in file data, file is most likely corrupt");
  case F_ERR_DATE_FORMAT:
    return ("Date format is invalid. Must be YYYY-MM-dd");
  case F_ERR_TIME_FORMAT:
    return ("Time format is invalid. Must be hh:mm:ss");
  case F_ERR_SIZE:
    return ("A size does not match (internal error). File is most likely corrupt");
  case F_ERR_TAG:
    return ("Error reading DHI DFS tag (DHI_). Most likely file is not a DFS file");
  case F_ERR_READONLY:
    return ("Trying to write to a file in read-only mode");
  case F_ERR_SKIP:
    return (" Error skipping a logical block (internal error). Most likely file is corrupt");
  case F_ERR_APPTAG:
    return (" Error reading DHI DFS API tag (DFS_). Most likely file is not a DFS file");
  case F_ERR_AXIS:
    return ("Wrong axis type number (internal error). Most likely the file is corrupt");
  case F_ERR_CTYPE:
    return ("Error reading logical block type (internal error). Most likely the file is corrupt");
  case F_ERR_EUM:
    return ("EUM unit and type does not match");
  case F_ERR_NOT_DTX:
    return ("File is not a dtx file, though loaded as such");
  case F_ERR_PLUGIN:
    return ("Plugin extension error");
  default:
    return ("Unknown error");
  }
}

/**
 * Convert float array to double array.
 * It is malloc'ed, so must be free'd again
 */
double* ConvertFloat2Double(float* flt, int size)
{
  double* dbl = (double*)malloc(size * sizeof(double));
  for (int i = 0; i < size; i++)
    dbl[i] = flt[i];
  return dbl;
}

/** Get geographical/projection string */
long GetDfsGeoInfoProjString(LPHEAD pdfs, LPCTSTR* projection_id)
{
  long rc = F_NO_ERROR;
  GeoInfoType   geo_info_type = dfsGetGeoInfoType(pdfs);
  double        lon0, lat0;  // Not used for dfsu files
  double        orientation; // Not used for dfsu files
  if (geo_info_type == F_UTM_PROJECTION)
  {
    rc = dfsGetGeoInfoUTMProj(pdfs, projection_id, &lon0, &lat0, &orientation);
  }
  else if (geo_info_type != F_UNDEFINED_GEOINFO)
  {
    projection_id = NULL;
    rc = -1;
  }
  return rc;
}

/** Get geographical/projection info, assuming file has projection defined */
long GetDfsGeoInfo(LPHEAD pdfsIn, LPCTSTR* projection_id, double* lon0, double* lat0, double* orientation)
{
  long rc;
  GeoInfoType   geo_info_type = dfsGetGeoInfoType(pdfsIn);
  if (geo_info_type == F_UTM_PROJECTION)
  {
    rc = dfsGetGeoInfoUTMProj(pdfsIn, projection_id, lon0, lat0, orientation);
  }
  else if (geo_info_type != F_UNDEFINED_GEOINFO)
    rc = -1;
  return rc;
}

/**
 * Method handling all types of time axes. Only relevant out parameters are set.
 */
void GetDfsTimeAxis(
  LPHEAD pdfsIn,                ///< [in]  Pointer to dfs header
  TimeAxisType* taxis_type,     ///< [out] Type of time axis
  long*         num_timesteps,  ///< [out] Number of time steps
  LPCTSTR*      start_date,     ///< [out] Start date and time for the calendar axes.
  LPCTSTR*      start_time,     ///< [out] Start date and time for the calendar axes.
  double*       tstart,         ///< [out] Start time for the first time step in the file. Usually zero
  double*       tstep,          ///< [out] Time step size of equidistant axes
  double*       tspan,          ///< [out] Time span of non-equidistant axes
  long*         neum_unit,      ///< [out] Time unit in time axis, EUM unit id
  long*         index           ///< [out] Index of first time step. Currently not used, always zero.
)
{
  long rc;
  LPCTSTR      teum_Unit_rtn;

  switch (*taxis_type = dfsGetTimeAxisType(pdfsIn))
  {
  case F_TM_EQ_AXIS: // Equidistant time axis
    rc = dfsGetEqTimeAxis(pdfsIn, neum_unit, &teum_Unit_rtn, tstart, tstep, num_timesteps, index);
    break;
  case F_TM_NEQ_AXIS: // Non-equidistant time axis
    rc = dfsGetNeqTimeAxis(pdfsIn, neum_unit, &teum_Unit_rtn, tstart, tspan, num_timesteps, index);
    break;
  case F_CAL_EQ_AXIS:  // Equidistant calendar axis
    rc = dfsGetEqCalendarAxis(pdfsIn, start_date, start_time, neum_unit, &teum_Unit_rtn, tstart, tstep, num_timesteps, index);
    break;
  case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
    rc = dfsGetNeqCalendarAxis(pdfsIn, start_date, start_time, neum_unit, &teum_Unit_rtn, tstart, tspan, num_timesteps, index);
    break;
  default:
    break;
  }
}

/**
 * Set item info to dynamic item, adding dummy 0D or 1D axis.
 * Used by DFS0 and DFSU dynamic items.
 */
void SetDfsDynamicItemInfo(LPHEAD pdfs, int i_item, LPCSTR item_name, int item_type, int item_unit, SimpleType item_datatype, int size)
{
  LPITEM item = dfsItemD(pdfs, i_item);
  long rc = dfsSetItemInfo(pdfs, item, item_type, item_name, item_unit, item_datatype);
  CheckRc(rc, "Error setting dynamic item info");
  // The second argument (0) is eumUUnitUndefined - the dummy axis need no unit
  if (size == 1)
    rc = dfsSetItemAxisEqD0(item, 0);
  else
    rc = dfsSetItemAxisEqD1(item, 0, size, 0, 1);
  CheckRc(rc, "Error setting dynamic item axis");

}

/**
 * Read static item and return the content of the static item
 * The name and the sitemtype are validated.
 * Automatic conversion from float to double will be performed
 */
void* ReadDfsStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype, int* size)
{
  int rc;

  LONG          item_type;                     // Item EUM type id
  LPCTSTR       item_type_str;                 // Name of item type
  LPCTSTR       item_name;                     // Name of item
  LONG          item_unit;                     // Item EUM unit id
  LPCTSTR       item_unit_str;                 // Item EUM unit string
  SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double

  LPVECTOR    pvec;
  LONG        error;

  // Read static item
  pvec = dfsStaticRead(fp, &error);
  if (pvec == NULL)
    return NULL;

  // Read static item info
  LPITEM static_item = dfsItemS(pvec);
  rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
  CheckRc(rc, "Error reading static item info");
  int num_elmts = dfsGetItemElements(static_item);
  // Return size (num_elmts) of item, in case a size pointer has been provided
  if (size != nullptr)
    *size = num_elmts;

  // Read static item data
  void* data_topo = malloc(dfsGetItemBytes(static_item));
  rc = dfsStaticGetData(pvec, data_topo);
  CheckRc(rc, "Error reading static data");
  rc = dfsStaticDestroy(&pvec);
  CheckRc(rc, "Error destroying static item");

  // Do automatically convert from float to double
  if (item_datatype == UFS_FLOAT && sitemtype == UFS_DOUBLE)
  {
    double* data_topo_double = ConvertFloat2Double(static_cast<float*>(data_topo), num_elmts);
    free(data_topo);
    data_topo = data_topo_double;
  }
  // Check that item data type is matching the required type
  else if (item_datatype != sitemtype)
  {
    LOG("Static item type not matching: %d vs %d\n", sitemtype, item_datatype);
    exit(-1);
  }
  // Check that item name type is matching the required name
  if (strcmp(name, item_name) != 0)
  {
    LOG("Static item name not matching: %s vs %s\n", name, item_name);
    exit(-1);
  }
  return data_topo;
}

/**
 * Write static item to file.
 * Static item is written with dummy item, only item name, and item data is relevant
 */
void WriteDfsStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype, int size, void* data)
{
  LPVECTOR static_pvec = 0;
  long rc= dfsStaticCreate(&static_pvec);
  CheckRc(rc, "Error creating static vector");

  // Set static item info
  LPITEM static_item = dfsItemS(static_pvec);
  // The third argument (999) is eumIItemUndefined - the dummy item need no item type value
  // The fifth argument (0) is eumUUnitUndefined   - the dummy item need no unit
  rc = dfsSetItemInfo(pdfs, static_item, 999, name, 0, sitemtype);
  CheckRc(rc, "Error setting item info to Static item");
  // The second argument (0) is eumUUnitUndefined - the dummy axis need no unit
  rc = dfsSetItemAxisEqD1(dfsItemS(static_pvec), 0, size, 0, 1);
  CheckRc(rc, "Error setting item axis to Static item");

  // Write static item and data to file
  rc = dfsStaticWrite(static_pvec, fp, data);
  CheckRc(rc, "Error writing static item");
  // Clean up
  dfsStaticDestroy(&static_pvec);
}

/** Get number of static items, reading all available static items */
int GetNbOfStaticItems(LPHEAD pdfsIn, LPFILE fp)
{
  int nbOfStaticItems = 0;
  // Move file pointer to the first static items
  LONG rc = dfsFindBlockStatic(pdfsIn, fp);
  if (rc != 0)
    return nbOfStaticItems;
  BOOL success = true;
  while (success)
  {
    // Read static item
    LPVECTOR pvec = dfsStaticRead(fp, &rc);
    CheckRc(rc, "Error reading static data from file");
    if (pvec == NULL)
      success = false;
    else
    {
      nbOfStaticItems++;
      rc = dfsStaticDestroy(&pvec);
      CheckRc(rc, "Error destroying static data");
    }
  }
  // Reset the pointer to the starting of the static items block
  rc = dfsFindBlockStatic(pdfsIn, fp); 
  CheckRc(rc, "Error resetting file pointer to static item");
  return nbOfStaticItems;
}


void GetDfsDeleteVals(LPHEAD pdfsIn, DeleteValues* DelVals)
{

  DelVals->dataTypeIn = dfsGetDataType(pdfsIn);
  DelVals->deleteF = dfsGetDeleteValFloat(pdfsIn);
  DelVals->deleteD = dfsGetDeleteValDouble(pdfsIn);
  DelVals->deleteByte = dfsGetDeleteValByte(pdfsIn);
  DelVals->deleteInt = dfsGetDeleteValInt(pdfsIn);
  DelVals->deleteUint = dfsGetDeleteValUnsignedInt(pdfsIn);
}

void SetDfsDeleteVals(LPHEAD pdfsWr, DeleteValues DelVals)
{
  long rc = dfsSetDataType(pdfsWr, DelVals.dataTypeIn);
  rc = dfsSetDeleteValFloat(pdfsWr, DelVals.deleteF);
  rc = dfsSetDeleteValDouble(pdfsWr, DelVals.deleteD);
  rc = dfsSetDeleteValByte(pdfsWr, DelVals.deleteByte);
  rc = dfsSetDeleteValInt(pdfsWr, DelVals.deleteInt);
  rc = dfsSetDeleteValUnsignedInt(pdfsWr, DelVals.deleteUint);
}

void CopyDfsHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr, long num_items)
{
  FileType ft = dfsGetFileType(pdfsIn);
  LPCTSTR fileTitle = dfsGetFileTitle(pdfsIn);
  LPCTSTR appTitle = dfsGetAppTitle(pdfsIn);
  int appVersion = dfsGetAppVersionNo(pdfsIn);
  StatType statT = StatType::F_NO_STAT;
  long rc = dfsHeaderCreate(ft, fileTitle, appTitle, appVersion, num_items, statT, pdfsWr);
}

LONG CopyDfsTimeAxis(LPHEAD pdfsIn, LPHEAD pdfsWr)
{
  long rc;

  TimeAxisType  time_axis_type;
  LPCTSTR     start_date, start_time;          // Start date and time for the calendar axes.
  double      tstart = 0;                      // Start time for the first time step in the file. 
  double      tstep = 0;                       // Time step size of equidistant axes
  double      tspan = 0;                       // Time span of non-equidistant axes
  LONG        num_timesteps = 0;               // Number of time steps in file
  LONG        index;                           // Index of first time step. Currently not used, always zero.
  LONG        ntime_unit;                      // Time unit in time axis, EUM unit id
  LPCTSTR     ttime_unit;                      // Time unit in time axis, EUM unit string

  switch (time_axis_type = dfsGetTimeAxisType(pdfsIn))
  {
  case F_TM_EQ_AXIS: // Equidistant time axis
    rc = dfsGetEqTimeAxis(pdfsIn, &ntime_unit, &ttime_unit, &tstart, &tstep, &num_timesteps, &index);
    CheckRc(rc, "Error getting time axis");
    rc = dfsSetEqTimeAxis(pdfsWr, ntime_unit, tstart, tstep, index);
    CheckRc(rc, "Error setting time axis");
    break;
  case F_TM_NEQ_AXIS: // Non-equidistant time axis
    rc = dfsGetNeqTimeAxis(pdfsIn, &ntime_unit, &ttime_unit, &tstart, &tspan, &num_timesteps, &index);
    CheckRc(rc, "Error getting time axis");
    rc = dfsSetNeqTimeAxis(pdfsWr, ntime_unit, tstart, index);
    CheckRc(rc, "Error setting time axis");
    break;
  case F_CAL_EQ_AXIS:  // Equidistant calendar axis
    rc = dfsGetEqCalendarAxis(pdfsIn, &start_date, &start_time, &ntime_unit, &ttime_unit, &tstart, &tstep, &num_timesteps, &index);
    CheckRc(rc, "Error getting time axis");
    rc = dfsSetEqCalendarAxis(pdfsWr, start_date, start_time, ntime_unit, tstart, tstep, 0);
    CheckRc(rc, "Error setting time axis");
    break;
  case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
    rc = dfsGetNeqCalendarAxis(pdfsIn, &start_date, &start_time, &ntime_unit, &ttime_unit, &tstart, &tspan, &num_timesteps, &index);
    CheckRc(rc, "Error getting time axis");
    rc = dfsSetNeqCalendarAxis(pdfsWr, start_date, start_time, ntime_unit, tstart, index);
    CheckRc(rc, "Error setting time axis");
    break;
  default:
    break;
  }
  return num_timesteps;
}

void CopyDfsDynamicItemInfo(LPHEAD pdfsIn, LPHEAD pdfsWr, int num_items)
{
  LONG rc;

  LONG          item_type;      // Item EUM type id
  LPCTSTR       item_type_str;  // Name of item type
  LPCTSTR       item_name;      // Name of item
  LONG          item_unit;      // Item EUM unit id
  LPCTSTR       item_unit_str;  // Item EUM unit string
  SimpleType    item_datatype;  // Simple type stored in item, usually float but can be double

  for (int i_item = 1; i_item <= num_items; i_item++)
  {
    // Name, quantity type and unit, and datatype of sourc efile
    LPITEM itemIn = dfsItemD(pdfsIn, i_item);
    rc = dfsGetItemInfo(itemIn, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
    CheckRc(rc, "Error getting dynamic item info");
    // Copy to target file item
    LPITEM itemWr = dfsItemD(pdfsWr, i_item);
    rc = dfsSetItemInfo(pdfsWr, itemWr, item_type, item_name, item_unit, item_datatype);
    CheckRc(rc, "Error setting dynamic item info");

    // Copy of item axis
    SpaceAxisType  axisIn = dfsGetItemAxisType(itemIn);
    long jGp, kGp, lGp, mGp;
    float x0, dx, y0, dy, z0, dz, f0, df;
    float alpha, phi, theta;
    Coords* coords;
    double* xCoords, *yCoords, *zCoords;
    bool copy = false;
    switch (axisIn)
    {
    case SpaceAxisType::F_UNDEFINED_SAXIS:
      throw new std::exception("undefined spatial axis");
    case SpaceAxisType::F_EQ_AXIS_D0:
      rc = dfsGetItemAxisEqD0(itemIn, &item_unit, &item_unit_str);
      rc = dfsSetItemAxisEqD0(itemWr, item_unit);
      break;
    case SpaceAxisType::F_EQ_AXIS_D1:
      rc = dfsGetItemAxisEqD1(itemIn, &item_unit, &item_unit_str, &jGp, &x0, &dx);
      rc = dfsSetItemAxisEqD1(itemWr, item_unit, jGp, x0, dx);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D1:
      rc = dfsGetItemAxisNeqD1(itemIn, &item_unit, &item_unit_str, &jGp, &coords);
      rc = dfsSetItemAxisNeqD1(itemWr, item_unit, jGp, coords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D2:
      rc = dfsGetItemAxisEqD2(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &x0, &y0, &dx, &dy);
      rc = dfsSetItemAxisEqD2(itemWr, item_unit, jGp, kGp, x0, y0, dx, dy);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D2:
      rc = dfsGetItemAxisNeqD2(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &xCoords, &yCoords);
      rc = dfsSetItemAxisNeqD2(itemWr, item_unit, jGp, kGp, xCoords, yCoords, copy);

      break;
    case SpaceAxisType::F_EQ_AXIS_D3:
      rc = dfsGetItemAxisEqD3(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &x0, &y0, &z0, &dx, &dy, &dz);
      rc = dfsSetItemAxisEqD3(itemWr, item_unit, jGp, kGp, lGp, x0, y0, z0, dx, dy, dz);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D3:
      rc = dfsGetItemAxisNeqD3(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisNeqD3(itemWr, item_unit, jGp, kGp, lGp, xCoords, yCoords, zCoords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D4:
      rc = dfsGetItemAxisEqD4(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &mGp, &x0, &y0, &z0, &f0, &dx, &dy, &dz, &df);
      rc = dfsSetItemAxisEqD4(itemWr, item_unit, jGp, kGp, lGp, mGp, x0, y0, z0, f0, dx, dy, dz, df);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D2:
      rc = dfsGetItemAxisCurveLinearD2(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &xCoords, &yCoords);
      rc = dfsSetItemAxisCurveLinearD2(itemWr, item_unit, jGp, kGp, xCoords, yCoords, copy);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D3:
      rc = dfsGetItemAxisCurveLinearD3(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisCurveLinearD3(itemWr, item_unit, jGp, kGp, lGp, xCoords, yCoords, zCoords, copy);
      break;
    default:
      throw new std::exception("spatial axis not supported");
    }
    rc = dfsGetItemAxisOrientation(itemIn, &alpha, &phi, &theta);
    rc = dfsSetItemAxisOrientation(itemWr, alpha, phi, theta);
    float x, y, z;
    rc = dfsGetItemRefCoords(itemIn, &x, &y, &z);
    rc = dfsSetItemRefCoords(itemWr, x, y, z);
  }
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


void CopyDfsStaticItems(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr)
{
  long rc;
  int countStatic = 0;
  LPVECTOR pVecIn = nullptr;
  while (nullptr != (pVecIn = dfsStaticRead(fpIn, &rc)))
  {
    LPITEM static_item = dfsItemS(pVecIn);
    // Read static item and its data
    LONG memorySize = dfsGetItemBytes(static_item);
    void* data = malloc(memorySize);
    rc = dfsStaticGetData(pVecIn, data);
    // Get static item info
    LONG          item_type;                     // Item EUM type id
    LPCTSTR       item_type_str;                 // Name of item type
    LPCTSTR       item_name;                     // Name of item
    LONG          item_unit;                     // Item EUM unit id
    LPCTSTR       item_unit_str;                 // Item EUM unit string
    SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double
    rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);

    // Create new static item and set item info
    LPVECTOR pVecOut = nullptr;
    rc = dfsStaticCreate(&pVecOut);
    LPITEM static_itemOut = dfsItemS(pVecOut);
    rc = dfsSetItemInfo(pdfsWr, static_itemOut, item_type, item_name, item_unit, item_datatype);
    // Copy static item axis
    long eumAxisUnit;
    long jStatic, kStatic, lStatic, mStatic;
    float x0Static, dxStatic, y0Static, dyStatic, z0Static, dzStatic, f0Static, dfStatic;
    Coords* coords;
    double* xCoords, *yCoords, *zCoords;
    BOOL copy = false;
    SpaceAxisType  axisStaticIn = dfsGetItemAxisType(static_item);
    switch (axisStaticIn)
    {
    case SpaceAxisType::F_UNDEFINED_SAXIS:
      throw new std::exception("undefined spatial axis");
    case SpaceAxisType::F_EQ_AXIS_D0:
      rc = dfsGetItemAxisEqD0(static_item, &eumAxisUnit, &item_unit_str);
      rc = dfsSetItemAxisEqD0(dfsItemS(pVecOut), eumAxisUnit);
      break;
    case SpaceAxisType::F_EQ_AXIS_D1:
      rc = dfsGetItemAxisEqD1(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &x0Static, &dxStatic);
      rc = dfsSetItemAxisEqD1(dfsItemS(pVecOut), eumAxisUnit, jStatic, x0Static, dxStatic);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D1:
      rc = dfsGetItemAxisNeqD1(static_item, &eumAxisUnit, &item_name, &jStatic, &coords);
      rc = dfsSetItemAxisNeqD1(dfsItemS(pVecOut), eumAxisUnit, jStatic, coords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D2:
      rc = dfsGetItemAxisEqD2(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &x0Static, &y0Static, &dxStatic, &dyStatic);
      dfsSetItemAxisEqD2(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, x0Static, y0Static, dxStatic, dyStatic);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D2:
      rc = dfsGetItemAxisNeqD2(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &xCoords, &yCoords);
      rc = dfsSetItemAxisNeqD2(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, xCoords, yCoords, copy);

      break;
    case SpaceAxisType::F_EQ_AXIS_D3:
      rc = dfsGetItemAxisEqD3(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &lStatic, &x0Static, &y0Static, &z0Static, &dxStatic, &dyStatic, &dzStatic);
      rc = dfsSetItemAxisEqD3(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, lStatic, x0Static, y0Static, z0Static, dxStatic, dyStatic, dzStatic);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D3:
      rc = dfsGetItemAxisNeqD3(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &lStatic, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisNeqD3(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, lStatic, xCoords, yCoords, zCoords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D4:
      rc = dfsGetItemAxisEqD4(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &lStatic, &mStatic, &x0Static, &y0Static, &z0Static, &f0Static, &dxStatic, &dyStatic, &dzStatic, &dfStatic);
      rc = dfsSetItemAxisEqD4(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, lStatic, mStatic, x0Static, y0Static, z0Static, f0Static, dxStatic, dyStatic, dzStatic, dfStatic);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D2:
      rc = dfsGetItemAxisCurveLinearD2(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &xCoords, &yCoords);
      rc = dfsSetItemAxisCurveLinearD2(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, xCoords, yCoords, copy);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D3:
      rc = dfsGetItemAxisCurveLinearD3(static_item, &eumAxisUnit, &item_unit_str, &jStatic, &kStatic, &lStatic, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisCurveLinearD3(dfsItemS(pVecOut), eumAxisUnit, jStatic, kStatic, lStatic, xCoords, yCoords, zCoords, copy);
      break;
    default:
      throw new std::exception("spatial axis not supported");
    }

    float x, y, z;
    float alpha, phi, theta;
    rc = dfsGetItemRefCoords(static_item, &x, &y, &z);
    rc = dfsGetItemAxisOrientation(static_item, &alpha, &phi, &theta);
    rc = dfsSetItemRefCoords(dfsItemS(pVecOut), x, y, z);
    rc = dfsSetItemAxisOrientation(dfsItemS(pVecOut), alpha, phi, theta);

    // Write static item and data to new file
    rc = dfsStaticWrite(pVecOut, fpWr, data);

    // Clean up
    rc = dfsStaticDestroy(&pVecOut);
    rc = dfsStaticDestroy(&pVecIn);
    free(data);
    countStatic++;
  }
}

void CopyDfsTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr, void** item_timestep_dataf, long num_timesteps, long num_items)
{
  long rc;
  long current_tstep = 0;
  double      time;

  // Loop over all timesteps
  while (current_tstep < num_timesteps)
  {
    // Loop over all items
    for (int i_item = 1; i_item <= num_items; i_item++)
    {
      // Read item-timestep where the file pointer points to,
      // and move the filepointer to the next item-timestep
      rc = dfsReadItemTimeStep(pdfsIn, fpIn, &time, item_timestep_dataf[i_item - 1]);
      CheckRc(rc, "Error reading dynamic item data");

      rc = dfsWriteItemTimeStep(pdfsWr, fpWr, time, item_timestep_dataf[i_item - 1]);
      CheckRc(rc, "Error writing dynamic item data");
    }
    current_tstep++;
  }
}


bool TestDataPathValueSet = false;
char TestDataPathValue[_MAX_DIR];

#define STRINGIFY(x) #x
#define EXPAND(x) STRINGIFY(x)

char* TestDataPath()
{
  if (!TestDataPathValueSet)
  {
    TestDataPathValueSet = true;
    LPTSTR s = EXPAND(PROJECTDIR);
    size_t len = strlen(s);
    // Remove initial character, the final two characters, and
    // the entire "Examples\C\DHI.MikeCore.CExamples\"
    strncpy(TestDataPathValue, s + 1, len - 37);
    // Then add "TestData\"
    strncpy(TestDataPathValue+ len - 37, "TestData\\\0", 10);
  }
  return TestDataPathValue;
}
