/*
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "TeamMemoryBlock.h"


#include <AutoLocker.h>

#include "TeamMemoryBlockManager.h"


// #pragma mark - TeamMemoryBlock


TeamMemoryBlock::TeamMemoryBlock(target_addr_t baseAddress,
	TeamMemoryBlockManager* manager)
	:
	fValid(false),
	fBaseAddress(baseAddress),
	fBlockManager(manager)
{
}


TeamMemoryBlock::~TeamMemoryBlock()
{
}


void
TeamMemoryBlock::AddListener(Listener* listener)
{
	AutoLocker<BLocker> lock(fLock);
	fListeners.Add(listener);
}


bool
TeamMemoryBlock::HasListener(Listener* listener)
{
	AutoLocker<BLocker> lock(fLock);
	ListenerList::Iterator iterator = fListeners.GetIterator();
	while (iterator.HasNext()) {
		if (iterator.Next() == listener)
			return true;
	}

	return false;
}


void
TeamMemoryBlock::RemoveListener(Listener* listener)
{
	AutoLocker<BLocker> lock(fLock);
	fListeners.Remove(listener);
}


void
TeamMemoryBlock::MarkValid()
{
	fValid = true;
	NotifyDataRetrieved();
}


void
TeamMemoryBlock::Invalidate()
{
	fValid = false;
}


void
TeamMemoryBlock::NotifyDataRetrieved()
{
	for (ListenerList::Iterator it = fListeners.GetIterator();
			Listener* listener = it.Next();) {
		listener->MemoryBlockRetrieved(this);
	}
}


void
TeamMemoryBlock::LastReferenceReleased()
{
	fBlockManager->_RemoveBlock(fBaseAddress);

	BReferenceable::LastReferenceReleased();
}


// #pragma mark - TeamMemoryBlock


TeamMemoryBlock::Listener::~Listener()
{
}


void
TeamMemoryBlock::Listener::MemoryBlockRetrieved(TeamMemoryBlock* block)
{
}
