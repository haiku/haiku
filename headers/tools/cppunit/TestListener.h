#ifndef _beos_test_listener_h_
#define _beos_test_listener_h_

#include <cppunit/TestListener.h>
#include <SupportDefs.h>

namespace CppUnit {
class Test;
class TestFailure;
class Exception;
}

//! Handles printing of test information
/*! Receives notification of the beginning and end of each test,
	and notification of all failures and errors. Prints out	said
	information in a standard format to standard output.

	You should not need to explicitly use this class in any
	of your tests.
*/
class CPPUNIT_API BTestListener : public CppUnit::TestListener {
public:
    virtual void startTest( CppUnit::Test *test );
	virtual void addFailure( const CppUnit::TestFailure &failure );
    virtual void endTest( CppUnit::Test *test );
protected:
	void printTime(bigtime_t time);
	bool fOkay;
	bigtime_t startTime;
};

#endif // _beos_test_listener_h_
