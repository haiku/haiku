#include <TestSuite.h>
#include <cppunit/Test.h>
#include <cppunit/TestResult.h>

// Default constructor
BTestSuite::BTestSuite( string name )
	: fName(name)
{
}

// Destructor
BTestSuite::~BTestSuite() { 
	deleteContents(); 
}


// Deletes all tests in the suite.
void 
BTestSuite::deleteContents() {
	for ( map<string, CppUnit::Test*>::iterator it = fTests.begin();
		   it != fTests.end();
		     ++it)
		delete it->second;
	fTests.clear();
}


/// Runs the tests and collects their result in a TestResult.
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
void 
BTestSuite::addTest(string name, CppUnit::Test *test) { 
	fTests[name] = test; 
}


// Returns a string representation of the test suite.
string 
BTestSuite::toString() const { 
	return "suite " + getName();
}


// Returns the name of the test suite.
string 
BTestSuite::getName() const { 
	return fName; 
}


const map<string, CppUnit::Test*> &
BTestSuite::getTests() const {
	return fTests;
}


