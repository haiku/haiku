#include "MallocBufferLengthTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>

MallocBufferLengthTest::MallocBufferLengthTest(std::string name) :
		BTestCase(name)
{
}

 

MallocBufferLengthTest::~MallocBufferLengthTest()
{
}


void 
MallocBufferLengthTest::PerformTest(void)
{
	BMallocIO mem;
	size_t size;
	size_t bufLen;
	status_t error;
	char writeBuf[11] = "0123456789";
	
	NextSubTest();
	bufLen = mem.BufferLength();
	CPPUNIT_ASSERT(bufLen == 0);
	
	NextSubTest();
	size = mem.Write(writeBuf, 10);
	bufLen = mem.BufferLength();
	CPPUNIT_ASSERT(bufLen == 10);
	CPPUNIT_ASSERT(size = 10);
	
	NextSubTest();
	error = mem.SetSize(0);
	bufLen = mem.BufferLength();
	CPPUNIT_ASSERT(bufLen == 0);
	CPPUNIT_ASSERT(error == B_OK);
}


CppUnit::Test *MallocBufferLengthTest::suite(void)
{	
	typedef CppUnit::TestCaller<MallocBufferLengthTest>
		MallocBufferLengthTestCaller;
		
	return(new MallocBufferLengthTestCaller("BMallocIO::BufferLength Test", &MallocBufferLengthTest::PerformTest));
}