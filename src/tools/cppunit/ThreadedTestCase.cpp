#include <ThreadedTestCase.h>
#include <SafetyLock.h>

BThreadedTestCase::BThreadedTestCase(std::string name, std::string progressSeparator)
	: BTestCase(name)
	, fProgressSeparator(progressSeparator)
	, fNumberMapLock(new BLocker())
{
}

BThreadedTestCase::~BThreadedTestCase() {
	// Kill our locker
	delete fNumberMapLock;

	// Clean up
	for (std::map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.begin();
		   i != fNumberMap.end();
		     i++)
	{
		delete i->second;
	}		   
}

void
BThreadedTestCase::NextSubTest() {
	// Find out what thread we're in
	thread_id id = find_thread(NULL);

	{
		// Lock the number map
		SafetyLock lock(fNumberMapLock);
		std::map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.find(id);
		if (i != fNumberMap.end() && i->second) {
			// Handle multi-threaded case
			ThreadSubTestInfo *info = i->second;
			cout << "[" <<  info->subTestNum++ << fProgressSeparator << info->name << "]";
			return;
		}
	}
	
	// Handle single-threaded case
	BTestCase::NextSubTest();	
}

void
BThreadedTestCase::InitThreadInfo(thread_id id, std::string threadName) {
	SafetyLock lock(fNumberMapLock);	// Lock the number map
	std::map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.find(id);
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
