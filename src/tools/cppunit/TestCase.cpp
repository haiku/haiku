#include <TestCase.h>
#include <TestShell.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>

BTestCase::BTestCase(string name)
	: CppUnit::TestCase(name)
	, fValidCWD(false)
	, fSubTestNum(0)
{
}

void
BTestCase::tearDown() {
	if (fSubTestNum != 0)
		NextSubTestBlock();
}

void
BTestCase::NextSubTest() {
	if (BTestShell::GlobalBeVerbose()) {
		printf("[%ld]", fSubTestNum++);
		fflush(stdout);
	}
}

void
BTestCase::NextSubTestBlock() {
	if (BTestShell::GlobalBeVerbose()) 
		printf("\n");
}

void
BTestCase::Outputf(const char *str, ...) {
	if (BTestShell::GlobalBeVerbose()) {
		va_list args;
		va_start(args, str);
		vprintf(str, args);
		va_end(args);
		fflush(stdout);
	}
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

