/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "PartitionDelegate.h"

#include <stdio.h>

#include <DiskSystemAddOn.h>
#include <DiskSystemAddOnManager.h>

//#define TRACE_PARTITION_DELEGATE
#undef TRACE
#ifdef TRACE_PARTITION_DELEGATE
# define TRACE(x...) printf(x)
#else
# define TRACE(x...) do {} while (false)
#endif


// constructor
BPartition::Delegate::Delegate(BPartition* partition)
	:
	fPartition(partition),
	fMutablePartition(this),
	fDiskSystem(NULL),
	fPartitionHandle(NULL)
{
}


// destructor
BPartition::Delegate::~Delegate()
{
}


// MutablePartition
BMutablePartition*
BPartition::Delegate::MutablePartition()
{
	return &fMutablePartition;
}


// MutablePartition
const BMutablePartition*
BPartition::Delegate::MutablePartition() const
{
	return &fMutablePartition;
}


// InitHierarchy
status_t
BPartition::Delegate::InitHierarchy(
	const user_partition_data* partitionData, Delegate* parent)
{
	return fMutablePartition.Init(partitionData,
		parent ? &parent->fMutablePartition : NULL);
}


// InitAfterHierarchy
status_t
BPartition::Delegate::InitAfterHierarchy()
{
	TRACE("%p->BPartition::Delegate::InitAfterHierarchy()\n", this);

	if (!fMutablePartition.ContentType()) {
		TRACE("  no content type\n");
		return B_OK;
	}

	// init disk system and handle
	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(
		fMutablePartition.ContentType());
	if (!addOn) {
		TRACE("  add-on for disk system \"%s\" not found\n",
			fMutablePartition.ContentType());
		return B_OK;
	}

	BPartitionHandle* handle;
	status_t error = addOn->CreatePartitionHandle(&fMutablePartition, &handle);
	if (error != B_OK) {
		TRACE("  failed to create partition handle for partition %ld, disk "
			"system: \"%s\": %s\n",
			Partition()->ID(), addOn->Name(), strerror(error));
		manager->PutAddOn(addOn);
		return error;
	}

	// everything went fine -- keep the disk system add-on reference and the
	// handle
	fDiskSystem = addOn;
	fPartitionHandle = handle;

	return B_OK;
}


// PartitionData
const user_partition_data*
BPartition::Delegate::PartitionData() const
{
	return fMutablePartition.PartitionData();
}


// ChildAt
BPartition::Delegate*
BPartition::Delegate::ChildAt(int32 index) const
{
	BMutablePartition* child = fMutablePartition.ChildAt(index);
	return child ? child->GetDelegate() : NULL;
}


// CountChildren
int32
BPartition::Delegate::CountChildren() const
{
	return fMutablePartition.CountChildren();
}


// IsModified
bool
BPartition::Delegate::IsModified() const
{
	return fMutablePartition.ChangeFlags() != 0;
}


// SupportedOperations
uint32
BPartition::Delegate::SupportedOperations(uint32 mask)
{
	if (!fPartitionHandle)
		return 0;

	return fPartitionHandle->SupportedOperations(mask);
}


// SupportedChildOperations
uint32
BPartition::Delegate::SupportedChildOperations(Delegate* child,
	uint32 mask)
{
	if (!fPartitionHandle)
		return 0;

	return fPartitionHandle->SupportedChildOperations(child->MutablePartition(),
		mask);
}


// Defragment
status_t
BPartition::Delegate::Defragment()
{
// TODO: Implement!
	return B_BAD_VALUE;
}


// Repair
status_t
BPartition::Delegate::Repair(bool checkOnly)
{
	if (fPartitionHandle == NULL)
		return B_NO_INIT;

	return fPartitionHandle->Repair(checkOnly);
}


// ValidateResize
status_t
BPartition::Delegate::ValidateResize(off_t* size) const
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->ValidateResize(size);
}


// ValidateResizeChild
status_t
BPartition::Delegate::ValidateResizeChild(Delegate* child, off_t* size) const
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->ValidateResizeChild(&child->fMutablePartition,
		size);
}


// Resize
status_t
BPartition::Delegate::Resize(off_t size)
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->Resize(size);
}


// ResizeChild
status_t
BPartition::Delegate::ResizeChild(Delegate* child, off_t size)
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->ResizeChild(&child->fMutablePartition, size);
}


// ValidateMove
status_t
BPartition::Delegate::ValidateMove(off_t* offset) const
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->ValidateMove(offset);
}


// ValidateMoveChild
status_t
BPartition::Delegate::ValidateMoveChild(Delegate* child, off_t* offset) const
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->ValidateMoveChild(&child->fMutablePartition,
		offset);
}


// Move
status_t
BPartition::Delegate::Move(off_t offset)
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->Move(offset);
}


// MoveChild
status_t
BPartition::Delegate::MoveChild(Delegate* child, off_t offset)
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->MoveChild(&child->fMutablePartition, offset);
}


// ValidateSetContentName
status_t
BPartition::Delegate::ValidateSetContentName(BString* name) const
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->ValidateSetContentName(name);
}


// ValidateSetName
status_t
BPartition::Delegate::ValidateSetName(Delegate* child, BString* name) const
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->ValidateSetName(&child->fMutablePartition, name);
}


