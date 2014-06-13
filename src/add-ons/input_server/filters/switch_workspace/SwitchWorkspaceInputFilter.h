/*
 * Copyright 2014 Haiku, Inc. All rights reserved
 * Distributed under the terms of the MIT License.
 */
#ifndef _SWITCH_WORKSPACE_INPUT_FILTER_H
#define _SWITCH_WORKSPACE_INPUT_FILTER_H


#include <InputServerFilter.h>


extern "C" _EXPORT BInputServerFilter* instantiate_input_filter();


class SwitchWorkspaceInputFilter : public BInputServerFilter {
public:
								SwitchWorkspaceInputFilter();

	virtual	filter_result		Filter(BMessage* message, BList* _list);
	virtual	status_t			InitCheck();
};


#endif // _SWITCH_WORKSPACE_INPUT_FILTER_H
