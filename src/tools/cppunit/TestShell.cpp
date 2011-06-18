#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <image.h>
#include <Locker.h>
#include <Path.h>
#include <TLS.h>

#include <cppunit/Exception.h>
#include <cppunit/Test.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestSuite.h>

#include <TestShell.h>
#include <TestListener.h>

#ifndef NO_ELF_SYMBOL_PATCHING
#	include <ElfSymbolPatcher.h>
#endif

using std::cout;
using std::endl;
using std::set;

_EXPORT BTestShell *BTestShell::fGlobalShell = NULL;
const char BTestShell::indent[] = "  ";

_EXPORT
BTestShell::BTestShell(const string &description, SyncObject *syncObject)
	: fVerbosityLevel(v2)
	, fTestResults(syncObject)
	, fDescription(description)
	, fListTestsAndExit(false)
	, fTestDir(NULL)
#ifndef NO_ELF_SYMBOL_PATCHING
	, fPatchGroupLocker(new(nothrow) BLocker)
	, fPatchGroup(NULL)
	, fOldDebuggerHook(NULL)
	, fOldLoadAddOnHook(NULL)
	, fOldUnloadAddOnHook(NULL)
#endif // ! NO_ELF_SYMBOL_PATCHING
{
	fTLSDebuggerCall = tls_allocate();
}

_EXPORT
BTestShell::~BTestShell() {
	delete fTestDir;
#ifndef NO_ELF_SYMBOL_PATCHING
	delete fPatchGroupLocker;
#endif // ! NO_ELF_SYMBOL_PATCHING
}


_EXPORT
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
			      i++) {
			AddTest(i->first, i->second);
			if (Verbosity() >= v4 && i->second)
				cout << "  " << i->first << endl;
		}

		return B_OK;
	} else
		return B_BAD_VALUE;
}

_EXPORT
void
BTestShell::AddTest(const string &name, CppUnit::Test *test) {
	if (test != NULL)
		fTests[name] = test;
	else
		fTests.erase(name);
}

_EXPORT
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
//			cout << "Checking " << addonPath.Path() << "..." << endl;
			addonImage = load_add_on(addonPath.Path());
			err = (addonImage >= 0 ? B_OK : B_ERROR);
		}
		if (err == B_OK) {
//			cout << "..." << endl;
			err = get_image_symbol(addonImage, "getTestSuite",
				B_SYMBOL_TYPE_TEXT, reinterpret_cast<void **>(&func));
		} else {
//			cout << " !!! err == " << err << endl;
		}
		if (err == B_OK)
			err = AddSuite(func());
		if (err == B_OK)
			count++;
	}
	return count;
}

_EXPORT
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
		set<string>::const_iterator i;
		set<string> suitesToRemove;

		// Add all the tests from any specified suites to the list of
		// tests to run (since we use a set, this eliminates the concern
		// of having duplicate entries).
		for (i = fTestsToRun.begin(); i != fTestsToRun.end(); ++i) {
			// See if it's a suite (since it may just be a single test)
			if (fSuites.find(*i) != fSuites.end()) {
				// Note the suite name for later removal unless the
				// name is also the name of an available individual test
				if (fTests.find(*i) == fTests.end()) {
					suitesToRemove.insert(*i);
				}
				const TestMap &tests = fSuites[*i]->getTests();
				TestMap::const_iterator j;
				for (j = tests.begin(); j != tests.end(); j++) {
					fTestsToRun.insert( j->first );
				}
			}
		}

		// Remove the names of all of the suites we discovered from the
		// list of tests to run (unless there's also an installed individual
		// test of the same name).
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
	InstallPatches();
	suite.run(&fTestResults);
	UninstallPatches();
	PrintResults();

	return 0;
}

_EXPORT
BTestShell::VerbosityLevel
BTestShell::Verbosity() const {
	return fVerbosityLevel;
}

_EXPORT
const char*
BTestShell::TestDir() const {
	return (fTestDir ? fTestDir->Path() : NULL);
}

// ExpectDebuggerCall
/*!	\brief Marks the current thread as ready for a debugger() call.

	A subsequent call of debugger() will be intercepted and a respective
	flag will be set. WasDebuggerCalled() will then return \c true.
*/
_EXPORT
void
BTestShell::ExpectDebuggerCall()
{
	void *var = tls_get(fTLSDebuggerCall);
	::CppUnit::Asserter::failIf(var, "ExpectDebuggerCall(): Already expecting "
		"a debugger() call.");
	tls_set(fTLSDebuggerCall, (void*)1);
}

