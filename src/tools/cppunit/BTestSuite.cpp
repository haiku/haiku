#include <TestSuite.h>
#include <cppunit/Test.h>
#include <cppunit/TestResult.h>

using std::map;
using std::string;

// Default constructor
_EXPORT
BTestSuite::BTestSuite( string name )
	: fName(name)
{
}

// Destructor
_EXPORT
BTestSuite::~BTestSuite() {
	deleteContents();
}


// Deletes all tests in the suite.
_EXPORT
void
BTestSuite::deleteContents() {
	for ( map<string, CppUnit::Test*>::iterator it = fTests.begin();
		   it != fTests.end();
		     ++it)
		delete it->second;
	fTests.clear();
}


/// Runs the tests and collects their result in a TestResult.
_EXPORT
void
BTestSuite::run( CppUnit::TestResult *result ) {
	for ( map<string, CppUnit::Test*>::iterator it = fTests.begin();
    	   it != fTests.end();
		     ++it )
	{
		if ( result->shouldStop() )
			break;

		Test *test = it->second;
		test->run( result );
	}
}


// Counts the number of test cases that will be run by this test.
_EXPORT
int
BTestSuite::countTestCases() const {
	int count = 0;

	for ( map<string, CppUnit::Test *>::const_iterator it = fTests.begin();
		   it != fTests.end();
		     ++it )
		count += it->second->countTestCases();

	return count;
}


// Adds a test to the suite.
_EXPORT
void
BTestSuite::addTest(string name, CppUnit::Test *test) {
	fTests[name] = test;
}


// Returns a string representation of the test suite.
_EXPORT
string
BTestSuite::toString() const {
	return "suite " + getName();
}


// Returns the name of the test suite.
_EXPORT
string
BTestSuite::getName() const {
	return fName;
}


_EXPORT
const map<string, CppUnit::Test*> &
BTestSuite::getTests() const {
	return fTests;
}


