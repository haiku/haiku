// Node.h

#ifndef NODE_H
#define NODE_H

#include <fs_interface.h>
#include <NodeMonitor.h>

#include "Attribute.h"
#include "Entry.h"
#include "String.h"

class AllocationInfo;
class AttributeIterator;
class Directory;
class Volume;

// node type
enum {
	NODE_TYPE_DIRECTORY,
	NODE_TYPE_FILE,
	NODE_TYPE_SYMLINK,
};

// access modes
enum {
	ACCESS_R	= S_IROTH,
	ACCESS_W	= S_IWOTH,
	ACCESS_X	= S_IXOTH,
};

class Node : public DoublyLinkedListLinkImpl<Node> {
public:
	Node(Volume *volume, uint8 type);
	virtual ~Node();

	virtual status_t InitCheck() const;

	inline void SetVolume(Volume *volume)	{ fVolume = volume; }
	inline Volume *GetVolume() const		{ return fVolume; }

	inline ino_t GetID() const	{ return fID; }

	status_t AddReference();
	void RemoveReference();
	int32 GetRefCount()	{ return fRefCount; }

	virtual status_t Link(Entry *entry);
	virtual status_t Unlink(Entry *entry);

	inline bool IsDirectory() const	{ return S_ISDIR(fMode); }
	inline bool IsFile() const		{ return S_ISREG(fMode); }
	inline bool IsSymLink() const	{ return S_ISLNK(fMode); }

	virtual status_t SetSize(off_t newSize) = 0;
	virtual off_t GetSize() const = 0;

	// stat data

	inline void SetMode(mode_t mode)
		{ fMode = (fMode & ~S_IUMSK) | (mode & S_IUMSK); }
	inline mode_t GetMode() const	{ return fMode; }

	inline void SetUID(uid_t uid)	{ fUID = uid; MarkModified(B_STAT_UID); }
	inline uid_t GetUID() const		{ return fUID; }

	inline void SetGID(uid_t gid)	{ fGID = gid; MarkModified(B_STAT_GID); }
	inline uid_t GetGID() const		{ return fGID; }

	inline void SetATime(time_t aTime)	{ fATime = aTime; }
	inline time_t GetATime() const		{ return fATime; }

	void SetMTime(time_t mTime);
	inline time_t GetMTime() const		{ return fMTime; }

	inline void SetCTime(time_t cTime)	{ fCTime = cTime; }
	inline time_t GetCTime() const		{ return fCTime; }

	inline void SetCrTime(time_t crTime)	{ fCrTime = crTime; }
	inline time_t GetCrTime() const			{ return fCrTime; }

	inline void MarkModified(uint32 flags)	{ fModified |= flags; }
	inline uint32 MarkUnmodified();
	inline bool IsModified() const			{ return fModified; }

	status_t CheckPermissions(int mode) const;

	bool IsKnownToVFS() const	{ return fIsKnownToVFS; }

	// attributes
	status_t CreateAttribute(const char *name, Attribute **attribute);
	status_t DeleteAttribute(Attribute *attribute);
	status_t AddAttribute(Attribute *attribute);
	status_t RemoveAttribute(Attribute *attribute);

	status_t FindAttribute(const char *name, Attribute **attribute) const;

	status_t GetPreviousAttribute(Attribute **attribute) const;
	status_t GetNextAttribute(Attribute **attribute) const;

	Entry *GetFirstReferrer() const;
	Entry *GetLastReferrer() const;
	Entry *GetPreviousReferrer(Entry *entry) const;
	Entry *GetNextReferrer(Entry *entry) const;

	// debugging
	virtual void GetAllocationInfo(AllocationInfo &info);

private:
	Volume					*fVolume;
	ino_t					fID;
	int32					fRefCount;
	mode_t					fMode;
	uid_t					fUID;
	uid_t					fGID;
	time_t					fATime;
	time_t					fMTime;
	time_t					fCTime;
	time_t					fCrTime;
	uint32					fModified;
	bool					fIsKnownToVFS;

	// attribute management
	DoublyLinkedList<Attribute>		fAttributes;

protected:
	// entries referring to this node
	DoublyLinkedList<Entry, GetNodeReferrerLink>	fReferrers;
};

// MarkUnmodified
inline
uint32
Node::MarkUnmodified()
{
	uint32 modified = fModified;
	if (modified) {
		fCTime = time(NULL);
		SetMTime(fCTime);
		fModified = 0;
	}
	return modified;
}

// open_mode_to_access
inline static
int
open_mode_to_access(int openMode)
{
	switch (openMode & O_RWMASK) {
		case O_RDONLY:
			return ACCESS_R;
		case O_WRONLY:
			return ACCESS_W;
		case O_RDWR:
			return ACCESS_R | ACCESS_W;
	}
	return 0;
}


// NodeMTimeUpdater
class NodeMTimeUpdater {
public:
	NodeMTimeUpdater(Node *node) : fNode(node) {}
	~NodeMTimeUpdater()
	{
		if (fNode && fNode->IsModified())
			fNode->MarkUnmodified();
	}

private:
	Node	*fNode;
};

#endif	// NODE_H
