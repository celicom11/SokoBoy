#include "..\StdAfx.h"
#include "CppUnitTest.h"
#include "..\Sokoban.h"
#include "..\Corral.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SokobanTests
{
    TEST_CLASS(SokobanTests)
    {
    public:
        Sokoban sokoban;
        CStageCorrals corrals;

        SokobanTests():corrals(sokoban){

        }
        void LoadStage(PCWSTR szXsbFilePath)
        {
          //wchar_t buffer[260];
          //_wgetcwd(buffer, _countof(buffer));

          // Load the stage from the XSB file
            bool bOk = sokoban.Initialize(szXsbFilePath);
            Assert::IsTrue(bOk);
            corrals.Init(&sokoban.InitStage());
        }

        TEST_METHOD(ValidPICorral)
        {
            LoadStage(L"..\\..\\UnitTests\\ValidPICorrals\\1.xsb");
            int8_t result = corrals.GetPICorral();
            Assert::IsTrue(result != 0);
        }

        TEST_METHOD(InvalidPICorral)
        {
            LoadStage(L"..\\..\\UnitTests\\InvalidPICorrals\\1.xsb");
            int8_t result = corrals.GetPICorral();
            Assert::AreEqual(0, (int)result);
        }
    };
}
