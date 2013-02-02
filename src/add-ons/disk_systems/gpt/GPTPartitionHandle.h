/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GPT_PARTITION_HANDLE_H
#define GPT_PARTITION_HANDLE_H


#include <DiskSystemAddOn.h>

#include "Header.h"


class GPTPartitionHandle : public BPartitionHandle {
public:
								GPTPartitionHandle(
									BMutablePartition* partition);
	virtual						~GPTPartitionHandle();

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
			EFI::Header*		fHeader;
};


#endif	// GPT_PARTITION_HANDLE_H
