#include "UnitTester.h"
#include <SemaphoreSyncObject.h>
#include <Directory.h>

// ##### Include headers for statically linked tests here #####
//#include <ExampleTest.h>

UnitTesterShell shell("OpenBeOS Unit Testing Framework", new SemaphoreSyncObject);

int main(int argc, char *argv[]) {
	// ##### Add test suites for statically linked tests here #####
//	shell.AddTest( "Example", ExampleTest::Suite() );

	BTestShell::SetGlobalShell(&shell);

	// Load our dynamically linked tests

	return shell.Run(argc, argv);
}

//const std::string UnitTesterShell::defaultLibDir = "./lib";

UnitTesterShell::UnitTesterShell(const std::string &description, SyncObject *syncObject)
	: BTestShell(description, syncObject)
	, doR5Tests(false)
{
}

void
UnitTesterShell::PrintDescription(int argc, char *argv[]) {
	std::string AppName = argv[0];
	cout << endl;
	cout << "This program is the central testing framework for the purpose" << endl;
	cout << "of testing and verifying the various kits, classes, functions," << endl;
	cout << "and the like that comprise OpenBeOS." << endl;
		
/*
	if (AppName.rfind("UnitTester_r5") != std::string::npos) {
		cout << endl;
		cout << "Judging by its name (UnitTester_r5), this copy was" << endl;
		cout << "probably linked against Be Inc.'s R5 implementations" << endl;
		cout << "for the sake of comparison." << endl;
	} else if (AppName.rfind("UnitTester") != std::string::npos) {
		cout << endl;
		cout << "Judging by its name (UnitTester), this copy was probably" << endl;
		cout << "linked against our own OpenBeOS implementations." << endl;
	}
*/
}

void
UnitTesterShell::PrintValidArguments() {
	BTestShell::PrintValidArguments();
	cout << indent << "-obos        Runs tests linked against our OpenBeOS libraries (*default*)" << endl;
	cout << indent << "-r5          Runs tests linked against Be Inc.'s R5 libraries (instead" << endl;
	cout << indent << "             of our libraries) for the sake of comparison." << endl;
}

bool
UnitTesterShell::ProcessArgument(std::string arg, int argc, char *argv[]) {
	if (arg == "-r5") {
		doR5Tests = true;
	} else if (arg == "-obos") {
		doR5Tests = false;
	} else
		return BTestShell::ProcessArgument(arg, argc, argv);
		
	return true;
}

void
UnitTesterShell::LoadDynamicSuites() {
	// Add the appropriate test lib path 
	string defaultLibDir = string(GlobalTestDir()) + "/lib";
	fLibDirs.insert(defaultLibDir + (doR5Tests ? "_r5" : ""));

	// Load away
	BTestShell::LoadDynamicSuites();
}
