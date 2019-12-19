# MIKE Core Examples
Examples of reading dfs files and pfs files in different environments.

Content:
- Examples: Examples in different programming languages
- TestData: Data of various formats.

In the release zip file there is also (version 1.1.1 and later):
- bin: C#  assemblies and library files for building and running

Most examples are in C#, located in Examples/CSharp folder.

Some prerequisites:
- Be sure to use 64 bit. For scripting, use the 64 bit version of e.g. Python or MATLAB. When building in Visual Studio or similar, be sure to target x64.
- The "Microsoft Visual C++ Redistributable" is required. That is installed by all MIKE Products, and also by many other applications made by Visual Studio. If it is not installed, get it from [here](https://visualstudio.microsoft.com/downloads/) or [here](https://support.microsoft.com/en-gb/help/2977003/the-latest-supported-visual-c-downloads).

## Introduction and Documentation
An introduction to MIKE Core and its libraries, including more documentation can be found on:

http://docs.mikepoweredbydhi.com/core_libraries/core-libraries/

## News for Release 2019
MIKE .NET assemblies are as of Release 2019 no longer installed in the Global Assembly Cache (GAC), but installed in the MIKE bin folder with the rest of the installed files. This implies that MIKE .NET assemblies may not be found by your script or application.

In scripting environments, like Python and MATLAB, there are two options:
1. Get the DHI.MikeCore.Util release zip file, unzip it, and use the files in the bin folder.
2. Install a MIKE product (e.g. MIKE SDK), and use DHI.Mike.Install to locate the MIKE installation bin folder.

When building and deploying an application or a tool, there are two options:
1. Deploy your application and all the MIKE Core files together.
2. Deploy your application only, install a MIKE product (e.g. MIKE SDK) and use DHI.Mike.Install to locate the MIKE installation bin folder.

If building the application in Visual Studio, when referencing the MIKE Core components using NuGet packages, all the necessary MIKE Core files are copied automatically to the build output folder, so deploying the content of that folder should work.

If building the application in other ways, you can get the DHI.MikeCore.Util release zip file, unzip it, and include all  the files in the bin folder together with your application.

## Use DHI.Mike.Install to locate the MIKE installation bin folder.
To help script or application to locate .NET assemblies in the MIKE bin folder, we have added a single assembly to the GAC which will do the trick. It is called `DHI.Mike.Install`. It is required to use one of the Setup methods from there before any usage of MIKE components in scripts or tools. In C# it looks like

```
MikeImport.SetupLatest(MikeProducts.MikeCore)
```

If more than one version of MIKE Software is installed, it is possible to load a specific version - with a bit of error checking:

```
if (!MikeImport.Setup(17, MikeProducts.MikeCore))
    throw new Exception();
```

In case of xcopy deployment, it is possible to specify the root explicitly, as e.g.:
```
MikeImport.SetupInstallRoot(@"C:\Program Files (x86)\DHI\2019");
MikeImport.SetupInstallRoot(@"C:\folder\with\MIKE\Core\bin");
```

This must be called before any method using MIKE components. From C# we recommend doing this in a static constructor, which will also make sure it is only done once.

Also, make sure to work in a 64 bit environment, since the MIKE Core components are only available in 64 bit versions.

The way that the .NET runtime searches for .NET assemblies is different from how a "normal"/unmanaged/C/C++ program searches for dll's and files. The .NET runtime does NOT use the PATH environment variable, so updating that does NOT help. Details on how the .NET runtime searches for assemblies can be found here:

https://docs.microsoft.com/en-us/dotnet/framework/deployment/how-the-runtime-locates-assemblies

