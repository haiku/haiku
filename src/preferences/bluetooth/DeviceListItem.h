/*
 * Copyright 2009, Oliver Ruiz Dorantes, <oliver.ruiz.dorantes_at_gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DEVICELISTITEM_H_
#define DEVICELISTITEM_H_

#include <ListItem.h>

namespace bluetooth {

// TODO: Implement a BluetoothDeviceListItem class, this one is stolen from somewhere .....
class RangeItem : public BListItem
{
	public:
		RangeItem(uint32 lowAddress, uint32 highAddress, const char* name);
		~RangeItem();
		virtual void DrawItem(BView *, BRect, bool = false);
		static int Compare(const void *firstArg, const void *secondArg);
	private:
		char* fName;
		uint32 fLowAddress, fHighAddress;
};

}


#endif
