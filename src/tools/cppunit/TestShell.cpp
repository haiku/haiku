#include <TestShell.h>
#include <iostream>
#include <Path.h>
#include <stdio.h>

TestShell::TestShell(const std::string &description, SyncObject *syncObject)
		  : CppUnitShell(description, syncObject),
		    fTestDir(NULL)
{
}

TestShell::~TestShell()
{
	delete fTestDir;
}

int
TestShell::Run(int argc, char *argv[])
{
	// Let's hope BPath does work. ;-)
	BPath path(argv[0]);
	if (path.InitCheck() == B_OK) {
		fTestDir = new BPath();
		if (path.GetParent(fTestDir) != B_OK)
			printf("Couldn't get test dir.\n");
	} else
		printf("Couldn't find the path to the test app.\n");
	return CppUnitShell::Run(argc, argv);
}

const char*
TestShell::TestDir() const
{
	return (fTestDir ? fTestDir->Path() : NULL);
}

