/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#ifndef USERLAND_HID
#include "Driver.h"
#else
#include "UserlandHID.h"
#endif

#include "HIDCollection.h"
#include "HIDReportItem.h"

#include <new>
#include <stdlib.h>
#include <string.h>


HIDCollection::HIDCollection(HIDCollection *parent, uint8 type,
	local_item_state &localState)
	:	fParent(parent),
		fType(type),
		fStringID(localState.string_index),
		fPhysicalID(localState.designator_index),
		fChildCount(0),
		fChildren(NULL),
		fItemCount(0),
		fItemsAllocated(0),
		fItems(NULL)
{
	usage_value usageValue;
	if (localState.usage_stack != NULL && localState.usage_stack_used > 0)
		usageValue.u.extended = localState.usage_stack[0].u.extended;
	else if (localState.usage_minimum_set)
		usageValue.u.extended = localState.usage_minimum.u.extended;
	else if (localState.usage_maximum_set)
		usageValue.u.extended = localState.usage_maximum.u.extended;
	else {
		TRACE_ALWAYS("non of the possible usages for the collection are set\n");
	}

	fUsage = usageValue.u.extended;
}


HIDCollection::~HIDCollection()
{
	for (uint32 i = 0; i < fChildCount; i++)
		delete fChildren[i];
	free(fChildren);
	free(fItems);
}


status_t
HIDCollection::AddChild(HIDCollection *child)
{
	HIDCollection **newChildren = (HIDCollection **)realloc(fChildren,
		(fChildCount + 1) * sizeof(HIDCollection *));
	if (newChildren == NULL) {
		TRACE_ALWAYS("no memory when trying to resize collection child list\n");
		return B_NO_MEMORY;
	}

	fChildren = newChildren;
	fChildren[fChildCount++] = child;
	return B_OK;
}


HIDCollection *
HIDCollection::ChildAt(uint32 index)
{
	if (index >= fChildCount)
		return NULL;

	return fChildren[index];
}


void
HIDCollection::AddItem(HIDReportItem *item)
{
	if (fItemCount >= fItemsAllocated) {
		fItemsAllocated += 10;
		HIDReportItem **newItems = (HIDReportItem **)realloc(fItems,
			fItemsAllocated * sizeof(HIDReportItem *));
		if (newItems == NULL) {
			TRACE_ALWAYS("no memory when trying to resize collection items\n");
			fItemsAllocated -= 10;
			return;
		}

		fItems = newItems;
	}

	fItems[fItemCount++] = item;
}


HIDReportItem *
HIDCollection::ItemAt(uint32 index)
{
	if (index >= fItemCount)
		return NULL;

	return fItems[index];
}


void
HIDCollection::PrintToStream(uint32 indentLevel)
{
	char indent[indentLevel + 1];
	memset(indent, '\t', indentLevel);
	indent[indentLevel] = 0;

	const char *typeName = "unknown";
	switch (fType) {
		case COLLECTION_PHYSICAL:
			typeName = "physical";
			break;
		case COLLECTION_APPLICATION:
			typeName = "application";
			break;
		case COLLECTION_LOGICAL:
			typeName = "logical";
			break;
		case COLLECTION_REPORT:
			typeName = "report";
			break;
		case COLLECTION_NAMED_ARRAY:
			typeName = "named array";
			break;
		case COLLECTION_USAGE_SWITCH:
			typeName = "usage switch";
			break;
		case COLLECTION_USAGE_MODIFIER:
			typeName = "usage modifier";
			break;
	}

	TRACE_ALWAYS("%sHIDCollection %p\n", indent, this);
	TRACE_ALWAYS("%s\ttype: %u %s\n", indent, fType, typeName);
	TRACE_ALWAYS("%s\tusage: 0x%08lx\n", indent, fUsage);
	TRACE_ALWAYS("%s\tstring id: %u\n", indent, fStringID);
	TRACE_ALWAYS("%s\tphysical id: %u\n", indent, fPhysicalID);

	TRACE_ALWAYS("%s\titem count: %lu\n", indent, fItemCount);
	for (uint32 i = 0; i < fItemCount; i++) {
		HIDReportItem *item = fItems[i];
		if (item != NULL)
			item->PrintToStream(indentLevel + 1);
	}

	TRACE_ALWAYS("%s\tchild count: %lu\n", indent, fChildCount);
	for (uint32 i = 0; i < fChildCount; i++) {
		HIDCollection *child = fChildren[i];
		if (child != NULL)
			child->PrintToStream(indentLevel + 1);
	}
}
