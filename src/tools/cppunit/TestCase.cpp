#include <TestCase.h>
#include <unistd.h>

TestCase::TestCase()
	: CppUnit::TestCase()
	, fValidCWD(false)
{
}

TestCase::TestCase(std::string name)
	: CppUnit::TestCase(name)
	, fValidCWD(false)
{
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
