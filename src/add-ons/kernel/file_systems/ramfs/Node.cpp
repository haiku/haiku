/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "DebugSupport.h"
#include "EntryIterator.h"
#include "LastModifiedIndex.h"
#include "Node.h"
#include "Volume.h"

// is_user_in_group
inline static
bool
is_user_in_group(gid_t gid)
{
// Either I miss something, or we don't have getgroups() in the kernel. :-(
/*
	gid_t groups[NGROUPS_MAX];
	int groupCount = getgroups(NGROUPS_MAX, groups);
	for (int i = 0; i < groupCount; i++) {
		if (gid == groups[i])
			return true;
	}
*/
	return (gid == getegid());
}


// constructor
Node::Node(Volume *volume, uint8 type)
	: fVolume(volume),
	  fID(fVolume->NextNodeID()),
	  fRefCount(0),
	  fMode(0),
	  fUID(0),
	  fGID(0),
	  fATime(0),
	  fMTime(0),
	  fCTime(0),
	  fCrTime(0),
	  fModified(0),
	  fIsKnownToVFS(false),
	  // attribute management
	  fAttributes(),
	  // referrers
	  fReferrers()
{
	// set file type
	switch (type) {
		case NODE_TYPE_DIRECTORY:
			fMode = S_IFDIR;
			break;
		case NODE_TYPE_FILE:
			fMode = S_IFREG;
			break;
		case NODE_TYPE_SYMLINK:
			fMode = S_IFLNK;
			break;
	}
	// set defaults for time
	fATime = fMTime = fCTime = fCrTime = time(NULL);
}

// destructor
Node::~Node()
{
	// delete all attributes
	while (Attribute *attribute = fAttributes.First()) {
		status_t error = DeleteAttribute(attribute);
		if (error != B_OK) {
			FATAL("Node::~Node(): Failed to delete attribute!\n");
			break;
		}
	}
}

// InitCheck
status_t
Node::InitCheck() const
{
	return (fVolume && fID >= 0 ? B_OK : B_NO_INIT);
}

// AddReference
status_t
Node::AddReference()
{
	if (++fRefCount == 1) {
		status_t error = GetVolume()->PublishVNode(this);
		if (error != B_OK) {
			fRefCount--;
			return error;
		}

		fIsKnownToVFS = true;
	}

	return B_OK;
}

// RemoveReference
void
Node::RemoveReference()
{
	if (--fRefCount == 0) {
		GetVolume()->RemoveVNode(this);
		fRefCount++;
	}
}

// Link
status_t
Node::Link(Entry *entry)
{
PRINT("Node[%Ld]::Link(): %" B_PRId32 " ->...\n", fID, fRefCount);
	fReferrers.Insert(entry);

	status_t error = AddReference();
	if (error != B_OK)
		fReferrers.Remove(entry);

	return error;
}

// Unlink
status_t
Node::Unlink(Entry *entry)
{
PRINT("Node[%Ld]::Unlink(): %" B_PRId32 " ->...\n", fID, fRefCount);
	RemoveReference();
	fReferrers.Remove(entry);

	return B_OK;
}

// SetMTime
void
Node::SetMTime(time_t mTime)
{
	time_t oldMTime = fMTime;
	fATime = fMTime = mTime;
	if (oldMTime != fMTime) {
		if (LastModifiedIndex *index = fVolume->GetLastModifiedIndex())
			index->Changed(this, oldMTime);
	}
}

// CheckPermissions
status_t
Node::CheckPermissions(int mode) const
{
	int userPermissions = (fMode & S_IRWXU) >> 6;
	int groupPermissions = (fMode & S_IRWXG) >> 3;
	int otherPermissions = fMode & S_IRWXO;
	// get the permissions for this uid/gid
	int permissions = 0;
	uid_t uid = geteuid();
	// user is root
	if (uid == 0) {
		// root has always read/write permission, but at least one of the
		// X bits must be set for execute permission
		permissions = userPermissions | groupPermissions | otherPermissions
			| ACCESS_R | ACCESS_W;
	// user is node owner
	} else if (uid == fUID)
		permissions = userPermissions;
	// user is in owning group
	else if (is_user_in_group(fGID))
		permissions = groupPermissions;
	// user is one of the others
	else
		permissions = otherPermissions;
	// do the check
	return ((mode & ~permissions) ? B_NOT_ALLOWED : B_OK);
}

