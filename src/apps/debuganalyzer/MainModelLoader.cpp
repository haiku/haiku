/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "MainModelLoader.h"

#include <new>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <DebugEventStream.h>

#include "MainModel.h"
#include "MessageCodes.h"


MainModelLoader::MainModelLoader()
	:
	fLock("main model loader"),
	fModel(NULL),
	fInput(NULL),
	fTarget(),
	fLoaderThread(-1),
	fLoading(false),
	fAborted(false)
{
}


MainModelLoader::~MainModelLoader()
{
	if (fLoaderThread >= 0) {
		Abort();
		wait_for_thread(fLoaderThread, NULL);
	}

	delete fInput;
	delete fModel;
}


status_t
MainModelLoader::StartLoading(BDataIO& input, const BMessenger& target)
{
	// check initialization
	status_t error = fLock.InitCheck();
	if (error != B_OK)
		return error;

	AutoLocker<BLocker> locker(fLock);

	if (fModel != NULL)
		return B_BAD_VALUE;

	// create a model
	MainModel* model = new(std::nothrow) MainModel;
	if (model == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<MainModel> modelDeleter(model);

	// create a debug input stream
	BDebugEventInputStream* inputStream
		= new(std::nothrow) BDebugEventInputStream;
	if (inputStream == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BDebugEventInputStream> inputDeleter(inputStream);

	error = inputStream->SetTo(&input);
	if (error != B_OK)
		return error;

	// spawn the loader thread
	fLoaderThread = spawn_thread(&_LoaderEntry, "main model loader",
		B_NORMAL_PRIORITY, this);
	if (fLoaderThread < 0)
		return fLoaderThread;

	modelDeleter.Detach();
	inputDeleter.Detach();
	fModel = model;
	fInput = inputStream;

	fLoading = true;
	fAborted = false;

	resume_thread(fLoaderThread);

	return B_OK;
}


void
MainModelLoader::Abort()
{
	AutoLocker<BLocker> locker(fLock);

	if (!fLoading || fAborted)
		return;

	fAborted = true;
}


MainModel*
MainModelLoader::DetachModel()
{
	AutoLocker<BLocker> locker(fLock);

	if (fModel == NULL || fLoading)
		return NULL;

	MainModel* model = fModel;
	fModel = NULL;

	return model;
}


/*static*/ status_t
MainModelLoader::_LoaderEntry(void* data)
{
	return ((MainModelLoader*)data)->_Loader();
}


status_t
MainModelLoader::_Loader()
{
	bool success = false;

	uint32 count = 0;

	try {
		while (true) {
			// get next event
			uint32 event;
			uint32 cpu;
			const void* buffer;
			ssize_t bufferSize = fInput->ReadNextEvent(&event, &cpu, &buffer);
			if (bufferSize < 0)
				break;
			if (buffer == NULL) {
				success = true;
				break;
			}

			// process the event
			status_t error = _ProcessEvent(event, cpu, buffer, bufferSize);
			if (error != B_OK)
				break;

			if (++count % 32 == 0) {
				AutoLocker<BLocker> locker(fLock);
				if (fAborted)
					break;
			}
		}
	} catch (...) {
	}

	// clean up and notify the target
	AutoLocker<BLocker> locker(fLock);

	delete fInput;
	fInput = NULL;

	if (success) {
		fTarget.SendMessage(MSG_MODEL_LOADED_SUCCESSFULLY);
	} else {
		delete fModel;
		fModel = NULL;

		fTarget.SendMessage(
			fAborted ? MSG_MODEL_LOADED_ABORTED : MSG_MODEL_LOADED_FAILED);
	}

	fLoading = false;

	return B_OK;
}


status_t
MainModelLoader::_ProcessEvent(uint32 event, uint32 cpu, const void* buffer,
	size_t size)
{
	// TODO: Implement!
	return B_ERROR;
}
