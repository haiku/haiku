/*
 * Copyright 2005, Jérôme DUVAL. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef __PARTITIONMENUITEM_H_
#define __PARTITIONMENUITEM_H_


#include <MenuItem.h>


class PartitionMenuItem : public BMenuItem {
	public:
		PartitionMenuItem(const char *label, BMessage *msg, const char* path);
		~PartitionMenuItem();
		const char* Path() { return fPath; };
	private:
		char *fPath;
};

#endif	/* __PARTITIONMENUITEM_H_ */
