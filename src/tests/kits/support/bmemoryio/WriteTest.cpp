#include "WriteTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>
#include <string.h>

WriteTest::WriteTest(std::string name) :
		BTestCase(name)
{
}

 

WriteTest::~WriteTest()
{
}


void 
WriteTest::PerformTest(void)
{
	char buf[10];
	const char *writeBuf = "ABCDEFG";	
	
	BMemoryIO mem(buf, 10);
	
	size_t err;
	off_t pos;
	
	NextSubTest();
	pos = mem.Position();
	err = mem.Write(writeBuf, 7);
	CPPUNIT_ASSERT(err == 7); // Check how much data we wrote
	CPPUNIT_ASSERT(strcmp(writeBuf, buf) == 0); // Check if we wrote it correctly
	CPPUNIT_ASSERT(mem.Position() == pos + err); // Check if Position changed
	
	NextSubTest();
	pos = mem.Position();
	err = mem.WriteAt(3, writeBuf, 2);
	CPPUNIT_ASSERT(err == 2);
	CPPUNIT_ASSERT(strncmp(buf + 3, writeBuf, 2) == 0);
	CPPUNIT_ASSERT(mem.Position() == pos);
	
	NextSubTest();
	pos = mem.Position();
	err = mem.WriteAt(9, writeBuf, 5);
	CPPUNIT_ASSERT(err == 1);
	CPPUNIT_ASSERT(strncmp(buf + 9, writeBuf, 1) == 0);
	CPPUNIT_ASSERT(mem.Position() == pos);

}


CppUnit::Test *WriteTest::suite(void)
{	
	typedef CppUnit::TestCaller<WriteTest>
		WriteTestCaller;
		
	return(new WriteTestCaller("BMemoryIO::Write Test", &WriteTest::PerformTest));
}