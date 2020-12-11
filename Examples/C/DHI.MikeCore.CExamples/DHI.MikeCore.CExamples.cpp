#include "pch.h"
#include <iostream>
#include "eum.h"
#include "dfsio.h"
#include "ExampleDfs.h"
#include "ExampleDfsu.h"


int LastIndexOf(LPCTSTR s1, char c)
{
  int l = lstrlen(s1);
  for (int i = l-1;i>=0;i--)
  {
    if (s1[i] == c)
      return i;
  }
  return -1;
}


/******************************************
 * Example of how to generally read data from a dfs file, 
 * especially dfs0, dfs1 and dfs2 files.
 ******************************************/
int main(unsigned int argc, _TCHAR  **argv)
{
    // Check arguments, must be a file name argument
    // TODO: Should check that the file is there 
    if (argc < 2)
    {
      printf("missing arguments\n");
      printf("Usage:  %s filename\n", argv[0]);
      exit(-1);
    }

    // Test that EUM library is available
    LPCTSTR baseUnitStr;
    LONG baseUnit;
    eumGetBaseUnit(1002, &baseUnit, &baseUnitStr);
    printf("Base Unit: %s\n", baseUnitStr);


    // Get filename from arguments
    LPCTSTR filename = argv[1];
    printf("filename = %s\n", filename);

    int dotIndex = LastIndexOf(filename, '.');
    if (dotIndex < 0)
    {
      printf("Could not find extension of file = %s\n", filename);
      return -1;
    }

    LPCTSTR ext = &filename[dotIndex + 1];
    if (_strcmpi(ext, "dfsu") == 0)
    {
      return readDfsu(filename);
    }
    else
    {
      return readDfs(filename);
    }

}
