#include "UnitTester.h"
#include <LockerSyncObject.h>
#include <string>

// ##### Include headers for statically linked tests here #####
#include <ExampleTest.h>

UnitTesterShell shell("OpenBeOS Unit Testing Framework", new LockerSyncObject);

int main(int argc, char *argv[]) {
	// ##### Add test suites for statically linked tests here #####
	shell.AddSuite( "Example", &ExampleTest::Suite );

	return shell.Run(argc, argv);
}

UnitTesterShell::UnitTesterShell(const std::string &description, SyncObject *syncObject)
	: BTestShell(description, syncObject)
{
}

void
UnitTesterShell::PrintDescription(int argc, char *argv[]) {
	std::string AppName = argv[0];
	cout << endl;
	cout << "This program is the central testing framework for the purpose" << endl;
	cout << "of testing and verifying the various kits, classes, functions," << endl;
	cout << "and the like that comprise OpenBeOS." << endl;
		
	if (AppName.rfind("UnitTester.R5") != std::string::npos) {
		cout << endl;
		cout << "Judging by its name (UnitTester.R5), this copy was" << endl;
		cout << "probably linked against Be Inc.'s R5 implementations" << endl;
		cout << "for the sake of comparison." << endl;
	} else if (AppName.rfind("UnitTester.OpenBeOS") != std::string::npos) {
		cout << endl;
		cout << "Judging by its name (UnitTester.OpenBeOS), this copy was probably" << endl;
		cout << "linked against our OpenBeOS implementations." << endl;
	}
}

