/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef SPECIFIC_IMAGE_DEBUG_INFO_LOADING_STATE_H
#define SPECIFIC_IMAGE_DEBUG_INFO_LOADING_STATE_H


#include <Referenceable.h>


class SpecificImageDebugInfoLoadingState : public BReferenceable {
public:
								SpecificImageDebugInfoLoadingState();
	virtual						~SpecificImageDebugInfoLoadingState();

	virtual	bool				UserInputRequired() const = 0;
};


#endif // SPECIFIC_IMAGE_DEBUG_INFO_LOADING_STATE_H
