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
	
	// BEOS:APP_FLAGS tests
	void GetRecentAppsTestC1();
	void GetRecentAppsTestC2();
	void GetRecentAppsTestC3();
	
	//-----------------------------
	// GetRecentDocs() 
	//-----------------------------

	// Invalid params
	void GetRecentDocumentsTest1();
	void GetRecentDocumentsTest2();
	void GetRecentDocumentsTest3();

	// Normal function
	void GetRecentDocumentsTest4();
	
	// Repititon filter tests
	void GetRecentDocumentsTest5();
	
	//-----------------------------
	// GetRecentFolders() 
	//-----------------------------

	// Invalid params
	void GetRecentFoldersTest1();
	void GetRecentFoldersTest2();
	void GetRecentFoldersTest3();

	// Normal function
	void GetRecentFoldersTest4();
	
	// Repititon filter tests
	void GetRecentFoldersTest5();
	
	//-----------------------------
	// Load/Save/Clear
	//-----------------------------
	void RecentListsLoadSaveClearTest();

	//-----------------------------
	// misc 
	//-----------------------------

	// called for *each* test
	virtual void setUp();
	virtual void tearDown();
	
	static Test* Suite();
};

#endif	// GET_RECENT_TESTER_H

