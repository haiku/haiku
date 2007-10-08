/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_SYSTEM_ADD_ON_H
#define _DISK_SYSTEM_ADD_ON_H

#include <String.h>
#include <SupportDefs.h>


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

	virtual	bool				CanInitialize(BMutablePartition* partition);
	virtual	bool				ValidateInitialize(BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** handle);

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
									BMutablePartition* child, uint32 mask);

	virtual	bool				SupportsInitializingChild(
									BMutablePartition* child,
									const char* diskSystem);
	virtual	bool				IsSubSystemFor(BMutablePartition* child);

	virtual	status_t			GetNextSupportedType(BMutablePartition* child,
									int32* cookie, BString* type);
									// child can be NULL
	virtual	status_t			GetTypeForContentType(const char* contentType,
									BString* type);

	virtual	status_t			GetPartitioningInfo(BPartitioningInfo* info);


	virtual	status_t			Repair(bool checkOnly);
		// TODO: How is this supposed to work?

	virtual	bool				ValidateResize(off_t* size);
	virtual	bool				ValidateResizeChild(BMutablePartition* child,
									off_t* size);
	virtual	status_t			Resize(off_t size);
	virtual	status_t			ResizeChild(BMutablePartition* child,
									off_t size);

	virtual	bool				ValidateMove(off_t* offset);
	virtual	bool				ValidateMoveChild(BMutablePartition* child,
									off_t* offset);
	virtual	status_t			Move(off_t offset);
	virtual	status_t			MoveChild(BMutablePartition* child,
									off_t offset);

	virtual	bool				ValidateSetContentName(BString* name);
	virtual	bool				ValidateSetName(BMutablePartition* child,
									BString* name);
	virtual	status_t			SetContentName(const char* name);
	virtual	status_t			SetName(BMutablePartition* child,
									const char* name);

	virtual	bool				ValidateSetType(BMutablePartition* child,
									const char* type);
	virtual	status_t			SetType(BMutablePartition* child,
									const char* type);

	virtual	bool				ValidateSetContentParameters(
									const char* parameters);
	virtual	bool				ValidateSetParameters(BMutablePartition* child,
									const char* parameters);
	virtual	status_t			SetContentParameters(const char* parameters);
	virtual	status_t			SetParameters(BMutablePartition* child,
									const char* parameters);

	virtual	bool				ValidateCreateChild(off_t* offset,
									off_t* size, const char* type,
									const char* parameters);
	virtual	status_t			CreateChild(BMutablePartition* child);

	virtual	status_t			DeleteChild(BMutablePartition* child);

private:
			BMutablePartition*	fPartition;
};


extern "C" status_t get_disk_system_add_ons(BList* addOns);
	// Implemented in the add-on

#endif	// _DISK_SYSTEM_ADD_ON_H
