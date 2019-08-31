/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "Directory.h"
#include "Entry.h"
#include "EntryIterator.h"
#include "File.h"
#include "SymLink.h"
#include "Volume.h"

// constructor
Directory::Directory(Volume *volume)
	: Node(volume, NODE_TYPE_DIRECTORY),
	  fEntries()
{
}

// destructor
Directory::~Directory()
{
	// delete all entries
	while (Entry *entry = fEntries.First()) {
		if (DeleteEntry(entry) != B_OK) {
			FATAL("Could not delete all entries in directory.\n");
			break;
		}
	}
}

// Link
status_t
Directory::Link(Entry *entry)
{
	if (fReferrers.IsEmpty())
		return Node::Link(entry);
	return B_IS_A_DIRECTORY;
}

// Unlink
status_t
Directory::Unlink(Entry *entry)
{
	if (entry == fReferrers.First())
		return Node::Unlink(entry);
	return B_BAD_VALUE;
}

// SetSize
status_t
Directory::SetSize(off_t /*newSize*/)
{
	return B_IS_A_DIRECTORY;
}

// GetSize
off_t
Directory::GetSize() const
{
	return 0;
}

// GetParent
Directory *
Directory::GetParent() const
{
	Entry *entry = fReferrers.First();
	return (entry ? entry->GetParent() : NULL);
}

