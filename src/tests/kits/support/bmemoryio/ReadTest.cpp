#include "ReadTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>
#include <string.h>

ReadTest::ReadTest(std::string name) :
		BTestCase(name)
{
}

 

ReadTest::~ReadTest()
{
}


void 
ReadTest::PerformTest(void)
{
	char buf[20] = "0123456789ABCDEFGHI";
	char readBuf[10];
	
	memset(readBuf, 0, 10);	
	
	BMemoryIO mem(buf, 20);	
	ssize_t err;
	off_t pos;
	
	NextSubTest();
	pos = mem.Position();
	err = mem.Read(readBuf, 10);
	CPPUNIT_ASSERT(err == 10); 
	CPPUNIT_ASSERT(strncmp(readBuf, buf, 10) == 0); 
	CPPUNIT_ASSERT(mem.Position() == pos + err); 
	
	NextSubTest();
	pos = mem.Position();
	err = mem.ReadAt(30, readBuf, 10);
	CPPUNIT_ASSERT(err == 0);
	CPPUNIT_ASSERT(mem.Position() == pos);
	
	NextSubTest();
	pos = mem.Seek(0, SEEK_END);
	err = mem.Read(readBuf, 10);
	CPPUNIT_ASSERT(err == 0);
	CPPUNIT_ASSERT(mem.Position() == pos);
	
}


CppUnit::Test *ReadTest::suite(void)
{	
	typedef CppUnit::TestCaller<ReadTest>
		ReadTestCaller;
		
	return(new ReadTestCaller("BMemoryIO::Read Test", &ReadTest::PerformTest));
}
