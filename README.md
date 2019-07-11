# MIKE Core Examples
Examples of reading dfs files and pfs files in different environments.

Content:
- Examples: Examples in different programming languages
- bin: C# reference assemblies for building the C# solution. It is not sufficient for running the examples, it is required to install the MIKE SDK in order to run the examples
- TestData: Data of various formats.

Most examples are in C#, located in Examples/CSharp folder

## Introduction and Documentation
An introduction to MIKE Core and its libraries, including more documentation can be found on:

http://docs.mikepoweredbydhi.com/core_libraries/core-libraries/

## News for Release 2019
MIKE .NET assemblies are as of Release 2019 no longer installed in the Global Assembly Cache (GAC), but installed in the MIKE bin folder with the rest of the files.

To help script or tool to locate .NET assemblies in the MIKE bin folder, we have added a single assembly to the GAC which will do the trick. It is called `DHI.Mike.Install`. It is required to use one of the Setup methods from there before any usage of MIKE components in scripts or tools. In C# it looks like

```
MikeImport.SetupLatest(MikeProducts.MikeCore)
```

If more than one version of MIKE Software is installed, it is possible to load a specific version - with a bit of error checking:

```
if (!MikeImport.Setup(17, MikeProducts.MikeCore))
    throw new Exception();
```

In case of xcopy deployment, it is possible to specify the root explicitly:
```
MikeImport.SetupInstallRoot(@"C:\Program Files (x86)\DHI\2019");
```

This must be called before any method using MIKE components. From C# we recommend doing this in a static constructor, which will also make sure it is only done once.

Also, make sure to work in a 64 bit environment, since the MIKE Core components are only available in 64 bit versions.

The way that the .NET runtime searches for .NET assemblies is different from how a "normal"/unmanaged/C/C++ program searches for dll's and files. The .NET runtime does NOT use the PATH environment variable, so updating that does NOT help. Details on how the .NET runtime searches for assemblies can be found here:

https://docs.microsoft.com/en-us/dotnet/framework/deployment/how-the-runtime-locates-assemblies

