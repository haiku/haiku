#include "UnitTester.h"
#include <SemaphoreSyncObject.h>
#include <string>
#include <Directory.h>

// ##### Include headers for statically linked tests here #####
//#include <ExampleTest.h>

UnitTesterShell shell("OpenBeOS Unit Testing Framework", new SemaphoreSyncObject);

int main(int argc, char *argv[]) {
	// ##### Add test suites for statically linked tests here #####
//	shell.AddTest( "Example", ExampleTest::Suite() );

	BTestShell::SetShell(&shell);

	// Load our dynamically linked tests
	BDirectory libDir("./lib");
	int count = shell.LoadSuitesFrom(&libDir);
	cout << "Loaded " << count << " suites" << endl;

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
}

