#include <TestResult.h>
#include <LockerSyncObject.h>

TestResult::TestResult()
	: CppUnit::TestResult(new LockerSyncObject())
{
}

