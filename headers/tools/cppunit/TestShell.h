#ifndef _beos_test_shell_h_
#define _beos_test_shell_h_

#include <LockerSyncObject.h>
#include <cppunit/Exception.h>
#include <cppunit/Test.h>
#include <cppunit/TestListener.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <map>
#include <set>
#include <string>

// Defines SuiteFunction to be a pointer to a function that
// takes no arguments and returns a pointer to a CppUnit::Test
typedef CppUnit::Test* (*SuiteFunction)(void);

// This is just absurd to have to type...
typedef CppUnit::SynchronizedObject::SynchronizationObject SyncObject;

//! BeOS savvy command line interface for the CppUnit testing framework.
/*! This class provides a fully functional command-line testing interface
	built on top of the CppUnit testing library. You add named test suites
	via AddSuite(), and then call Run(), which does all the dirty work. The
	user can get a list of each test installed via AddSuite(), and optionally
	can opt to run only a specified set of them.
*/
class BTestShell {
public:
	BTestShell(const std::string &description = "", SyncObject *syncObject = 0);
	
	// This function is used to add test suites to the list of available
	// tests. A SuiteFunction is just a function that takes no parameters
	// and returns a pointer to a CppUnit::Test object. Return NULL at
	// your own risk :-). The name given is the name that will be presented
	// when the program is run with "--list" as an argument. Usually the
	// given suite would be a test suite for an entire class, but that's
	// not a requirement.
	void AddSuite(const std::string &name, const SuiteFunction suite);

	// This is the function you call after you've added all your test
	// suites with calls to AddSuite(). It runs the test, or displays
	// help, or lists installed tests, or whatever, depending on the
	// command-line arguments passed in.
	int Run(int argc, char *argv[]);
	
	// Verbosity Level enumeration and accessor function
	enum VerbosityLevel { v0, v1, v2, v3 };
	VerbosityLevel Verbosity() const;

	// Returns true if verbosity is high enough that individual tests are
	// allowed to make noise.
	bool BeVerbose() const { return Verbosity() >= v2; };
	

protected:	
	VerbosityLevel fVerbosityLevel;
	std::set<std::string> fTestsToRun;
	std::map<std::string, SuiteFunction> fTests;	
	CppUnit::TestResult fTestResults;
	CppUnit::TestResultCollector fResultsCollector;
	std::string fDescription;

	// Prints a brief description of the program and a guess as to
	// which Storage Kit library the app was linked with based on
	// the filename of the app
	virtual void PrintDescription(int argc, char *argv[]);

	// Prints out command line argument instructions
	void PrintHelp();

	// Handles command line arguments; returns true if everything goes
	// okay, false if not (or if the program just needs to terminate without
	// running any tests). Modifies settings in "settings" as necessary.
	bool ProcessArguments(int argc, char *argv[]);

	// Makes any necessary pre-test preparations
	void InitOutput();

	// Prints out the test results in the proper format per
	// the specified verbosity level.
	void PrintResults();
	
private:
  //! Prevents the use of the copy constructor.
  BTestShell( const BTestShell &copy );
  
  //! Prevents the use of the copy operator.
  void operator =( const BTestShell &copy );

};	// class BTestShell

#endif // _beos_test_shell_h_