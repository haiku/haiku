#ifndef _beos_test_suite_h_
#define _beos_test_suite_h_

#include <BeBuild.h>
#include <cppunit/Test.h>
#include <map>
#include <string>

class CppUnit::TestResult;

//! Groups together a set of tests for a given kit.
class CPPUNIT_API BTestSuite : public CppUnit::Test {
public:
	BTestSuite( string name = "" );
	virtual ~BTestSuite();

	virtual void run( CppUnit::TestResult *result );
	virtual int countTestCases() const;
	virtual string getName() const;
	virtual string toString() const;

	virtual void addTest(string name, CppUnit::Test *test);
	virtual void deleteContents();

	const map<string, CppUnit::Test*> &getTests() const;

protected:
	map<string, CppUnit::Test*> fTests;
	const string fName;

private:
	BTestSuite(const BTestSuite &other);
	BTestSuite& operator=(const BTestSuite &other); 

};

#endif // _beos_test_listener_h_