// CreateDirectory
status_t
Directory::CreateDirectory(const char *name, Directory **directory)
{
	status_t error = (name && directory ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// create directory
		if (Directory *node = new(nothrow) Directory(GetVolume())) {
			error = _CreateCommon(node, name);
				// deletes the node on failure
			if (error == B_OK)
				*directory = node;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// CreateFile
status_t
Directory::CreateFile(const char *name, File **file)
{
	status_t error = (name && file ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// create file
		if (File *node = new(nothrow) File(GetVolume())) {
			error = _CreateCommon(node, name);
				// deletes the node on failure
			if (error == B_OK)
				*file = node;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// CreateSymLink
status_t
Directory::CreateSymLink(const char *name, const char *path, SymLink **symLink)
{
	status_t error = (name && symLink ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// create symlink
		if (SymLink *node = new(nothrow) SymLink(GetVolume())) {
			error = node->SetLinkedPath(path);
			if (error == B_OK) {
				error = _CreateCommon(node, name);
					// deletes the node on failure
				if (error == B_OK)
					*symLink = node;
			} else
				delete node;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// AddEntry
status_t
Directory::AddEntry(Entry *entry)
{
	status_t error = (entry && !entry->GetParent() ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		fEntries.Insert(entry);
		entry->SetParent(this);
		error = GetVolume()->EntryAdded(GetID(), entry);
		if (error == B_OK) {
			MarkModified(B_STAT_MODIFICATION_TIME);
		} else {
			fEntries.Remove(entry);
			entry->SetParent(NULL);
		}
	}
	return error;
}

// CreateEntry
status_t
Directory::CreateEntry(Node *node, const char *name, Entry **_entry)
{
	status_t error = (node ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// create an entry
		Entry *entry = new(nothrow) Entry(name);
		if (entry) {
			error = entry->InitCheck();
			if (error == B_OK) {
				// link to the node
				error = entry->Link(node);
				if (error == B_OK) {
					// add the entry
					error = AddEntry(entry);
					if (error == B_OK) {
						if (_entry)
							*_entry = entry;
					} else {
						// failure: unlink the node
						entry->Unlink();
					}
				}
			}
			// delete the entry on failure
			if (error != B_OK)
				delete entry;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// RemoveEntry
status_t
Directory::RemoveEntry(Entry *entry)
{
	status_t error = (entry && entry->GetParent() == this ? B_OK
														  : B_BAD_VALUE);
	if (error == B_OK) {
		// move all iterators pointing to the entry to the next entry
		if (GetVolume()->IteratorLock()) {
			// set the iterators' current entry
			Entry *nextEntry = fEntries.GetNext(entry);
			DoublyLinkedList<EntryIterator> *iterators
				= entry->GetEntryIteratorList();
			for (EntryIterator *iterator = iterators->First();
				 iterator;
				 iterator = iterators->GetNext(iterator)) {
				iterator->SetCurrent(nextEntry, true);
			}
			// Move the iterators from one list to the other, or just remove
			// them, if there is no next entry.
			if (nextEntry) {
				DoublyLinkedList<EntryIterator> *nextIterators
					= nextEntry->GetEntryIteratorList();
				nextIterators->MoveFrom(iterators);
			} else
				iterators->RemoveAll();
			GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
		// remove the entry
		if (error == B_OK) {
			error = GetVolume()->EntryRemoved(GetID(), entry);
			if (error == B_OK) {
				fEntries.Remove(entry);
				entry->SetParent(NULL);
				MarkModified(B_STAT_MODIFICATION_TIME);
			}
		}
	}
	return error;
}

// DeleteEntry
status_t
Directory::DeleteEntry(Entry *entry)
{
	status_t error = RemoveEntry(entry);
	if (error == B_OK) {
		error = entry->Unlink();
		if (error == B_OK)
			delete entry;
		else {
			FATAL("Failed to Unlink() entry %p from node %" B_PRIdINO "!\n", entry,
				   entry->GetNode()->GetID());
			AddEntry(entry);
		}
	}
	return error;
}

// FindEntry
status_t
Directory::FindEntry(const char *name, Entry **_entry) const
{
	status_t error = (name && _entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
/*
		Entry *entry = NULL;
		while (GetNextEntry(&entry) == B_OK) {
			if (!strcmp(entry->GetName(), name)) {
				*_entry = entry;
				return B_OK;
			}
		}
		error = B_ENTRY_NOT_FOUND;
*/
		error = GetVolume()->FindEntry(GetID(), name, _entry);
	}
	return error;
}

// FindNode
status_t
Directory::FindNode(const char *name, Node **node) const
{
	status_t error = (name && node ? B_OK : B_BAD_VALUE);
	Entry *entry = NULL;
	if (error == B_OK && (error = FindEntry(name, &entry)) == B_OK)
		*node = entry->GetNode();
	return error;
}

// FindAndGetNode
status_t
Directory::FindAndGetNode(const char *name, Node **node, Entry **_entry) const
{
	status_t error = (name && node ? B_OK : B_BAD_VALUE);
	Entry *entry = NULL;
	if (error == B_OK && (error = FindEntry(name, &entry)) == B_OK) {
		*node = entry->GetNode();
		if (_entry)
			*_entry = entry;
		error = GetVolume()->GetVNode(*node);
	}
	return error;
}

// GetPreviousEntry
status_t
Directory::GetPreviousEntry(Entry **entry) const
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!*entry)
			*entry = fEntries.Last();
		else if ((*entry)->GetParent() == this)
			*entry = fEntries.GetPrevious(*entry);
		else
			error = B_BAD_VALUE;
		if (error == B_OK && !*entry)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetNextEntry
status_t
Directory::GetNextEntry(Entry **entry) const
{
	status_t error = (entry ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!*entry)
			*entry = fEntries.First();
		else if ((*entry)->GetParent() == this)
			*entry = fEntries.GetNext(*entry);
		else
			error = B_BAD_VALUE;
		if (error == B_OK && !*entry)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetAllocationInfo
void
Directory::GetAllocationInfo(AllocationInfo &info)
{
	Node::GetAllocationInfo(info);
	info.AddDirectoryAllocation();
	Entry *entry = NULL;
	while (GetNextEntry(&entry) == B_OK)
		entry->GetAllocationInfo(info);
}

// _CreateCommon
status_t
Directory::_CreateCommon(Node *node, const char *name)
{
	status_t error = node->InitCheck();
	if (error == B_OK) {
		// add node to directory
		error = CreateEntry(node, name);
	}
	if (error != B_OK)
		delete node;
	return error;
}

