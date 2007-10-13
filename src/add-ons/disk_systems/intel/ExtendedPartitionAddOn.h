/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */
#ifndef _EXTENDED_PARTITION_ADD_ON_H
#define _EXTENDED_PARTITION_ADD_ON_H

#include <DiskSystemAddOn.h>

#include "PartitionMap.h"


class ExtendedPartitionAddOn : public BDiskSystemAddOn {
public:
								ExtendedPartitionAddOn();
	virtual						~ExtendedPartitionAddOn();

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** handle);

};


class ExtendedPartitionHandle : public BPartitionHandle {
public:
								ExtendedPartitionHandle(
									BMutablePartition* partition);
	virtual						~ExtendedPartitionHandle();

			status_t			Init();

private:
			PrimaryPartition*	fPrimaryPartition;
};


#endif	// _EXTENDED_PARTITION_ADD_ON_H
