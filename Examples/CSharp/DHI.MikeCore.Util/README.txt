
Usage
    DHI.MikeCore.Util -[tool] [arguments]

Tools:

    -MaxVelocityField: Create maximum velocity field for a dfs2 file

        DHI.MikeCore.Util -MaxVelocityField [sourceFilename] [outputFilename]

        From a dfs2 file containing items (H-P-Q), (P-Q-Speed) or  (u-v-Speed), 
        find maximum velocity for each cell and store in [outputFilename]

    -Resample: Resample dfs2 file in x/y space

        DHI.MikeCore.Util -Resample [inputFilename] [outputFilename] [xCount] [yCount]

        Resample the [inputFilename] to contain [xCount] x [yCount] number of cells, and
        store the result in [outputFilename]

    -dfsuExtractLayer: Extract layer from 3D dfsu

        DHI.MikeCore.Util -dfsuExtractLayer [filenameDfsu3] [outputFilenameDfsu2] [layerNumber]

        Extract a single layer from a 3D dfsu file, and write it to a 2D dfsu file.

        [LayerNumber] is the layer to extract
          - Positive values count from bottom up i.e. 1 is bottom layer, 2 is 
            second layer from bottom etc.
          - Negative values count from top down, i.e. -1 is toplayer, -2 is 
            second layer from top etc.

        If a layer value does not exist for a certain 2D element, delete value is written
        to the 2D resut file. This is relevant for Sigma-Z type of files.

    -dfsuExtractSubArea: Extract subarea of dfsu file

        DHI.MikeCore.Util -dfsuExtractSubArea [sourceFilename] [outputFilename] [x1] [y2] [x2] [y2]

        Extract sub-area of dfsu (2D) file to a new dfsu file

        (x1,y1) is coordiante for lower left corner of sub area
        (x2,y2) is coordiante for upper right corner of sub area

