//------------------------------------------------------------------------------
//	GetRunningTester.h
//
//------------------------------------------------------------------------------

#ifndef GET_RECENT_TESTER_H
#define GET_RECENT_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <TestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class GetRecentTester : public BTestCase
{
public:
	GetRecentTester() {;}
	GetRecentTester(std::string name) : BTestCase(name) {;}

	// NULL refList, NULL fileType, NULL, appSig
	void GetRecentDocumentsTestA1();
	void GetRecentDocumentsTestA2();
	void GetRecentDocumentsTestA3();

	// valid refList, NULL fileType, NULL, appSig
	void GetRecentDocumentsTestB1();
	
	//-----------------------------
	// GetRecentApps() 
	//-----------------------------

	// NULL refList, variable maxCount
	void GetRecentAppsTestA1();
	void GetRecentAppsTestA2();
	void GetRecentAppsTestA3();
	
	// Valid refList, variable maxCount
	void GetRecentAppsTestB1();
	void GetRecentAppsTestB2();
	void GetRecentAppsTestB3();
	
	// BEOS:APP_SIG tests
	void GetRecentAppsTestC1();
	void GetRecentAppsTestC2();
	void GetRecentAppsTestC3();
	void GetRecentAppsTestC4();
	void GetRecentAppsTestC5();
	void GetRecentAppsTestC6();
	void GetRecentAppsTestC7();
	void GetRecentAppsTestC8();
	void GetRecentAppsTestC9();
	void GetRecentAppsTestC10();

	// BEOS:APP_FLAGS tests
	
	
	// called for *each* test
	virtual void setUp();
	virtual void tearDown();
	
	static Test* Suite();
};

#endif	// GET_RECENT_TESTER_H

