/*
 * Copyright 2014 Haiku, Inc. All rights reserved
 * Distributed under the terms of the MIT License.
 */
#ifndef _MINIMIZE_ALL_INPUT_FILTER_H
#define _MINIMIZE_ALL_INPUT_FILTER_H


#include <InputServerFilter.h>


extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();


class MinimizeAllInputFilter : public BInputServerFilter {
public:
								MinimizeAllInputFilter();

	virtual	filter_result		Filter(BMessage* message, BList* _list);
	virtual	status_t			InitCheck();
};


#endif // _MINIMIZE_ALL_INPUT_FILTER_H
