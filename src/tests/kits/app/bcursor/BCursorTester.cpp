//-----------------------------------------------------------------------------
//	BCursorTester.cpp
//
//-----------------------------------------------------------------------------

// Standard Includes ----------------------------------------------------------

// System Includes ------------------------------------------------------------
#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <Message.h>

#define CHK	CPPUNIT_ASSERT

// Project Includes -----------------------------------------------------------

// Local Includes -------------------------------------------------------------
#include "BCursorTester.h"

// Local Defines --------------------------------------------------------------

// Globals --------------------------------------------------------------------

//-----------------------------------------------------------------------------

/*
	BCursor(const void *cursorData)
	@case 1
	@results		nothing apparent (no segfault)
 */
void BCursorTester::BCursor1()
{
  BApplication app("application/x-vnd.cursortest");
  BCursor cur((void *)NULL);
}

/*
	BCursor(const void *cursorData)
	@case 2
	@results		nothing apparent
 */
void BCursorTester::BCursor2()
{
  BApplication app("application/x-vnd.cursortest");
  char data[68];
  int i;

  data[0] = 16;
  data[1] = 1;
  data[2] = 0;
  data[3] = 0;
  for (i=4; i<68; i++)
    data[i] = 1;

  BCursor cur(data);
}

/*
	BCursor(const void *cursorData)
	@case 3
	@results		nothing apparent (no segfaults)
 */
void BCursorTester::BCursor3()
{
  BApplication app("application/x-vnd.cursortest");
  int x;
  BCursor cur1(&x);
  char data[68];
  data[0] = 32;
  BCursor cur2(data);
  data[0] = 16;
  data[1] = 8;
  BCursor cur3(data);
  data[1] = 1;
  data[2] = 16;
  data[3] = 16;
  BCursor cur4(data);
}

/*
	BCursor(BMessage *archive)
	@case 1
	@results		nothing apparent (no segfault)
 */
void BCursorTester::BCursor4()
{
  BApplication app("application/x-vnd.cursortest");
  BCursor cur((BMessage *)NULL);
}

/*
	BCursor(BMessage *archive)
	@case 2
	@results		nothing apparent (empty cursor)
 */
void BCursorTester::BCursor5()
{
  BApplication app("application/x-vnd.cursortest");
  /* The message really should contain a valid archive, but Cursor doesn't
     support archiving anyway, so until R2, this is a moot point.
  */
  BMessage msg;
  BCursor cur(&msg);
}

/*
	BCursor(BBitmap *bitmap, BPoint* hotspot)
	@case 1
	@results		nothing apparent (empty cursor)
 */
void BCursorTester::BCursor6()
{
  BApplication app("application/x-vnd.cursortest");

  BBitmap *bitmap;
  BPoint hotspot(0, 0);

  get_mouse_bitmap(&bitmap, &hotspot);
  hotspot.x += 1;
  hotspot.y += 1;
  BCursor cur(bitmap, hotspot);
}

/*
	static BArchivable *Instantiate(BMessage *archive)
	@case 1
	@results		return NULL
 */
void BCursorTester::Instantiate1()
{
  BApplication app("application/x-vnd.cursortest");
  CHK(BCursor::Instantiate(NULL) == NULL);
}

/*
	static BArchivable *Instantiate(BMessage *archive)
	@case 2
	@results		return NULL
 */
void BCursorTester::Instantiate2()
{
  BApplication app("application/x-vnd.cursortest");
  /* The message really should contain a valid archive, but Cursor doesn't
     support archiving anyway, so until R2, this is a moot point.
  */
  BMessage msg;
  CHK(BCursor::Instantiate(&msg) == NULL);
}

/*
	status_t Archive(BMessage* into, bool deep = true)
	@case 1
	@results		return B_OK
 */
void BCursorTester::Archive1()
{
  BApplication app("application/x-vnd.cursortest");
  char data[68];
  int i;

  data[0] = 16;
  data[1] = 1;
  data[2] = 0;
  data[3] = 0;
  for (i=4; i<68; i++)
    data[i] = 1;

  BCursor cur(data);
  CHK(cur.Archive(NULL) == B_OK);
}

/*
	status_t Archive(BMessage* into, bool deep = true)
	@case 2
	@results		return B_OK
 */
void BCursorTester::Archive2()
{
  BApplication app("application/x-vnd.cursortest");
  char data[68];
  int i;

  data[0] = 16;
  data[1] = 1;
  data[2] = 0;
  data[3] = 0;
  for (i=4; i<68; i++)
    data[i] = 1;

  BCursor cur(data);
  BMessage msg;
  CHK(cur.Archive(&msg) == B_OK);
}


Test* BCursorTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, BCursor1);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, BCursor2);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, BCursor3);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, BCursor4);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, BCursor5);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, Instantiate1);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, Instantiate2);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, Archive1);
	ADD_TEST4(BCursor, SuiteOfTests, BCursorTester, Archive2);

	return SuiteOfTests;
}

