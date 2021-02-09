#pragma once

#include "pch.h"
#include "eum.h"
#include <dfsio.h>


/** Check return code of dfs methods */
void CheckRc(LONG rc, LPCTSTR errMsg);

/** Get error string from return code */
const char* GetRCString(LONG rc);

double* ConvertFloat2Double(float* flt, int size);

long GetDfsGeoInfoProjString(LPHEAD pdfs, LPCTSTR* projection_id);
long GetDfsGeoInfo(LPHEAD pdfsIn, LPCTSTR* projection_id, double* lon0, double* lat0, double* orientation);

void GetDfsTimeAxis(LPHEAD pdfsIn, TimeAxisType* taxis_type, long* num_timesteps, 
                    LPCTSTR* start_date, LPCTSTR* start_time, 
                    double* tstart, double* tstep, double* tspan,
                    long* neum_unit, long* index);

void  SetDfsDynamicItemInfo(LPHEAD pdfs, int i_item, LPCSTR item_name, int item_type, int item_unit, SimpleType item_datatype, int size);

void* ReadDfsStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype, int* size = NULL);
void  WriteDfsStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype, int size, void* data);
int   GetNbOfStaticItems(LPHEAD pdfsIn, LPFILE fp);


struct DeleteValues
{
  long           dataTypeIn;
  float          deleteF;
  double         deleteD;
  char           deleteByte;
  int            deleteInt;
  unsigned int   deleteUint;
};


// Various copy methods, for copying data from one DFS file to another

void GetDfsDeleteVals(LPHEAD pdfsIn, DeleteValues* DelVals);
void SetDfsDeleteVals(LPHEAD pdfsWr, DeleteValues DelVals);

void CopyDfsHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr, long num_items);
long CopyDfsTimeAxis(LPHEAD pdfsIn, LPHEAD pdfsWr);
void CopyDfsDynamicItemInfo(LPHEAD pdfsIn, LPHEAD pdfsWr, int num_items);
void CopyDfsCustomBlocks(LPHEAD pdfsIn, LPHEAD pdfsWr);
void CopyDfsStaticItems(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr);
void CopyDfsTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr, void** item_timestep_dataf, long num_timesteps, long num_items);
inline void CopyDfsTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr, float** item_timestep_dataf, long num_timesteps, long num_items)
{
  void** item_timestep_data = (void**)item_timestep_dataf;
  CopyDfsTemporalData(pdfsIn, fpIn, pdfsWr, fpWr, item_timestep_data, num_timesteps, num_items);
}


/** LOG method, redirecting message to log files */
#define LOG(...) { \
  char buf[257]; \
  snprintf(buf, 256, __VA_ARGS__); \
  Microsoft::VisualStudio::CppUnitTestFramework::Logger::WriteMessage(buf);\
  }

/** Method returning the full path to the TestData folder, including a final "\" */
char* TestDataPath();
