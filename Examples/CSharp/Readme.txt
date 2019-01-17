This folder contains a number of files which each contains examples
for different tasks. The examples are in the form of static methods,
usually taking a filename as argument. For testing, one of the files
provided in the test data folder can be used.

- ExampleRun.cs : Example that can run one of the static methods, by adding
                  a main method. The ExampleRunBuild.bat builds that code
                  into an executable.

- ExampleDfs0.cs: Reading and creating a dfs0 file.

- ExampleDfs2.cs: Reading, modifying and creating dfs2 files. Also
                  examples of how to get from geographical coordinates
                  to 2D grid indices.

- ExampleDfsu.cs: Reading, modifying and creating dfsu files.

- ExampleMisc.cs: Examples of modifying the file info (header), custom
                  blocks, item info and temporal axis.

- MeshMerger.cs:  Example of program that merges two mesh files. 
                  The MeshMergerBuild.bat builds that code into an
                  executable.

- DfsDiff.cs:     Example of program that creates a new dfs file being
                  the difference of two other files.
                  The DfsDiffBuild.bat builds that code into an executable.

- DHI.NetCDF:     An example project compatible with Visual Studio and 
                  #Develop (SharpDevelop) that converts a text NetCDF file
                  into a DFS2 file. There is an NetCDFExample.txt that can
                  be used for testing.

- ExamplePFS.cs:  Examples of working with PFS files.
