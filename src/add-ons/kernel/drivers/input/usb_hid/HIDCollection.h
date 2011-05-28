/*
 * Copyright 2009-2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_COLLECTION_H
#define HID_COLLECTION_H

#include "HIDParser.h"

class HIDReport;
class HIDReportItem;

class HIDCollection {
public:
								HIDCollection(HIDCollection *parent,
									uint8 type, local_item_state &localState);
								~HIDCollection();

		uint8					Type() { return fType; };

		uint16					UsagePage();
		uint16					UsageID();

		HIDCollection *			Parent() { return fParent; };

		status_t				AddChild(HIDCollection *child);
		uint32					CountChildren() { return fChildCount; };
		HIDCollection *			ChildAt(uint32 index);

		uint32					CountChildrenFlat(uint8 type);
		HIDCollection *			ChildAtFlat(uint8 type, uint32 index);

		void					AddItem(HIDReportItem *item);
		uint32					CountItems() { return fItemCount; };
		HIDReportItem *			ItemAt(uint32 index);

		uint32					CountItemsFlat();
		HIDReportItem *			ItemAtFlat(uint32 index);

		void					BuildReportList(uint8 reportType,
									HIDReport **reportList,
									uint32 &reportCount);

		void					PrintToStream(uint32 indentLevel = 0);

private:
		HIDCollection *			_ChildAtFlat(uint8 type, uint32 &index);
		HIDReportItem *			_ItemAtFlat(uint32 &index);

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
