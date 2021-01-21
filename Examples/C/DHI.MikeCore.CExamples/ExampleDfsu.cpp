#include "pch.h"
#include "eum.h"
#include "dfsio.h"
#include "Util.h"
#include <CppUnitTest.h>


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

/// since it is not easy to get relative paths relative to folder located above of the directory tree structure, 
/// a simple concatenation of strings would do the trick
/// replace this path with the corresponding location of the files in your PC
LPCTSTR TestDataFolder = "C:\\Users\\ejq\\OneDrive - DHI\\Documents\\Development\\Rel2021.1\\2021-03\\18507\\Last\\MIKECore-Examples\\TestData\\";

namespace UnitestForC_MikeCore
{

  TEST_CLASS(Dfsu_tests)
  {
  public:
    /// This method reads the file OresundHD.dfsu and create a gnuplot input file plotting the geometry 
    TEST_METHOD(ReadDfsuFileTest)
    {
      LPCTSTR fileName = "OresundHD.dfsu";
      inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      // Open file for reading
      LPFILE      fp;
      LPHEAD      pdfs;
      rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      // Get some general information on the file
      int app_ver_no = dfsGetAppVersionNo(pdfs);
      float float_delete = dfsGetDeleteValFloat(pdfs);
      double double_delete = dfsGetDeleteValDouble(pdfs);
      int dfs_data_type = dfsGetDataType(pdfs);

      /******************************************
       * Geographic information
       ******************************************/
      num_items = dfsGetNoOfItems(pdfs);
      ReadDfsDataTypes(pdfs);
      ReadDfsGeoInfo(pdfs);

      /****************************************
       * Time axis information - almost all dfsu files are F_CAL_EQ_AXIS
       ****************************************/
      ReadTimeAxis(pdfs);

      /***********************************
       * Dynamic item information
       ***********************************/
      ReadDynamicItemInfo(pdfs);


      /****************************************
       * Geometry sizes - read from custom block "MIKE_FM"
       ****************************************/
      ReadMikeFMCustomBlocks(pdfs);
      MakeGnuPlotFile(pdfs, fp);


      /*****************************
       * Time loop
       *****************************/
      ReadTemporalData(pdfs, fp);

      // Close file and destroy header
      FreeDataMemory();
      rc = dfsFileClose(pdfs, &fp);
      CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfs);
      CheckRc(rc, "Error destroying header");
    }

