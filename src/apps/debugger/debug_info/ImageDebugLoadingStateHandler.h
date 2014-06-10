/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAGE_DEBUG_LOADING_STATE_HANDLER_H
#define IMAGE_DEBUG_LOADING_STATE_HANDLER_H


#include <Referenceable.h>


class SpecificImageDebugInfoLoadingState;
class UserInterface;


class ImageDebugLoadingStateHandler : public BReferenceable {
public:
	virtual						~ImageDebugLoadingStateHandler();

	virtual	bool				SupportsState(
									SpecificImageDebugInfoLoadingState* state)
									= 0;

	virtual	void				HandleState(
									SpecificImageDebugInfoLoadingState* state,
									UserInterface* interface) = 0;
};


#endif	// IMAGE_DEBUG_LOADING_STATE_HANDLER_H
