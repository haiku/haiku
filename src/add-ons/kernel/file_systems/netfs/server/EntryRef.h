// EntryRef.h

#ifndef NET_FS_ENTRY_REF_H
#define NET_FS_ENTRY_REF_H

#include <Entry.h>

// EntryRef
class EntryRef : public entry_ref {
public:
								EntryRef();
								EntryRef(dev_t volumeID, ino_t nodeID,
									const char* name);
								EntryRef(const entry_ref& ref);

			status_t			InitCheck() const;

			uint32				GetHashCode() const;
};

// NoAllocEntryRef
//
// EntryRef that doesn't clone the supplied name. Use with care!
class NoAllocEntryRef : public EntryRef {
public:
								NoAllocEntryRef();
								NoAllocEntryRef(dev_t volumeID, ino_t nodeID,
									const char* name);
								NoAllocEntryRef(const entry_ref& ref);
								~NoAllocEntryRef();

			NoAllocEntryRef&	operator=(const entry_ref& ref);
};


#endif	// NET_FS_ENTRY_REF_H
