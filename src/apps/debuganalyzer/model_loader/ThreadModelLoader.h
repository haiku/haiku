/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_MODEL_LOADER_H
#define THREAD_MODEL_LOADER_H

#include "AbstractModelLoader.h"
#include "Model.h"


class ThreadModel;


class ThreadModelLoader : public AbstractModelLoader {
public:
								ThreadModelLoader(Model* model,
									Model::Thread* thread,
									const BMessenger& target,
									void* targetCookie);

protected:
								~ThreadModelLoader();

public:
			ThreadModel*		DetachModel();

protected:
	virtual	status_t			PrepareForLoading();
	virtual	status_t			Load();
	virtual	void				FinishLoading(bool success);

private:
			// shorthands for the longish structure names
			typedef system_profiler_thread_enqueued_in_run_queue
				thread_enqueued_in_run_queue;
			typedef system_profiler_thread_removed_from_run_queue
				thread_removed_from_run_queue;

private:
			status_t			_Load();

private:
			Model*				fModel;
			Model::Thread*		fThread;
			ThreadModel*		fThreadModel;
};


#endif	// THREAD_MODEL_LOADER_H
