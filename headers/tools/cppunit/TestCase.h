#ifndef _beos_test_case_h_
#define _beos_test_case_h_

#include <BeBuild.h>
#include <cppunit/TestCase.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

//! Base class for single threaded unit tests
class CPPUNIT_API BTestCase : public CppUnit::TestCase {
public:
	BTestCase(std::string Name = "");

	//! Displays the next sub test progress indicator (i.e. [0][1][2][3]...).
	virtual void NextSubTest();

	//! Starts a new sub test block (i.e. prints a newline :-)
	virtual void NextSubTestBlock();

	/*! \brief Prints to standard out just like printf, except shell verbosity
		settings are honored.
	*/
	virtual void Outputf(const char *str, ...);

	//! Saves the location of the current working directory.
	void SaveCWD();

	virtual void tearDown();

	//! Restores the current working directory to last directory saved by a	call to SaveCWD().
	void RestoreCWD(const char *alternate = NULL);
protected:
	bool fValidCWD;
	char fCurrentWorkingDir[B_PATH_NAME_LENGTH+1];
	int32 fSubTestNum;
};

#endif // _beos_test_case_h_