// WasDebuggerCalled
/*!	\brief Returns whether the current thread has invoked debugger() since
		   the last ExpectDebuggerCall() invocation and resets the mode so
		   that subsequent debugger() calls will hit the debugger.
	\return \c true, if debugger() has been called by the current thread since
			the last invocation of ExpectDebuggerCall(), \c false otherwise.
*/
_EXPORT
bool
BTestShell::WasDebuggerCalled()
{
	void *var = tls_get(fTLSDebuggerCall);
	tls_set(fTLSDebuggerCall, NULL);
	return ((int)var > 1);
}

_EXPORT
void
BTestShell::PrintDescription(int argc, char *argv[]) {
	cout << endl << fDescription;
}

_EXPORT
void
BTestShell::PrintHelp() {
	cout << endl;
	cout << "VALID ARGUMENTS:     " << endl;
	PrintValidArguments();
	cout << endl;

}

_EXPORT
void
BTestShell::PrintValidArguments() {
	cout << indent << "--help       Displays this help text plus some other garbage" << endl;
	cout << indent << "--list       Lists the names of classes with installed tests" << endl;
	cout << indent << "-v0          Sets verbosity level to 0 (concise summary only)" << endl;
	cout << indent << "-v1          Sets verbosity level to 1 (complete summary only)" << endl;
	cout << indent << "-v2          Sets verbosity level to 2 (*default* -- per-test results plus" << endl;
	cout << indent << "             complete summary)" << endl;
	cout << indent << "-v3          Sets verbosity level to 3 (partial dynamic loading information, " << endl;
	cout << indent << "             per-test results and timing info, plus complete summary)" << endl;
	cout << indent << "-v4          Sets verbosity level to 4 (complete dynamic loading information, " << endl;
	cout << indent << "             per-test results and timing info, plus complete summary)" << endl;
	cout << indent << "NAME         Instructs the program to run the test for the given class or all" << endl;
	cout << indent << "             the tests for the given suite. If some bonehead adds both a class" << endl;
	cout << indent << "             and a suite with the same name, the suite will be run, not the class" << endl;
	cout << indent << "             (unless the class is part of the suite with the same name :-). If no" << endl;
	cout << indent << "             classes or suites are specified, all available tests are run" << endl;
	cout << indent << "-lPATH       Adds PATH to the search path for dynamically loadable test" << endl;
	cout << indent << "             libraries" << endl;
}

_EXPORT
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

_EXPORT
bool
BTestShell::ProcessArguments(int argc, char *argv[]) {
	// If we're given no parameters, the default settings
	// will do just fine
	if (argc < 2)
		return true;

	// Handle each command line argument (skipping the first
	// which is just the app name)
	for (int i = 1; i < argc; i++) {
		string str(argv[i]);

		if (!ProcessArgument(str, argc, argv))
			return false;
	}

	return true;
}

_EXPORT
bool
BTestShell::ProcessArgument(string arg, int argc, char *argv[]) {
	if (arg == "--help") {
		PrintDescription(argc, argv);
		PrintHelp();
		return false;
	} else if (arg == "--list") {
		fListTestsAndExit = true;
	} else if (arg == "-v0") {
		fVerbosityLevel = v0;
	} else if (arg == "-v1") {
		fVerbosityLevel = v1;
	} else if (arg == "-v2") {
		fVerbosityLevel = v2;
	} else if (arg == "-v3") {
		fVerbosityLevel = v3;
	} else if (arg == "-v4") {
		fVerbosityLevel = v4;
	} else if (arg.length() >= 2 && arg[0] == '-' && arg[1] == 'l') {
		fLibDirs.insert(arg.substr(2, arg.size()-2));
	} else {
		fTestsToRun.insert(arg);
	}
	return true;
}

_EXPORT
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

_EXPORT
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

_EXPORT
void
BTestShell::LoadDynamicSuites() {
	if (Verbosity() >= v3) {
		cout << "------------------------------------------------------------------------------" << endl;
		cout << "Loading " << endl;
		cout << "------------------------------------------------------------------------------" << endl;
	}

	set<string>::iterator i;
	for (i = fLibDirs.begin(); i != fLibDirs.end(); i++) {
		BDirectory libDir((*i).c_str());
		if (Verbosity() >= v3)
			cout << "Checking " << *i << endl;
/*		int count =*/ LoadSuitesFrom(&libDir);
		if (Verbosity() >= v3) {
//			cout << "Loaded " << count << " suite" << (count == 1 ? "" : "s");
//			cout << " from " << *i << endl;
		}
	}

	if (Verbosity() >= v3)
		cout << endl;

	// Look for suites and tests with the same name and give a
	// warning, as this is only asking for trouble... :-)
	for (SuiteMap::const_iterator i = fSuites.begin(); i != fSuites.end(); i++) {
		if (fTests.find(i->first) != fTests.end() && Verbosity() > v0) {
			cout << "WARNING: '" << i->first << "' refers to both a test suite *and* an individual" <<
			endl << "         test. Both will be executed, but it is reccommended you rename" <<
			endl << "         one of them to resolve the conflict." <<
			endl << endl;
		}
	}

}

