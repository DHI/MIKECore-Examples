#pragma once

#include "pch.h"
#include "eum.h"
#include <dfsio.h>

// Check return code of dfs methods
void CheckRc(LONG rc, LPCTSTR errMsg);

// Convert float array to double array
double* ConvertFloat2Double(float* flt, int size);

// Read static item and return the content of the static item
// The values should be of type sitemtype. Only conversion from
// float to double will be performed.
void* readStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype);

int GetNbOfStaticItems(LPHEAD pdfsIn, LPFILE fp);
void CopyTimeAxis(LPHEAD pdfsIn, LPHEAD pdfsWr, long* num_timesteps);
long GetDfsGeoInfo(LPHEAD pdfsIn, LPCTSTR* projection_id, double* lon0, double* lat0, double* orientation);
void ReadTimeAxis(LPHEAD pdfsIn, LPCTSTR* start_date, long* num_timesteps, LPCTSTR* start_time, double* tstart, double* tstep, double* tspan, long* neum_unit, long* index);

struct DeleteValues
{
  long           dataTypeIn;
  float          deleteF;
  double         deleteD;
  char           deleteByte;
  int            deleteInt;
  unsigned int   deleteUint;
};
void ReadDfsDeleteVals(LPHEAD pdfsIn, DeleteValues* DelVals);

void WriteDfsDeleteVals(LPHEAD pdfsWr, DeleteValues DelVals);
void CopyDynamicItemInfo(LPHEAD pdfsIn, LPHEAD pdfsWr, float** item_timestep_dataf, int num_items);
void CopyDfsStaticInfo(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr);
void CopyHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr, long num_items);
void CopyTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr, float** item_timestep_dataf, long num_timesteps, long num_items);
