#include "ConstTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>
#include <string.h>

ConstTest::ConstTest(std::string name) :
		BTestCase(name)
{
}

 

ConstTest::~ConstTest()
{
}


void 
ConstTest::PerformTest(void)
{
	const char buf[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	BMemoryIO mem(buf, 10);
	status_t err;
	
	NextSubTest();
	err = mem.SetSize(4);
	CPPUNIT_ASSERT(err == B_NOT_ALLOWED);
	
	NextSubTest();
	err = mem.SetSize(20);
	CPPUNIT_ASSERT(err == B_NOT_ALLOWED);

	NextSubTest();
	char readBuf[10] = "";
	err = mem.Write(readBuf, 3);
	CPPUNIT_ASSERT(err == B_NOT_ALLOWED);
	CPPUNIT_ASSERT(strcmp(readBuf, "") == 0); 
	
	NextSubTest();
	err = mem.WriteAt(2, readBuf, 1);
	CPPUNIT_ASSERT(err == B_NOT_ALLOWED);
	CPPUNIT_ASSERT(strcmp(readBuf, "") == 0); 
}


CppUnit::Test *ConstTest::suite(void)
{	
	typedef CppUnit::TestCaller<ConstTest>
		ConstTestCaller;
		
	return(new ConstTestCaller("BMemoryIO::Const Test", &ConstTest::PerformTest));
}
