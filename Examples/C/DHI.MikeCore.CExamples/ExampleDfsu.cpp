#include "pch.h"
#include "eum.h"
#include "dfsio.h"
#include "Util.h"
#include <CppUnitTest.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitestForC_MikeCore
{

  TEST_CLASS(Dfsu_tests)
  {
  public:

    /**
     * Geometry of mesh in DFSU file. For details, read the
     * "DFS Flexible File Formats, DFSU 2D/3D, Vertical Profile/Column, and Mesh File, Technical Documentation"
     * https://manuals.mikepoweredbydhi.help/2020/General/FM_FileSpecification.pdf
     * available from the "MIKE SDK Documentation Index"
     * https://manuals.mikepoweredbydhi.help/2020/MIKE_SDK.htm
     */
    struct MeshGeometry
    {
      MeshGeometry() = default;

      int num_nodes = 0;                ///< Number of nodes
      int num_elmts = 0;                ///< Number of elements
      int dimension = 0;                ///< Dimension of the file 
      int max_num_layers = 0;           ///< Maximum number of layers, vertical 
      int num_sigma_layers = 0;         ///< Number of sigma layers, vertical

      int num_conn = 0;                 ///< Size of elmt_conn array

      int*    node_ids = nullptr;       ///< Node Id's
      double* node_x = nullptr;         ///< X coordinates of nodes
      double* node_y = nullptr;         ///< Y Coordinates of nodes
      float*  node_z = nullptr;         ///< Z Coordinates of nodes
      int*    node_codes = nullptr;     ///< Node boundary code

      int*    elmt_ids = nullptr;       ///< Element Id's
      int*    elmt_types = nullptr;     ///< Element Type
      int*    elmt_num_nodes = nullptr; ///< Number of nodes in each element
      int*    elmt_conn = nullptr;      ///< Indices of nodes in each element
    };


    /**
     * Reads the file OresundHD.dfsu and create a gnuplot input file plotting the geometry 
     */
    TEST_METHOD(ReadDfsuFileTest)
    {

      LPCTSTR fileName = "OresundHD.dfsu";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      // Open file for reading
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);
      CheckRc(rc, "Error opening file");

      // Get some general information on the file
      int app_ver_no = dfsGetAppVersionNo(pdfs);
      float float_delete = dfsGetDeleteValFloat(pdfs);
      double double_delete = dfsGetDeleteValDouble(pdfs);
      int dfs_data_type = dfsGetDataType(pdfs);

      if (dfs_data_type != 2001)
      {
        LOG("This tool currently only supports standard 2D (horizontal) dfsu files\n");
        exit(-1);
      }

      /******************************************
       * Geographic information
       ******************************************/
      LPCTSTR projection_id;
      rc = GetDfsGeoInfoProjString(pdfs, &projection_id);
      CheckRc(rc, "Error reading Geographic information");
      LOG("Projection string: %s", projection_id);

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
        LOG("Time axis: Equidistant time: no_of_timesteps = %ld, tstep = %f", num_timesteps, tstep);
        break;
      case F_TM_NEQ_AXIS: // Non-equidistant time axis
        rc = dfsGetNeqTimeAxis(pdfs, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        CheckRc(rc, "Error reading Non-equidistant time axis");
        LOG("Time axis: Non-equidistant time: no_of_timesteps = %ld", num_timesteps);
        break;
      case F_CAL_EQ_AXIS:  // Equidistant calendar axis
        is_time_equidistant = true;
        rc = dfsGetEqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tstep, &num_timesteps, &index);
        CheckRc(rc, "Error reading Equidistant calendar axis");
        LOG("Time axis: Equidistant calendar: no_of_timesteps = %ld, start = %s %s, tstep = %f", num_timesteps, start_date, start_time, tstep);
        break;
      case F_CAL_NEQ_AXIS: // Non-equidistant calendar axis
        rc = dfsGetNeqCalendarAxis(pdfs, &start_date, &start_time, &ntime_unit, &ttime_Unit, &tstart, &tspan, &num_timesteps, &index);
        CheckRc(rc, "Error reading Non-equidistant calendar axis");
        LOG("Time axis: Non-equidistant calendar: no_of_timesteps = %ld, start = %s %s", num_timesteps, start_date, start_time);
        break;
      default:
        LOG("Error in time definition\n");
        exit(-1);
      }

      /***********************************
       * Dynamic item information
       ***********************************/
      int num_items = dfsGetNoOfItems(pdfs);
      LOG("Number of items in file: %d", num_items);

      LONG          item_type;                     // Item EUM type id
      LPCTSTR       item_type_str;                 // Name of item type
      LPCTSTR       item_name;                     // Name of item
      LONG          item_unit;                     // Item EUM unit id
      LPCTSTR       item_unit_str;                 // Item EUM unit string
      SimpleType    item_datatype;                 // Simple type stored in item, usually float but can be double
      float        **item_timestep_dataf;          // Time step data for all items - assuming float

      item_timestep_dataf  = new float*[num_items];
      LPCTSTR* item_names  = new LPCTSTR[num_items];

      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        // Name, quantity type and unit, and datatype
        rc = dfsGetItemInfo(dfsItemD(pdfs, i_item), &item_type, &item_type_str, &item_name, &item_unit, &item_unit_str, &item_datatype);
        CheckRc(rc, "Error reading dynamic item info");
        int item_num_elmts = dfsGetItemElements(dfsItemD(pdfs, i_item));
        LOG("Dynamic Item: %s, unit: %s,  %i elements", item_name, item_unit_str, item_num_elmts);

        // Store item names and create buffer for when reading data.
        item_names[i_item - 1] = item_name;
        item_timestep_dataf[i_item - 1] = new float[item_num_elmts];
      }

      /****************************************
       * Read DFSU mesh geometry - stored in custom block and static item
       ****************************************/
      MeshGeometry mesh;
      ReadDfsuGeometry(pdfs, fp, &mesh);
      MakeGnuPlotFile(inputFullPath, mesh);

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
        LOG("time = %lf", time);
        // Print out first item value for all items
        for (int i_item = 1; i_item <= num_items; i_item++)
        {
          LOG("  %20s = %f,", item_names[i_item-1], item_timestep_dataf[i_item-1][0]);
        }
        current_tstep++;
      }

      // Clean up
      Cleanup(&mesh);
      for (int i_item = 1; i_item <= num_items; i_item++)
      {
        delete[] item_timestep_dataf[i_item - 1];
      }
      delete[] item_timestep_dataf;

      // Close file and destroy header
      rc = dfsFileClose(pdfs, &fp);
      CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfs);
      CheckRc(rc, "Error destroying header");

    }


    /// Writes a copy of a dfsu 2D file from after reading its internal components.
    TEST_METHOD(CreateDfsu2DOdenseFromSourceTest)
    {
      LPCTSTR fileName = "OdenseHD2D.dfsu";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);

      LPCTSTR OutfileName = "test_OdenseHD2D_Ccreate.dfsu";
      char outputFullPath[_MAX_PATH];
      snprintf(outputFullPath, _MAX_PATH, "%s%s", TestDataPath(), OutfileName);

      CreateDfsu2DFromSource(inputFullPath, outputFullPath);
    }

    void CreateDfsu2DFromSource(LPCTSTR inputFullPath, LPCTSTR outputFullPath)
    {
      LPFILE      fpIn;
      LPHEAD      pdfsIn;
      long rc = dfsFileRead(inputFullPath, &pdfsIn, &fpIn);
      CheckRc(rc, "Error reading file");

      // Read a bit of data from the input file
      long num_items = dfsGetNoOfItems(pdfsIn);
      MeshGeometry mesh;
      ReadDfsuGeometry(pdfsIn, fpIn, &mesh);

      LPHEAD pdfsWr;
      rc = dfsHeaderCreate(FileType::F_EQTIME_FIXEDSPACE_ALLITEMS, 
                                "Area Series", "MIKE Core C SDK", 
                                1900, num_items, StatType::F_NO_STAT, &pdfsWr);
      CheckRc(rc, "Error creating header");
      // Data type is always 2001 for 2D dfsu files
      rc = dfsSetDataType(pdfsWr, 2001);
      CheckRc(rc, "Error setting data type");

      rc = dfsSetEqCalendarAxis(pdfsWr, "2002-01-03", "00:00:00", 1400, 0, 86400, 0);
      CheckRc(rc, "Error setting time axis");
      rc = dfsSetDeleteValFloat(pdfsWr, 1e-35f);
      CheckRc(rc, "Error setting deletevalue");
      rc = dfsSetGeoInfoUTMProj(pdfsWr, "UTM-33", 15, 0, 0);
      CheckRc(rc, "Error setting projection");

      /***********************************
       * Dynamic item information
       ***********************************/

      SetDfsDynamicItemInfo(pdfsWr, 1, "Surface elevation", 100078, 1000, UFS_FLOAT, mesh.num_elmts);
      SetDfsDynamicItemInfo(pdfsWr, 2, "Depth averaged U velocity", 100269, 2000, UFS_FLOAT, mesh.num_elmts);
      SetDfsDynamicItemInfo(pdfsWr, 3, "Depth averaged V velocity", 100270, 2000, UFS_FLOAT, mesh.num_elmts);

      /****************************************
       * Geometry sizes - write custom block "MIKE_FM"
       ****************************************/
      WriteDfsuGeometryHeader(pdfsWr, &mesh);

      /****************************************
       * Create the file
       ****************************************/
      LPFILE fpWr;
      rc = dfsFileCreate(outputFullPath, pdfsWr, &fpWr);
      CheckRc(rc, "Error creating file");

      /***********************************
       * Geometry information - write mesh to static item
       ***********************************/
      WriteDfsuGeometryStatic(pdfsWr, fpWr, &mesh);

      /***********************************
       * Write dynamic item-timestep data, copy from source file
       ***********************************/
      // All items are element based, so reuse elmtData array for all items
      float* item_timestep_dataf[3];
      float* elmtData = new float[mesh.num_elmts];
      item_timestep_dataf[0] = elmtData;
      item_timestep_dataf[1] = elmtData;
      item_timestep_dataf[2] = elmtData;

      CopyDfsTemporalData(pdfsIn, fpIn, pdfsWr, fpWr, item_timestep_dataf, 13, 3);

      /***********************************
       * Close file and destroy header
       ***********************************/
      rc = dfsFileClose(pdfsWr, &fpWr); CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfsWr);   CheckRc(rc, "Error destroying header");
      rc = dfsFileClose(pdfsIn, &fpIn); CheckRc(rc, "Error closing file");
      rc = dfsHeaderDestroy(&pdfsIn);   CheckRc(rc, "Error destroying header");
      Cleanup(&mesh);
      delete[] elmtData;

    }

    /**
     * Read Geometry from DFSU file:
     * Mesh sizes are read from custom block "MIKE_FM"
     * Mesh definition are read from static items
     */
    void ReadDfsuGeometry(LPHEAD pdfs, LPFILE fp, MeshGeometry* mesh)
    {
      // Get reference to the first custom block
      LPBLOCK customblock_ptr;
      long rc = dfsGetCustomBlockRef(pdfs, &customblock_ptr);
      CheckRc(rc, "Error reading custom block");
      // Search for "MIKE_FM" custom block containing int data
      while (customblock_ptr)
      {
        SimpleType csdata_type;      // Type of data stored in custom block
        LPCTSTR name;                // Name of custom block
        LONG size;                   // Number of values in custom block
        void* customblock_data_ptr;  // custom block data, updated with every call below to next custom block
        rc = dfsGetCustomBlock(customblock_ptr, &csdata_type, &name,
                               &size, &customblock_data_ptr, &customblock_ptr);
        CheckRc(rc, "Error reading custom block");
        if (0 == strcmp(name, "MIKE_FM") && csdata_type == UFS_INT)
        {
          int* intData         = (int*)customblock_data_ptr;
          mesh->num_nodes      = intData[0];
          mesh->num_elmts      = intData[1];
          mesh->dimension      = intData[2];
          mesh->max_num_layers = intData[3];
          if (size < 5)
            mesh->num_sigma_layers = mesh->max_num_layers;
          else
            mesh->num_sigma_layers = intData[4];
          break;
        }
      }
      if (mesh->num_nodes < 0)
      {
        Logger::WriteMessage("Error in Geometry definition: Could not find custom block \"MIKE_FM\"\n");
        exit(-1);
      }
      if (mesh->dimension != 2 || mesh->max_num_layers > 0 || mesh->num_sigma_layers > 0)
      {
        Logger::WriteMessage("This tool currently only supports standard 2D (horizontal) dfsu files\n");
        exit(-1);
      }

      // Read mesh geometry from static items in DFSU file
      mesh->node_ids       = (int*)    ReadDfsStaticItem(fp, pdfs, "Node id", UFS_INT);
      mesh->node_x         = (double*) ReadDfsStaticItem(fp, pdfs, "X-coord", UFS_DOUBLE);
      mesh->node_y         = (double*) ReadDfsStaticItem(fp, pdfs, "Y-coord", UFS_DOUBLE);
      mesh->node_z         = (float*)  ReadDfsStaticItem(fp, pdfs, "Z-coord", UFS_FLOAT);
      mesh->node_codes     = (int*)    ReadDfsStaticItem(fp, pdfs, "Code", UFS_INT);

      mesh->elmt_ids       = (int*)    ReadDfsStaticItem(fp, pdfs, "Element id", UFS_INT);
      mesh->elmt_types     = (int*)    ReadDfsStaticItem(fp, pdfs, "Element type", UFS_INT);
      mesh->elmt_num_nodes = (int*)    ReadDfsStaticItem(fp, pdfs, "No of nodes", UFS_INT);
      mesh->elmt_conn      = (int*)    ReadDfsStaticItem(fp, pdfs, "Connectivity", UFS_INT, &(mesh->num_conn));
    }

    /**
     * Write Geometry to DFSU file:
     * Mesh sizes are written to custom block "MIKE_FM"
     */
    void WriteDfsuGeometryHeader(LPHEAD pdfs, MeshGeometry* mesh)
    {
      int custblock_data[5];
      custblock_data[0] = mesh->num_nodes;
      custblock_data[1] = mesh->num_elmts;
      custblock_data[2] = mesh->dimension;
      custblock_data[3] = mesh->max_num_layers;
      custblock_data[4] = mesh->num_sigma_layers;
      long rc = dfsAddCustomBlock(pdfs, UFS_INT, "MIKE_FM", 5, custblock_data);
      CheckRc(rc, "Error adding MIKE_FM Custom block");
    }

    /**
     * Write Geometry to DFSU file:
     * Mesh definition are written to static items
     */
    void WriteDfsuGeometryStatic(LPHEAD pdfs, LPFILE fp, MeshGeometry* mesh)
    {
      // Write mesh geometry from static items in DFSU file
      WriteDfsStaticItem(fp, pdfs, "Node id"     , UFS_INT   , mesh->num_nodes, mesh->node_ids      );
      WriteDfsStaticItem(fp, pdfs, "X-coord"     , UFS_DOUBLE, mesh->num_nodes, mesh->node_x        );
      WriteDfsStaticItem(fp, pdfs, "Y-coord"     , UFS_DOUBLE, mesh->num_nodes, mesh->node_y        );
      WriteDfsStaticItem(fp, pdfs, "Z-coord"     , UFS_FLOAT , mesh->num_nodes, mesh->node_z        );
      WriteDfsStaticItem(fp, pdfs, "Code"        , UFS_INT   , mesh->num_nodes, mesh->node_codes    );

      WriteDfsStaticItem(fp, pdfs, "Element id"  , UFS_INT   , mesh->num_elmts, mesh->elmt_ids      );
      WriteDfsStaticItem(fp, pdfs, "Element type", UFS_INT   , mesh->num_elmts, mesh->elmt_types    );
      WriteDfsStaticItem(fp, pdfs, "No of nodes" , UFS_INT   , mesh->num_elmts, mesh->elmt_num_nodes);
      WriteDfsStaticItem(fp, pdfs, "Connectivity", UFS_INT   , mesh->num_conn,  mesh->elmt_conn     );
    }


    /***
     * Example of how to navigate the Geometry of a DFSU file.
     * This method writes the DFSU mesh to a text file in a gnuplot compatible format
     */
    void MakeGnuPlotFile(char* inputFullPath, MeshGeometry mesh)
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
      for (int i = 0; i < mesh.num_elmts; i++)
      {
        fprintf(fgp_ptr, "# Element %6d, id = %6d\n", i + 1, mesh.elmt_ids[i]);
        int num_nodes_in_elmt = mesh.elmt_num_nodes[i];
        // Loop over all nodes in element, print out node coordinate for all nodes in the element
        for (int j = 0; j < num_nodes_in_elmt; j++)
        {
          // Lookup nodes of element in connectivity table (elmt_conn).
          // The elmt_conn is 1-based, so subtract 1 to get zero-based indices
          nodeIndex = mesh.elmt_conn[curr_elmt_conn_index + j] - 1;
          fprintf(fgp_ptr, "%f %f\n", mesh.node_x[nodeIndex], mesh.node_y[nodeIndex]);
        }
        // Print out the first element-node coordinate again, to close the polygon
        nodeIndex = mesh.elmt_conn[curr_elmt_conn_index] - 1;
        fprintf(fgp_ptr, "%f %f\n", mesh.node_x[nodeIndex], mesh.node_y[nodeIndex]);
        // Empty line to tell gnuplot that a new polygon is coming
        fprintf(fgp_ptr, "\n");
        // Prepare for next element
        curr_elmt_conn_index += num_nodes_in_elmt;
      }

      fclose(fgp_ptr);
    }

    void Cleanup(MeshGeometry* mesh)
    {
      free(mesh->node_ids);
      free(mesh->node_x);
      free(mesh->node_y);
      free(mesh->node_z);
      free(mesh->node_codes);
      free(mesh->elmt_ids);
      free(mesh->elmt_types);
      free(mesh->elmt_num_nodes);
      free(mesh->elmt_conn);
    }

  };
}