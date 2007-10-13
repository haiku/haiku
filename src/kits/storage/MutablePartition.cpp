/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <MutablePartition.h>

#include <stdlib.h>
#include <string.h>

#include <new>

#include <Partition.h>

#include <disk_device_manager/ddm_userland_interface.h>

#include "PartitionDelegate.h"


using std::nothrow;


// set_string
static status_t
set_string(char*& location, const char* newString)
{
	char* string = NULL;
	if (newString) {
		string = strdup(newString);
		if (!string)
			return B_NO_MEMORY;
	}

	free(location);
	location = string;

	return B_OK;
}


#define SET_STRING_RETURN_ON_ERROR(location, string)	\
{														\
	status_t error = set_string(location, string);		\
	if (error != B_OK)									\
		return error;									\
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
	fData->offset = offset;
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
	fData->size = size;
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
	fData->content_size = size;
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
	fData->block_size = blockSize;
}


// Status
uint32
BMutablePartition::Status() const
{
	return fData->status;
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
	fData->flags = flags;
}


// Volume
dev_t
BMutablePartition::Volume() const
{
	return fData->volume;
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
	return set_string(fData->name, name);
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
	return set_string(fData->content_name, name);
}


// Type
const char*
BMutablePartition::Type() const
{
	return fData->type;
}


// SetType
status_t
BMutablePartition::SetType(const char* type) const
{
	return set_string(fData->type, type);
}


// ContentType
const char*
BMutablePartition::ContentType() const
{
	return fData->content_type;
}


// SetContentType
status_t
BMutablePartition::SetContentType(const char* type) const
{
	return set_string(fData->content_type, type);
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
	return set_string(fData->parameters, parameters);
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
	return set_string(fData->content_parameters, parameters);
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

	// create the MutableDelegate
	BPartition::MutableDelegate* delegate
		= new(nothrow) BPartition::MutableDelegate(partition);
	if (!delegate) {
		delete partition;
		return B_NO_MEMORY;
	}
	partition->fDelegate = delegate;
// TODO: Any further initialization required?

	// add the child
	BMutablePartition* child = delegate->MutablePartition();
	if (!fChildren.AddItem(child, index)) {
		delete partition;
		return B_NO_MEMORY;
	}
	child->fParent = this;

	*_child = child;

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
	error = SetType(type);
	if (error == B_OK)
		error = SetName(name);
	if (error == B_OK)
		error = SetParameters(parameters);

	// cleanup on error
	if (error != B_OK) {
		DeleteChild(child);
		return error;
	}

	*_child = child;
	return B_OK;
}


// DeleteChild
status_t
BMutablePartition::DeleteChild(int32 index)
{
	BMutablePartition* child = (BMutablePartition*)fChildren.RemoveItem(index);
	if (!child)
		return B_BAD_VALUE;

	delete child->fDelegate->Partition();

	return B_OK;
}


// DeleteChild
status_t
BMutablePartition::DeleteChild(BMutablePartition* child)
{
	return DeleteChild(IndexOfChild(child));
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
BMutablePartition::BMutablePartition(BPartition::MutableDelegate* delegate)
	: fDelegate(delegate),
	  fData(NULL),
	  fParent(NULL),
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
BPartition::MutableDelegate*
BMutablePartition::GetDelegate() const
{
	return fDelegate;
}

