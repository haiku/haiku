#include <ThreadedTestCase.h>
#include <Autolock.h>
#include <stdio.h>	

BThreadedTestCase::BThreadedTestCase(std::string name, std::string progressSeparator)
	: BTestCase(name)
	, fProgressSeparator(progressSeparator)
	, fUpdateLock(new BLocker())
{
}

BThreadedTestCase::~BThreadedTestCase() {
	// Kill our locker
	delete fUpdateLock;

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
		// Acquire the update lock
		BAutolock lock(fUpdateLock);
		std::map<thread_id, ThreadSubTestInfo*>::iterator i = fNumberMap.find(id);
		if (i != fNumberMap.end() && i->second) {
			// Handle multi-threaded case
			ThreadSubTestInfo *info = i->second;
			char num[32];
			sprintf(num, "%ld", info->subTestNum++);
			std::string str = std::string("[") + info->name + fProgressSeparator + num + "]";
			fUpdateList.push_back(str);
			return;
		}
	}
	
	// Handle single-threaded case
	BTestCase::NextSubTest();	
}

void
BThreadedTestCase::InitThreadInfo(thread_id id, std::string threadName) {
	BAutolock lock(fUpdateLock);	// Lock the number map
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

bool
BThreadedTestCase::RegisterForUse() {
	if (!fInUse) {
		fInUse = true;
		return true;
	} else
		return false;
}

void
BThreadedTestCase::UnregisterForUse() {
	fInUse = false;
}

std::vector<std::string>&
BThreadedTestCase::AcquireUpdateList() {
	fUpdateLock->Lock();
	return fUpdateList;
}

void
BThreadedTestCase::ReleaseUpdateList() {
	fUpdateLock->Unlock();
}
