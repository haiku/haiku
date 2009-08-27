/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _INPUTSERVERFILTER_H
#define _INPUTSERVERFILTER_H


#include <MessageFilter.h>


class BRegion;


class BInputServerFilter {
public:
								BInputServerFilter();
	virtual						~BInputServerFilter();

	virtual	status_t			InitCheck();

	virtual	filter_result		Filter(BMessage* message, BList* _list);
	
			status_t			GetScreenRegion(BRegion* region) const;

private:
	// FBC Padding
	virtual	void				_ReservedInputServerFilter1();
	virtual	void				_ReservedInputServerFilter2();
	virtual	void				_ReservedInputServerFilter3();
	virtual	void				_ReservedInputServerFilter4();

	uint32						_reserved[4];
};

#endif // _INPUTSERVERFILTER_H
