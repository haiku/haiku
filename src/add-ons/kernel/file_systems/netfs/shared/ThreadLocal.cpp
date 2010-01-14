// ThreadLocal.cpp

#include <new>

#include <OS.h>

#include "HashMap.h"
#include "ThreadLocal.h"

// ThreadLocalFreeHandler

// constructor
ThreadLocalFreeHandler::ThreadLocalFreeHandler()
{
}

// destructor
ThreadLocalFreeHandler::~ThreadLocalFreeHandler()
{
}


// ThreadLocal

// ThreadLocalMap
struct ThreadLocal::ThreadLocalMap
	: public SynchronizedHashMap<HashKey32<thread_id>, void*> {
};

// constructor
ThreadLocal::ThreadLocal(ThreadLocalFreeHandler* freeHandler)
	: fMap(NULL),
	  fFreeHandler(freeHandler)
{
	fMap = new(std::nothrow) ThreadLocalMap;
}

// destructor
ThreadLocal::~ThreadLocal()
{
	delete fMap;
}

// Set
status_t
ThreadLocal::Set(void* data)
{
	if (!fMap)
		return B_NO_MEMORY;
	thread_id thread = find_thread(NULL);
	fMap->Lock();
	// free old data, if any
	if (fFreeHandler &&fMap->ContainsKey(thread))
		fFreeHandler->Free(fMap->Get(thread));
	// put the new data
	status_t error = fMap->Put(thread, data);
	fMap->Unlock();
	return error;
}

// Unset
void
ThreadLocal::Unset()
{
	if (!fMap)
		return;
	thread_id thread = find_thread(NULL);
	fMap->Lock();
	if (fFreeHandler) {
		if (fMap->ContainsKey(thread))
			fFreeHandler->Free(fMap->Remove(thread));
	} else
		fMap->Remove(thread);
	fMap->Unlock();
}

// Get
void*
ThreadLocal::Get() const
{
	if (!fMap)
		return NULL;
	return fMap->Get(find_thread(NULL));
}

