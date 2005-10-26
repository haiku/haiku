/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __PARTITIONMENUITEM_H_
#define __PARTITIONMENUITEM_H_

#include <MenuItem.h>
#include <Partition.h>

const uint32 SRC_PARTITION = 'iSPT';
const uint32 TARGET_PARTITION = 'iTPT';

class PartitionMenuItem : public BMenuItem {
	public:
		PartitionMenuItem(const char *label, BMessage *msg, partition_id id);
		~PartitionMenuItem();
		partition_id ID() const { return fID; };
	private:
		partition_id fID;
};

#endif	/* __PARTITIONMENUITEM_H_ */
