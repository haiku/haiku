// AlertTest.cpp
#include "AlertTest.h"
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Alert.h>
#include <TextView.h>
#include <Button.h>
#include <Rect.h>

// Suite
CppUnit::Test *
AlertTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<AlertTest> TC;
			
	suite->addTest(
		new TC("Alert ControlMetricsTest",
			&AlertTest::ControlMetricsTest));
		
	return suite;
}		

// setUp
void
AlertTest::setUp()
{
	BTestCase::setUp();
}
	
// tearDown
void
AlertTest::tearDown()
{
	BTestCase::tearDown();
}

// Test state of controls in the BAlert's window
void
AlertTest::ControlMetricsTest()
{
	NextSubTest();
	// Dummy application object required to create Window objects.
	BApplication app("application/x-vnd.Haiku-interfacekit_alerttest");
	BAlert *pAlert = new BAlert(
		"alert1",
		"X",
		"OK", NULL, NULL,
		B_WIDTH_AS_USUAL, // widthStyle
		B_EVEN_SPACING,
		B_INFO_ALERT		// alert_type		
	);
	CPPUNIT_ASSERT(pAlert);
	
	// Only button0 should be available
	NextSubTest();
	CPPUNIT_ASSERT_EQUAL((BButton*)NULL, pAlert->ButtonAt(1));
	CPPUNIT_ASSERT_EQUAL((BButton*)NULL, pAlert->ButtonAt(2));
	BButton *pBtn0 = pAlert->ButtonAt(0);
	CPPUNIT_ASSERT(pBtn0);
	
	NextSubTest();
	CPPUNIT_ASSERT(strcmp(pBtn0->Label(), "OK") == 0);
	
	// Width / Height
	NextSubTest();
	CPPUNIT_ASSERT_DOUBLES_EQUAL(75, pBtn0->Bounds().Width(), 0.01);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(30, pBtn0->Bounds().Height(), 0.01);
	
	// Horizontal / Vertical position
	NextSubTest();
	BPoint pt = pBtn0->ConvertToParent(BPoint(0, 0));
	CPPUNIT_ASSERT_DOUBLES_EQUAL(229, pt.x, 0.01);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(28, pt.y, 0.01);
	
	// TextView
	NextSubTest();
	BTextView *textView = pAlert->TextView();
	CPPUNIT_ASSERT(textView);
	
	NextSubTest();
	CPPUNIT_ASSERT(strcmp(textView->Text(), "X") == 0);
	
	delete pAlert;
	pAlert = NULL;
}