// SetContentName
status_t
BPartition::Delegate::SetContentName(const char* name)
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->SetContentName(name);
}


// SetName
status_t
BPartition::Delegate::SetName(Delegate* child, const char* name)
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->SetName(&child->fMutablePartition, name);
}


// ValidateSetType
status_t
BPartition::Delegate::ValidateSetType(Delegate* child, const char* type) const
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->ValidateSetType(&child->fMutablePartition, type);
}


// SetType
status_t
BPartition::Delegate::SetType(Delegate* child, const char* type)
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->SetType(&child->fMutablePartition, type);
}


// SetContentParameters
status_t
BPartition::Delegate::SetContentParameters(const char* parameters)
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->SetContentParameters(parameters);
}


// SetParameters
status_t
BPartition::Delegate::SetParameters(Delegate* child, const char* parameters)
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->SetParameters(&child->fMutablePartition,
		parameters);
}


// GetNextSupportedChildType
status_t
BPartition::Delegate::GetNextSupportedChildType(Delegate* child,
	int32* cookie, BString* type) const
{
	TRACE("%p->BPartition::Delegate::GetNextSupportedChildType(child: %p, "
		"cookie: %ld)\n", this, child, *cookie);

	if (!fPartitionHandle) {
		TRACE("  no partition handle!\n");
		return B_NO_INIT;
	}

	return fPartitionHandle->GetNextSupportedType(
		child ? &child->fMutablePartition : NULL, cookie, type);
}


// IsSubSystem
bool
BPartition::Delegate::IsSubSystem(Delegate* child,
	const char* diskSystem) const
{
	// get the disk system add-on
	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(diskSystem);
	if (!addOn)
		return false;

	bool result = addOn->IsSubSystemFor(&child->fMutablePartition);

	// put the add-on
	manager->PutAddOn(addOn);

	return result;
}


// CanInitialize
bool
BPartition::Delegate::CanInitialize(const char* diskSystem) const
{
	// get the disk system add-on
	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(diskSystem);
	if (!addOn)
		return false;

	bool result = addOn->CanInitialize(&fMutablePartition);

	// put the add-on
	manager->PutAddOn(addOn);

	return result;
}


// ValidateInitialize
status_t
BPartition::Delegate::ValidateInitialize(const char* diskSystem,
	BString* name, const char* parameters)
{
	// get the disk system add-on
	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(diskSystem);
	if (!addOn)
		return B_ENTRY_NOT_FOUND;

	status_t result = addOn->ValidateInitialize(&fMutablePartition,
		name, parameters);

	// put the add-on
	manager->PutAddOn(addOn);

	return result;
}


// Initialize
status_t
BPartition::Delegate::Initialize(const char* diskSystem,
	const char* name, const char* parameters)
{
	// get the disk system add-on
	DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
	BDiskSystemAddOn* addOn = manager->GetAddOn(diskSystem);
	if (!addOn)
		return B_ENTRY_NOT_FOUND;

	BPartitionHandle* handle;
	status_t result = addOn->Initialize(&fMutablePartition, name, parameters,
		&handle);

	// keep the add-on or put it on error
	if (result == B_OK) {
		// TODO: This won't suffice. If this partition had children, we have
		// to delete them before the new disk system plays with it.
		_FreeHandle();
		fDiskSystem = addOn;
		fPartitionHandle = handle;
	} else {
		manager->PutAddOn(addOn);
	}

	return result;
}


// Uninitialize
status_t
BPartition::Delegate::Uninitialize()
{
	if (fPartitionHandle) {
		_FreeHandle();

		fMutablePartition.UninitializeContents();
	}

	return B_OK;
}


// GetPartitioningInfo
status_t
BPartition::Delegate::GetPartitioningInfo(BPartitioningInfo* info)
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->GetPartitioningInfo(info);
}


// GetParameterEditor
status_t
BPartition::Delegate::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor) const
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->GetParameterEditor(type, editor);
}


// ValidateCreateChild
status_t
BPartition::Delegate::ValidateCreateChild(off_t* start, off_t* size,
	const char* type, BString* name, const char* parameters) const
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	return fPartitionHandle->ValidateCreateChild(start, size, type, name,
		parameters);
}


// CreateChild
status_t
BPartition::Delegate::CreateChild(off_t start, off_t size, const char* type,
	const char* name, const char* parameters, BPartition** child)
{
	if (!fPartitionHandle)
		return B_NO_INIT;

	BMutablePartition* mutableChild;
	status_t error = fPartitionHandle->CreateChild(start, size, type, name,
		parameters, &mutableChild);
	if (error != B_OK)
		return error;

	if (child)
		*child = mutableChild->GetDelegate()->Partition();

	return B_OK;
}


// DeleteChild
status_t
BPartition::Delegate::DeleteChild(Delegate* child)
{
	if (!fPartitionHandle || !child)
		return B_NO_INIT;

	return fPartitionHandle->DeleteChild(&child->fMutablePartition);
}


// _FreeHandle
void
BPartition::Delegate::_FreeHandle()
{
	if (fPartitionHandle) {
		delete fPartitionHandle;
		fPartitionHandle = NULL;

		DiskSystemAddOnManager* manager = DiskSystemAddOnManager::Default();
		manager->PutAddOn(fDiskSystem);
		fDiskSystem = NULL;
	}
}
