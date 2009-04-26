/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ThreadModelLoader.h"

#include <new>

#include <AutoLocker.h>

#include "ThreadModel.h"


static int
compare_by_type_and_name(const Model::ThreadWaitObject* a,
	const Model::ThreadWaitObject* b)
{
	if (a->Type() != b->Type())
		return a->Type() < b->Type() ? -1 : 1;

	return strcmp(a->Name(), b->Name());
}


// #pragma mark -


ThreadModelLoader::ThreadModelLoader(Model* model, Model::Thread* thread,
	const BMessenger& target, void* targetCookie)
	:
	AbstractModelLoader(target, targetCookie),
	fModel(model),
	fThread(thread),
	fThreadModel(NULL)
{
}


ThreadModelLoader::~ThreadModelLoader()
{
	delete fThreadModel;
}


ThreadModel*
ThreadModelLoader::DetachModel()
{
	AutoLocker<BLocker> locker(fLock);

	if (fThreadModel == NULL || fLoading)
		return NULL;

	ThreadModel* model = fThreadModel;
	fThreadModel = NULL;

	return model;
}


status_t
ThreadModelLoader::PrepareForLoading()
{
	return B_OK;
}


status_t
ThreadModelLoader::Load()
{
	try {
		return _Load();
	} catch(...) {
		return B_ERROR;
	}
}


void
ThreadModelLoader::FinishLoading(bool success)
{
	if (!success) {
		delete fThreadModel;
		fThreadModel = NULL;
	}
}


status_t
ThreadModelLoader::_Load()
{
	// create a model
	fThreadModel = new(std::nothrow) ThreadModel(fModel, fThread);
	if (fThreadModel == NULL)
		return B_NO_MEMORY;

	// collect all wait objects
	BObjectList<Model::ThreadWaitObject> waitObjects;

	int32 groupCount = fThread->CountThreadWaitObjectGroups();
	for (int32 i = 0; i < groupCount; i++) {
		Model::ThreadWaitObjectGroup* group
			= fThread->ThreadWaitObjectGroupAt(i);

		if (!group->GetThreadWaitObjects(waitObjects))
			return B_NO_MEMORY;
	}

	// sort them by type and name
	waitObjects.SortItems(&compare_by_type_and_name);

	// create the groups
	int32 count = waitObjects.CountItems();
	for (int32 i = 0; i < count; i++) {
		// collect the objects for this group
		Model::ThreadWaitObject* firstObject = waitObjects.ItemAt(i);
		int32 k = i + 1;
		for (; k < count; k++) {
			if (compare_by_type_and_name(firstObject, waitObjects.ItemAt(k))
					!= 0) {
				break;
			}
		}

		if (fThreadModel->AddWaitObjectGroup(waitObjects, i, k) == NULL)
			return B_NO_MEMORY;
	}

	return B_OK;
}
