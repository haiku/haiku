#include <iostream>

#include "UnitTesterHelper.h"
#include <SemaphoreSyncObject.h>
#include <Directory.h>

// ##### Include headers for statically linked tests here #####
//#include <ExampleTest.h>


int main(int argc, char *argv[]) {
	UnitTesterShell shell("OpenBeOS Unit Testing Framework", new SemaphoreSyncObject);
	// ##### Add test suites for statically linked tests here #####
//	shell.AddTest( "Example", ExampleTest::Suite() );

	BTestShell::SetGlobalShell(&shell);

	// Load our dynamically linked tests

	int result = shell.Run(argc, argv);

	// Unset global shell, just to be sure
	BTestShell::SetGlobalShell(NULL);

	return result;
}

//const string UnitTesterShell::defaultLibDir = "./lib";

UnitTesterShell::UnitTesterShell(const string &description, SyncObject *syncObject)
	: BTestShell(description, syncObject)
{
}

void
UnitTesterShell::PrintDescription(int argc, char *argv[]) {
	string AppName = argv[0];
	cout << endl;
	cout << "This program is the central testing framework for the purpose" << endl;
	cout << "of testing and verifying the various kits, classes, functions," << endl;
	cout << "and the like that comprise OpenBeOS." << endl;
}

void
UnitTesterShell::PrintValidArguments() {
	BTestShell::PrintValidArguments();
	cout << indent << "-obos        Runs tests linked against our OpenBeOS libraries (*default*)" << endl;
	cout << indent << "-r5          Runs tests linked against Be Inc.'s R5 libraries (instead" << endl;
	cout << indent << "             of our libraries) for the sake of comparison." << endl;
}

void
UnitTesterShell::LoadDynamicSuites() {
	// Add the appropriate test lib path 
	string defaultLibDir = string(GlobalTestDir()) + "/lib";
	fLibDirs.insert(defaultLibDir);

	// Load away
	BTestShell::LoadDynamicSuites();
}
