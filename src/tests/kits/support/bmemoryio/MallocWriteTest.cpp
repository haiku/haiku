#include "MallocWriteTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>
#include <string.h>

MallocWriteTest::MallocWriteTest(std::string name) :
		BTestCase(name)
{
}

 

MallocWriteTest::~MallocWriteTest()
{
}


void 
MallocWriteTest::PerformTest(void)
{
	const char *writeBuf = "ABCDEFG";	
	
	BMallocIO mem;	
	ssize_t err;
	
	NextSubTest();
	err = mem.Write(writeBuf, 7);
	CPPUNIT_ASSERT(err == 7); // Check how much data we wrote
	
	NextSubTest();	
	err = mem.WriteAt(0, writeBuf, 4);
	CPPUNIT_ASSERT(err == 4);

	NextSubTest();
	err = mem.WriteAt(34, writeBuf, 256);
	CPPUNIT_ASSERT(err == 256);
}


CppUnit::Test *MallocWriteTest::suite(void)
{	
	typedef CppUnit::TestCaller<MallocWriteTest>
		MallocWriteTestCaller;
		
	return(new MallocWriteTestCaller("BMallocIO::Write Test", &MallocWriteTest::PerformTest));
}
