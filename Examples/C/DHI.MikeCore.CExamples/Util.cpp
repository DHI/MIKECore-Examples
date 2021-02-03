#include "pch.h"
#include "eum.h"
#include <dfsio.h>
#include <CppUnitTestAssert.h>
#include "Util.h"

// Check return code of dfs methods
void CheckRc(LONG rc, LPCTSTR errMsg)
{
  if (rc != F_NO_ERROR)
  {
    // Check the error code in 
    printf("%s (%li)\n", errMsg, rc);
    exit(-1);
  }
}

// Convert float array to double array
double* ConvertFloat2Double(float* flt, int size)
{
  double* dbl = (double*)malloc(size * sizeof(double));
  for (int i = 0; i < size; i++)
    dbl[i] = flt[i];
  return dbl;
}

// Read next static item and return the content of the static item
// The values should be of type sitemtype. Only conversion from
// float to double will be performed.
void* readStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype)
{
  int rc;

  LONG          item_type;                     // Item EUM type id
  LPCTSTR       item_type_str;                 // Name of item type
  LPCTSTR       item_name;                     // Name of item
  LONG          item_unit;                     // Item EUM unit id
  LPCTSTR       item_unit_str;                 // Item EUM unit string
  SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double
  LONG          naxis_unit;                    // Axis EUM unit id
  LPCTSTR       taxis_unit;                    // Axis EUM unit string
  LONG          n_item, m_item;                // Axis dimension sizes
  float         x0, y0;                        // Axis start coordinate
  float         dx, dy;                        // Axis coordinate delta
  Coords       *C1 = NULL;                     // Axis coordinates

  LPVECTOR    pvec;
  LONG        error;

  // Read static item
  pvec = dfsStaticRead(fp, &error);
  if (pvec == NULL)
    return NULL;

  // Read static item info
  LPITEM static_item;
  static_item = dfsItemS(pvec);
  rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
  CheckRc(rc, "Error reading static item info");
  int num_elmts = dfsGetItemElements(static_item);

  void* data_topo = NULL;
  SpaceAxisType axis_type;
  // A static item can have all the same axes as the dynamic item can.
  // For most files using static items (dfs2, dfs3) the static items are of same type a
  // the dynamic item.
  switch (axis_type = dfsGetItemAxisType(static_item))
  {
  case F_EQ_AXIS_D2:
    rc = dfsGetItemAxisEqD2(static_item, &naxis_unit, &taxis_unit, &n_item, &m_item, &x0, &y0, &dx, &dy);
    CheckRc(rc, "Error reading static axis");
    break;
  case F_EQ_AXIS_D1:
    rc = dfsGetItemAxisEqD1(static_item, &naxis_unit, &taxis_unit, &n_item, &x0, &dx);
    CheckRc(rc, "Error reading static axis");
    break;
  case F_NEQ_AXIS_D1:
    rc = dfsGetItemAxisNeqD1(static_item, &naxis_unit, &taxis_unit, &n_item, &C1);
    CheckRc(rc, "Error reading static axis");
    break;
  default:
    printf("Static item spatial axis not yet implemented: %d\n", axis_type);
    exit(-1);
  }

  // Read static item data
  data_topo = malloc(dfsGetItemBytes(static_item));
  rc = dfsStaticGetData(pvec, data_topo);
  CheckRc(rc, "Error reading static data");
  rc = dfsStaticDestroy(&pvec);
  CheckRc(rc, "Error destroying static item");

  // Do automatically convert from float to double
  if (item_datatype == UFS_FLOAT && sitemtype == UFS_DOUBLE)
  {
    data_topo = ConvertFloat2Double(static_cast<float*>(data_topo), num_elmts);
  }
  else if (item_datatype != sitemtype)
  {
    printf("Static item type not matching: %d vs %d\n", sitemtype, item_datatype);
    exit(-1);
  }
  return data_topo;
}

int GetNbOfStaticItems(LPHEAD pdfsIn, LPFILE fp)
{
  int nbOfStaticItems = 0;
  LPVECTOR    pvec;
  LONG        error;
  long isOk = dfsFindBlockStatic(pdfsIn, fp);
  if (isOk != 0)
    return nbOfStaticItems;
  BOOL success = true;
  while (success)
  {
    // Read static item
    pvec = dfsStaticRead(fp, &error);
    if (pvec == NULL)
      success = false;
    else
      nbOfStaticItems++;
    LONG rc = dfsStaticDestroy(&pvec);
  }
  dfsFindBlockStatic(pdfsIn, fp); //reset the pointer to the starting of the static items block
  return nbOfStaticItems;
}

