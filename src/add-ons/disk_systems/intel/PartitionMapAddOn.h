/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PARTITION_MAP_ADD_ON_H
#define _PARTITION_MAP_ADD_ON_H

#include <DiskSystemAddOn.h>

#include "PartitionMap.h"


class PartitionMapAddOn : public BDiskSystemAddOn {
public:
								PartitionMapAddOn();
	virtual						~PartitionMapAddOn();

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** handle);

	virtual	bool				CanInitialize(
									const BMutablePartition* partition);
	virtual	status_t			GetInitializationParameterEditor(
									const BMutablePartition* partition,
									BPartitionParameterEditor** editor);
	virtual	status_t			ValidateInitialize(
									const BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** handle);
};


class PartitionMapHandle : public BPartitionHandle {
public:
								PartitionMapHandle(
									BMutablePartition* partition);
	virtual						~PartitionMapHandle();

			status_t			Init();

	virtual	uint32				SupportedOperations(uint32 mask);
	virtual	uint32				SupportedChildOperations(
									const BMutablePartition* child,
									uint32 mask);

	virtual	status_t			GetNextSupportedType(
									const BMutablePartition* child,
									int32* cookie, BString* type);

	virtual	status_t			GetPartitioningInfo(BPartitioningInfo* info);

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
	virtual status_t			DeleteChild(BMutablePartition* child);

private:
			PartitionMap		fPartitionMap;
};


#endif	// _PARTITION_MAP_ADD_ON_H
