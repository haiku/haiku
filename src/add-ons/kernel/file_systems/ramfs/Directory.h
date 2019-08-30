/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <util/DoublyLinkedList.h>

#include "Node.h"


class Entry;
class File;
class SymLink;

class Directory : public Node {
public:
	Directory(Volume *volume);
	virtual ~Directory();

	virtual status_t Link(Entry *entry);
	virtual status_t Unlink(Entry *entry);

	virtual status_t SetSize(off_t newSize);
	virtual off_t GetSize() const;

	Directory *GetParent() const;

	status_t CreateDirectory(const char *name, Directory **directory);
	status_t CreateFile(const char *name, File **file);
	status_t CreateSymLink(const char *name, const char *path,
						   SymLink **symLink);

	bool IsEmpty() const { return fEntries.IsEmpty(); }

	status_t AddEntry(Entry *entry);
	status_t CreateEntry(Node *node, const char *name, Entry **entry = NULL);
	status_t RemoveEntry(Entry *entry);
	status_t DeleteEntry(Entry *entry);

	status_t FindEntry(const char *name, Entry **entry) const;
	status_t FindNode(const char *name, Node **node) const;
	status_t FindAndGetNode(const char *name, Node **node,
							Entry **entry = NULL) const;

	status_t GetPreviousEntry(Entry **entry) const;
	status_t GetNextEntry(Entry **entry) const;

	// debugging
	virtual void GetAllocationInfo(AllocationInfo &info);

private:
	status_t _CreateCommon(Node *node, const char *name);

private:
	DoublyLinkedList<Entry>	fEntries;
};

#endif	// DIRECTORY_H
