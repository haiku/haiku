// AlertTest.h

#ifndef ALERT_TEST_H
#define ALERT_TEST_H

#include <TestCase.h>
#include <TestShell.h>

#define TEXT_MIME_STRING "text/plain"
#define STXT_MIME_STRING "text/x-vnd.Be-stxt"

class CppUnit::Test;

class AlertTest : public BTestCase {
public:
	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void ControlMetricsTest();
};

#endif	// ALERT_TEST_H
