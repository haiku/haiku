/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2019, Les De Ridder, les@lesderid.net
 *
 * Distributed under the terms of the MIT License.
 */

#ifndef _BTRFS_ADD_ON_H
#define _BTRFS_ADD_ON_H

#include <DiskSystemAddOn.h>

class BTRFSAddOn : public BDiskSystemAddOn {
public:
								BTRFSAddOn();
	virtual						~BTRFSAddOn();

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


class BTRFSPartitionHandle : public BPartitionHandle {
public:
								BTRFSPartitionHandle(
									BMutablePartition* partition);
								~BTRFSPartitionHandle();

			status_t			Init();

	virtual	uint32				SupportedOperations(uint32 mask);

	virtual	status_t			Repair(bool checkOnly);
};


#endif	// _BTRFS_ADD_ON_H