    /// write a copy of a dfsu 2D file from after reading its internal components.
    TEST_METHOD(CreateDfsu2DOdenseTest)
    {
      LPCTSTR fileName = "OdenseHD2D.dfsu";
      inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      LPCTSTR OutfileName = "OdenseHD2D_created.dfsu";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);
      CreateDfsu2DFromSource(outputFullPath);
      delete[] outputFullPath;
    }

    void CreateDfsu2DFromSource(LPCTSTR outputFullPath)
    {
      // dfsDebugOn(true);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      CreateDfsuFileFromSource(outputFullPath, pdfs, fp);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      dfsHeaderDestroy(&pdfs);
    }

    void CreateDfsuFileFromSource(LPCTSTR outputFullPath, LPHEAD  pdfsIn, LPFILE fpIn)
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

      //create the file to write using the pointers to header
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

    void ReadDynamicItemInfo(LPHEAD pdfs)
    {
      char buff[100];
      snprintf(buff, sizeof(buff), "Number of items in file: %d", num_items);
      Logger::WriteMessage(buff);
      // Buffer arrays, used when reading data. dfsu always stores floats
      item_timestep_dataf = new float*[num_items];

      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        // Name, quantity type and unit, and datatype
        rc = dfsGetItemInfo(dfsItemD(pdfs, i_item), &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        CheckRc(rc, "Error reading dynamic item info");
        int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, i_item));
        snprintf(buff, sizeof(buff), "Dynamic Item: %s, unit: %s,  %i elements\n", item_name, item_unit_str, item_num_elmts);
        Logger::WriteMessage(buff);

        // Create buffer for when reading data.
        item_timestep_dataf[i_item - 1] = new float[item_num_elmts];
      }
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

    void WriteDfsDataTypes(LPHEAD pdfsWr)
    {
      rc = dfsSetDataType(pdfsWr, dataTypeIn);
      rc = dfsSetDeleteValFloat(pdfsWr, deleteF);
      rc = dfsSetDeleteValDouble(pdfsWr, deleteD);
      rc = dfsSetDeleteValByte(pdfsWr, deleteByte);
      rc = dfsSetDeleteValInt(pdfsWr, deleteInt);
      rc = dfsSetDeleteValUnsignedInt(pdfsWr, deleteUint);
    }

    void ReadTemporalData(LPHEAD pdfs, LPFILE fp)
    {
      // Data are stored in the file in item major order
      // i.e. for each time step, all items are stored in order.
      // To read specific time steps or items, you reposition the file pointer using:
      //   dfsFindTimeStep(pdfs, fp, timestepIndex);
      //   dfsFindItemDynamic(pdfs, fp, timestepIndex, itemNumber);
      // The first will position the file pointer at the first item of that timestep
      // The second will position the file pointer at the specified timestep and item

      char buff[100];
      long current_tstep = 0;
      double      time;
      // Loop over the first 10 time steps
      int tstep_end = num_timesteps > 10 ? 10 : num_timesteps;
      while (current_tstep < tstep_end)
      {
        // Loop over all items
        for (int i_item = 1; i_item <= num_items; i_item++)
        {
          // Name, quantity type and unit, and datatype
          // Read item-timestep where the file pointer points to,
          // and move the filepointer to the next item-timestep
          rc = dfsReadItemTimeStep(pdfs, fp, &time, item_timestep_dataf[i_item - 1]);
          CheckRc(rc, "Error reading dynamic item dCopyDynamicItemInfoata");
          // If the temporal axis is equidistant, the time variable is the timestep index value.
          // If temporal axis is non-equidistant, this is the time from start of the file
          if (is_time_equidistant)
            time *= tstep;
        }

        // Print out time of time step, relative to start time and in time unit of axis
        snprintf(buff, sizeof(buff), "time = %lf", time);
        Logger::WriteMessage(buff);
        // Print out first item value for all items
        for (int i_item = 1; i_item <= num_items; i_item++)
        {
          rc = dfsGetItemInfo(dfsItemD(pdfs, i_item), &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
          snprintf(buff, sizeof(buff), "%s: %f,", item_name, item_timestep_dataf[i_item - 1][0]);
          Logger::WriteMessage(buff);
        }
        current_tstep++;
      }
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

    void ReadMikeFMCustomBlocks(LPHEAD pdfsIn)
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
        if (0 == strcmp(name, "MIKE_FM") && csdata_type == UFS_INT)
        {
          int* intData = (int*)customblock_data_ptr;
          num_nodes = intData[0];
          num_elmts = intData[1];
          dimension = intData[2];
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
        Logger::WriteMessage("Error in Geometry definition: Could not find custom block \"MIKE_FM\"\n");
        exit(-1);
      }
      if (dimension != 2 || max_num_layers > 0 || num_sigma_layers > 0)
      {
        Logger::WriteMessage("This tool currently only supports standard 2D (horizontal) dfsu files\n");
        exit(-1);
      }
    }

    void CopyDfsuStaticInfo(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr)
    {
      // Read DFSU geometry from static items in DFSU file
      int countStatic = 1;
      while (countStatic < 10)
      {
        void* data;
        LPVECTOR pVecIn = 0;
        pVecIn = dfsStaticRead(fpIn, &rc);
        LPITEM static_item = dfsItemS(pVecIn);
        // Read static item
        data = malloc(dfsGetItemBytes(static_item));
        rc = dfsStaticGetData(pVecIn, data);
        rc = dfsGetItemInfo(static_item, &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        SpaceAxisType  axisStaticIn = dfsGetItemAxisType(static_item);
        long eumStaticUnit;
        LPCTSTR eumUnitStr;
        long jStatic;
        float x0Static, dxStatic;
        rc = dfsGetItemAxisEqD1(static_item, &eumStaticUnit, &eumUnitStr, &jStatic, &x0Static, &dxStatic);
        float x, y, z;
        rc = dfsGetItemRefCoords(static_item, &x, &y, &z);
        float alpha, phi, theta;
        rc = dfsGetItemAxisOrientation(static_item, &alpha, &phi, &theta);
        LPVECTOR pVecOut = 0;
        long io_error = dfsStaticCreate(&pVecOut);
        rc = dfsSetItemInfo_(pdfsWr, dfsItemS(pVecOut), item_type, item_name, eumUnitStr, item_datatype);
        dfsSetItemAxisEqD1(dfsItemS(pVecOut), eumStaticUnit, jStatic, x0Static, dxStatic);
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

    void MakeGnuPlotFile(LPHEAD pdfs, LPFILE fp)
    {
      // Read DFSU geometry from static items in DFSU file
      int*    node_ids = (int*)readStaticItem(fp, pdfs, "Node id", UFS_INT);
      double* node_x = (double*)readStaticItem(fp, pdfs, "X-coord", UFS_DOUBLE);
      double* node_y = (double*)readStaticItem(fp, pdfs, "Y-coord", UFS_DOUBLE);
      float*  node_z = (float*)readStaticItem(fp, pdfs, "Z-coord", UFS_FLOAT);
      int*    node_codes = (int*)readStaticItem(fp, pdfs, "Code", UFS_INT);

      int*    elmt_ids = (int*)readStaticItem(fp, pdfs, "Element id", UFS_INT);
      int*    elmt_types = (int*)readStaticItem(fp, pdfs, "Element type", UFS_INT);
      int*    elmt_num_nodes = (int*)readStaticItem(fp, pdfs, "No of nodes", UFS_INT);
      int*    elmt_conn = (int*)readStaticItem(fp, pdfs, "Connectivity", UFS_INT);

      /***********************************
       * Print out element polygons to file that can be plotted in gnuplot
       ***********************************/
       // Create filename for gnuplot element output
      int fnLen = lstrlen(inputFullPath);
      char filename_gnuplot[MAX_PATH];
      strcpy(filename_gnuplot, inputFullPath);
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
      for (int i = 0; i < num_elmts; i++)
      {
        fprintf(fgp_ptr, "# Element %6d, id = %6d\n", i + 1, elmt_ids[i]);
        int num_nodes_in_elmt = elmt_num_nodes[i];
        // Loop over all nodes in element, print out node coordinate for all nodes in the element
        for (int j = 0; j < num_nodes_in_elmt; j++)
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
        curr_elmt_conn_index += num_nodes_in_elmt;
      }

      fclose(fgp_ptr);

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
    }


    void FreeDataMemory()
    {
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;

      delete[] inputFullPath;
    }

    /// data that could be used as class variables
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