/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// TypedList is a type-safe template version of BList

#include <Debug.h>
#include "TypedList.h"

void *
_PointerList::EachElement(GenericEachFunction func, void *passThru)
{
	// iterates through all elements, calling func on each
	// if each function returns a nonzero value, terminates early
	void *result = NULL; 
	int32 numElements = CountItems();
	
	for (int32 index = 0; index < numElements; index++)
		if ((result = func(ItemAtFast(index), passThru)) != NULL)
			break;

	return result;
}

_PointerList::_PointerList(const _PointerList &list)
	:	BList(list),
		owning(list.owning)
{
}

_PointerList::_PointerList(int32 itemsPerBlock = 20, bool owningList)
	:	BList(itemsPerBlock),
		owning(owningList)
{
}

_PointerList::~_PointerList()
{
}

bool
_PointerList::Owning() const
{
	return owning;
}

bool 
_PointerList::AddUnique(void *newItem)
{
	if (IndexOf(newItem) >= 0)
		return false;

	return AddItem(newItem);
}

struct OneMatchParams {
	void *matchThis;
	_PointerList::GenericCompareFunction matchFunction;
};


static void *
MatchOne(void *item, void *castToParams)
{
	OneMatchParams *params = (OneMatchParams *)castToParams;
	if (params->matchFunction(item, params->matchThis) == 0)
		// got a match, terminate search
		return item;

	return 0;
}

bool 
_PointerList::AddUnique(void *newItem, GenericCompareFunction function)
{
	OneMatchParams params;
	params.matchThis = newItem;
	params.matchFunction = function;

	if (EachElement(MatchOne, &params))
		// already in list
		return false;
	
	return AddItem(newItem);
}

