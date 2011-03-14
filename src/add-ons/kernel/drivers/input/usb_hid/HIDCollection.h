/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_COLLECTION_H
#define HID_COLLECTION_H

#include "HIDParser.h"

class HIDReportItem;

class HIDCollection {
public:
								HIDCollection(HIDCollection *parent,
									uint8 type, local_item_state &localState);
								~HIDCollection();

		HIDCollection *			Parent() { return fParent; };

		status_t				AddChild(HIDCollection *child);
		uint32					CountChildren() { return fChildCount; };
		HIDCollection *			ChildAt(uint32 index);

		void					AddItem(HIDReportItem *item);
		uint32					CountItems() { return fItemCount; };
		HIDReportItem *			ItemAt(uint32 index);

		void					PrintToStream(uint32 indentLevel = 0);

private:
		HIDCollection *			fParent;

		uint8					fType;
		uint32					fUsage;
		uint8					fStringID;
		uint8					fPhysicalID;

		uint32					fChildCount;
		HIDCollection **		fChildren;

		uint32					fItemCount;
		uint32					fItemsAllocated;
		HIDReportItem **		fItems;
};

#endif // HID_COLLECTION_H
