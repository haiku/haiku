// Directory.h

#ifndef NET_FS_DIRECTORY_H
#define NET_FS_DIRECTORY_H

#include "DLList.h"
#include "Node.h"
#include "NodeHandle.h"

struct DIR;

class Directory;

// Directory
class Directory : public Node {
public:
								Directory(Volume* volume,
									const struct stat& st);
	virtual						~Directory();

	virtual	Entry*				GetActualReferringEntry() const;

			void				AddEntry(Entry* entry);
			void				RemoveEntry(Entry* entry);
			Entry*				GetFirstEntry() const;
			Entry*				GetNextEntry(Entry* entry) const;
			int32				CountEntries() const;
									// WARNING: O(n)! Use for debugging only!

			status_t			OpenDir(DirIterator** iterator);

			bool				HasDirIterators() const;
			void				RemoveDirIterator(DirIterator* iterator);

			void				SetComplete(bool complete);
			bool				IsComplete() const;

private:
			typedef DLList<Entry, Entry::GetDirEntryLink> EntryList;
			typedef DLList<DirIterator> IteratorList;

			EntryList			fEntries;
			IteratorList		fIterators;
			bool				fIsComplete;
};


#endif	// NET_FS_DIRECTORY_H
