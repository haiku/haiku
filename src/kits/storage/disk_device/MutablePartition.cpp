/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <MutablePartition.h>

#include <stdlib.h>
#include <string.h>

#include <new>

#include <Partition.h>

#include <ddm_userland_interface_defs.h>

#include "DiskDeviceUtils.h"
#include "PartitionDelegate.h"


using std::nothrow;


// UninitializeContents
void
BMutablePartition::UninitializeContents()
{
	DeleteAllChildren();
	SetVolumeID(-1);
	SetContentName(NULL);
	SetContentParameters(NULL);
	SetContentSize(0);
	SetBlockSize(Parent()->BlockSize());
	SetContentType(NULL);
	SetStatus(B_PARTITION_UNINITIALIZED);
	ClearFlags(B_PARTITION_FILE_SYSTEM | B_PARTITION_PARTITIONING_SYSTEM);
//	if (!Device()->IsReadOnlyMedia())
//		ClearFlags(B_PARTITION_READ_ONLY);
}


// Offset
off_t
BMutablePartition::Offset() const
{
	return fData->offset;
}


// SetOffset
void
BMutablePartition::SetOffset(off_t offset)
{
	if (fData->offset != offset) {
		fData->offset = offset;
		Changed(B_PARTITION_CHANGED_OFFSET);
	}
}


// Size
off_t
BMutablePartition::Size() const
{
	return fData->size;
}


// SetSize
void
BMutablePartition::SetSize(off_t size)
{
	if (fData->size != size) {
		fData->size = size;
		Changed(B_PARTITION_CHANGED_SIZE);
	}
}


// ContentSize
off_t
BMutablePartition::ContentSize() const
{
	return fData->content_size;
}


// SetContentSize
void
BMutablePartition::SetContentSize(off_t size)
{
	if (fData->content_size != size) {
		fData->content_size = size;
		Changed(B_PARTITION_CHANGED_CONTENT_SIZE);
	}
}


// BlockSize
off_t
BMutablePartition::BlockSize() const
{
	return fData->block_size;
}


// SetBlockSize
void
BMutablePartition::SetBlockSize(off_t blockSize)
{
	if (fData->block_size != blockSize) {
		fData->block_size = blockSize;
		Changed(B_PARTITION_CHANGED_BLOCK_SIZE);
	}
}


// Status
uint32
BMutablePartition::Status() const
{
	return fData->status;
}


// SetStatus
void
BMutablePartition::SetStatus(uint32 status)
{
	if (fData->status != status) {
		fData->status = status;
		Changed(B_PARTITION_CHANGED_STATUS);
	}
}


// Flags
uint32
BMutablePartition::Flags() const
{
	return fData->flags;
}


// SetFlags
void
BMutablePartition::SetFlags(uint32 flags)
{
	if (fData->flags != flags) {
		fData->flags = flags;
		Changed(B_PARTITION_CHANGED_FLAGS);
	}
}


// ClearFlags
void
BMutablePartition::ClearFlags(uint32 flags)
{
	if (flags & fData->flags) {
		fData->flags &= ~flags;
		Changed(B_PARTITION_CHANGED_FLAGS);
	}
}


// VolumeID
dev_t
BMutablePartition::VolumeID() const
{
	return fData->volume;
}


// SetVolumeID
void
BMutablePartition::SetVolumeID(dev_t volumeID)
{
	if (fData->volume != volumeID) {
		fData->volume = volumeID;
		Changed(B_PARTITION_CHANGED_VOLUME);
	}
}


// Index
int32
BMutablePartition::Index() const
{
	return fData->index;
}


// Name
const char*
BMutablePartition::Name() const
{
	return fData->name;
}


// SetName
status_t
BMutablePartition::SetName(const char* name)
{
	if (compare_string(name, fData->name) == 0)
		return B_OK;

	if (set_string(fData->name, name) != B_OK)
		return B_NO_MEMORY;

	Changed(B_PARTITION_CHANGED_NAME);
	return B_OK;
}


// ContentName
const char*
BMutablePartition::ContentName() const
{
	return fData->content_name;
}


// SetContentName
status_t
BMutablePartition::SetContentName(const char* name)
{
	if (compare_string(name, fData->content_name) == 0)
		return B_OK;

	if (set_string(fData->content_name, name) != B_OK)
		return B_NO_MEMORY;

	Changed(B_PARTITION_CHANGED_CONTENT_NAME);
	return B_OK;
}


