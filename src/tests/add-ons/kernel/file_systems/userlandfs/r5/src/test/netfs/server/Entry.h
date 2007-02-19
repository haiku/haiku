// Entry.h

#ifndef NET_FS_ENTRY_H
#define NET_FS_ENTRY_H

#include "DLList.h"
#include "EntryRef.h"
#include "String.h"

class Directory;
class Node;
class Path;
class Volume;

// Entry
class Entry : public DLListLinkImpl<Entry> {
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

			Volume*				fVolume;
			Directory*			fDirectory;
			String				fName;
			Node*				fNode;
			DLListLink<Entry>	fDirEntryLink;
};

// GetDirEntryLink
struct Entry::GetDirEntryLink {
	DLListLink<Entry>* operator()(Entry* entry) const
	{
		return &entry->fDirEntryLink;
	}

	const DLListLink<Entry>* operator()(const Entry* entry) const
	{
		return &entry->fDirEntryLink;
	}
};

#endif	// NET_FS_ENTRY_H
