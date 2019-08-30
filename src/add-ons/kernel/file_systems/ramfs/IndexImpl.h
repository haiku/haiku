/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INDEX_IMPL_H
#define INDEX_IMPL_H

#include "Index.h"
#include "Node.h"

// AbstractIndexEntryIterator
class AbstractIndexEntryIterator {
public:
	AbstractIndexEntryIterator();
	virtual ~AbstractIndexEntryIterator();

	virtual Entry *GetCurrent() = 0;
	virtual Entry *GetCurrent(uint8 *buffer, size_t *keyLength) = 0;
	virtual Entry *GetPrevious() = 0;
	virtual Entry *GetNext() = 0;

	virtual status_t Suspend();
	virtual status_t Resume();
};


// NodeEntryIterator
template<typename NodeIterator>
class NodeEntryIterator : public AbstractIndexEntryIterator {
public:
	NodeEntryIterator();
	virtual ~NodeEntryIterator();

	void Unset();

	virtual Entry *GetCurrent();
	virtual Entry *GetCurrent(uint8 *buffer, size_t *keyLength) = 0;
	virtual Entry *GetPrevious();
	virtual Entry *GetNext();

	virtual status_t Suspend();
	virtual status_t Resume();

	Node *GetCurrentNode() const			{ return fNode; }

protected:
	NodeIterator	fIterator;
	Node			*fNode;
	Entry			*fEntry;
	bool			fInitialized;
	bool			fIsNext;
	bool			fSuspended;
};

// constructor
template<typename NodeIterator>
NodeEntryIterator<NodeIterator>::NodeEntryIterator()
	: AbstractIndexEntryIterator(),
	  fIterator(),
	  fNode(NULL),
	  fEntry(NULL),
	  fInitialized(false),
	  fIsNext(false),
	  fSuspended(false)
{
}

// destructor
template<typename NodeIterator>
NodeEntryIterator<NodeIterator>::~NodeEntryIterator()
{
}

// Unset
template<typename NodeIterator>
void
NodeEntryIterator<NodeIterator>::Unset()
{
	fNode = NULL;
	fEntry = NULL;
	fInitialized = false;
	fIsNext = false;
	fSuspended = false;
}

// GetCurrent
template<typename NodeIterator>
Entry *
NodeEntryIterator<NodeIterator>::GetCurrent()
{
	return fEntry;
}

// GetPrevious
template<typename NodeIterator>
Entry *
NodeEntryIterator<NodeIterator>::GetPrevious()
{
	return NULL;	// backwards iteration not implemented
}

// GetNext
template<typename NodeIterator>
Entry *
NodeEntryIterator<NodeIterator>::GetNext()
{
	if (!fInitialized || !fNode || fSuspended)
		return NULL;
	if (!(fEntry && fIsNext)) {
		while (fNode) {
			if (fEntry)
				fEntry = fNode->GetNextReferrer(fEntry);
			while (fNode && !fEntry) {
				fNode = NULL;
				if (Node **nodeP = fIterator.GetNext()) {
					fNode = *nodeP;
					fEntry = fNode->GetFirstReferrer();
				}
			}
			if (fEntry)
				break;
		}
	}
	fIsNext = false;
	return fEntry;
}

// Suspend
template<typename NodeIterator>
status_t
NodeEntryIterator<NodeIterator>::Suspend()
{
	status_t error = (fInitialized && !fSuspended ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		fSuspended = true;
	return error;
}

// Resume
template<typename NodeIterator>
status_t
NodeEntryIterator<NodeIterator>::Resume()
{
	status_t error = (fInitialized && fSuspended ? B_OK : B_BAD_VALUE);
	if (error == B_OK)
		fSuspended = false;
	return error;
}

#endif	// INDEX_IMPL_H
