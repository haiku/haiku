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
class DataSource;
class Model;
struct system_profiler_event_header;


class ModelLoader {
public:
								ModelLoader(DataSource* dataSource,
									const BMessenger& target,
									void* targetCookie);
								~ModelLoader();

			status_t			StartLoading();
			void				Abort();

			Model*				DetachModel();

private:
	static	status_t			_LoaderEntry(void* data);
			status_t			_Loader();
			status_t			_Load();
			status_t			_ReadDebugEvents(void** _eventData,
									size_t* _size);
			status_t			_ProcessEvent(uint32 event, uint32 cpu,
									const void* buffer, size_t size);

	inline	void				_UpdateLastEventTime(bigtime_t time);

private:
			BLocker				fLock;
			Model*				fModel;
			DataSource*			fDataSource;
			BMessenger			fTarget;
			void*				fTargetCookie;
			thread_id			fLoaderThread;
			bool				fLoading;
			bool				fAborted;
			bigtime_t			fBaseTime;
			bigtime_t			fLastEventTime;
};


#endif	// MAIN_MODEL_LOADER_H
