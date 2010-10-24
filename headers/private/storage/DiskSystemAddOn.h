/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_SYSTEM_ADD_ON_H
#define _DISK_SYSTEM_ADD_ON_H

#include <DiskDeviceDefs.h>
#include <String.h>
#include <SupportDefs.h>


class BPartitionParameterEditor;
class BList;
class BMutablePartition;
class BPartitionHandle;
class BPartitioningInfo;


class BDiskSystemAddOn {
public:
								BDiskSystemAddOn(const char* name,
									uint32 flags);
	virtual						~BDiskSystemAddOn();

			const char*			Name() const;
			uint32				Flags() const;

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** handle) = 0;

	virtual	bool				CanInitialize(
									const BMutablePartition* partition);
	virtual	status_t			ValidateInitialize(
									const BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** handle);

	virtual	status_t			GetParameterEditor(
									B_PARAMETER_EDITOR_TYPE type,
									BPartitionParameterEditor** editor);
	virtual	status_t			GetTypeForContentType(const char* contentType,
									BString* type);
	virtual	bool				IsSubSystemFor(const BMutablePartition* child);

private:
			BString				fName;
			uint32				fFlags;
};


class BPartitionHandle {
public:
								BPartitionHandle(BMutablePartition* partition);
	virtual						~BPartitionHandle();

			BMutablePartition*	Partition() const;

	virtual	uint32				SupportedOperations(uint32 mask);
	virtual	uint32				SupportedChildOperations(
									const BMutablePartition* child,
									uint32 mask);

	virtual	bool				SupportsInitializingChild(
									const BMutablePartition* child,
									const char* diskSystem);

	virtual	status_t			GetNextSupportedType(
									const BMutablePartition* child,
									int32* cookie, BString* type);
									// child can be NULL

	virtual	status_t			GetPartitioningInfo(BPartitioningInfo* info);


	virtual	status_t			Defragment();
	virtual	status_t			Repair(bool checkOnly);

	virtual	status_t			ValidateResize(off_t* size);
	virtual	status_t			ValidateResizeChild(
									const BMutablePartition* child,
									off_t* size);
	virtual	status_t			Resize(off_t size);
	virtual	status_t			ResizeChild(BMutablePartition* child,
									off_t size);

	virtual	status_t			ValidateMove(off_t* offset);
	virtual	status_t			ValidateMoveChild(
									const BMutablePartition* child,
									off_t* offset);
	virtual	status_t			Move(off_t offset);
	virtual	status_t			MoveChild(BMutablePartition* child,
									off_t offset);

	virtual	status_t			ValidateSetContentName(BString* name);
	virtual	status_t			ValidateSetName(const BMutablePartition* child,
									BString* name);
	virtual	status_t			SetContentName(const char* name);
	virtual	status_t			SetName(BMutablePartition* child,
									const char* name);

	virtual	status_t			ValidateSetType(const BMutablePartition* child,
									const char* type);
	virtual	status_t			SetType(BMutablePartition* child,
									const char* type);

	virtual	status_t			GetContentParameterEditor(
									BPartitionParameterEditor** editor);

	virtual	status_t			ValidateSetContentParameters(
									const char* parameters);
	virtual	status_t			ValidateSetParameters(
									const BMutablePartition* child,
									const char* parameters);
	virtual	status_t			SetContentParameters(const char* parameters);
	virtual	status_t			SetParameters(BMutablePartition* child,
									const char* parameters);

	virtual	status_t			GetParameterEditor(
									B_PARAMETER_EDITOR_TYPE type,
									BPartitionParameterEditor** editor);
	virtual	status_t			ValidateCreateChild(off_t* offset,
									off_t* size, const char* type,
									BString* name, const char* parameters);
	virtual	status_t			CreateChild(off_t offset, off_t size,
									const char* type, const char* name,
									const char* parameters,
									BMutablePartition** child);

	virtual	status_t			DeleteChild(BMutablePartition* child);

private:
			BMutablePartition*	fPartition;
};


extern "C" status_t get_disk_system_add_ons(BList* addOns);
	// Implemented in the add-on

#endif	// _DISK_SYSTEM_ADD_ON_H
