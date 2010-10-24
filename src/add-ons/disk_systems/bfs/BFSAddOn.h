/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BFS_ADD_ON_H
#define _BFS_ADD_ON_H

#include <DiskSystemAddOn.h>


class BFSAddOn : public BDiskSystemAddOn {
public:
								BFSAddOn();
	virtual						~BFSAddOn();

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** handle);
	virtual	status_t			GetParameterEditor(
									B_PARAMETER_EDITOR_TYPE type,
									BPartitionParameterEditor** editor);

	virtual	bool				CanInitialize(
									const BMutablePartition* partition);
	virtual	status_t			ValidateInitialize(
									const BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** handle);
};


class BFSPartitionHandle : public BPartitionHandle {
public:
								BFSPartitionHandle(
									BMutablePartition* partition);
								~BFSPartitionHandle();

			status_t			Init();

	virtual	uint32				SupportedOperations(uint32 mask);

	virtual	status_t			Repair(bool checkOnly);
};


#endif	// _BFS_ADD_ON_H
