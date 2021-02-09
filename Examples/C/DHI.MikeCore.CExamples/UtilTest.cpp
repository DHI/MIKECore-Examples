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
namespace UnitestForC_MikeCore
{
  TEST_CLASS(Util_tests)
  {
  public:
    /// reads the dfs0 file "data_ndr_roese.dfs0"

    TEST_METHOD(GetNbOfStaticItemsDfs0Test)
    {
      LPCTSTR fileName = "data_ndr_roese.dfs0";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      int nbOfStaticItems = GetNbOfStaticItems(pdfs, fp);
      Assert::AreEqual(0, nbOfStaticItems);

      rc = dfsFileClose(pdfs, &fp);
      rc = dfsHeaderDestroy(&pdfs);
    }

    TEST_METHOD(GetNbOfStaticItemsDfs2Test)
    {
      LPCTSTR fileName = "OresundHD.dfs2";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      int nbOfStaticItems = GetNbOfStaticItems(pdfs, fp);
      Assert::AreEqual(1, nbOfStaticItems);

      rc = dfsFileClose(pdfs, &fp);
      rc = dfsHeaderDestroy(&pdfs);
    }

    TEST_METHOD(GetNbOfStaticItemsDfsUTest)
    {
      LPCTSTR fileName = "OresundHD.dfsu";
      char inputFullPath[_MAX_PATH];
      snprintf(inputFullPath, _MAX_PATH, "%s%s", TestDataPath(), fileName);
      LPFILE      fp;
      LPHEAD      pdfs;
      long rc = dfsFileRead(inputFullPath, &pdfs, &fp);

      int nbOfStaticItems = GetNbOfStaticItems(pdfs, fp);
      Assert::AreEqual(9, nbOfStaticItems);

      rc = dfsFileClose(pdfs, &fp);
      rc = dfsHeaderDestroy(&pdfs);
    }

  };
}