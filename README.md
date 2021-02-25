# MIKE Core Examples
Examples of reading dfs files and pfs files in different environments.

Content:
- Examples: Examples in different programming languages
- TestData: Data of various formats.

In the release zip file there is also (version 1.1.1 and later):
- bin: .NET assemblies and native libraries and files for building and running.

Most examples are in C#, located in Examples/CSharp folder.

Some prerequisites:
- Be sure to use 64 bit. For scripting, use the 64 bit version of e.g. Python or MATLAB. When building in Visual Studio or similar, be sure to target x64.
- The "Microsoft Visual C++ Redistributable" is required. That is installed by all MIKE Products, and also by many other applications made by Visual Studio. If it is not installed, get it from [here](https://visualstudio.microsoft.com/downloads/) or [here](https://support.microsoft.com/en-gb/help/2977003/the-latest-supported-visual-c-downloads).

## Introduction and Documentation
An introduction to MIKE Core and its libraries, including more documentation can be found on:

http://docs.mikepoweredbydhi.com/core_libraries/core-libraries/

## Changelog

For changes to MIKE Core components in general, visit:
http://docs.mikepoweredbydhi.com/core_libraries/core-changelog/

The list here contains only major updates to the MIKE Core Examples.

- 2021-02-09: 
    - Adding C/C++ examples. 
    - Updating NuGet to version 19.0 (MIKE Release 2021)
- 2019-05-06: 
    - Updating to version 18.1 (MIKE Release 2020 Update 1)
- 2019-12-20: 
    - Update references to use NuGet packages. 
    - Updating to version 18.0 (MIKE Release 2020)

## Usage of MIKE Core in .NET environments

MIKE Core .NET assemblies are (as of Release 2019 an onwards) installed in the MIKE installation bin folder. This implies that MIKE .NET assemblies may not be found by your script or application.

In scripting environments, like Python and MATLAB, there are two options:
1. Get the DHI.MikeCore.Util release zip file, unzip it, and use the files in the bin folder.
2. Install a MIKE product (e.g. MIKE SDK), and use `DHI.Mike.Install` to locate the MIKE installation bin folder.

When building and deploying an application or a tool, there are two options:
1. Deploy your application and all the MIKE Core files together.
2. Deploy your application only, install a MIKE product (e.g. MIKE SDK) and use DHI.Mike.Install to locate the MIKE installation bin folder.

If building the application in Visual Studio, when referencing the MIKE Core components using NuGet packages, all the necessary MIKE Core files are copied automatically to the build output folder, so deploying the content of that folder should work.

If building the application in other ways, you can get the DHI.MikeCore.Util release zip file, unzip it, and include all  the files in the bin folder together with your application.

## Use DHI.Mike.Install to locate the MIKE installation bin folder.

Check out (DHI.Mike.Install)[http://docs.mikepoweredbydhi.com/core_libraries/core-mikeinstall/]
on how to locate the MIKE installation bin folder.
