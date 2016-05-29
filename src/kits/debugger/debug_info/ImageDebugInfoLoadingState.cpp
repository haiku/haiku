/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ImageDebugInfoLoadingState.h"

#include "SpecificImageDebugInfoLoadingState.h"


ImageDebugInfoLoadingState::ImageDebugInfoLoadingState()
	:
	fSpecificInfoLoadingState(),
	fSpecificInfoIndex(0)
{
}


ImageDebugInfoLoadingState::~ImageDebugInfoLoadingState()
{
}


bool
ImageDebugInfoLoadingState::HasSpecificDebugInfoLoadingState() const
{
	return fSpecificInfoLoadingState.Get() != NULL;
}


void
ImageDebugInfoLoadingState::SetSpecificDebugInfoLoadingState(
	SpecificImageDebugInfoLoadingState* state)
{
	fSpecificInfoLoadingState.SetTo(state, true);
}


void
ImageDebugInfoLoadingState::ClearSpecificDebugInfoLoadingState()
{
	fSpecificInfoLoadingState = NULL;
}


bool
ImageDebugInfoLoadingState::UserInputRequired() const
{
	if (HasSpecificDebugInfoLoadingState())
		return fSpecificInfoLoadingState->UserInputRequired();

	return false;
}


void
ImageDebugInfoLoadingState::SetSpecificInfoIndex(int32 index)
{
	fSpecificInfoIndex = index;
}
