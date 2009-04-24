/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_MODEL_LOADER_H
#define MAIN_MODEL_LOADER_H

#include <Locker.h>
#include <Messenger.h>

#include <util/OpenHashTable.h>

#include "Model.h"


class BDataIO;
class BDebugEventInputStream;
class DataSource;
struct system_profiler_thread_added;


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
			enum ScheduleState {
				RUNNING,
				STILL_RUNNING,
				PREEMPTED,
				READY,
				WAITING,
				UNKNOWN
			};

			struct ThreadInfo : HashTableLink<ThreadInfo> {
				Model::Thread*	thread;
				ScheduleState	state;
				bigtime_t		lastTime;
				Model::ThreadWaitObject* waitObject;

				ThreadInfo(Model::Thread* thread);

				thread_id ID() const	{ return thread->ID(); }
			};

			struct ThreadTableDefinition {
				typedef thread_id	KeyType;
				typedef	ThreadInfo	ValueType;

				size_t HashKey(thread_id key) const
					{ return (size_t)key; }

				size_t Hash(const ThreadInfo* value) const
					{ return (size_t)value->ID(); }

				bool Compare(thread_id key, const ThreadInfo* value) const
					{ return key == value->ID(); }

				HashTableLink<ThreadInfo>* GetLink(ThreadInfo* value) const
					{ return value; }
			};

			typedef OpenHashTable<ThreadTableDefinition> ThreadTable;

			// shorthands for the longish structure names
			typedef system_profiler_thread_enqueued_in_run_queue
				thread_enqueued_in_run_queue;
			typedef system_profiler_thread_removed_from_run_queue
				thread_removed_from_run_queue;

private:
	static	status_t			_LoaderEntry(void* data);
			status_t			_Loader();
			status_t			_Load();
			status_t			_ReadDebugEvents(void** _eventData,
									size_t* _size);
			status_t			_ProcessEvent(uint32 event, uint32 cpu,
									const void* buffer, size_t size);

	inline	void				_UpdateLastEventTime(bigtime_t time);

			void				_HandleTeamAdded(
									system_profiler_team_added* event);
			void				_HandleTeamRemoved(
									system_profiler_team_removed* event);
			void				_HandleTeamExec(
									system_profiler_team_exec* event);
			void				_HandleThreadAdded(
									system_profiler_thread_added* event);
			void				_HandleThreadRemoved(
									system_profiler_thread_removed* event);
			void				_HandleThreadScheduled(
									system_profiler_thread_scheduled* event);
			void				_HandleThreadEnqueuedInRunQueue(
									thread_enqueued_in_run_queue* event);
			void				_HandleThreadRemovedFromRunQueue(
									thread_removed_from_run_queue* event);
			void				_HandleWaitObjectInfo(
									system_profiler_wait_object_info* event);

			ThreadInfo*			_AddThread(system_profiler_thread_added* event);
			void				_AddThreadWaitObject(ThreadInfo* thread,
									uint32 type, addr_t object);

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
			ThreadTable			fThreads;
};


#endif	// MAIN_MODEL_LOADER_H
