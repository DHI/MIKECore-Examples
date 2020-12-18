#include "pch.h"
#include "eum.h"
#include <dfsio.h>

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
