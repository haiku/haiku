#include <TestListener.h>

#include <cppunit/Exception.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <iostream>

void
BTestListener::startTest( CppUnit::Test *test ) {
   	fOkay = true;
   	cout << test->getName() << endl;
}

void
BTestListener::addFailure( const CppUnit::TestFailure &failure ) {
   	fOkay = false;
   	cout << "  - ";
   	cout << (failure.isError() ? "ERROR" : "FAILURE");
   	cout << " -- ";
   	cout << (failure.thrownException() != NULL
   	           ? failure.thrownException()->what()
   	             : "(unknown error)");
   	cout << endl;	
}

void
BTestListener::endTest( CppUnit::Test *test )  {
   	if (fOkay)
   		cout << "  + PASSED" << endl;
//   	else
//   		cout << "  - FAILED" << endl;
   	cout << endl;
}	 
