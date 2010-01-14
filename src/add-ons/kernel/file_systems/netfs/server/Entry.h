// Entry.h

#ifndef NET_FS_ENTRY_H
#define NET_FS_ENTRY_H

#include <HashString.h>
#include <util/DoublyLinkedList.h>

#include "EntryRef.h"

class Directory;
class Node;
class Path;
class Volume;

// Entry
class Entry : public DoublyLinkedListLinkImpl<Entry> {
public:
								Entry(Volume* volume, Directory* directory,
									const char* name, Node* node);
								~Entry();

			status_t			InitCheck() const;

			Volume*				GetVolume() const;
			Directory*			GetDirectory() const;
			NoAllocEntryRef		GetEntryRef() const;
			dev_t				GetVolumeID() const;
			ino_t				GetDirectoryID() const;
			const char*			GetName() const;
			Node*				GetNode() const;

			status_t			GetPath(Path* path);

			bool				Exists() const;

			bool				IsActualEntry() const;

private:
			struct GetDirEntryLink;
			friend struct GetDirEntryLink;
			friend class Directory;

			Volume*				fVolume;
			Directory*			fDirectory;
			HashString			fName;
			Node*				fNode;
			DoublyLinkedListLink<Entry> fDirEntryLink;
};

// GetDirEntryLink
struct Entry::GetDirEntryLink {
	DoublyLinkedListLink<Entry>* operator()(Entry* entry) const
	{
		return &entry->fDirEntryLink;
	}

	const DoublyLinkedListLink<Entry>* operator()(const Entry* entry) const
	{
		return &entry->fDirEntryLink;
	}
};

#endif	// NET_FS_ENTRY_H
