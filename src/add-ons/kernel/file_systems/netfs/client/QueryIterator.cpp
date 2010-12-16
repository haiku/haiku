// QueryIterator.cpp

#include "QueryIterator.h"

// constructor
QueryIterator::QueryIterator(Volume* volume)
	:
	BReferenceable(),
	fVolume(volume),
	fParentIterator(NULL),
	fVolumeLink()
{
}

// destructor
QueryIterator::~QueryIterator()
{
}

// GetVolume
Volume*
QueryIterator::GetVolume() const
{
	return fVolume;
}

// SetParentIterator
void
QueryIterator::SetParentIterator(HierarchicalQueryIterator* parent)
{
	fParentIterator = parent;
}

// GetParentIterator
HierarchicalQueryIterator*
QueryIterator::GetParentIterator() const
{
	return fParentIterator;
}

// ReadQuery
status_t
QueryIterator::ReadQuery(struct dirent* buffer, size_t bufferSize, int32 count,
	int32* countRead, bool* done)
{
	*countRead = 0;
	*done = true;
	return B_OK;
}


void
QueryIterator::LastReferenceReleased()
{
	// don't delete
}


// #pragma mark -

// constructor
HierarchicalQueryIterator::HierarchicalQueryIterator(Volume* volume)
	: QueryIterator(volume),
	  fSubIterators(),
	  fCurrentSubIterator(NULL)
{
}

// destructor
HierarchicalQueryIterator::~HierarchicalQueryIterator()
{
}

// GetCurrentSubIterator
QueryIterator*
HierarchicalQueryIterator::GetCurrentSubIterator() const
{
	return fCurrentSubIterator;
}

// NextSubIterator
QueryIterator*
HierarchicalQueryIterator::NextSubIterator()
{
	if (fCurrentSubIterator)
		fCurrentSubIterator = fSubIterators.GetNext(fCurrentSubIterator);
	return fCurrentSubIterator;
}

// RewindSubIterator
void
HierarchicalQueryIterator::RewindSubIterator()
{
	fCurrentSubIterator = fSubIterators.First();
}

// AddSubIterator
void
HierarchicalQueryIterator::AddSubIterator(QueryIterator* subIterator)
{
	if (!subIterator)
		return;

	fSubIterators.Insert(subIterator);
	subIterator->SetParentIterator(this);
	if (!fCurrentSubIterator)
		fCurrentSubIterator = subIterator;
}

// RemoveSubIterator
void
HierarchicalQueryIterator::RemoveSubIterator(QueryIterator* subIterator)
{
	if (!subIterator)
		return;

	if (fCurrentSubIterator == subIterator)
		NextSubIterator();
	subIterator->SetParentIterator(NULL);
	fSubIterators.Remove(subIterator);
}

// RemoveAllSubIterators
void
HierarchicalQueryIterator::RemoveAllSubIterators(
	DoublyLinkedList<QueryIterator>& subIterators)
{
	while (QueryIterator* iterator = fSubIterators.First()) {
		RemoveSubIterator(iterator);
		subIterators.Insert(iterator);
	}
}

