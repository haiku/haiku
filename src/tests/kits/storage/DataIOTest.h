// DataIOTest.h

#ifndef __sk_data_io_test_h__
#define __sk_data_io_test_h__

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <StorageDefs.h>
#include <SupportDefs.h>

#include "BasicTest.h"

class DataIOTest : public BasicTest
{
public:
	static CppUnit::Test* Suite();

	void BufferedDataIOTest();
};


#endif	// __sk_data_io_test_h__
