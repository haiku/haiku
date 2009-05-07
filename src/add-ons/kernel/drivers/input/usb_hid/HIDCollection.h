/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_COLLECTION_H
#define HID_COLLECTION_H

#include "HIDParser.h"

class HIDCollection {
public:
								HIDCollection(HIDCollection *parent,
									uint8 type, global_item_state &globalState,
									local_item_state &localState);
								~HIDCollection();

		HIDCollection *			Parent() { return fParent; };

		status_t				AddChild(HIDCollection *child);
		uint32					CountChildren() { return fChildCount; };
		HIDCollection *			ChildAt(uint32 index);

		void					AddMainItem(global_item_state &globalState,
									local_item_state &localState,
									main_item_data &mainData);

private:
		HIDCollection *			fParent;

		uint8					fType;
		uint32					fUsage;
		uint8					fStringID;
		uint8					fPhysicalID;

		uint32					fChildCount;
		HIDCollection **		fChildren;
};

#endif // HID_COLLECTION_H
