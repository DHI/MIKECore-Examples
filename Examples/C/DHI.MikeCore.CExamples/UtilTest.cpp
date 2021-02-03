#include "pch.h"
#include <iostream>
#include "eum.h"
#include "dfsio.h"
#include "Util.h"
#include <CppUnitTest.h>

/******************************************
 * Example of how to generally read data from a dfs file,
 * especially dfs0, dfs1 and dfs2 files.
 ******************************************/


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// a global variable so it only has to be setup once
extern LPCTSTR TestDataFolder;
namespace UnitestForC_MikeCore
{
  TEST_CLASS(Util_tests)
  {
  public:
    /// reads the dfs0 file "data_ndr_roese.dfs0"

    TEST_METHOD(GetNbOfStaticItemsDfs0Test)
    {
      LPCTSTR fileName = "data_ndr_roese.dfs0";
      auto inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      int nbOfStaticItems = GetNbOfStaticItems(pdfs, fp);
      Assert::AreEqual(0, nbOfStaticItems);
      delete[] inputFullPath;
    }

    TEST_METHOD(GetNbOfStaticItemsDfs2Test)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      auto inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      int nbOfStaticItems = GetNbOfStaticItems(pdfs, fp);
      Assert::AreEqual(1, nbOfStaticItems);
      delete[] inputFullPath;
    }

    TEST_METHOD(GetNbOfStaticItemsDfsUTest)
    {
      LPCTSTR fileName = "OresundHD.dfsu";
      auto inputFullPath = new char[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataFolder, fileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      int nbOfStaticItems = GetNbOfStaticItems(pdfs, fp);
      Assert::AreEqual(9, nbOfStaticItems);
      delete[] inputFullPath;
    }

  };
}