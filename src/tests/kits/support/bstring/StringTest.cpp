#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"
#include "StringTest.h"
#include "StringConstructionTest.h"
#include "StringAccessTest.h"
#include "StringAssignTest.h"
#include "StringAppendTest.h"
#include "StringSubCopyTest.h"
#include "StringPrependTest.h"
#include "StringCaseTest.h"
#include "StringInsertTest.h"
#include "StringEscapeTest.h"
#include "StringRemoveTest.h"
#include "StringCompareTest.h"
#include "StringFormatAppendTest.h"
#include "StringCharAccessTest.h"
#include "StringReplaceTest.h"
#include "StringSearchTest.h"

CppUnit::Test *StringTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(StringConstructionTest::suite());
	testSuite->addTest(StringAccessTest::suite());
	testSuite->addTest(StringAssignTest::suite());
	testSuite->addTest(StringAppendTest::suite());
	testSuite->addTest(StringSubCopyTest::suite());
	testSuite->addTest(StringPrependTest::suite());
	testSuite->addTest(StringCaseTest::suite());
	testSuite->addTest(StringInsertTest::suite());
	testSuite->addTest(StringEscapeTest::suite());
	testSuite->addTest(StringRemoveTest::suite());
	testSuite->addTest(StringCompareTest::suite());
	testSuite->addTest(StringFormatAppendTest::suite());
	testSuite->addTest(StringCharAccessTest::suite());
	testSuite->addTest(StringReplaceTest::suite());
	testSuite->addTest(StringSearchTest::suite());
	
	return(testSuite);
}
