#include "Test.StorageKit.h"
#include <unistd.h>
#include "TestUtils.h"

#include <Path.h>

// ##### Include your test headers here #####
#include "DirectoryTest.h"
#include "EntryTest.h"
#include "FileTest.h"
#include "FindDirectoryTest.h"
#include "MimeTypeTest.h"
#include "NodeTest.h"
#include "PathTest.h"
#include "QueryTest.h"
#include "ResourcesTest.h"
#include "ResourceStringsTest.h"
#include "SymLinkTest.h"


StorageKit::TestShell shell;

int main(int argc, char *argv[]) {
	// ##### Add your test suites here #####
	shell.AddSuite( "BDirectory", &DirectoryTest::Suite );
	shell.AddSuite( "BEntry", &EntryTest::Suite );
	shell.AddSuite( "BFile", &FileTest::Suite );
	shell.AddSuite( "BMimeType", &MimeTypeTest::Suite );
	shell.AddSuite( "BNode", &NodeTest::Suite );
	shell.AddSuite( "BPath", &PathTest::Suite );
	shell.AddSuite( "BQuery", &QueryTest::Suite );
	shell.AddSuite( "BResources", &ResourcesTest::Suite );
	shell.AddSuite( "BResourceStrings", &ResourceStringsTest::Suite );
	shell.AddSuite( "BSymLink", &SymLinkTest::Suite );
	shell.AddSuite( "FindDirectory", &FindDirectoryTest::Suite );

	return shell.Run(argc, argv);
}

namespace StorageKit {

//-------------------------------------------------------------------------------
// StorageKit::TestShell
//-------------------------------------------------------------------------------

TestShell::TestShell()
		  : CppUnitShell(),
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

void
TestShell::PrintDescription(int argc, char *argv[]) {
	std::string AppName = argv[0];
	cout << endl;
	cout << "This program is the central testing framework for the purpose" << endl;
	cout << "of testing and verifying the behavior of the OpenBeOS Storage" << endl;
	cout << "Kit." << endl;
		
	if (AppName.rfind("Test.StorageKit.R5") != std::string::npos) {
		cout << endl;
		cout << "Judging by its name (Test.StorageKit.R5), this copy was" << endl;
		cout << "probably linked again Be Inc.'s R5 Storage Kit for the sake" << endl;
		cout << "of comparison against our own implementation." << endl;
	}
	else if (AppName.rfind("Test.StorageKit.OpenBeOS") != std::string::npos) {
		cout << endl;
		cout << "Judging by its name (Test.StorageKit.OpenBeOS), this copy" << endl;
		cout << "was probably linked against the OpenBeOS implementation of" << endl;
		cout << "the Storage Kit." << endl;
	}
}

const char*
TestShell::TestDir() const
{
	return (fTestDir ? fTestDir->Path() : NULL);
}

//-------------------------------------------------------------------------------
// TestCase
//-------------------------------------------------------------------------------

TestCase::TestCase() : fValidCWD(false) {
}

// Saves the location of the current working directory. To return to the
// last saved working directory, all \ref RestorCWD().
void
TestCase::SaveCWD() {
	fValidCWD = getcwd(fCurrentWorkingDir, B_PATH_NAME_LENGTH);
}

/*	Restores the current working directory to last directory saved by a
	call to SaveCWD(). If SaveCWD() has not been called and an alternate
	directory is specified by alternate, the current working directory is
	changed to alternate. If alternate is null, the current working directory
	is not modified.
*/
void
TestCase::RestoreCWD(const char *alternate) {
	if (fValidCWD)
		chdir(fCurrentWorkingDir);
	else if (alternate != NULL)
		chdir(alternate);
}

}
