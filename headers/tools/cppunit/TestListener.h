#ifndef _beos_test_listener_h_
#define _beos_test_listener_h_

#include <cppunit/TestListener.h>

class CppUnit::Test;
class CppUnit::TestFailure;
class CppUnit::Exception;

//! Handles printing of test information
/*! Receives notification of the beginning and end of each test,
	and notification of all failures and errors. Prints out	said
	information in a standard format to standard output.
	
	You should not need to explicitly use this class in any
	of your tests.
*/
class BTestListener : public CppUnit::TestListener {
public:
    virtual void startTest( CppUnit::Test *test );
	virtual void addFailure( const CppUnit::TestFailure &failure );
    virtual void endTest( CppUnit::Test *test );
protected:
	bool fOkay;
};

#endif // _beos_test_listener_h_
