// RegistrarThreadManagerTest.h

#ifndef _REGISTRAR_THREAD_MANAGER_TEST_H
#define _REGISTRAR_THREAD_MANAGER_TEST_H

#include <cppunit/Test.h>
#include <TestCase.h>

#include <Mime.h>

class BTestApp;

class RegistrarThreadManagerTest : public BTestCase {
public:
	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void ShutdownTest();
	void ThreadLimitTest();
private:
	BTestApp	*fApplication;
};

#endif	// _REGISTRAR_THREAD_MANAGER_TEST_H
