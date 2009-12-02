/*
 * Copyright 2003 Marcus Overhagen
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_RT_LIST_H
#define _MEDIA_RT_LIST_H


/*!	A simple list template that uses realtime
	memory and does no error checking. Since
	it doesn't call constructors or destructors,
	don't use it to store objects.

	// TODO: no error checking? Great.
*/


#include <RealtimeAlloc.h>


template<class value> class RtList {
public:
	RtList()
		:
		item_max(INIT_COUNT),
	 	item_count(0),
	 	items((value*)rtm_alloc(NULL, sizeof(value) * INIT_COUNT))
	{
	}

	~RtList()
	{
		rtm_free(items);
	}

	value* Create()
	{
		if (item_count == item_max) {
			item_max += INC_COUNT;
			rtm_realloc((void**)&items, sizeof(value) * item_max);
		}
		return &items[item_count++];
	}

	value* ItemAt(int index)
	{
		return &items[index];
	}

	int CountItems()
	{
		return item_count;
	}

	void MakeEmpty()
	{
		item_count = 0;
	}

private:
	RtList(const RtList<value>& other);
	RtList<value> &operator=(const RtList<value>& other);

private:
	enum { INIT_COUNT = 8, INC_COUNT = 16 };

	int		item_max;
	int		item_count;
	value*	items;
};

#endif // _MEDIA_RT_LIST_H
