//------------------------------------------------------------------------------
//	BClipboardTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Clipboard.h>

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "BClipboardTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	BClipboard(const char *name, bool transient = false)
	@case 1
	@results		clipboard defaults to system clipboard
 */
void BClipboardTester::BClipboard1()
{
  BApplication app("application/x-vnd.clipboardtest");
#ifdef TEST_R5
  CHK(false);
#endif
  BClipboard clip(NULL);
  CHK(strcmp(clip.Name(),"system") == 0);
}

/*
	BClipboard(const char *name, bool transient = false)
	@case 2
	@results		return string from Name() should match *name
 */
void BClipboardTester::BClipboard2()
{
  BApplication app("application/x-vnd.clipboardtest");
  char name[18] = "BClipboard Case 2";
  BClipboard clip(name);

  CHK(strcmp(clip.Name(),name) == 0);
}

Test* BClipboardTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BClipboard, SuiteOfTests, BClipboardTester, BClipboard1);
	ADD_TEST4(BClipboard, SuiteOfTests, BClipboardTester, BClipboard2);

	return SuiteOfTests;
}



