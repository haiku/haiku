#include <TestShell.h>
#include <ThreadedTestCase.h>
#include <Autolock.h>
#include <stdio.h>
#include <stdarg.h>

using std::map;
using std::string;
using std::vector;

_EXPORT
BThreadedTestCase::BThreadedTestCase(string name, string progressSeparator)
	: BTestCase(name)
	, fInUse(false)
	, fProgressSeparator(progressSeparator)
	, fUpdateLock(new BLocker())
{
}

_EXPORT
BThreadedTestCase::~BThreadedTestCase() {
	// Kill our locker
	delete fUpdateLock;

	// Clean up
	for (map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.begin();
		   i != fNumberMap.end();
		     i++)
	{
		delete i->second;
	}
}

_EXPORT
void
BThreadedTestCase::NextSubTest() {
	// Find out what thread we're in
	thread_id id = find_thread(NULL);

	{
		// Acquire the update lock
		BAutolock lock(fUpdateLock);
		map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.find(id);
		if (i != fNumberMap.end() && i->second) {
			// Handle multi-threaded case
			ThreadSubTestInfo *info = i->second;
			char num[32];
			sprintf(num, "%ld", info->subTestNum++);
			string str = string("[") + info->name + fProgressSeparator + num + "]";
			fUpdateList.push_back(str);
			return;
		}
	}

	// Handle single-threaded case
	BTestCase::NextSubTest();
}

_EXPORT
void
BThreadedTestCase::Outputf(const char *str, ...) {
	if (BTestShell::GlobalBeVerbose()) {
		// Figure out if this is a multithreaded test or not
		thread_id id = find_thread(NULL);
		bool isSingleThreaded;
		{
			BAutolock lock(fUpdateLock);
			isSingleThreaded = fNumberMap.find(id) == fNumberMap.end();
		}
		if (isSingleThreaded) {
			va_list args;
			va_start(args, str);
			vprintf(str, args);
			va_end(args);
			fflush(stdout);
		} else {
			va_list args;
			va_start(args, str);
			char msg[1024];	// Need a longer string? Change the constant or change the function. :-)
			vsprintf(msg, str, args);
			va_end(args);
			{
				// Acquire the update lock and post our update
				BAutolock lock(fUpdateLock);
				fUpdateList.push_back(string(msg));
			}
		}
	}
}

_EXPORT
void
BThreadedTestCase::InitThreadInfo(thread_id id, string threadName) {
	BAutolock lock(fUpdateLock);	// Lock the number map
	map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.find(id);
	if (i != fNumberMap.end() && i->second) {
		i->second->name = threadName;
		i->second->subTestNum = 0;
	} else {
		// New addition
		ThreadSubTestInfo *info = new ThreadSubTestInfo();
		info->name = threadName;
		info->subTestNum = 0;
		fNumberMap[id] = info;
	}
}

_EXPORT
bool
BThreadedTestCase::RegisterForUse() {
	if (!fInUse) {
		fInUse = true;
		return true;
	} else
		return false;
}

_EXPORT
void
BThreadedTestCase::UnregisterForUse() {
	fInUse = false;
}

_EXPORT
vector<string>&
BThreadedTestCase::AcquireUpdateList() {
	fUpdateLock->Lock();
	return fUpdateList;
}

_EXPORT
void
BThreadedTestCase::ReleaseUpdateList() {
	fUpdateLock->Unlock();
}
