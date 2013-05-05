/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GPT_DISK_ADD_ON_H
#define GPT_DISK_ADD_ON_H


#include <DiskSystemAddOn.h>


class GPTDiskAddOn : public BDiskSystemAddOn {
public:
								GPTDiskAddOn();
	virtual						~GPTDiskAddOn();

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** handle);

	virtual	bool				CanInitialize(
									const BMutablePartition* partition);
	virtual	status_t			ValidateInitialize(
									const BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** handle);
};


#endif	// GPT_DISK_ADD_ON_H