void CopyTimeAxis(LPHEAD pdfsIn, LPHEAD pdfsWr, long* num_timesteps)
{
  long rc;
  TimeAxisType time_axis_type;
  LPCTSTR      start_date, start_time;
  long         neum_unit, index;
  LPCTSTR      teum_Unit_rtn;
  double       tstart, tstep, tspan;
  switch (time_axis_type = dfsGetTimeAxisType(pdfsIn))
  {
  case F_TM_EQ_AXIS: // Equidistant time axis
    rc = dfsGetEqTimeAxis(pdfsIn, &neum_unit, &teum_Unit_rtn, &tstart, &tstep, num_timesteps, &index);
    rc = dfsSetEqTimeAxis(pdfsWr, neum_unit, tstart, tstep, index);
    break;
  case F_TM_NEQ_AXIS: // Non-equidistant time axis
    rc = dfsGetNeqTimeAxis(pdfsIn, &neum_unit, &teum_Unit_rtn, &tstart, &tspan, num_timesteps, &index);
    rc = dfsSetNeqTimeAxis(pdfsWr, neum_unit, tstart, index);
    break;
  case F_CAL_EQ_AXIS:  // Equidistant calendar axis
    rc = dfsGetEqCalendarAxis(pdfsIn, &start_date, &start_time, &neum_unit, &teum_Unit_rtn, &tstart, &tstep, num_timesteps, &index);
    rc = dfsSetEqCalendarAxis(pdfsWr, start_date, start_time, neum_unit, tstart, tstep, 0);
    break;
  case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
    rc = dfsGetNeqCalendarAxis(pdfsIn, &start_date, &start_time, &neum_unit, &teum_Unit_rtn, &tstart, &tspan, num_timesteps, &index);
    rc = dfsSetNeqCalendarAxis(pdfsWr, start_date, start_time, neum_unit, tstart, index);
    break;
  default:
    break;
  }
}
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

