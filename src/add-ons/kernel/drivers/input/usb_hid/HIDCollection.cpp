#include "Driver.h"
#include "HIDCollection.h"

#include <new>
#include <stdlib.h>


HIDCollection::HIDCollection(HIDCollection *parent, uint8 type,
	global_item_state &globalState, local_item_state &localState)
	:	fParent(parent),
		fType(type),
		fStringID(localState.string_index),
		fPhysicalID(localState.designator_index),
		fChildCount(0),
		fChildren(NULL)
{
	usage_value usageValue;
	usageValue.u.s.usage_page = globalState.usage_page;
	if (localState.usage_stack != NULL && localState.usage_stack_used > 0) {
		if (localState.usage_stack[0].is_extended)
			usageValue.u.extended = localState.usage_stack[0].u.extended;
		else
			usageValue.u.s.usage_id = localState.usage_stack[0].u.s.usage_id;
	} else if (localState.usage_minimum_set) {
		if (localState.usage_minimum.is_extended)
			usageValue.u.extended = localState.usage_minimum.u.extended;
		else
			usageValue.u.s.usage_id = localState.usage_minimum.u.s.usage_id;
	}

	fUsage = usageValue.u.extended;
}


HIDCollection::~HIDCollection()
{
	for (uint32 i = 0; i < fChildCount; i++)
		delete fChildren[i];
	free(fChildren);
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
HIDCollection::AddMainItem(global_item_state &globalState,
	local_item_state &localState, main_item_data &mainData)
{
	// TODO: implement
}
