#pragma once

#include "pch.h"
#include "eum.h"
#include <dfsio.h>

// Check return code of dfs methods
void CheckRc(LONG rc, LPCTSTR errMsg);

// Convert float array to double array
double* ConvertFloat2Double(float* flt, int size);

// Read static item and return the content of the static item
// The values should be of type sitemtype. Only conversion from
// float to double will be performed.
void* readStaticItem(LPFILE fp, LPHEAD pdfs, LPCSTR name, SimpleType sitemtype);