void ReadTimeAxis(LPHEAD pdfsIn, LPCTSTR* start_date, long* num_timesteps, LPCTSTR* start_time, double* tstart, double* tstep, double* tspan, long* neum_unit, long* index)
{
  long rc;
  TimeAxisType time_axis_type;
  LPCTSTR      teum_Unit_rtn;

  switch (time_axis_type = dfsGetTimeAxisType(pdfsIn))
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

void ReadDfsDeleteVals(LPHEAD pdfsIn, DeleteValues* DelVals)
{

  DelVals->dataTypeIn = dfsGetDataType(pdfsIn);
  DelVals->deleteF = dfsGetDeleteValFloat(pdfsIn);
  DelVals->deleteD = dfsGetDeleteValDouble(pdfsIn);
  DelVals->deleteByte = dfsGetDeleteValByte(pdfsIn);
  DelVals->deleteInt = dfsGetDeleteValInt(pdfsIn);
  DelVals->deleteUint = dfsGetDeleteValUnsignedInt(pdfsIn);
}


void WriteDfsDeleteVals(LPHEAD pdfsWr, DeleteValues DelVals)
{
  long rc = dfsSetDataType(pdfsWr, DelVals.dataTypeIn);
  rc = dfsSetDeleteValFloat(pdfsWr, DelVals.deleteF);
  rc = dfsSetDeleteValDouble(pdfsWr, DelVals.deleteD);
  rc = dfsSetDeleteValByte(pdfsWr, DelVals.deleteByte);
  rc = dfsSetDeleteValInt(pdfsWr, DelVals.deleteInt);
  rc = dfsSetDeleteValUnsignedInt(pdfsWr, DelVals.deleteUint);
}

void CopyDynamicItemInfo(LPHEAD pdfsIn, LPHEAD pdfsWr, float** item_timestep_dataf, int num_items)
{
  long item_type, item_unit, rc;
  LPCTSTR item_type_str, item_name, item_unit_str;
  SimpleType item_datatype;
  for (int i_item = 1; i_item <= num_items; i_item++)
  {
    LPITEM itemIn = dfsItemD(pdfsIn, i_item);
    // Name, quantity type and unit, and datatype
    rc = dfsGetItemInfo(itemIn, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
    CheckRc(rc, "Error reading dynamic item info");
    int item_num_elmts = dfsGetItemElements(dfsItemD(pdfsIn, i_item));
    // Create buffer for when reading data.
    item_timestep_dataf[i_item - 1] = new float[item_num_elmts];
    LPITEM item1 = dfsItemD(pdfsWr, i_item);
    rc = dfsSetItemInfo(pdfsWr, item1, item_type, item_name, item_unit, item_datatype);
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
      rc = dfsSetItemAxisEqD0(item1, item_unit);
      break;
    case SpaceAxisType::F_EQ_AXIS_D1:
      rc = dfsGetItemAxisEqD1(itemIn, &item_unit, &item_unit_str, &jGp, &x0, &dx);
      rc = dfsSetItemAxisEqD1(item1, item_unit, jGp, x0, dx);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D1:
      rc = dfsGetItemAxisNeqD1(itemIn, &item_unit, &item_unit_str, &jGp, &coords);
      rc = dfsSetItemAxisNeqD1(item1, item_unit, jGp, coords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D2:
      rc = dfsGetItemAxisEqD2(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &x0, &y0, &dx, &dy);
      rc = dfsSetItemAxisEqD2(item1, item_unit, jGp, kGp, x0, y0, dx, dy);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D2:
      rc = dfsGetItemAxisNeqD2(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &xCoords, &yCoords);
      rc = dfsSetItemAxisNeqD2(item1, item_unit, jGp, kGp, xCoords, yCoords, copy);

      break;
    case SpaceAxisType::F_EQ_AXIS_D3:
      rc = dfsGetItemAxisEqD3(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &x0, &y0, &z0, &dx, &dy, &dz);
      rc = dfsSetItemAxisEqD3(item1, item_unit, jGp, kGp, lGp, x0, y0, z0, dx, dy, dz);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D3:
      rc = dfsGetItemAxisNeqD3(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisNeqD3(item1, item_unit, jGp, kGp, lGp, xCoords, yCoords, zCoords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D4:
      rc = dfsGetItemAxisEqD4(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &mGp, &x0, &y0, &z0, &f0, &dx, &dy, &dz, &df);
      rc = dfsSetItemAxisEqD4(item1, item_unit, jGp, kGp, lGp, mGp, x0, y0, z0, f0, dx, dy, dz, df);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D2:
      rc = dfsGetItemAxisCurveLinearD2(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &xCoords, &yCoords);
      rc = dfsSetItemAxisCurveLinearD2(item1, item_unit, jGp, kGp, xCoords, yCoords, copy);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D3:
      rc = dfsGetItemAxisCurveLinearD3(itemIn, &item_unit, &item_unit_str, &jGp, &kGp, &lGp, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisCurveLinearD3(item1, item_unit, jGp, kGp, lGp, xCoords, yCoords, zCoords, copy);
      break;
    default:
      throw new std::exception("spatial axis not supported");
    }
    rc = dfsGetItemAxisOrientation(itemIn, &alpha, &phi, &theta);
    rc = dfsSetItemAxisOrientation(item1, alpha, phi, theta);
    float x, y, z;
    dfsGetItemRefCoords(itemIn, &x, &y, &z);
    dfsSetItemRefCoords(item1, x, y, z);
  }
}

void CopyDfsStaticInfo(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr)
{
  long rc;
  // Read DFSU geometry from static items in DFSU file
  int nbOfStaticItems = GetNbOfStaticItems(pdfsIn, fpIn);
  int countStatic = 1;
  long item_type, item_unit;
  LPCTSTR item_name, eumUnitStr, item_typeStr;
  SimpleType item_datatype;
  while (countStatic <= nbOfStaticItems)
  {
    void* data;
    LPVECTOR pVecIn = 0;
    pVecIn = dfsStaticRead(fpIn, &rc);
    LPITEM static_item = dfsItemS(pVecIn);
    // Read static item
    auto memorySize = dfsGetItemBytes(static_item);
    data = malloc(memorySize);
    rc = dfsStaticGetData(pVecIn, data);
    rc = dfsGetItemInfo(static_item, &item_type, &item_typeStr, &item_name, &item_unit, &eumUnitStr, &item_datatype);

    long eumStaticUnit;
    long jStatic, kStatic, lStatic, mStatic;
    float x0Static, dxStatic, y0Static, dyStatic, z0Static, dzStatic, f0Static, dfStatic;

    LPVECTOR pVecOut = 0;
    long io_error = dfsStaticCreate(&pVecOut);
    LPITEM static_itemOut = dfsItemS(pVecOut);
    rc = dfsSetItemInfo_(pdfsWr, static_itemOut, item_type, item_name, eumUnitStr, item_datatype);
    Coords* coords;
    double* xCoords, *yCoords, *zCoords;
    BOOL copy = false;
    SpaceAxisType  axisStaticIn = dfsGetItemAxisType(static_item);
    switch (axisStaticIn)
    {
    case SpaceAxisType::F_UNDEFINED_SAXIS:
      throw new std::exception("undefined spatial axis");
    case SpaceAxisType::F_EQ_AXIS_D0:
      rc = dfsGetItemAxisEqD0(static_item, &eumStaticUnit, &eumUnitStr);
      rc = dfsSetItemAxisEqD0(dfsItemS(pVecOut), eumStaticUnit);
      break;
    case SpaceAxisType::F_EQ_AXIS_D1:
      rc = dfsGetItemAxisEqD1(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &x0Static, &dxStatic);
      rc = dfsSetItemAxisEqD1(dfsItemS(pVecOut), eumStaticUnit, jStatic, x0Static, dxStatic);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D1:
      rc = dfsGetItemAxisNeqD1(static_item, &eumStaticUnit, &item_name, &jStatic, &coords);
      rc = dfsSetItemAxisNeqD1(dfsItemS(pVecOut), eumStaticUnit, jStatic, coords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D2:
      rc = dfsGetItemAxisEqD2(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &x0Static, &y0Static, &dxStatic, &dyStatic);
      dfsSetItemAxisEqD2(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, x0Static, y0Static, dxStatic, dyStatic);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D2:
      rc = dfsGetItemAxisNeqD2(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &xCoords, &yCoords);
      rc = dfsSetItemAxisNeqD2(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, xCoords, yCoords, copy);

      break;
    case SpaceAxisType::F_EQ_AXIS_D3:
      rc = dfsGetItemAxisEqD3(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &lStatic, &x0Static, &y0Static, &z0Static, &dxStatic, &dyStatic, &dzStatic);
      rc = dfsSetItemAxisEqD3(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, lStatic, x0Static, y0Static, z0Static, dxStatic, dyStatic, dzStatic);
      break;
    case SpaceAxisType::F_NEQ_AXIS_D3:
      rc = dfsGetItemAxisNeqD3(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &lStatic, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisNeqD3(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, lStatic, xCoords, yCoords, zCoords, copy);
      break;
    case SpaceAxisType::F_EQ_AXIS_D4:
      rc = dfsGetItemAxisEqD4(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &lStatic, &mStatic, &x0Static, &y0Static, &z0Static, &f0Static, &dxStatic, &dyStatic, &dzStatic, &dfStatic);
      rc = dfsSetItemAxisEqD4(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, lStatic, mStatic, x0Static, y0Static, z0Static, f0Static, dxStatic, dyStatic, dzStatic, dfStatic);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D2:
      rc = dfsGetItemAxisCurveLinearD2(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &xCoords, &yCoords);
      rc = dfsSetItemAxisCurveLinearD2(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, xCoords, yCoords, copy);
      break;
    case SpaceAxisType::F_CURVE_LINEAR_AXIS_D3:
      rc = dfsGetItemAxisCurveLinearD3(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &lStatic, &xCoords, &yCoords, &zCoords);
      rc = dfsSetItemAxisCurveLinearD3(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, lStatic, xCoords, yCoords, zCoords, copy);
      break;
    default:
      throw new std::exception("spatial axis not supported");
    }

    float x, y, z;
    rc = dfsGetItemRefCoords(static_item, &x, &y, &z);
    float alpha, phi, theta;
    rc = dfsGetItemAxisOrientation(static_item, &alpha, &phi, &theta);
    dfsSetItemRefCoords(dfsItemS(pVecOut), x, y, z);
    dfsSetItemAxisOrientation(dfsItemS(pVecOut), alpha, phi, theta);
    rc = dfsStaticWrite(pVecOut, fpWr, data);
    dfsStaticDestroy(&pVecOut);
    dfsStaticDestroy(&pVecIn);
    countStatic++;
    free(data);
  }
}

void CopyHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr, long num_items)
{
  FileType ft = dfsGetFileType(pdfsIn);
  LPCTSTR title = dfsGetFileTitle(pdfsIn);
  LPCTSTR appTitle = dfsGetAppTitle(pdfsIn);
  StatType statT = StatType::F_NO_STAT;
  long rc = dfsHeaderCreate(ft, title, appTitle, 0, num_items, statT, pdfsWr);
}
void CopyTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr, float** item_timestep_dataf, long num_timesteps, long num_items)
{
  long rc;
  long current_tstep = 0;
  double      time;
  // Loop over the first 10 time steps
  int tstep_end = num_timesteps; // > 13 ? 13 : num_timesteps;
  while (current_tstep < tstep_end)
  {
    // Loop over all items
    for (int i_item = 1; i_item <= num_items; i_item++)
    {
      // Read item-timestep where the file pointer points to,
      // and move the filepointer to the next item-timestep
      rc = dfsReadItemTimeStep(pdfsIn, fpIn, &time, item_timestep_dataf[i_item - 1]);
      CheckRc(rc, "Error reading dynamic item data");
      rc = dfsWriteItemTimeStep(pdfsWr, fpWr, time, item_timestep_dataf[i_item - 1]);
      // If the temporal axis is equidistant, the time variable is the timestep index value.
      // If temporal axis is non-equidistant, this is the time from start of the file
    }
    current_tstep++;
  }
}
