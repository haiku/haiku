#include "SeekTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>

SeekTest::SeekTest(std::string name) :
		BTestCase(name)
{
}

 

SeekTest::~SeekTest()
{
}


void 
SeekTest::PerformTest(void)
{
	char buf[10];
	BMemoryIO mem(buf, 10);
	off_t err;
	
	NextSubTest();
	err = mem.Seek(3, SEEK_SET);
	CPPUNIT_ASSERT(err == 3);
	
	NextSubTest();
	err = mem.Seek(3, SEEK_CUR);
	CPPUNIT_ASSERT(err == 6);

	NextSubTest();
	err = mem.Seek(0, SEEK_END);
	CPPUNIT_ASSERT(err == 10);

	NextSubTest();
	err = mem.Seek(-5, SEEK_END);
	CPPUNIT_ASSERT(err == 5);
	
	NextSubTest();
	err = mem.Seek(5, SEEK_END);
	CPPUNIT_ASSERT(err == 15);
}


CppUnit::Test *SeekTest::suite(void)
{	
	typedef CppUnit::TestCaller<SeekTest>
		SeekTestCaller;
		
	return(new SeekTestCaller("BMemoryIO::Seek Test", &SeekTest::PerformTest));
}