// Type
const char*
BMutablePartition::Type() const
{
	return fData->type;
}


// SetType
status_t
BMutablePartition::SetType(const char* type)
{
	if (compare_string(type, fData->type) == 0)
		return B_OK;

	if (set_string(fData->type, type) != B_OK)
		return B_NO_MEMORY;

	Changed(B_PARTITION_CHANGED_TYPE);
	return B_OK;
}


// ContentType
const char*
BMutablePartition::ContentType() const
{
	return fData->content_type;
}


// SetContentType
status_t
BMutablePartition::SetContentType(const char* type)
{
	if (compare_string(type, fData->content_type) == 0)
		return B_OK;

	if (set_string(fData->content_type, type) != B_OK)
		return B_NO_MEMORY;

	Changed(B_PARTITION_CHANGED_CONTENT_TYPE
		| B_PARTITION_CHANGED_INITIALIZATION);
	return B_OK;
}


// Parameters
const char*
BMutablePartition::Parameters() const
{
	return fData->parameters;
}


// SetParameters
status_t
BMutablePartition::SetParameters(const char* parameters)
{
	if (compare_string(parameters, fData->parameters) == 0)
		return B_OK;

	if (set_string(fData->parameters, parameters) != B_OK)
		return B_NO_MEMORY;

	Changed(B_PARTITION_CHANGED_PARAMETERS);
	return B_OK;
}


// ContentParameters
const char*
BMutablePartition::ContentParameters() const
{
	return fData->content_parameters;
}


// SetContentParameters
status_t
BMutablePartition::SetContentParameters(const char* parameters)
{
	if (compare_string(parameters, fData->content_parameters) == 0)
		return B_OK;

	if (set_string(fData->content_parameters, parameters) != B_OK)
		return B_NO_MEMORY;

	Changed(B_PARTITION_CHANGED_CONTENT_PARAMETERS);
	return B_OK;
}


// CreateChild
status_t
BMutablePartition::CreateChild(int32 index, BMutablePartition** _child)
{
	if (index < 0)
		index = fChildren.CountItems();
	else if (index > fChildren.CountItems())
		return B_BAD_VALUE;

	// create the BPartition
	BPartition* partition = new(nothrow) BPartition;
	if (!partition)
		return B_NO_MEMORY;

	// create the delegate
	BPartition::Delegate* delegate
		= new(nothrow) BPartition::Delegate(partition);
	if (!delegate) {
		delete partition;
		return B_NO_MEMORY;
	}
	partition->fDelegate = delegate;

	// add the child
	BMutablePartition* child = delegate->MutablePartition();
	if (!fChildren.AddItem(child, index)) {
		delete partition;
		return B_NO_MEMORY;
	}
	child->fParent = this;
	child->fData = new(nothrow) user_partition_data;
	if (!child->fData) {
		fChildren.RemoveItem(child);
		delete partition;
		return B_NO_MEMORY;
	}

	memset(child->fData, 0, sizeof(user_partition_data));

	child->fData->id = -1;
	child->fData->status = B_PARTITION_UNINITIALIZED;
	child->fData->volume = -1;
	child->fData->index = -1;
	child->fData->disk_system = -1;

	*_child = child;

	Changed(B_PARTITION_CHANGED_CHILDREN);
	return B_OK;
}


// CreateChild
status_t
BMutablePartition::CreateChild(int32 index, const char* type, const char* name,
	const char* parameters, BMutablePartition** _child)
{
	// create the child
	BMutablePartition* child;
	status_t error = CreateChild(index, &child);
	if (error != B_OK)
		return error;

	// set the name, type, and parameters
	error = child->SetType(type);
	if (error == B_OK)
		error = child->SetName(name);
	if (error == B_OK)
		error = child->SetParameters(parameters);

	// cleanup on error
	if (error != B_OK) {
		DeleteChild(child);
		return error;
	}

	*_child = child;

	Changed(B_PARTITION_CHANGED_CHILDREN);
	return B_OK;
}


