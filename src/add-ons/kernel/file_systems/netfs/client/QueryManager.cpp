// QueryManager.cpp

#include <new>

#include <fsproto.h>

#include <AutoLocker.h>
#include <HashMap.h>

#include "DebugSupport.h"
#include "Locker.h"
#include "QueryManager.h"
#include "Volume.h"
#include "VolumeManager.h"

typedef DoublyLinkedList<QueryIterator, QueryIterator::GetVolumeLink>
	IteratorList;

// IteratorMap
struct QueryManager::IteratorMap : HashMap<HashKey64<vnode_id>, IteratorList*> {
};


// constructor
QueryManager::QueryManager(VolumeManager* volumeManager)
	: fLock("query manager"),
	  fVolumeManager(volumeManager),
	  fIterators(NULL)
{
}

// destructor
QueryManager::~QueryManager()
{
	// delete all iterator lists (there shouldn't be any, though)
	for (IteratorMap::Iterator it = fIterators->GetIterator(); it.HasNext();) {
		IteratorList* iteratorList = it.Next().value;
		delete iteratorList;
	}
	delete fIterators;
}

// Init
status_t
QueryManager::Init()
{
	// check lock
	if (fLock.Sem() < 0)
		return fLock.Sem();

	// allocate iterator map
	fIterators = new(std::nothrow) IteratorMap;
	if (!fIterators)
		return B_NO_MEMORY;
	status_t error = fIterators->InitCheck();
	if (error != B_OK)
		return error;

	return B_OK;
}

// AddIterator
status_t
QueryManager::AddIterator(QueryIterator* iterator)
{
	if (!iterator || !iterator->GetVolume())
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);

	// get the iterator list for the volume
	vnode_id nodeID = iterator->GetVolume()->GetRootID();
	IteratorList* iteratorList = fIterators->Get(nodeID);
	if (!iteratorList) {
		// no list yet: create one
		iteratorList = new(std::nothrow) IteratorList;
		if (!iteratorList)
			return B_NO_MEMORY;

		// add it to the map
		status_t error = fIterators->Put(nodeID, iteratorList);
		if (error != B_OK) {
			delete iteratorList;
			return error;
		}
	}

	// add the iterator
	iteratorList->Insert(iterator);

	// get a volume reference for the iterator
	iterator->GetVolume()->AcquireReference();

	return B_OK;
}

// AddSubIterator
status_t
QueryManager::AddSubIterator(HierarchicalQueryIterator* iterator,
	QueryIterator* subIterator)
{
	if (!iterator || !subIterator)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);
	if (subIterator->GetVolume()->IsUnmounting())
		return B_BAD_VALUE;

	iterator->AddSubIterator(subIterator);

	return B_OK;
}

// RemoveSubIterator
status_t
QueryManager::RemoveSubIterator(HierarchicalQueryIterator* iterator,
	QueryIterator* subIterator)
{
	if (!iterator || !subIterator)
		return B_BAD_VALUE;

	AutoLocker<Locker> _(fLock);
	if (subIterator->GetParentIterator() != iterator)
		return B_BAD_VALUE;

	iterator->RemoveSubIterator(subIterator);

	return B_OK;
}

// GetCurrentSubIterator
QueryIterator*
QueryManager::GetCurrentSubIterator(HierarchicalQueryIterator* iterator)
{
	if (!iterator)
		return NULL;

	AutoLocker<Locker> _(fLock);
	QueryIterator* subIterator = iterator->GetCurrentSubIterator();
	if (subIterator)
		subIterator->AcquireReference();
	return subIterator;
}

// NextSubIterator
void
QueryManager::NextSubIterator(HierarchicalQueryIterator* iterator,
	QueryIterator* subIterator)
{
	if (iterator) {
		AutoLocker<Locker> _(fLock);
		if (iterator->GetCurrentSubIterator() == subIterator)
			iterator->NextSubIterator();
	}
}

