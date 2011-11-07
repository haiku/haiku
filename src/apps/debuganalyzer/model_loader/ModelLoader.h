/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MAIN_MODEL_LOADER_H
#define MAIN_MODEL_LOADER_H


#include <util/DoublyLinkedList.h>

#include "AbstractModelLoader.h"
#include "Model.h"


class BDataIO;
class BDebugEventInputStream;
class DataSource;
struct system_profiler_thread_added;


class ModelLoader : public AbstractModelLoader {
public:
								ModelLoader(DataSource* dataSource,
									const BMessenger& target,
									void* targetCookie);

protected:
								~ModelLoader();

public:
			Model*				DetachModel();

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
			typedef system_profiler_io_request_scheduled io_request_scheduled;
			typedef system_profiler_io_request_finished io_request_finished;
			typedef system_profiler_io_operation_started io_operation_started;
			typedef system_profiler_io_operation_finished io_operation_finished;

			struct CPUInfo;
			struct IOOperation;
			struct IORequest;
			struct IORequestHashDefinition;
			struct ExtendedThreadSchedulingState;
			struct ExtendedSchedulingState;

			typedef DoublyLinkedList<ModelLoader::IOOperation> IOOperationList;
			typedef DoublyLinkedList<ModelLoader::IORequest> IORequestList;
			typedef BOpenHashTable<IORequestHashDefinition> IORequestTable;

private:
			status_t			_Load();
			status_t			_ReadDebugEvents(void** _eventData,
									size_t* _size);
			status_t			_CreateDebugEventArray(void* eventData,
									size_t eventDataSize,
									system_profiler_event_header**& _events,
									size_t& _eventCount);
			status_t			_ProcessEvent(uint32 event, uint32 cpu,
									const void* buffer, size_t size);
			bool				_SetThreadEvents();
			bool				_SetThreadIORequests();
			void				_SetThreadIORequests(Model::Thread* thread,
									Model::IORequest** requests,
									size_t requestCount);

	inline	void				_UpdateLastEventTime(nanotime_t time);

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
			void				_HandleThreadScheduled(uint32 cpu,
									system_profiler_thread_scheduled* event);
			void				_HandleThreadEnqueuedInRunQueue(
									thread_enqueued_in_run_queue* event);
			void				_HandleThreadRemovedFromRunQueue(uint32 cpu,
									thread_removed_from_run_queue* event);
			void				_HandleWaitObjectInfo(
									system_profiler_wait_object_info* event);
			void				_HandleIOSchedulerAdded(
									system_profiler_io_scheduler_added* event);
			void				_HandleIORequestScheduled(
									io_request_scheduled* event);
			void				_HandleIORequestFinished(
									io_request_finished* event);
			void				_HandleIOOperationStarted(
									io_operation_started* event);
			void				_HandleIOOperationFinished(
									io_operation_finished* event);

			ExtendedThreadSchedulingState* _AddThread(
									system_profiler_thread_added* event);
			ExtendedThreadSchedulingState* _AddUnknownThread(
									thread_id threadID);
			Model::Team*		_AddUnknownTeam();
			
			void				_AddThreadWaitObject(
									ExtendedThreadSchedulingState* thread,
									uint32 type, addr_t object);

			void				_AddIdleTime(uint32 cpu, nanotime_t time);

private:
			Model*				fModel;
			DataSource*			fDataSource;
			CPUInfo*			fCPUInfos;
			nanotime_t			fBaseTime;
			ExtendedSchedulingState* fState;
			IORequestTable*		fIORequests;
			uint32				fMaxCPUIndex;
};


#endif	// MAIN_MODEL_LOADER_H
