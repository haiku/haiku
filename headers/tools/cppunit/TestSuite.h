#ifndef _beos_test_suite_h_
#define _beos_test_suite_h_

#include <cppunit/Test.h>
#include <map>
#include <string>

class CppUnit::TestResult;

//! Groups together a set of tests for a given kit.
class BTestSuite : public CppUnit::Test {
public:
	BTestSuite( std::string name );
	virtual ~BTestSuite();

	virtual void run( CppUnit::TestResult *result );
	virtual int countTestCases() const;
	virtual std::string getName() const;
	virtual std::string toString() const;

	virtual void addTest(std::string name, CppUnit::Test *test);
	virtual void deleteContents();

	const std::map<std::string, CppUnit::Test*> &getTests() const;

protected:
	std::map<std::string, CppUnit::Test*> fTests;
	const std::string fName;

private:
	BTestSuite(const BTestSuite &other);
	BTestSuite& operator=(const BTestSuite &other); 

};

#endif // _beos_test_listener_h_