// DeleteChild
status_t
BMutablePartition::DeleteChild(int32 index)
{
	BMutablePartition* child = (BMutablePartition*)fChildren.RemoveItem(index);
	if (!child)
		return B_BAD_VALUE;

	// This will delete not only all delegates in the child's hierarchy, but
	// also the respective partitions themselves, if they are no longer
	// referenced.
	child->fDelegate->Partition()->_DeleteDelegates();

	Changed(B_PARTITION_CHANGED_CHILDREN);
	return B_OK;
}


// DeleteChild
status_t
BMutablePartition::DeleteChild(BMutablePartition* child)
{
	return DeleteChild(IndexOfChild(child));
}


// DeleteAllChildren
void
BMutablePartition::DeleteAllChildren()
{
	int32 count = CountChildren();
	for (int32 i = count - 1; i >= 0; i--)
		DeleteChild(i);
}


// Parent
BMutablePartition*
BMutablePartition::Parent() const
{
	return fParent;
}


// ChildAt
BMutablePartition*
BMutablePartition::ChildAt(int32 index) const
{
	return (BMutablePartition*)fChildren.ItemAt(index);
}


// CountChildren
int32
BMutablePartition::CountChildren() const
{
	return fChildren.CountItems();
}


// IndexOfChild
int32
BMutablePartition::IndexOfChild(BMutablePartition* child) const
{
	if (!child)
		return -1;
	return fChildren.IndexOf(child);
}


// SetChangeFlags
void
BMutablePartition::SetChangeFlags(uint32 flags)
{
	fChangeFlags = flags;
}


// ChangeFlags
uint32
BMutablePartition::ChangeFlags() const
{
	return fChangeFlags;
}


// Changed
void
BMutablePartition::Changed(uint32 flags, uint32 clearFlags)
{
	fChangeFlags &= ~clearFlags;
	fChangeFlags |= flags;

	if (Parent())
		Parent()->Changed(B_PARTITION_CHANGED_DESCENDANTS);
}


// ChildCookie
void*
BMutablePartition::ChildCookie() const
{
	return fChildCookie;
}


// SetChildCookie
void
BMutablePartition::SetChildCookie(void* cookie)
{
	fChildCookie = cookie;
}


// constructor
BMutablePartition::BMutablePartition(BPartition::Delegate* delegate)
	: fDelegate(delegate),
	  fData(NULL),
	  fParent(NULL),
	  fChangeFlags(0),
	  fChildCookie(NULL)
{
}


// Init
status_t
BMutablePartition::Init(const user_partition_data* partitionData,
	BMutablePartition* parent)
{
	fParent = parent;

	// add to the parent's child list
	if (fParent) {
		if (!fParent->fChildren.AddItem(this))
			return B_NO_MEMORY;
	}

	// allocate data structure
	fData = new(nothrow) user_partition_data;
	if (!fData)
		return B_NO_MEMORY;

	memset(fData, 0, sizeof(user_partition_data));

	// copy the flat data
	fData->id = partitionData->id;
	fData->offset = partitionData->offset;
	fData->size = partitionData->size;
	fData->content_size = partitionData->content_size;
	fData->block_size = partitionData->block_size;
	fData->status = partitionData->status;
	fData->flags = partitionData->flags;
	fData->volume = partitionData->volume;
	fData->index = partitionData->index;
	fData->change_counter = partitionData->change_counter;
	fData->disk_system = partitionData->disk_system;

	// copy the strings
	SET_STRING_RETURN_ON_ERROR(fData->name, partitionData->name);
	SET_STRING_RETURN_ON_ERROR(fData->content_name,
		partitionData->content_name);
	SET_STRING_RETURN_ON_ERROR(fData->type, partitionData->type);
	SET_STRING_RETURN_ON_ERROR(fData->content_type,
		partitionData->content_type);
	SET_STRING_RETURN_ON_ERROR(fData->parameters, partitionData->parameters);
	SET_STRING_RETURN_ON_ERROR(fData->content_parameters,
		partitionData->content_parameters);

	return B_OK;
}


// destructor
BMutablePartition::~BMutablePartition()
{
	if (fData) {
		free(fData->name);
		free(fData->content_name);
		free(fData->type);
		free(fData->content_type);
		free(fData->parameters);
		free(fData->content_parameters);
		delete fData;
	}
}


// PartitionData
const user_partition_data*
BMutablePartition::PartitionData() const
{
	return fData;
}


// GetDelegate
BPartition::Delegate*
BMutablePartition::GetDelegate() const
{
	return fDelegate;
}