// RewindSubIterator
void
QueryManager::RewindSubIterator(HierarchicalQueryIterator* iterator)
{
	if (iterator) {
		AutoLocker<Locker> _(fLock);
		iterator->RewindSubIterator();
	}
}

// PutIterator
void
QueryManager::PutIterator(QueryIterator* iterator)
{
	if (!iterator)
		return;

	AutoLocker<Locker> locker(fLock);
	if (iterator->ReleaseReference()) {
		// last reference removed: remove the iterator

		// remove its subiterators (if any)
		DoublyLinkedList<QueryIterator> subIterators;
		if (HierarchicalQueryIterator* hIterator
				= dynamic_cast<HierarchicalQueryIterator*>(iterator)) {
			hIterator->RemoveAllSubIterators(subIterators);
		}

		// remove from the parent iterator
		HierarchicalQueryIterator* parentIterator
			= iterator->GetParentIterator();
		if (parentIterator)
			parentIterator->RemoveSubIterator(iterator);

		// remove from the list
		vnode_id nodeID = iterator->GetVolume()->GetRootID();
		IteratorList* iteratorList = fIterators->Get(nodeID);
		if (iteratorList) {
			iteratorList->Remove(iterator);

			// if the list is empty, remove it completely
			if (!iteratorList->First()) {
				fIterators->Remove(nodeID);
				delete iteratorList;
			}
		} else {
			ERROR("QueryManager::PutIterator(): ERROR: No iterator list "
				"for volume %p!\n", iterator->GetVolume());
		}

		// free the iterator and surrender its volume reference
		Volume* volume = iterator->GetVolume();
		locker.Unlock();
		volume->FreeQueryIterator(iterator);
		volume->PutVolume();

		// put the subiterators
		while (QueryIterator* subIterator = subIterators.First()) {
			subIterators.Remove(subIterator);
			PutIterator(subIterator);
		}
	}
}

// VolumeUnmounting
//
// Removes all subiterators belonging to the volume from their parent iterators
// and puts the respective reference.
void
QueryManager::VolumeUnmounting(Volume* volume)
{
	if (!volume || !volume->IsUnmounting())
		return;

	vnode_id nodeID = volume->GetRootID();
	IteratorList iterators;
	DoublyLinkedList<QueryIterator> subIterators;
	AutoLocker<Locker> locker(fLock);
	if (IteratorList* iteratorList = fIterators->Get(nodeID)) {
		// Unset the parent of all iterators and remove one reference.
		// If the iterators are unreference, remove them.
		QueryIterator* iterator = iteratorList->First();
		while (iterator) {
			QueryIterator* nextIterator = iteratorList->GetNext(iterator);

			if (iterator->GetParentIterator()) {
				// remove its subiterators (if any)
				if (HierarchicalQueryIterator* hIterator
						= dynamic_cast<HierarchicalQueryIterator*>(iterator)) {
					hIterator->RemoveAllSubIterators(subIterators);
				}

				// remove from parent
				iterator->GetParentIterator()->RemoveSubIterator(iterator);

				// remove reference
				if (iterator->ReleaseReference()) {
					// no more reference: move to our local list
					iteratorList->Remove(iterator);
					iterators.Insert(iterator);
				}
			}

			iterator = nextIterator;
		}

		// if the list is empty now, remove it completely
		if (!iteratorList->First()) {
			fIterators->Remove(nodeID);
			delete iteratorList;
		}

		// free the iterators we have removed and surrender their volume
		// references
		locker.Unlock();
		while (QueryIterator* iterator = iterators.First()) {
			iterators.Remove(iterator);
			volume->FreeQueryIterator(iterator);
			volume->PutVolume();
		}

		// put the subiterators
		while (QueryIterator* subIterator = subIterators.First()) {
			subIterators.Remove(subIterator);
			PutIterator(subIterator);
		}
	}
}

