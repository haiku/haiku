/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "QueryList.h"

#include <pthread.h>

#include <Autolock.h>
#include <Debug.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>


static BLooper* sQueryLooper = NULL;
static pthread_once_t sInitOnce = PTHREAD_ONCE_INIT;


static void
initQueryLooper()
{
	sQueryLooper = new BLooper("query looper");
	sQueryLooper->Run();
}


// #pragma mark - QueryListener


QueryListener::~QueryListener()
{
}


// #pragma mark - QueryList


QueryList::QueryList()
	:
	fQuit(false),
	fListeners(5, true)
{
}


QueryList::~QueryList()
{
	fQuit = true;

	ThreadVector::const_iterator threadIterator = fFetchThreads.begin();
	for (; threadIterator != fFetchThreads.end(); threadIterator++) {
		wait_for_thread(*threadIterator, NULL);
	}

	QueryVector::iterator queryIterator = fQueries.begin();
	for (; queryIterator != fQueries.end(); queryIterator++) {
		delete *queryIterator;
	}
}


status_t
QueryList::Init(const char* predicate, BVolume* specificVolume)
{
	if (sQueryLooper == NULL)
		pthread_once(&sInitOnce, &initQueryLooper);

	if (Looper() != NULL)
		debugger("Init() called twice!");

	sQueryLooper->Lock();
	sQueryLooper->AddHandler(this);
	sQueryLooper->Unlock();

	if (specificVolume == NULL) {
		BVolumeRoster roster;
		BVolume volume;

		while (roster.GetNextVolume(&volume) == B_OK) {
			if (volume.KnowsQuery() && volume.KnowsAttr() && volume.KnowsMime())
				_AddVolume(volume, predicate);
		}
	} else
		_AddVolume(*specificVolume, predicate);

	return B_OK;
}


void
QueryList::AddListener(QueryListener* listener)
{
	BAutolock locker(this);

	// Add all entries that were already retrieved
	RefMap::const_iterator iterator = fRefs.begin();
	for (; iterator != fRefs.end(); iterator++) {
		listener->EntryCreated(*this, iterator->second, iterator->first.node);
	}

	fListeners.AddItem(listener);
}


void
QueryList::RemoveListener(QueryListener* listener)
{
	BAutolock locker(this);
	fListeners.RemoveItem(listener, false);
}


void
QueryList::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_QUERY_UPDATE:
		{
			int32 opcode = message->GetInt32("opcode", -1);
			int64 directory = message->GetInt64("directory", -1);
			int32 device = message->GetInt32("device", -1);
			int64 node = message->GetInt64("node", -1);

			if (opcode == B_ENTRY_CREATED) {
				const char* name = message->GetString("name");
				if (name != NULL) {
					entry_ref ref(device, directory, name);
					_AddEntry(ref, node);
				}
			} else if (opcode == B_ENTRY_REMOVED) {
				node_ref nodeRef(device, node);
				_RemoveEntry(nodeRef);
			}
			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


void
QueryList::_AddEntry(const entry_ref& ref, ino_t node)
{
	BAutolock locker(this);

	// TODO: catch bad_alloc
	fRefs.insert(std::make_pair(node_ref(ref.device, node), ref));

	_NotifyEntryCreated(ref, node);
}


void
QueryList::_RemoveEntry(const node_ref& nodeRef)
{
	BAutolock locker(this);
	RefMap::iterator found = fRefs.find(nodeRef);
	if (found != fRefs.end())
		_NotifyEntryRemoved(nodeRef);
}


void
QueryList::_NotifyEntryCreated(const entry_ref& ref, ino_t node)
{
	ASSERT(IsLocked());

	int32 count = fListeners.CountItems();
	for (int32 index = 0; index < count; index++) {
		fListeners.ItemAt(index)->EntryCreated(*this, ref, node);
	}
}


void
QueryList::_NotifyEntryRemoved(const node_ref& nodeRef)
{
	ASSERT(IsLocked());

	int32 count = fListeners.CountItems();
	for (int32 index = 0; index < count; index++) {
		fListeners.ItemAt(index)->EntryRemoved(*this, nodeRef);
	}
}


void
QueryList::_AddVolume(BVolume& volume, const char* predicate)
{
	BQuery* query = new BQuery();
	if (query->SetVolume(&volume) != B_OK
		|| query->SetPredicate(predicate) != B_OK
		|| query->SetTarget(this) != B_OK) {
		delete query;
	}

	// TODO: catch bad_alloc
	fQueries.push_back(query);
	Lock();
	fQueryQueue.push_back(query);
	Unlock();

	thread_id thread = spawn_thread(_FetchQuery, "query fetcher",
		B_NORMAL_PRIORITY, this);
	if (thread >= B_OK) {
		resume_thread(thread);

		fFetchThreads.push_back(thread);
	}
}


/*static*/ status_t
QueryList::_FetchQuery(void* self)
{
	return static_cast<QueryList*>(self)->_FetchQuery();
}


status_t
QueryList::_FetchQuery()
{
	RefMap map;

	BAutolock locker(this);
	BQuery* query = fQueryQueue.back();
	fQueryQueue.pop_back();
	locker.Unlock();

	query->Fetch();

	entry_ref ref;
	while (!fQuit && query->GetNextRef(&ref) == B_OK) {
		BEntry entry(&ref);
		node_ref nodeRef;
		if (entry.GetNodeRef(&nodeRef) == B_OK)
			map.insert(std::make_pair(nodeRef, ref));
	}
	if (fQuit)
		return B_INTERRUPTED;

	locker.Lock();

	RefMap::const_iterator iterator = map.begin();
	for (; iterator != map.end(); iterator++) {
		_AddEntry(iterator->second, iterator->first.node);
	}

	return B_OK;
}
