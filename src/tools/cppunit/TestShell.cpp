#include <TestShell.h>

#include <cppunit/Exception.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestSuite.h>
#include <Directory.h>
#include <Entry.h>
#include <image.h>
#include <Path.h>
#include <TestListener.h>
#include <set>
#include <map>
#include <string>
#include <vector>

BTestShell *BTestShell::fGlobalShell = NULL;
const char BTestShell::indent[] = "  ";

BTestShell::BTestShell(const std::string &description, SyncObject *syncObject)
	: fVerbosityLevel(v2)
	, fDescription(description)
	, fTestResults(syncObject)
	, fListTestsAndExit(false)
	, fTestDir(NULL)
{
};

BTestShell::~BTestShell() {
	delete fTestDir;
}


status_t
BTestShell::AddSuite(BTestSuite *suite) {
	if (suite) {
		if (Verbosity() >= v3)
			cout << "Adding suite '" << suite->getName() << "'" << endl;
	
		// Add the suite
		fSuites[suite->getName()] = suite;
		
		// Add its tests
		const TestMap &map = suite->getTests();
		for (TestMap::const_iterator i = map.begin();
			   i != map.end();
			      i++)
			AddTest(i->first, i->second);
			
		return B_OK;
	} else
		return B_BAD_VALUE;
}

void
BTestShell::AddTest(const std::string &name, CppUnit::Test *test) {
	if (test != NULL)
		fTests[name] = test;
	else
		fTests.erase(name);
}

int32
BTestShell::LoadSuitesFrom(BDirectory *libDir) {
	if (!libDir || libDir->InitCheck() != B_OK)
		return 0;

	BEntry addonEntry;
	BPath addonPath;
	image_id addonImage;
	int count = 0;

	typedef BTestSuite* (*suiteFunc)(void);
	suiteFunc func;

	while (libDir->GetNextEntry(&addonEntry, true) == B_OK) {
		status_t err;
		err = addonEntry.GetPath(&addonPath);
		if (!err) {
//			cout << "Checking " << addonPath.Path() << "..." << flush;
			addonImage = load_add_on(addonPath.Path());
			err = (addonImage > 0 ? B_OK : B_ERROR);
		}
		if (!err) {
//			cout << "..." << endl;
			err = get_image_symbol(addonImage,
				    "getTestSuite",
				      B_SYMBOL_TYPE_TEXT,
				        reinterpret_cast<void **>(&func));
		} else {
//			cout << " !!! err == " << err << endl;
		}
		if (!err) 
			err = AddSuite(func());
		if (!err)
			count++;
	}
	return count;
}

int
BTestShell::Run(int argc, char *argv[]) {
	// Make note of which directory we started in
	UpdateTestDir(argv);	

	// Parse the command line args
	if (!ProcessArguments(argc, argv))
		return 0;		

	// Load any dynamically loadable tests we can find
	LoadDynamicSuites();

	// See if the user requested a list of tests. If so,
	// print and bail.
	if (fListTestsAndExit) {
		PrintInstalledTests();
		return 0;
	}		

	// Add the proper tests to our suite (or exit if there
	// are no tests installed).
	CppUnit::TestSuite suite;	
	if (fTests.empty()) {
	
		// No installed tests whatsoever, so bail
		cout << "ERROR: No installed tests to run!" << endl;
		return 0;
		
	} else if (fSuitesToRun.empty() && fTestsToRun.empty()) {
	
		// None specified, so run them all
		TestMap::iterator i;
		for (i = fTests.begin(); i != fTests.end(); ++i)
			suite.addTest( i->second );
			
	} else {
		std::set<std::string>::const_iterator i;
		std::set<std::string> suitesToRemove;

		// Add all the tests from any specified suites to the list of
		// tests to run (since we use a set, this eliminates the concern
		// of having duplicate entries).
		for (i = fTestsToRun.begin(); i != fTestsToRun.end(); ++i) {
			// See if it's a suite (since it may just be a single test)
			if (fSuites.find(*i) != fSuites.end()) {
				suitesToRemove.insert(*i);	// Note the suite name for later
				const TestMap &tests = fSuites[*i]->getTests();
				TestMap::const_iterator j;
				for (j = tests.begin(); j != tests.end(); j++) {
					fTestsToRun.insert( j->first );
				}
			}
		}
		
		// Remove the names of all of the suites we discovered from the
		// list of tests to run
		for (i = suitesToRemove.begin(); i != suitesToRemove.end(); i++) {
			fTestsToRun.erase(*i);
		}
	
		// Everything still in fTestsToRun must then be an explicit test
		for (i = fTestsToRun.begin(); i != fTestsToRun.end(); ++i) {
			// Make sure it's a valid test
			if (fTests.find(*i) != fTests.end()) {
				suite.addTest( fTests[*i] );
			} else {
				cout << endl << "ERROR: Invalid argument \"" << *i << "\"" << endl;
				PrintHelp();
				return 0;
			}
		}
			
	}
	
	// Run all the tests
	InitOutput();
	suite.run(&fTestResults);
	PrintResults();

	return 0;
}