// CreateAttribute
status_t
Node::CreateAttribute(const char *name, Attribute **_attribute)
{
	status_t error = (name && _attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// create attribute
		Attribute *attribute = new(nothrow) Attribute(fVolume, NULL, name);
		if (attribute) {
			error = attribute->InitCheck();
			if (error == B_OK) {
				// add attribute to node
				error = AddAttribute(attribute);
				if (error == B_OK)
					*_attribute = attribute;
			}
			if (error != B_OK)
				delete attribute;
		} else
			SET_ERROR(error, B_NO_MEMORY);
	}
	return error;
}

// DeleteAttribute
status_t
Node::DeleteAttribute(Attribute *attribute)
{
	status_t error = RemoveAttribute(attribute);
	if (error == B_OK)
		delete attribute;
	return error;
}

// AddAttribute
status_t
Node::AddAttribute(Attribute *attribute)
{
	status_t error = (attribute && !attribute->GetNode() ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		error = GetVolume()->NodeAttributeAdded(GetID(), attribute);
		if (error == B_OK) {
			fAttributes.Insert(attribute);
			attribute->SetNode(this);
			MarkModified(B_STAT_MODIFICATION_TIME);
		}
	}
	return error;
}

// RemoveAttribute
status_t
Node::RemoveAttribute(Attribute *attribute)
{
	status_t error = (attribute && attribute->GetNode() == this
					  ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		// move all iterators pointing to the attribute to the next attribute
		if (GetVolume()->IteratorLock()) {
			// set the iterators' current entry
			Attribute *nextAttr = fAttributes.GetNext(attribute);
			DoublyLinkedList<AttributeIterator> *iterators
				= attribute->GetAttributeIteratorList();
			for (AttributeIterator *iterator = iterators->First();
				 iterator;
				 iterator = iterators->GetNext(iterator)) {
				iterator->SetCurrent(nextAttr, true);
			}
			// Move the iterators from one list to the other, or just remove
			// them, if there is no next attribute.
			if (nextAttr) {
				DoublyLinkedList<AttributeIterator> *nextIterators
					= nextAttr->GetAttributeIteratorList();
				nextIterators->MoveFrom(iterators);
			} else
				iterators->RemoveAll();
			GetVolume()->IteratorUnlock();
		} else
			error = B_ERROR;
		// remove the attribute
		if (error == B_OK) {
			error = GetVolume()->NodeAttributeRemoved(GetID(), attribute);
			if (error == B_OK) {
				fAttributes.Remove(attribute);
				attribute->SetNode(NULL);
				MarkModified(B_STAT_MODIFICATION_TIME);
			}
		}
	}
	return error;
}

// FindAttribute
status_t
Node::FindAttribute(const char *name, Attribute **_attribute) const
{
	status_t error = (name && _attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		Attribute *attribute = NULL;
		while (GetNextAttribute(&attribute) == B_OK) {
			if (!strcmp(attribute->GetName(), name)) {
				*_attribute = attribute;
				return B_OK;
			}
		}
		error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetPreviousAttribute
status_t
Node::GetPreviousAttribute(Attribute **attribute) const
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!*attribute)
			*attribute = fAttributes.Last();
		else if ((*attribute)->GetNode() == this)
			*attribute = fAttributes.GetPrevious(*attribute);
		else
			error = B_BAD_VALUE;
		if (error == B_OK && !*attribute)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetNextAttribute
status_t
Node::GetNextAttribute(Attribute **attribute) const
{
	status_t error = (attribute ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (!*attribute)
			*attribute = fAttributes.First();
		else if ((*attribute)->GetNode() == this)
			*attribute = fAttributes.GetNext(*attribute);
		else
			error = B_BAD_VALUE;
		if (error == B_OK && !*attribute)
			error = B_ENTRY_NOT_FOUND;
	}
	return error;
}

// GetFirstReferrer
Entry *
Node::GetFirstReferrer() const
{
	return fReferrers.First();
}

// GetLastReferrer
Entry *
Node::GetLastReferrer() const
{
	return fReferrers.Last();
}

// GetPreviousReferrer
Entry *
Node::GetPreviousReferrer(Entry *entry) const
{
	return (entry ? fReferrers.GetPrevious(entry) : NULL );
}

// GetNextReferrer
Entry *
Node::GetNextReferrer(Entry *entry) const
{
	return (entry ? fReferrers.GetNext(entry) : NULL );
}

// GetAllocationInfo
void
Node::GetAllocationInfo(AllocationInfo &info)
{
	Attribute *attribute = NULL;
	while (GetNextAttribute(&attribute) == B_OK)
		attribute->GetAllocationInfo(info);
}

