#ifndef __sk_testing_is_so_much_fun_h__
#define __sk_testing_is_so_much_fun_h__

#include <CppUnitShell.h>
#include <cppunit/TestCase.h>
#include <cppunit/TestSuite.h>
#include <StorageDefs.h>

#include <stdio.h>

// Handy defines :-)
#define CHK CPPUNIT_ASSERT
#define RES DecodeResult

class BPath;

namespace StorageKit {

class TestSuite : public CppUnit::TestSuite {
    virtual void run (CppUnit::TestResult *result) {
    	setUp();
    	CppUnit::TestSuite::run(result);
    	tearDown();
    }
    
	// This function called *once* before the entire suite of tests is run
	virtual void setUp() {}
	
	// This function called *once* after the entire suite of tests is run
	virtual void tearDown() {}   

};


class TestShell : public CppUnitShell {
public:
	TestShell();
	~TestShell();
	int Run(int argc, char *argv[]);
	virtual void PrintDescription(int argc, char *argv[]);
	const char* TestDir() const;
private:
	BPath *fTestDir;
};

class TestCase : public CppUnit::TestCase {
public:
	TestCase();
	void SaveCWD();
	void RestoreCWD(const char *alternate = NULL);
protected:
	bool fValidCWD;
	char fCurrentWorkingDir[B_PATH_NAME_LENGTH+1];
};

};

extern StorageKit::TestShell shell;


#endif // __sk_testing_is_so_much_fun_h__
