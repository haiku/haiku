/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_MODEL_LOADER_H
#define MAIN_MODEL_LOADER_H

#include <Locker.h>
#include <Messenger.h>


class BDataIO;
class BDebugEventInputStream;
class MainModel;
struct system_profiler_event_header;


class MainModelLoader {
public:
								MainModelLoader();
								~MainModelLoader();

			status_t			StartLoading(BDataIO& input,
									const BMessenger& target);
			void				Abort();

			MainModel*			DetachModel();

private:
	static	status_t			_LoaderEntry(void* data);
			status_t			_Loader();
			status_t			_ProcessEvent(uint32 event, uint32 cpu,
									const void* buffer, size_t size);

private:
			BLocker				fLock;
			MainModel*			fModel;
			BDebugEventInputStream* fInput;
			BMessenger			fTarget;
			thread_id			fLoaderThread;
			bool				fLoading;
			bool				fAborted;
};


#endif	// MAIN_MODEL_LOADER_H
