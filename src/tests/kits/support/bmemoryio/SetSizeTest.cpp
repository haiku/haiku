#include "SetSizeTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>
#include <string.h>

SetSizeTest::SetSizeTest(std::string name) :
		BTestCase(name)
{
}

 

SetSizeTest::~SetSizeTest()
{
}


void 
SetSizeTest::PerformTest(void)
{
	char buf[20] = "0123456789ABCDEFGHI";
	char readBuf[10];
	
	memset(readBuf, 0, 10);	
	
	BMemoryIO mem(buf, 10);	
	ssize_t size;
	off_t pos;
	status_t err;
	
	NextSubTest();
	err = mem.SetSize(5);
	pos = mem.Seek(0, SEEK_END);
	size = mem.WriteAt(10, readBuf, 3);
	CPPUNIT_ASSERT(err == B_OK);
	CPPUNIT_ASSERT(pos == 5);
	CPPUNIT_ASSERT(size == 0); 
	
	NextSubTest();
	err = mem.SetSize(10);
	pos = mem.Seek(0, SEEK_END);
	size = mem.WriteAt(5, readBuf, 6);
	CPPUNIT_ASSERT(err == B_OK);
	CPPUNIT_ASSERT(pos == 10);
	CPPUNIT_ASSERT(size == 5);
	
	NextSubTest();
	err = mem.SetSize(20);
	CPPUNIT_ASSERT(err == B_ERROR);	
}


CppUnit::Test *SetSizeTest::suite(void)
{	
	typedef CppUnit::TestCaller<SetSizeTest>
		SetSizeTestCaller;
		
	return(new SetSizeTestCaller("BMemoryIO::SetSize Test", &SetSizeTest::PerformTest));
}