BTestShell::VerbosityLevel
BTestShell::Verbosity() const {
	return fVerbosityLevel;
} 

const char*
BTestShell::TestDir() const {
	return (fTestDir ? fTestDir->Path() : NULL);
}

void
BTestShell::PrintDescription(int argc, char *argv[]) {
	cout << endl << fDescription;
}

void
BTestShell::PrintHelp() {
	cout << endl;
	cout << "VALID ARGUMENTS:     " << endl;
	PrintValidArguments();
	cout << endl;
	
}

void
BTestShell::PrintValidArguments() {
	cout << indent << "--help       Displays this help text plus some other garbage" << endl;
	cout << indent << "--list       Lists the names of classes with installed tests" << endl;
	cout << indent << "-v0          Sets verbosity level to 0 (concise summary only)" << endl;
	cout << indent << "-v1          Sets verbosity level to 1 (complete summary only)" << endl;
	cout << indent << "-v2          Sets verbosity level to 2 (*default* -- per-test results plus" << endl;
	cout << indent << "             complete summary)" << endl;
	cout << indent << "-v3          Sets verbosity level to 3 (dynamic loading information, per-test " << endl;
	cout << indent << "             results and timing info, plus complete summary)" << endl;
	cout << indent << "NAME         Instructs the program to run the test for the given class or all" << endl;
	cout << indent << "             the tests for the given suite. If some bonehead adds both a class" << endl;
	cout << indent << "             and a suite with the same name, the suite will be run, not the class" << endl;
	cout << indent << "             (unless the class is part of the suite with the same name :-). If no" << endl;
	cout << indent << "             classes or suites are specified, all available tests are run" << endl;
	cout << indent << "-lPATH       Adds PATH to the search path for dynamically loadable test" << endl;
	cout << indent << "             libraries" << endl;
}

void
BTestShell::PrintInstalledTests() {
	// Print out the list of installed suites
	cout << "------------------------------------------------------------------------------" << endl;
	cout << "Available Suites:" << endl;
	cout << "------------------------------------------------------------------------------" << endl;
	SuiteMap::const_iterator j;			
	for (j = fSuites.begin(); j != fSuites.end(); ++j)
		cout << j->first << endl;
	cout << endl;

	// Print out the list of installed tests
	cout << "------------------------------------------------------------------------------" << endl;
	cout << "Available Tests:" << endl;
	cout << "------------------------------------------------------------------------------" << endl;
	TestMap::const_iterator i;			
	for (i = fTests.begin(); i != fTests.end(); ++i)
		cout << i->first << endl;
	cout << endl;
}

bool
BTestShell::ProcessArguments(int argc, char *argv[]) {
	// If we're given no parameters, the default settings
	// will do just fine
	if (argc < 2)
		return true;

	// Handle each command line argument (skipping the first
	// which is just the app name)
	for (int i = 1; i < argc; i++) {
		std::string str(argv[i]);
		
		if (!ProcessArgument(str, argc, argv))
			return false;		
	}
	
	return true;
}

