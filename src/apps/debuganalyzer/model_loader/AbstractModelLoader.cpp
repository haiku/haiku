/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "AbstractModelLoader.h"

#include <AutoLocker.h>

#include "MessageCodes.h"


AbstractModelLoader::AbstractModelLoader(const BMessenger& target,
	void* targetCookie)
	:
	fLock("main model loader"),
	fTarget(target),
	fTargetCookie(targetCookie),
	fLoaderThread(-1),
	fLoading(false),
	fAborted(false)
{
}


AbstractModelLoader::~AbstractModelLoader()
{
}


status_t
AbstractModelLoader::StartLoading()
{
	// check initialization
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	AutoLocker<BLocker> locker(fLock);

	if (fLoading)
		return B_BAD_VALUE;

	// prepare for loading
	error = PrepareForLoading();
	if (error != B_OK)
		return error;

	// spawn the loader thread
	fLoaderThread = spawn_thread(&_LoaderEntry, "model loader",
		B_NORMAL_PRIORITY, this);
	if (fLoaderThread < 0)
		return fLoaderThread;

	fLoading = true;
	fAborted = false;

	resume_thread(fLoaderThread);

	return B_OK;
}


void
AbstractModelLoader::Abort(bool wait)
{
	AutoLocker<BLocker> locker(fLock);

	if (fLoaderThread < 0)
		return;

	thread_id thread = fLoaderThread;

	if (fLoading)
		fAborted = true;

	locker.Unlock();

	if (wait)
		wait_for_thread(thread, NULL);
}


void
AbstractModelLoader::Delete()
{
	Abort(true);
	delete this;
}


/*!	Called from StartLoading() with the lock held.
*/
status_t
AbstractModelLoader::PrepareForLoading()
{
	return B_OK;
}


status_t
AbstractModelLoader::Load()
{
	return B_OK;
}


/*!	Called after loading Load() is done with the lock held.
*/
void
AbstractModelLoader::FinishLoading(bool success)
{
}


void
AbstractModelLoader::NotifyTarget(bool success)
{
	BMessage message(success
		? MSG_MODEL_LOADED_SUCCESSFULLY
		: fAborted ? MSG_MODEL_LOADED_ABORTED : MSG_MODEL_LOADED_FAILED);

	message.AddPointer("loader", this);
	message.AddPointer("targetCookie", fTargetCookie);
	fTarget.SendMessage(&message);
}


/*static*/ status_t
AbstractModelLoader::_LoaderEntry(void* data)
{
	return ((AbstractModelLoader*)data)->_Loader();
}


status_t
AbstractModelLoader::_Loader()
{
	bool success = Load() == B_OK;

	// clean up and notify the target
	AutoLocker<BLocker> locker(fLock);

	FinishLoading(success);
	NotifyTarget(success);
	fLoading = false;

	return B_OK;

}
