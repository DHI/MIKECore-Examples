#include "pch.h"
#include "eum.h"
#include "dfsio.h"
#include "Util.h"
#include <CppUnitTest.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// a global variable so it only has to be setup once
extern LPCTSTR TestDataFolder;
namespace UnitestForC_MikeCore
{
  //extern LPCTSTR TestDataFolder;
  TEST_CLASS(Dfs2_tests)
  {
  public:
    /// write a copy of a dfs2 file from after reading its internal components.
    TEST_METHOD(WriteDfs2FileFromSourceTest)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      LPCTSTR OutfileName = "OresundHD_created.dfs2";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);
      CreateDfs2DFromSource(outputFullPath);
    }

    void CreateDfs2DFromSource(LPCTSTR outputFullPath)
    {
      // dfsDebugOn(true);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      CreateDfs2DFromSource(outputFullPath, pdfs, fp);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
    }

    void CreateDfs2DFromSource(LPCTSTR outputFullPath, LPHEAD  pdfsIn, LPFILE fpIn)
    {
      ReadTimeAxis(pdfsIn);

      LPHEAD pdfsWr;
      LPFILE fpWr;
      /***********************************
       * Dynamic item information
       ***********************************/
      num_items = dfsGetNoOfItems(pdfsIn);
      item_timestep_dataf = new float*[num_items];
      CreateHeader(pdfsIn, &pdfsWr);

      ReadDfsDataTypes(pdfsIn);
      ReadDfsGeoInfo(pdfsIn);

      CopyDynamicItemInfo(pdfsIn, pdfsWr, item_timestep_dataf);

      /****************************************
       * Geometry sizes - read from custom block "MIKE_FM"
       ****************************************/
      CopyDfsCustomBlocks(pdfsIn, pdfsWr);

      rc = dfsFileCreateEx(outputFullPath, pdfsWr, &fpWr, true);

      /***********************************
       * Geometry information
       ***********************************/

      CopyDfsuStaticInfo(pdfsIn, fpIn, pdfsWr, fpWr);

      CopyTemporalData(pdfsIn, fpIn, pdfsWr, fpWr);
      // Close file and destroy header
      FreeDataMemory();
      rc = dfsFileClose(pdfsWr, &fpWr);
      dfsHeaderDestroy(&pdfsWr);
    }

    void CreateHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr)
    {
      FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
      LPCTSTR title = "";
      LPCTSTR appTitle = dfsGetAppTitle(pdfsIn);
      StatType statT = StatType::F_NO_STAT;
      rc = dfsHeaderCreate(ft, title, appTitle, 0, num_items, statT, pdfsWr);
    }

    void ReadDfsDataTypes(LPHEAD pdfsIn)
    {
      dataTypeIn = dfsGetDataType(pdfsIn);
      deleteF = dfsGetDeleteValFloat(pdfsIn);
      deleteD = dfsGetDeleteValDouble(pdfsIn);
      deleteByte = dfsGetDeleteValByte(pdfsIn);
      deleteInt = dfsGetDeleteValInt(pdfsIn);
      deleteUint = dfsGetDeleteValUnsignedInt(pdfsIn);
    }


    void ReadDfsGeoInfo(LPHEAD pdfsIn)
    {
      geo_info_type = dfsGetGeoInfoType(pdfsIn);
      if (geo_info_type == F_UTM_PROJECTION)
      {
        rc = dfsGetGeoInfoUTMProj(pdfsIn, &projection_id, &lon0, &lat0, &orientation);
      }
      else if (geo_info_type != F_UNDEFINED_GEOINFO)
        Assert::Fail();
    }


    void CopyDynamicItemInfo(LPHEAD pdfsIn, LPHEAD pdfsWr, float** item_timestep_dataf)
    {
      WriteDfsDataTypes(pdfsWr);

      rc = dfsSetGeoInfoUTMProj(pdfsWr, projection_id, lon0, lat0, orientation);
      rc = dfsSetEqCalendarAxis(pdfsWr, start_date, start_time, ntime_unit, tstart, tstep, 0);

      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        LPITEM itemIn = dfsItemD(pdfsIn, i_item);
        // Name, quantity type and unit, and datatype
        rc = dfsGetItemInfo(itemIn, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        CheckRc(rc, "Error reading dynamic item info");
        float x, y, z;
        dfsGetItemRefCoords(itemIn, &x, &y, &z);
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
        dfsSetItemRefCoords(item1, x, y, z);
      }
    }

    void WriteDfsDataTypes(LPHEAD pdfsWr)
    {
      rc = dfsSetDataType(pdfsWr, dataTypeIn);
      rc = dfsSetDeleteValFloat(pdfsWr, deleteF);
      rc = dfsSetDeleteValDouble(pdfsWr, deleteD);
      rc = dfsSetDeleteValByte(pdfsWr, deleteByte);
      rc = dfsSetDeleteValInt(pdfsWr, deleteInt);
      rc = dfsSetDeleteValUnsignedInt(pdfsWr, deleteUint);
    }

    void CopyDfsCustomBlocks(LPHEAD pdfsIn, LPHEAD pdfsWr)
    {
      LPBLOCK customblock_ptr;
      rc = dfsGetCustomBlockRef(pdfsIn, &customblock_ptr);
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

    void CopyDfsuStaticInfo(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr)
    {
      // Read DFSU geometry from static items in DFSU file
      int nbOfStaticItems = 1;
      int countStatic = 1;
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
        rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        long eumStaticUnit;
        LPCTSTR eumUnitStr;
        long jStatic, kStatic;
        float x0Static, dxStatic, y0Static, dyStatic;
        SpaceAxisType  axisStaticIn = dfsGetItemAxisType(static_item);
        rc = dfsGetItemAxisEqD2(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &kStatic, &x0Static, &y0Static, &dxStatic, &dyStatic);
        float x, y, z;
        rc = dfsGetItemRefCoords(static_item, &x, &y, &z);
        float alpha, phi, theta;
        rc = dfsGetItemAxisOrientation(static_item, &alpha, &phi, &theta);
        LPVECTOR pVecOut = 0;
        long io_error = dfsStaticCreate(&pVecOut);
        rc = dfsSetItemInfo_(pdfsWr, dfsItemS(pVecOut), item_type, item_name, eumUnitStr, item_datatype);
        dfsSetItemAxisEqD2(dfsItemS(pVecOut), eumStaticUnit, jStatic, kStatic, x0Static, y0Static, dxStatic, dyStatic);
        dfsSetItemRefCoords(dfsItemS(pVecOut), x, y, z);
        dfsSetItemAxisOrientation(dfsItemS(pVecOut), alpha, phi, theta);
        rc = dfsStaticWrite(pVecOut, fpWr, data);
        dfsStaticDestroy(&pVecOut);
        dfsStaticDestroy(&pVecIn);
        countStatic++;
        free(data);
      }
    }

    void CopyTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr)
    {
      long current_tstep = 0;
      double      time;
      // Loop over the first 10 time steps
      int tstep_end = num_timesteps > 13 ? 13 : num_timesteps;
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
          if (is_time_equidistant)
            time *= tstep;
        }
        current_tstep++;
      }
    }

    void ReadTimeAxis(LPHEAD pdfsIn)
    {

      switch (time_axis_type = dfsGetTimeAxisType(pdfsIn))
      {
      case F_TM_EQ_AXIS: // Equidistant time axis
        is_time_equidistant = true;
        rc = dfsGetEqTimeAxis(pdfsIn, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
        break;
      case F_TM_NEQ_AXIS: // Non-equidistant time axis
        rc = dfsGetNeqTimeAxis(pdfsIn, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        break;
      case F_CAL_EQ_AXIS:  // Equidistant calendar axis
        is_time_equidistant = true;
        rc = dfsGetEqCalendarAxis(pdfsIn, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
        break;
      case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
        rc = dfsGetNeqCalendarAxis(pdfsIn, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        break;
      default:
        break;
      }
    }



    void FreeDataMemory()
    {
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;
    }

    /// path of the dfs file used as source.
    char* inputFullPath;

    GeoInfoType   geo_info_type;
    double      lon0, lat0;                      // Not used for dfsu files
    double      orientation;                     // Not used for dfsu files
    LPCTSTR     projection_id;                   // Projection string, either WKT or an abbreviation

    LONG        ntime_unit;                      // Time unit in time axis, EUM unit id
    LPCTSTR     ttime_Unit;                      // Time unit in time axis, EUM unit string
    TimeAxisType  time_axis_type;
    LPCTSTR     start_date, start_time;          // Start date and time for the calendar axes.
    double      tstart = 0;                      // Start time for the first time step in the file. 
    double      tstep = 0;                       // Time step size of equidistant axes
    double      tspan = 0;                       // Time span of non-equidistant axes
    LONG        num_timesteps = 0;               // Number of time steps in file
    LONG        index;                           // Index of first time step. Currently not used, always zero.
    BOOL        is_time_equidistant = false;
    LONG          item_type;                     // Item EUM type id
    LPCTSTR       item_type_str;                 // Name of item type
    LPCTSTR       item_name;                     // Name of item
    LONG          item_unit;                     // Item EUM unit id
    LPCTSTR       item_unit_str;                 // Item EUM unit string
    SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double
    float        **item_timestep_dataf;          // Time step data for all items - assuming float

    // file general data
    long           dataTypeIn;
    float          deleteF;
    double         deleteD;
    char           deleteByte;
    int            deleteInt;
    unsigned int   deleteUint;
    int            num_items;

    /// custom blocks
    float num_nodes = -1;
    float num_elmts = -1;
    float dimension = -1;
    float max_num_layers = 0;
    float num_sigma_layers = 0;
    long rc;
  };
}