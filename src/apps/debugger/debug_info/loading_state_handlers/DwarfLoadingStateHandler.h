/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_LOADING_STATE_HANDLER
#define DWARF_LOADING_STATE_HANDLER


#include "ImageDebugLoadingStateHandler.h"


class DwarfLoadingStateHandler : public ImageDebugLoadingStateHandler {
public:
								DwarfLoadingStateHandler();
	virtual						~DwarfLoadingStateHandler();

	virtual	bool				SupportsState(
									SpecificImageDebugInfoLoadingState* state);

	virtual	void				HandleState(
									SpecificImageDebugInfoLoadingState* state,
									UserInterface* interface);

};


#endif	// DWARF_LOADING_STATE_HANDLER_H
