#include "MallocSeekTest.h"
#include "cppunit/TestCaller.h"
#include <DataIO.h>
#include <stdio.h>

MallocSeekTest::MallocSeekTest(std::string name) :
		BTestCase(name)
{
}

 

MallocSeekTest::~MallocSeekTest()
{
}


void 
MallocSeekTest::PerformTest(void)
{
	BMallocIO mem;
	off_t err;
	
	NextSubTest();
	err = mem.Seek(3, SEEK_SET);
	CPPUNIT_ASSERT(err == 3);
	
	NextSubTest();
	err = mem.Seek(3, SEEK_CUR);
	CPPUNIT_ASSERT(err == 6);

	NextSubTest();
	err = mem.Seek(0, SEEK_END);
	CPPUNIT_ASSERT(err == 0);

	NextSubTest();
	err = mem.Seek(-5, SEEK_END);
	CPPUNIT_ASSERT(err == -5);
	
	NextSubTest();
	err = mem.Seek(5, SEEK_END);
	CPPUNIT_ASSERT(err == 5);
	
	NextSubTest();
	err = mem.Seek(-20, SEEK_SET);
	CPPUNIT_ASSERT((int)err == -20);
}


CppUnit::Test *MallocSeekTest::suite(void)
{	
	typedef CppUnit::TestCaller<MallocSeekTest>
		MallocSeekTestCaller;
		
	return(new MallocSeekTestCaller("BMallocIO::Seek Test", &MallocSeekTest::PerformTest));
}
