#ifndef _beos_test_case_h_
#define _beos_test_case_h_

#include <cppunit/TestCase.h>
#include <StorageDefs.h>

class TestCase : public CppUnit::TestCase {
public:
	TestCase();
	TestCase(std::string Name);
	void SaveCWD();
	void RestoreCWD(const char *alternate = NULL);
protected:
	bool fValidCWD;
	char fCurrentWorkingDir[B_PATH_NAME_LENGTH+1];
};

#endif // _beos_test_case_h_