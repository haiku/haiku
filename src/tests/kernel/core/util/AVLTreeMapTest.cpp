#include <AVLTreeMapTest.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <stdio.h>
#include <TestUtils.h>

#include <AVLTreeMap.h>

AVLTreeMapTest::AVLTreeMapTest(std::string name)
	: BTestCase(name)
{
}

CppUnit::Test*
AVLTreeMapTest::Suite()
{
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("AVLTreeMap");

	suite->addTest(new CppUnit::TestCaller<AVLTreeMapTest>(
		"SinglyLinkedList::User Strategy Test (default next parameter)",
		&AVLTreeMapTest::Test1));

	return suite;
}

//! Test1
void
AVLTreeMapTest::Test1()
{
}

