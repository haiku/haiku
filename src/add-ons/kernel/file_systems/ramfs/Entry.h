/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ENTRY_H
#define ENTRY_H

#include <fs_interface.h>
#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include "String.h"

class AllocationInfo;
class Directory;
class EntryIterator;
class Node;

class Entry : public DoublyLinkedListLinkImpl<Entry> {
public:
	Entry(const char *name, Node *node = NULL, Directory *parent = NULL);
	~Entry();

	status_t InitCheck() const;

	Entry*& HashLink()	{ return fHashLink; }

	inline void SetParent(Directory *parent)	{ fParent = parent; }
	Directory *GetParent() const				{ return fParent; }

//	inline void SetNode(Node *node)				{ fNode = node; }
	status_t Link(Node *node);
	status_t Unlink();
	Node *GetNode() const						{ return fNode; }

	status_t SetName(const char *newName);
	inline const char *GetName() const			{ return fName.GetString(); }

//	inline Volume *GetVolume() const			{ return fVolume; }

	inline DoublyLinkedListLink<Entry> *GetReferrerLink()
		{ return &fReferrerLink; }

	// entry iterator management
	void AttachEntryIterator(EntryIterator *iterator);
	void DetachEntryIterator(EntryIterator *iterator);
	inline DoublyLinkedList<EntryIterator> *GetEntryIteratorList()
		{ return &fIterators; }

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	Entry					*fHashLink;
	Directory				*fParent;
	Node					*fNode;
	String					fName;
	DoublyLinkedListLink<Entry>		fReferrerLink;
	// iterator management
	DoublyLinkedList<EntryIterator>	fIterators;
};

// GetNodeReferrerLink
class GetNodeReferrerLink {
private:
	typedef DoublyLinkedListLink<Entry> Link;

public:
	inline Link *operator()(Entry *entry) const
	{
		return entry->GetReferrerLink();
	}
};

#endif	// ENTRY_H
