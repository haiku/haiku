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
	off_t offset;
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
	
	//This is for the BResource crashing bug
	NextSubTest();
	error = mem.SetSize(200);
	bufLen = mem.BufferLength();
	offset = mem.Seek(0, SEEK_END);
	CPPUNIT_ASSERT(bufLen == 200);
	CPPUNIT_ASSERT(error == B_OK);
	CPPUNIT_ASSERT(offset == 200);	
	
	NextSubTest();
	offset = mem.Seek(0, SEEK_END);
	error = mem.SetSize(100);
	bufLen = mem.BufferLength();
	CPPUNIT_ASSERT(bufLen == 100);
	CPPUNIT_ASSERT(mem.Position() == offset);
}


CppUnit::Test *MallocBufferLengthTest::suite(void)
{	
	typedef CppUnit::TestCaller<MallocBufferLengthTest>
		MallocBufferLengthTestCaller;
		
	return(new MallocBufferLengthTestCaller("BMallocIO::BufferLength Test", &MallocBufferLengthTest::PerformTest));
}