_EXPORT
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

// InstallPatches
/*!	\brief Patches the debugger() function.

	load_add_on() and unload_add_on() are patches as well, to keep the
	patch group up to date, when images are loaded/unloaded.
*/
_EXPORT
void
BTestShell::InstallPatches()
{
#ifndef NO_ELF_SYMBOL_PATCHING
	if (fPatchGroup) {
		cerr << "BTestShell::InstallPatches(): Patch group already exist!"
			<< endl;
		return;
	}
	BAutolock locker(fPatchGroupLocker);
	if (!locker.IsLocked()) {
		cerr << "BTestShell::InstallPatches(): Failed to acquire patch "
			"group lock!" << endl;
		return;
	}
	fPatchGroup = new(nothrow) ElfSymbolPatchGroup;
	// init the symbol patch group
	if (!fPatchGroup) {
		cerr << "BTestShell::InstallPatches(): Failed to allocate patch "
			"group!" << endl;
		return;
	}
	if (// debugger()
		fPatchGroup->AddPatch("debugger", (void*)&_DebuggerHook,
							  (void**)&fOldDebuggerHook) == B_OK
		// load_add_on()
		&& fPatchGroup->AddPatch("load_add_on", (void*)&_LoadAddOnHook,
								 (void**)&fOldLoadAddOnHook) == B_OK
		// unload_add_on()
		&& fPatchGroup->AddPatch("unload_add_on", (void*)&_UnloadAddOnHook,
								 (void**)&fOldUnloadAddOnHook) == B_OK
		) {
		// everything went fine
		fPatchGroup->Patch();
	} else {
		cerr << "BTestShell::InstallPatches(): Failed to patch all symbols!"
			<< endl;
		UninstallPatches();
	}
#endif // ! NO_ELF_SYMBOL_PATCHING
}

// UninstallPatches
/*!	\brief Undoes the patches applied by InstallPatches().
*/
_EXPORT
void
BTestShell::UninstallPatches()
{
#ifndef NO_ELF_SYMBOL_PATCHING
	BAutolock locker(fPatchGroupLocker);
	if (!locker.IsLocked()) {
		cerr << "BTestShell::UninstallPatches(): "
			"Failed to acquire patch group lock!" << endl;
		return;
	}
	if (fPatchGroup) {
		fPatchGroup->Restore();
		delete fPatchGroup;
		fPatchGroup = NULL;
	}
#endif // ! NO_ELF_SYMBOL_PATCHING
}

#ifndef NO_ELF_SYMBOL_PATCHING

// _Debugger
_EXPORT
void
BTestShell::_Debugger(const char *message)
{
	if (!this || !fPatchGroup) {
		debugger(message);
		return;
	}
	BAutolock locker(fPatchGroupLocker);
	if (!locker.IsLocked() || !fPatchGroup) {
		debugger(message);
		return;
	}
cout << "debugger() called: " << message << endl;
	void *var = tls_get(fTLSDebuggerCall);
	if (var)
		tls_set(fTLSDebuggerCall, (void*)((int)var + 1));
	else
		(*fOldDebuggerHook)(message);
}

// _LoadAddOn
_EXPORT
image_id
BTestShell::_LoadAddOn(const char *path)
{
	if (!this || !fPatchGroup)
		return load_add_on(path);
	BAutolock locker(fPatchGroupLocker);
	if (!locker.IsLocked() || !fPatchGroup)
		return load_add_on(path);
	image_id result = (*fOldLoadAddOnHook)(path);
	fPatchGroup->Update();
	return result;
}

// _UnloadAddOn
_EXPORT
status_t
BTestShell::_UnloadAddOn(image_id image)
{
	if (!this || !fPatchGroup)
		return unload_add_on(image);
	BAutolock locker(fPatchGroupLocker);
	if (!locker.IsLocked() || !fPatchGroup)
		return unload_add_on(image);

	if (!this || !fPatchGroup)
		return unload_add_on(image);
	status_t result = (*fOldUnloadAddOnHook)(image);
	fPatchGroup->Update();
	return result;
}

// _DebuggerHook
_EXPORT
void
BTestShell::_DebuggerHook(const char *message)
{
	fGlobalShell->_Debugger(message);
}

// _LoadAddOnHook
_EXPORT
image_id
BTestShell::_LoadAddOnHook(const char *path)
{
	return fGlobalShell->_LoadAddOn(path);
}

// _UnloadAddOnHook
_EXPORT
status_t
BTestShell::_UnloadAddOnHook(image_id image)
{
	return fGlobalShell->_UnloadAddOn(image);
}

#endif // ! NO_ELF_SYMBOL_PATCHING
