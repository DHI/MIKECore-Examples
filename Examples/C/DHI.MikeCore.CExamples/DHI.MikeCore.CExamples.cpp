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

