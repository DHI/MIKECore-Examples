Files with the .cs extension in this folder can be run in two ways:
- Compile from the command line and then run.
- Run as script directly from the command line.


Compile from the command line and then run
------------------------------------------
This is supported by the .NET framework directly, no other tools 
need to be installed.

Example of compiling and running can be found in the 
  ConsoleAppExample.bat

  
Run as script directly from the command line
--------------------------------------------
A C#-script can run either from a command line, or from a tool 
supporting invokation of external script engines. A C#-script engine
compiles the code and executes it directly, without explicitly
creating a .exe file on the hard disk.

The scripting engine cs-script can be used for this. Installation 
instructions:
- Download and install the cs-script scripting engine from 
    http://www.csscript.net/
- Make sure the cs-script bin folder is in the environment search path

Then the scripts can be run as:
    css UpdateItemInfo.cs