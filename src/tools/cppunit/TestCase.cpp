#include <TestCase.h>
#include <TestShell.h>
#include <unistd.h>
#include <stdio.h>

BTestCase::BTestCase(std::string name)
	: CppUnit::TestCase(name)
	, fValidCWD(false)
	, fSubTestNum(0)
{
}

void
BTestCase::tearDown() {
	NextSubTestBlock();
}

void
BTestCase::NextSubTest() {
	BTestShell *shell = BTestShell::Shell();
	if (shell && shell->BeVerbose()) {
		printf("[%ld]", fSubTestNum++);
		fflush(stdout);
	}
}

void
BTestCase::NextSubTestBlock() {
	BTestShell *shell = BTestShell::Shell();
	if (shell && shell->BeVerbose()) 
		printf("\n");
}

/*! To return to the last saved working directory, call RestoreCWD(). */
void
BTestCase::SaveCWD() {
	fValidCWD = getcwd(fCurrentWorkingDir, B_PATH_NAME_LENGTH);
}

/*	If SaveCWD() has not been called and an alternate
	directory is specified by alternate, the current working directory is
	changed to alternate. If alternate is null, the current working directory
	is not modified.
*/
void
BTestCase::RestoreCWD(const char *alternate) {
	if (fValidCWD)
		chdir(fCurrentWorkingDir);
	else if (alternate != NULL)
		chdir(alternate);
}
