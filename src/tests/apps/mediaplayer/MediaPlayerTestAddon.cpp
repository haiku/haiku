#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "FileReadWriteTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("MediaPlayer");

	FileReadWriteTest::AddTests(*suite);

	return suite;
}
