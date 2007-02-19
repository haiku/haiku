// Node.h

#ifndef NET_FS_NODE_H
#define NET_FS_NODE_H

#include "AttributeDirectory.h"
#include "DLList.h"
#include "NodeRef.h"

// required by mwcc -- otherwise it can't instantiate the list
#include "Entry.h"

class AttrDirIterator;
class Entry;
class FileHandle;
class NodeHandle;
class Path;
class Volume;

// Node
class Node : public AttributeDirectory {
public:
								Node(Volume* volume, const struct stat& st);
	virtual						~Node();

			Volume*				GetVolume() const;
			node_ref			GetNodeRef() const;
			dev_t				GetVolumeID() const;
			ino_t				GetID() const;

			void				AddReferringEntry(Entry* entry);
			void				RemoveReferringEntry(Entry* entry);
			Entry*				FindReferringEntry(dev_t volumeID,
									ino_t directoryID, const char* name);
			Entry*				GetFirstReferringEntry() const;
			Entry*				GetNextReferringEntry(Entry* entry) const;
			Entry*				FindReferringEntry(const entry_ref& entryRef);
	virtual	Entry*				GetActualReferringEntry() const;

			const struct stat&	GetStat() const;
			status_t			UpdateStat();

			bool				IsDirectory() const;
			bool				IsFile() const;
			bool				IsSymlink() const;

			status_t			GetPath(Path* path);

			status_t			Open(int openMode, FileHandle** fileHandle);
			status_t			OpenAttrDir(AttrDirIterator** iterator);
	virtual	status_t			OpenNode(BNode& node);

			status_t			ReadSymlink(char* buffer, int32 bufferSize,
									int32* bytesRead = NULL);

protected:
			status_t			_CheckNodeHandle(NodeHandle* nodeHandle);

protected:
			typedef DLList<Entry> EntryList;

			Volume*				fVolume;
			struct stat			fStat;
			EntryList			fReferringEntries;
};



#endif	// NET_FS_NODE_H
