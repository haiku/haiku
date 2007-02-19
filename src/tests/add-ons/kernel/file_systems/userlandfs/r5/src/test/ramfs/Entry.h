// Entry.h

#ifndef ENTRY_H
#define ENTRY_H

#include <SupportDefs.h>

#include "DLList.h"
#include "fsproto.h"
#include "String.h"

class AllocationInfo;
class Directory;
class EntryIterator;
class Node;

class Entry : public DLListLinkImpl<Entry> {
public:
	Entry(const char *name, Node *node = NULL, Directory *parent = NULL);
	~Entry();

	status_t InitCheck() const;

	inline void SetParent(Directory *parent)	{ fParent = parent; }
	Directory *GetParent() const				{ return fParent; }

//	inline void SetNode(Node *node)				{ fNode = node; }
	status_t Link(Node *node);
	status_t Unlink();
	Node *GetNode() const						{ return fNode; }

	status_t SetName(const char *newName);
	inline const char *GetName() const			{ return fName.GetString(); }

//	inline Volume *GetVolume() const			{ return fVolume; }
	
	inline DLListLink<Entry> *GetReferrerLink()	{ return &fReferrerLink; }

	// entry iterator management
	void AttachEntryIterator(EntryIterator *iterator);
	void DetachEntryIterator(EntryIterator *iterator);
	inline DLList<EntryIterator> *GetEntryIteratorList()
		{ return &fIterators; }

	// debugging
	void GetAllocationInfo(AllocationInfo &info);

private:
	Directory				*fParent;
	Node					*fNode;
	String					fName;
	DLListLink<Entry>		fReferrerLink;
	// iterator management
	DLList<EntryIterator>	fIterators;
};

// GetNodeReferrerLink
class GetNodeReferrerLink {
private:
	typedef DLListLink<Entry> Link;

public:
	inline Link *operator()(Entry *entry) const
	{
		return entry->GetReferrerLink();
	}
};

#endif	// ENTRY_H
