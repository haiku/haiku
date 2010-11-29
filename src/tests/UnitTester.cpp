/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <SemaphoreSyncObject.h>
#include <Directory.h>

#include "UnitTester.h"


UnitTesterShell::UnitTesterShell(const std::string &description,
	SyncObject *syncObject)
	:
	BTestShell(description, syncObject)
{
}


void
UnitTesterShell::PrintDescription(int argc, char *argv[])
{
	printf("This program is the central testing framework for the purpose\n"
		"of testing and verifying the various kits, classes, functions,\n"
		"and the like that comprise Haiku.\n");
}


void
UnitTesterShell::PrintValidArguments()
{
	BTestShell::PrintValidArguments();
	printf("  -haiku       Runs tests linked against our Haiku "
			"libraries (*default*)\n"
		"  -r5          Runs tests linked against Be Inc.'s R5 "
			"libraries (instead\n"
		"               of our libraries) for the sake of comparison.\n");
}


void
UnitTesterShell::LoadDynamicSuites()
{
	// Add the appropriate test lib path
	std::string defaultLibDir = std::string(GlobalTestDir()) + "/lib";
	fLibDirs.insert(defaultLibDir);

	// Load away
	BTestShell::LoadDynamicSuites();
}


// #pragma mark -


int
main(int argc, char *argv[])
{
	UnitTesterShell shell("Haiku Unit Testing Framework",
		new SemaphoreSyncObject);

	// ##### Add test suites for statically linked tests here #####
	//shell.AddTest("Example", ExampleTest::Suite());

	BTestShell::SetGlobalShell(&shell);

	// Load our dynamically linked tests

	int result = shell.Run(argc, argv);

	// Unset global shell, just to be sure
	BTestShell::SetGlobalShell(NULL);

	return result;
}
