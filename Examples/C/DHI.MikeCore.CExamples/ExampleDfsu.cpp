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
    struct DfsuStaticData
    {
      int*    node_ids;
      double* node_x;
      double* node_y;
      float*  node_z;
      int*    node_codes;

      int*    elmt_ids;
      int*    elmt_types;
      int*    elmt_num_nodes;
      int*    elmt_conn;
    };

    struct DfsuCustomBlock
    {
      float num_nodes;
      float num_elmts;
      float dimension;
      float max_num_layers;
      float num_sigma_layers;
    };

    /// This method reads the file OresundHD.dfsu and create a gnuplot input file plotting the geometry 
    TEST_METHOD(ReadDfsuFileTest)
    {
      LPCTSTR fileName = "OresundHD.dfsu";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      // Open file for reading
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      // Get some general information on the file
      int app_ver_no = dfsGetAppVersionNo(pdfs);
      float float_delete = dfsGetDeleteValFloat(pdfs);
      double double_delete = dfsGetDeleteValDouble(pdfs);
      int dfs_data_type = dfsGetDataType(pdfs);

      /******************************************
       * Geographic information
       ******************************************/
      num_items = dfsGetNoOfItems(pdfs);
      float** item_timestep_dataf = new float*[num_items];
      DeleteValues delVals;
      ReadDfsDeleteVals(pdfs, &delVals);
      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      rc = GetDfsGeoInfo(pdfs, &projection_id, &lon0, &lat0, &orientation);

      /****************************************
       * Time axis information - almost all dfsu files are F_CAL_EQ_AXIS
       ****************************************/
      LPCTSTR start_date;
      long num_timesteps;
      LPCTSTR start_time;
      double tstart;
      double tstep;
      double tspan;
      long neum_unit;
      long index;
      ReadTimeAxis(pdfs, &start_date, &num_timesteps, &start_time, &tstart, &tstep, &tspan, &neum_unit, &index);

      /***********************************
       * Dynamic item information
       ***********************************/
      ReadDynamicItemInfo(pdfs, item_timestep_dataf);


      /****************************************
       * Geometry sizes - read from custom block "MIKE_FM"
       ****************************************/
      DfsuCustomBlock custBlock;
      ReadMikeFMCustomBlocks(pdfs, &custBlock);
      DfsuStaticData staticData;
      ReadStaticItems(pdfs, fp, &staticData);
      MakeGnuPlotFile(inputFullPath, staticData, custBlock);


      /*****************************
       * Time loop
       *****************************/
      ReadTemporalData(pdfs, fp, item_timestep_dataf, num_timesteps);

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfs);
      CheckRc(rc, "Error destroying header");
      delete[] inputFullPath;
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;
    }

    /// write a copy of a dfsu 2D file from after reading its internal components.
    TEST_METHOD(CreateDfsu2DOdenseFromSourceTest)
    {
      LPCTSTR fileName = "OdenseHD2D.dfsu";
      char* inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);

      LPCTSTR OutfileName = "test_OdenseHD2D_FromSource.dfsu";
      char* outputFullPath = new char[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataFolder, OutfileName);
      CreateDfsu2DFromSource(inputFullPath, outputFullPath);
      delete[] outputFullPath;
      delete[] inputFullPath;
    }

    void CreateDfsu2DFromSource(LPCTSTR inputFullPath, LPCTSTR outputFullPath)
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


      LPHEAD pdfsWr;
      LPFILE fpWr;
      /***********************************
       * Dynamic item information
       ***********************************/
      num_items = dfsGetNoOfItems(pdfsIn);
      float** item_timestep_dataf = new float*[num_items];
      CopyHeader(pdfsIn, &pdfsWr, num_items);
      long num_timesteps;
      CopyTimeAxis(pdfsIn, pdfsWr, &num_timesteps);

      DeleteValues delVals;
      ReadDfsDeleteVals(pdfsIn, &delVals);
      WriteDfsDeleteVals(pdfsWr, delVals);

      LPCTSTR projection_id;
      double lon0, lat0, orientation;
      long rc = GetDfsGeoInfo(pdfsIn, &projection_id, &lon0, &lat0, &orientation);
      rc = dfsSetGeoInfoUTMProj(pdfsWr, projection_id, lon0, lat0, orientation);

      CopyDynamicItemInfo(pdfsIn, pdfsWr, item_timestep_dataf, num_items);

      /****************************************
       * Geometry sizes - read from custom block "MIKE_FM"
       ****************************************/
      CopyDfsCustomBlocks(pdfsIn, pdfsWr);

      //create the file to write using the pointers to header
      rc = dfsFileCreateEx(outputFullPath, pdfsWr, &fpWr, true);

      /***********************************
       * Geometry information
       ***********************************/

      CopyDfsStaticInfo(pdfsIn, fpIn, pdfsWr, fpWr);

      CopyTemporalData(pdfsIn, fpIn, pdfsWr, fpWr, item_timestep_dataf, num_timesteps);
      // Close file and destroy header
      rc = dfsFileClose(pdfsWr, &fpWr);
      dfsHeaderDestroy(&pdfsWr);
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete item_timestep_dataf[i_item - 1];
      }
      delete item_timestep_dataf;
    }

    //void CreateHeader(LPHEAD pdfsIn, LPHEAD* pdfsWr)
    //{
    //  FileType ft = FileType::F_EQTIME_FIXEDSPACE_ALLITEMS;
    //  LPCTSTR title = "";
    //  LPCTSTR appTitle = dfsGetAppTitle(pdfsIn);
    //  StatType statT = StatType::F_NO_STAT;
    //  long rc = dfsHeaderCreate(ft, title, appTitle, 0, num_items, statT, pdfsWr);
    //}

    void ReadDynamicItemInfo(LPHEAD pdfs, float** item_timestep_dataf)
    {
      long rc;
      char buff[100];
      snprintf(buff, sizeof(buff), "Number of items in file: %d", num_items);
      Logger::WriteMessage(buff);
      // Buffer arrays, used when reading data. dfsu always stores floats
      item_timestep_dataf = new float*[num_items];
      long item_type, item_unit;
      LPCTSTR item_type_str, item_name, item_unit_str;
      SimpleType item_datatype;
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

    void ReadTemporalData(LPHEAD pdfs, LPFILE fp, float** item_timestep_dataf, long num_timesteps)
    {
      // Data are stored in the file in item major order
      // i.e. for each time step, all items are stored in order.
      // To read specific time steps or items, you reposition the file pointer using:
      //   dfsFindTimeStep(pdfs, fp, timestepIndex);
      //   dfsFindItemDynamic(pdfs, fp, timestepIndex, itemNumber);
      // The first will position the file pointer at the first item of that timestep
      // The second will position the file pointer at the specified timestep and item
      long rc;
      char buff[100];
      long current_tstep = 0;
      double      time;
      // Loop over the first 10 time steps
      int tstep_end = num_timesteps > 10 ? 10 : num_timesteps;
      long item_type, item_unit;
      LPCTSTR item_type_str, item_name, item_unit_str;
      SimpleType item_datatype;
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

    void ReadMikeFMCustomBlocks(LPHEAD pdfsIn, DfsuCustomBlock* custBlock)
    {
      LPBLOCK customblock_ptr;
      long rc = dfsGetCustomBlockRef(pdfsIn, &customblock_ptr);
      CheckRc(rc, "Error reading custom block");
      // Search for "MIKE_FM" custom block containing int data
    //  int num_nodes, num_elmts;
    //  int num_sigma_layers, dimension, max_num_layers;
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
          custBlock->num_nodes = intData[0];
          custBlock->num_elmts = intData[1];
          custBlock->dimension = intData[2];
          int max_num_layers = intData[3];
          if (size < 5)
            custBlock->num_sigma_layers = max_num_layers;
          else
            custBlock->num_sigma_layers = intData[4];
          break;
        }
      }
      if (custBlock->num_nodes < 0)
      {
        Logger::WriteMessage("Error in Geometry definition: Could not find custom block \"MIKE_FM\"\n");
        exit(-1);
      }
      if (custBlock->dimension != 2 || custBlock->max_num_layers > 0 || custBlock->num_sigma_layers > 0)
      {
        Logger::WriteMessage("This tool currently only supports standard 2D (horizontal) dfsu files\n");
        exit(-1);
      }
    }

    void CopyTemporalData(LPHEAD pdfsIn, LPFILE fpIn, LPHEAD pdfsWr, LPFILE fpWr, float** item_timestep_dataf, long num_timesteps)
    {
      long rc;
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
          //if (is_time_equidistant)
          //  time *= tstep;
        }
        current_tstep++;
      }
    }


    void ReadStaticItems(LPHEAD pdfs, LPFILE fp, DfsuStaticData* staticData)
    {
      // Read DFSU geometry from static items in DFSU file
      staticData->node_ids = (int*)readStaticItem(fp, pdfs, "Node id", UFS_INT);
      staticData->node_x = (double*)readStaticItem(fp, pdfs, "X-coord", UFS_DOUBLE);
      staticData->node_y = (double*)readStaticItem(fp, pdfs, "Y-coord", UFS_DOUBLE);
      staticData->node_z = (float*)readStaticItem(fp, pdfs, "Z-coord", UFS_FLOAT);
      staticData->node_codes = (int*)readStaticItem(fp, pdfs, "Code", UFS_INT);

      staticData->elmt_ids = (int*)readStaticItem(fp, pdfs, "Element id", UFS_INT);
      staticData->elmt_types = (int*)readStaticItem(fp, pdfs, "Element type", UFS_INT);
      staticData->elmt_num_nodes = (int*)readStaticItem(fp, pdfs, "No of nodes", UFS_INT);
      staticData->elmt_conn = (int*)readStaticItem(fp, pdfs, "Connectivity", UFS_INT);
    }

    void MakeGnuPlotFile(char* inputFullPath, DfsuStaticData staticData, DfsuCustomBlock custBlock)
    {

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
      for (int i = 0; i < custBlock.num_elmts; i++)
      {
        fprintf(fgp_ptr, "# Element %6d, id = %6d\n", i + 1, staticData.elmt_ids[i]);
        int num_nodes_in_elmt = staticData.elmt_num_nodes[i];
        // Loop over all nodes in element, print out node coordinate for all nodes in the element
        for (int j = 0; j < num_nodes_in_elmt; j++)
        {
          // Lookup nodes of element in connectivity table (elmt_conn).
          // The elmt_conn is 1-based, so subtract 1 to get zero-based indices
          nodeIndex = staticData.elmt_conn[curr_elmt_conn_index + j] - 1;
          fprintf(fgp_ptr, "%f %f\n", staticData.node_x[nodeIndex], staticData.node_y[nodeIndex]);
        }
        // Print out the first element-node coordinate again, to close the polygon
        nodeIndex = staticData.elmt_conn[curr_elmt_conn_index] - 1;
        fprintf(fgp_ptr, "%f %f\n", staticData.node_x[nodeIndex], staticData.node_y[nodeIndex]);
        // Empty line to tell gnuplot that a new polygon is coming
        fprintf(fgp_ptr, "\n");
        // Prepare for next element
        curr_elmt_conn_index += num_nodes_in_elmt;
      }

      fclose(fgp_ptr);
    }

    /// file deleteValues
    long           dataTypeIn;
    float          deleteF;
    double         deleteD;
    char           deleteByte;
    int            deleteInt;
    unsigned int   deleteUint;
    int            num_items;
  };
}