bool
BTestShell::ProcessArgument(std::string arg, int argc, char *argv[]) {
	if (arg == "--help") {
		PrintDescription(argc, argv);
		PrintHelp();
		return false;
	}
	else if (arg == "--list") {
		fListTestsAndExit = true;
	}
	else if (arg == "-v0") {
		fVerbosityLevel = v0;
	}
	else if (arg == "-v1") {
		fVerbosityLevel = v1;
	}
	else if (arg == "-v2") {
		fVerbosityLevel = v2;
	}
	else if (arg == "-v3") {
		fVerbosityLevel = v3;
	} else if (arg.length() >= 2 && arg[0] == '-' && arg[1] == 'l') {
		fLibDirs.insert(arg.substr(2, arg.size()-2));		
	} else {
		fTestsToRun.insert(arg);
	}
	return true;
}			

void
BTestShell::InitOutput() {
	// For vebosity level 2, we output info about each test
	// as we go. This involves a custom CppUnit::TestListener
	// class.
	if (fVerbosityLevel >= v2) {
		cout << "------------------------------------------------------------------------------" << endl;
		cout << "Tests" << endl;
		cout << "------------------------------------------------------------------------------" << endl;
		fTestResults.addListener(new BTestListener);
	}
	fTestResults.addListener(&fResultsCollector);
}

void
BTestShell::PrintResults() {

	if (fVerbosityLevel > v0) {
		// Print out detailed results for verbosity levels > 0
		cout << "------------------------------------------------------------------------------" << endl;
		cout << "Results " << endl;
		cout << "------------------------------------------------------------------------------" << endl;

		// Print failures and errors if there are any, otherwise just say "PASSED"
		::CppUnit::TestResultCollector::TestFailures::const_iterator iFailure;
		if (fResultsCollector.testFailuresTotal() > 0) {
			if (fResultsCollector.testFailures() > 0) {
				cout << "- FAILURES: " << fResultsCollector.testFailures() << endl;
				for (iFailure = fResultsCollector.failures().begin();
				     iFailure != fResultsCollector.failures().end();
				     ++iFailure)
				{
					if (!(*iFailure)->isError()) 
						cout << "    " << (*iFailure)->toString() << endl;
				}
			}
			if (fResultsCollector.testErrors() > 0) {
				cout << "- ERRORS: " << fResultsCollector.testErrors() << endl;
				for (iFailure = fResultsCollector.failures().begin();
				     iFailure != fResultsCollector.failures().end();
				     ++iFailure)
				{
					if ((*iFailure)->isError()) 
						cout << "    " << (*iFailure)->toString() << endl;
				}
			}
			
		}
		else
			cout << "+ PASSED" << endl;
			
		cout << endl;
			
	}
	else {
		// Print out concise results for verbosity level == 0
		if (fResultsCollector.testFailuresTotal() > 0)
			cout << "- FAILED" << endl;
		else
			cout << "+ PASSED" << endl;
	}
	
}

void
BTestShell::LoadDynamicSuites() {
	if (Verbosity() >= v3) {
		cout << "------------------------------------------------------------------------------" << endl;
		cout << "Loading " << endl;
		cout << "------------------------------------------------------------------------------" << endl;
	}

	std::set<std::string>::iterator i;
	for (i = fLibDirs.begin(); i != fLibDirs.end(); i++) {
		BDirectory libDir((*i).c_str());
		if (Verbosity() >= v3) 
			cout << "Checking " << *i << endl;
		int count = LoadSuitesFrom(&libDir);
		if (Verbosity() >= v3) {			
//			cout << "Loaded " << count << " suite" << (count == 1 ? "" : "s");
//			cout << " from " << *i << endl;
		}
	}
	
	if (Verbosity() >= v3)
		cout << endl;
}

void
BTestShell::UpdateTestDir(char *argv[]) {
	BPath path(argv[0]);
	if (path.InitCheck() == B_OK) {
		delete fTestDir;
		fTestDir = new BPath();
		if (path.GetParent(fTestDir) != B_OK)
			cout << "Couldn't get test dir." << endl;
	} else
		cout << "Couldn't find the path to the test app." << endl;
}

