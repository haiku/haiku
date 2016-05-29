/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include "ImageDebugLoadingStateHandlerRoster.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>

#include "DwarfLoadingStateHandler.h"
#include "ImageDebugInfoLoadingState.h"
#include "ImageDebugLoadingStateHandler.h"
#include "SpecificImageDebugInfoLoadingState.h"


/*static*/ ImageDebugLoadingStateHandlerRoster*
	ImageDebugLoadingStateHandlerRoster::sDefaultInstance = NULL;


ImageDebugLoadingStateHandlerRoster::ImageDebugLoadingStateHandlerRoster()
	:
	fLock("type handler roster")
{
}


ImageDebugLoadingStateHandlerRoster::~ImageDebugLoadingStateHandlerRoster()
{
}


/*static*/ ImageDebugLoadingStateHandlerRoster*
ImageDebugLoadingStateHandlerRoster::Default()
{
	return sDefaultInstance;
}


/*static*/ status_t
ImageDebugLoadingStateHandlerRoster::CreateDefault()
{
	if (sDefaultInstance != NULL)
		return B_OK;

	ImageDebugLoadingStateHandlerRoster* roster
		= new(std::nothrow) ImageDebugLoadingStateHandlerRoster;
	if (roster == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<ImageDebugLoadingStateHandlerRoster> rosterDeleter(roster);

	status_t error = roster->Init();
	if (error != B_OK)
		return error;

	error = roster->RegisterDefaultHandlers();
	if (error != B_OK)
		return error;

	sDefaultInstance = rosterDeleter.Detach();
	return B_OK;
}


/*static*/ void
ImageDebugLoadingStateHandlerRoster::DeleteDefault()
{
	ImageDebugLoadingStateHandlerRoster* roster = sDefaultInstance;
	sDefaultInstance = NULL;
	delete roster;
}


status_t
ImageDebugLoadingStateHandlerRoster::Init()
{
	return fLock.InitCheck();
}


status_t
ImageDebugLoadingStateHandlerRoster::RegisterDefaultHandlers()
{
	ImageDebugLoadingStateHandler* handler;
	BReference<ImageDebugLoadingStateHandler> handlerReference;

	handler = new(std::nothrow) DwarfLoadingStateHandler();
	if (handler == NULL)
		return B_NO_MEMORY;
	handlerReference.SetTo(handler, true);

	if (!RegisterHandler(handler))
		return B_NO_MEMORY;

	return B_OK;
}


status_t
ImageDebugLoadingStateHandlerRoster::FindStateHandler(
	SpecificImageDebugInfoLoadingState* state,
	ImageDebugLoadingStateHandler*& _handler)
{
	AutoLocker<BLocker> locker(fLock);

	bool found = false;
	ImageDebugLoadingStateHandler* handler = NULL;
	for (int32 i = 0; (handler = fStateHandlers.ItemAt(i)); i++) {
		if ((found = handler->SupportsState(state)))
			break;
	}

	if (!found)
		return B_ENTRY_NOT_FOUND;

	handler->AcquireReference();
	_handler = handler;
	return B_OK;
}


bool
ImageDebugLoadingStateHandlerRoster::RegisterHandler(
	ImageDebugLoadingStateHandler* handler)
{
	if (!fStateHandlers.AddItem(handler))
		return false;

	handler->AcquireReference();
	return true;
}


void
ImageDebugLoadingStateHandlerRoster::UnregisterHandler(
	ImageDebugLoadingStateHandler* handler)
{
	if (fStateHandlers.RemoveItem(handler))
		handler->ReleaseReference();
}
