/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H


#include <stdlib.h>

#include <OS.h>
#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>
#include <util/OpenHashTable.h>

#include <system_profiler_defs.h>
#include <util/SinglyLinkedList.h>


enum ThreadState {
	RUNNING,
	STILL_RUNNING,
	PREEMPTED,
	READY,
	WAITING,
	UNKNOWN
};

const char* thread_state_name(ThreadState state);
const char* wait_object_type_name(uint32 type);


class Model : public BReferenceable {
public:
			struct creation_time_id;
			struct type_and_object;
			class CPU;
			struct IOOperation;
			struct IORequest;
			class IOScheduler;
			class WaitObjectGroup;
			class WaitObject;
			class ThreadWaitObject;
			class ThreadWaitObjectGroup;
			class Team;
			class Thread;
			struct CompactThreadSchedulingState;
			struct ThreadSchedulingState;
			struct ThreadSchedulingStateDefinition;
			typedef BOpenHashTable<ThreadSchedulingStateDefinition>
				ThreadSchedulingStateTable;
			class SchedulingState;
			class CompactSchedulingState;

public:
								Model(const char* dataSourceName,
									void* eventData, size_t eventDataSize,
									system_profiler_event_header** events,
									size_t eventCount);
								~Model();

	inline	const char*			DataSourceName() const;
	inline	void*				EventData() const;
	inline	size_t				EventDataSize() const;
	inline	system_profiler_event_header** Events() const;
	inline	size_t				CountEvents() const;
			size_t				ClosestEventIndex(nanotime_t eventTime) const;
									// finds the greatest event with event
									// time >= eventTime; may return
									// CountEvents()

			bool				AddAssociatedData(void* data);
			void				RemoveAssociatedData(void* data);

			void				LoadingFinished();

	inline	nanotime_t			BaseTime() const;
			void				SetBaseTime(nanotime_t time);

	inline	nanotime_t			LastEventTime() const;
			void				SetLastEventTime(nanotime_t time);

	inline	nanotime_t			IdleTime() const;

	inline	int32				CountCPUs() const;
			bool				SetCPUCount(int32 count);
	inline	CPU*				CPUAt(int32 index) const;

			int32				CountTeams() const;
			Team*				TeamAt(int32 index) const;
			Team*				TeamByID(team_id id) const;
			Team*				AddTeam(
									const system_profiler_team_added* event,
									nanotime_t time);

			int32				CountThreads() const;
			Thread*				ThreadAt(int32 index) const;
			Thread*				ThreadByID(thread_id id) const;
			Thread*				AddThread(
									const system_profiler_thread_added* event,
									nanotime_t time);

			WaitObject*			AddWaitObject(
									const system_profiler_wait_object_info*
										event,
									WaitObjectGroup** _waitObjectGroup);

			int32				CountWaitObjectGroups() const;
			WaitObjectGroup*	WaitObjectGroupAt(int32 index) const;
			WaitObjectGroup*	WaitObjectGroupFor(uint32 type,
									addr_t object) const;

			ThreadWaitObject*	AddThreadWaitObject(thread_id threadID,
									WaitObject* waitObject,
									ThreadWaitObjectGroup**
										_threadWaitObjectGroup);
			ThreadWaitObjectGroup* ThreadWaitObjectGroupFor(
									thread_id threadID, uint32 type,
									addr_t object) const;

			int32				CountIOSchedulers() const;
			IOScheduler*		IOSchedulerAt(int32 index) const;
			IOScheduler*		IOSchedulerByID(int32 id) const;
			IOScheduler*		AddIOScheduler(
									system_profiler_io_scheduler_added* event);

			bool				AddSchedulingStateSnapshot(
									const SchedulingState& state,
									off_t eventOffset);
									// must be added in order (of time)
			const CompactSchedulingState* ClosestSchedulingState(
									nanotime_t eventTime) const;
									// returns the closest previous state

private:
			typedef BObjectList<CPU> CPUList;
			typedef BObjectList<Team> TeamList;
			typedef BObjectList<Thread> ThreadList;
			typedef BObjectList<WaitObjectGroup> WaitObjectGroupList;
			typedef BObjectList<IOScheduler> IOSchedulerList;
			typedef BObjectList<CompactSchedulingState> SchedulingStateList;

private:
	static	int					_CompareEventTimeSchedulingState(
									const nanotime_t* time,
									const CompactSchedulingState* state);

private:
			BString				fDataSourceName;
			void*				fEventData;
			system_profiler_event_header** fEvents;
			size_t				fEventDataSize;
			size_t				fEventCount;
			int32				fCPUCount;
			nanotime_t			fBaseTime;
			nanotime_t			fLastEventTime;
			nanotime_t			fIdleTime;
			CPUList				fCPUs;
			TeamList			fTeams;		// sorted by ID
			ThreadList			fThreads;	// sorted by ID
			WaitObjectGroupList	fWaitObjectGroups;
			IOSchedulerList		fIOSchedulers;
			SchedulingStateList	fSchedulingStates;
			BList				fAssociatedData;
};


struct Model::creation_time_id {
	nanotime_t	time;
	thread_id	id;
};


struct Model::type_and_object {
	uint32		type;
	addr_t		object;
};


class Model::CPU {
public:
								CPU();

	inline	nanotime_t			IdleTime() const;
			void				SetIdleTime(nanotime_t time);

private:
			nanotime_t			fIdleTime;
};


struct Model::IOOperation {
	system_profiler_io_operation_started*	startedEvent;
	system_profiler_io_operation_finished*	finishedEvent;

	static inline int			CompareByTime(const IOOperation* a,
									const IOOperation* b);

	inline	nanotime_t			StartedTime() const;
	inline	nanotime_t			FinishedTime() const;
	inline	bool				IsFinished() const;
	inline	off_t				Offset() const;
	inline	size_t				Length() const;
	inline	bool				IsWrite() const;
	inline	status_t			Status() const;
	inline	size_t				BytesTransferred() const;
};


struct Model::IORequest {
	system_profiler_io_request_scheduled*	scheduledEvent;
	system_profiler_io_request_finished*	finishedEvent;
	size_t									operationCount;
	IOOperation								operations[0];

								IORequest(
									system_profiler_io_request_scheduled*
										scheduledEvent,
									system_profiler_io_request_finished*
										finishedEvent,
									size_t operationCount);
								~IORequest();

	static	IORequest*			Create(
									system_profiler_io_request_scheduled*
										scheduledEvent,
									system_profiler_io_request_finished*
									finishedEvent,
									size_t operationCount);
			void				Delete();

	inline	nanotime_t			ScheduledTime() const;
	inline	nanotime_t			FinishedTime() const;
	inline	bool				IsFinished() const;
	inline	int32				Scheduler() const;
	inline	off_t				Offset() const;
	inline	size_t				Length() const;
	inline	bool				IsWrite() const;
	inline	uint8				Priority() const;
	inline	status_t			Status() const;
	inline	size_t				BytesTransferred() const;


	static inline bool			TimeLess(const IORequest* a,
									const IORequest* b);
	static inline bool			SchedulerTimeLess(const IORequest* a,
									const IORequest* b);
	static inline int			CompareSchedulerTime(const IORequest* a,
									const IORequest* b);
};


class Model::IOScheduler {
public:
								IOScheduler(
									system_profiler_io_scheduler_added* event,
									int32 index);

	inline	int32				ID() const;
	inline	const char*			Name() const;
	inline	int32				Index() const;

private:
			system_profiler_io_scheduler_added* fAddedEvent;
			int32				fIndex;
};


class Model::WaitObject {
public:
								WaitObject(
									const system_profiler_wait_object_info*
										event);
								~WaitObject();

	inline	uint32				Type() const;
	inline	addr_t				Object() const;
	inline	const char*			Name() const;
	inline	addr_t				ReferencedObject();

	inline	int64				Waits() const;
	inline	nanotime_t			TotalWaitTime() const;

			void				AddWait(nanotime_t waitTime);

	static inline int			CompareByTypeObject(const WaitObject* a,
									const WaitObject* b);
	static inline int			CompareWithTypeObject(
									const type_and_object* key,
									const WaitObject* object);

private:
			const system_profiler_wait_object_info* fEvent;

private:
			int64				fWaits;
			nanotime_t			fTotalWaitTime;
};


class Model::WaitObjectGroup {
public:
								WaitObjectGroup(WaitObject* waitObject);
								~WaitObjectGroup();

	inline	uint32				Type() const;
	inline	addr_t				Object() const;
	inline	const char*			Name() const;

			int64				Waits();
			nanotime_t			TotalWaitTime();

	inline	WaitObject*			MostRecentWaitObject() const;

	inline	int32				CountWaitObjects() const;
	inline	Model::WaitObject*	WaitObjectAt(int32 index) const;

	inline	void				AddWaitObject(WaitObject* waitObject);

	static inline int			CompareByTypeObject(const WaitObjectGroup* a,
									const WaitObjectGroup* b);
	static inline int			CompareWithTypeObject(
									const type_and_object* key,
									const WaitObjectGroup* group);

private:
			typedef BObjectList<WaitObject> WaitObjectList;

			void				_ComputeWaits();

private:
			WaitObjectList		fWaitObjects;
			int64				fWaits;
			nanotime_t			fTotalWaitTime;
};


class Model::ThreadWaitObject
	: public SinglyLinkedListLinkImpl<ThreadWaitObject> {
public:
								ThreadWaitObject(WaitObject* waitObject);
								~ThreadWaitObject();

	inline	WaitObject*			GetWaitObject() const;

	inline	uint32				Type() const;
	inline	addr_t				Object() const;
	inline	const char*			Name() const;
	inline	addr_t				ReferencedObject();

	inline	int64				Waits() const;
	inline	nanotime_t			TotalWaitTime() const;

			void				AddWait(nanotime_t waitTime);

private:
			WaitObject*			fWaitObject;
			int64				fWaits;
			nanotime_t			fTotalWaitTime;
};


class Model::ThreadWaitObjectGroup {
public:
								ThreadWaitObjectGroup(
									ThreadWaitObject* threadWaitObject);
								~ThreadWaitObjectGroup();

	inline	uint32				Type() const;
	inline	addr_t				Object() const;
	inline	const char*			Name() const;

	inline	ThreadWaitObject*	MostRecentThreadWaitObject() const;
	inline	WaitObject*			MostRecentWaitObject() const;

	inline	void				AddWaitObject(
									ThreadWaitObject* threadWaitObject);

			bool				GetThreadWaitObjects(
									BObjectList<ThreadWaitObject>& objects);

	static inline int			CompareByTypeObject(
									const ThreadWaitObjectGroup* a,
									const ThreadWaitObjectGroup* b);
	static inline int			CompareWithTypeObject(
									const type_and_object* key,
									const ThreadWaitObjectGroup* group);

private:
			typedef SinglyLinkedList<ThreadWaitObject> ThreadWaitObjectList;

private:
			ThreadWaitObjectList fWaitObjects;
};


class Model::Team {
public:
								Team(const system_profiler_team_added* event,
									nanotime_t time);
								~Team();

	inline	team_id				ID() const;
	inline	const char*			Name() const;

	inline	nanotime_t			CreationTime() const;
	inline	nanotime_t			DeletionTime() const;

			bool				AddThread(Thread* thread);

	inline	void				SetDeletionTime(nanotime_t time);

	static inline int			CompareByID(const Team* a, const Team* b);
	static inline int			CompareWithID(const team_id* id,
									const Team* team);

private:
			typedef BObjectList<Thread> ThreadList;

private:
			const system_profiler_team_added* fCreationEvent;
			nanotime_t			fCreationTime;
			nanotime_t			fDeletionTime;
			ThreadList			fThreads;	// sorted by creation time, ID
};


class Model::Thread {
public:
								Thread(Team* team,
									const system_profiler_thread_added* event,
									nanotime_t time);
								~Thread();

	inline	thread_id			ID() const;
	inline	const char*			Name() const;
	inline	Team*				GetTeam() const;

	inline	int32				Index() const;
	inline	void				SetIndex(int32 index);

	inline	system_profiler_event_header** Events() const;
	inline	size_t				CountEvents() const;
			void				SetEvents(system_profiler_event_header** events,
									size_t eventCount);

	inline	IORequest**			IORequests() const;
	inline	size_t				CountIORequests() const;
			void				SetIORequests(IORequest** requests,
									size_t requestCount);
			size_t				ClosestRequestStartIndex(
									nanotime_t minRequestStartTime) const;
									// Returns the index of the first request
									// with a start time >= minRequestStartTime.
									// minRequestStartTime is absolute, not
									// base time relative.

	inline	nanotime_t			CreationTime() const;
	inline	nanotime_t			DeletionTime() const;

	inline	int64				Runs() const;
	inline	nanotime_t			TotalRunTime() const;
	inline	int64				Reruns() const;
	inline	nanotime_t			TotalRerunTime() const;
	inline	int64				Latencies() const;
	inline	nanotime_t			TotalLatency() const;
	inline	int64				Preemptions() const;
	inline	int64				Waits() const;
	inline	nanotime_t			TotalWaitTime() const;
	inline	nanotime_t			UnspecifiedWaitTime() const;

	inline	int64				IOCount() const;
	inline	nanotime_t			IOTime() const;

			ThreadWaitObjectGroup* ThreadWaitObjectGroupFor(uint32 type,
									addr_t object) const;
	inline	int32				CountThreadWaitObjectGroups() const;
	inline	ThreadWaitObjectGroup* ThreadWaitObjectGroupAt(int32 index) const;

	inline	void				SetDeletionTime(nanotime_t time);

			void				AddRun(nanotime_t runTime);
			void				AddRerun(nanotime_t runTime);
			void				AddLatency(nanotime_t latency);
			void				AddPreemption(nanotime_t runTime);
			void				AddWait(nanotime_t waitTime);
			void				AddUnspecifiedWait(nanotime_t waitTime);

			ThreadWaitObject*	AddThreadWaitObject(WaitObject* waitObject,
									ThreadWaitObjectGroup**
										_threadWaitObjectGroup);

			void				SetIOs(int64 count, nanotime_t time);

	static inline int			CompareByID(const Thread* a, const Thread* b);
	static inline int			CompareWithID(const thread_id* id,
									const Thread* thread);

	static inline int			CompareByCreationTimeID(const Thread* a,
									const Thread* b);
	static inline int			CompareWithCreationTimeID(
									const creation_time_id* key,
									const Thread* thread);

private:
			typedef BObjectList<ThreadWaitObjectGroup>
				ThreadWaitObjectGroupList;

private:
			system_profiler_event_header** fEvents;
			size_t				fEventCount;

			IORequest**			fIORequests;
			size_t				fIORequestCount;

			Team*				fTeam;
			const system_profiler_thread_added* fCreationEvent;
			nanotime_t			fCreationTime;
			nanotime_t			fDeletionTime;

			int64				fRuns;
			nanotime_t			fTotalRunTime;
			nanotime_t			fMinRunTime;
			nanotime_t			fMaxRunTime;

			int64				fLatencies;
			nanotime_t			fTotalLatency;
			nanotime_t			fMinLatency;
			nanotime_t			fMaxLatency;

			int64				fReruns;
			nanotime_t			fTotalRerunTime;
			nanotime_t			fMinRerunTime;
			nanotime_t			fMaxRerunTime;

			int64				fWaits;
			nanotime_t			fTotalWaitTime;
			nanotime_t			fUnspecifiedWaitTime;

			int64				fIOCount;
			nanotime_t			fIOTime;

			int64				fPreemptions;

			int32				fIndex;

			ThreadWaitObjectGroupList fWaitObjectGroups;
};


struct Model::CompactThreadSchedulingState {
			nanotime_t			lastTime;
			Model::Thread*		thread;
			ThreadWaitObject*	waitObject;
			ThreadState			state;
			uint8				priority;

public:
			thread_id			ID() const	{ return thread->ID(); }

	inline	CompactThreadSchedulingState& operator=(
									const CompactThreadSchedulingState& other);
};


struct Model::ThreadSchedulingState : CompactThreadSchedulingState {
			ThreadSchedulingState* next;

public:
	inline						ThreadSchedulingState(
									const CompactThreadSchedulingState& other);
	inline						ThreadSchedulingState(Thread* thread);
};


struct Model::ThreadSchedulingStateDefinition {
	typedef thread_id				KeyType;
	typedef	ThreadSchedulingState	ValueType;

	size_t HashKey(thread_id key) const
		{ return (size_t)key; }

	size_t Hash(const ThreadSchedulingState* value) const
		{ return (size_t)value->ID(); }

	bool Compare(thread_id key, const ThreadSchedulingState* value) const
		{ return key == value->ID(); }

	ThreadSchedulingState*& GetLink(ThreadSchedulingState* value) const
		{ return value->next; }
};


class Model::SchedulingState {
public:
	inline						SchedulingState();
	virtual						~SchedulingState();

			status_t			Init();
			status_t			Init(const CompactSchedulingState* state);
			void				Clear();

	inline	nanotime_t			LastEventTime() const { return fLastEventTime; }
	inline	void				SetLastEventTime(nanotime_t time);

	inline	ThreadSchedulingState* LookupThread(thread_id threadID) const;
	inline	void				InsertThread(ThreadSchedulingState* thread);
	inline	void				RemoveThread(ThreadSchedulingState* thread);
	inline	const ThreadSchedulingStateTable& ThreadStates() const;

protected:
	virtual	void				DeleteThread(ThreadSchedulingState* thread);

private:
			nanotime_t			fLastEventTime;
			ThreadSchedulingStateTable fThreadStates;
};


class Model::CompactSchedulingState {
public:
	static	CompactSchedulingState* Create(const SchedulingState& state,
									off_t eventOffset);
			void				Delete();

	inline	off_t				EventOffset() const;
	inline	nanotime_t			LastEventTime() const;

	inline	int32				CountThreadsStates() const;
	inline	const CompactThreadSchedulingState* ThreadStateAt(int32 index)
									const;

private:
	friend class BObjectList<CompactSchedulingState>;
		// work-around for our private destructor

private:
								CompactSchedulingState();
	inline						~CompactSchedulingState() {}

private:
			nanotime_t			fLastEventTime;
			off_t				fEventOffset;
			int32				fThreadCount;
			CompactThreadSchedulingState fThreadStates[0];
};


// #pragma mark - Model


const char*
Model::DataSourceName() const
{
	return fDataSourceName.String();
}


void*
Model::EventData() const
{
	return fEventData;
}


size_t
Model::EventDataSize() const
{
	return fEventDataSize;
}


system_profiler_event_header**
Model::Events() const
{
	return fEvents;
}


size_t
Model::CountEvents() const
{
	return fEventCount;
}


nanotime_t
Model::BaseTime() const
{
	return fBaseTime;
}


nanotime_t
Model::LastEventTime() const
{
	return fLastEventTime;
}


nanotime_t
Model::IdleTime() const
{
	return fIdleTime;
}


int32
Model::CountCPUs() const
{
	return fCPUCount;
}


Model::CPU*
Model::CPUAt(int32 index) const
{
	return fCPUs.ItemAt(index);
}


// #pragma mark - CPU


nanotime_t
Model::CPU::IdleTime() const
{
	return fIdleTime;
}


// #pragma mark - IOOperation


nanotime_t
Model::IOOperation::StartedTime() const
{
	return startedEvent->time;
}


nanotime_t
Model::IOOperation::FinishedTime() const
{
	return finishedEvent != NULL ? finishedEvent->time : 0;
}


bool
Model::IOOperation::IsFinished() const
{
	return finishedEvent != NULL;
}


off_t
Model::IOOperation::Offset() const
{
	return startedEvent->offset;
}


size_t
Model::IOOperation::Length() const
{
	return startedEvent->length;
}


bool
Model::IOOperation::IsWrite() const
{
	return startedEvent->write;
}


status_t
Model::IOOperation::Status() const
{
	return finishedEvent != NULL ? finishedEvent->status : B_OK;
}


size_t
Model::IOOperation::BytesTransferred() const
{
	return finishedEvent != NULL ? finishedEvent->transferred : 0;
}


/*static*/ int
Model::IOOperation::CompareByTime(const IOOperation* a, const IOOperation* b)
{
	nanotime_t timeA = a->startedEvent->time;
	nanotime_t timeB = b->startedEvent->time;

	if (timeA < timeB)
		return -1;
	return timeA == timeB ? 0 : 1;
}


// #pragma mark - IORequest


nanotime_t
Model::IORequest::ScheduledTime() const
{
	return scheduledEvent->time;
}


nanotime_t
Model::IORequest::FinishedTime() const
{
	return finishedEvent != NULL ? finishedEvent->time : 0;
}


bool
Model::IORequest::IsFinished() const
{
	return finishedEvent != NULL;
}


int32
Model::IORequest::Scheduler() const
{
	return scheduledEvent->scheduler;
}


off_t
Model::IORequest::Offset() const
{
	return scheduledEvent->offset;
}


size_t
Model::IORequest::Length() const
{
	return scheduledEvent->length;
}


bool
Model::IORequest::IsWrite() const
{
	return scheduledEvent->write;
}


uint8
Model::IORequest::Priority() const
{
	return scheduledEvent->priority;
}


status_t
Model::IORequest::Status() const
{
	return finishedEvent != NULL ? finishedEvent->status : B_OK;
}


size_t
Model::IORequest::BytesTransferred() const
{
	return finishedEvent != NULL ? finishedEvent->transferred : 0;
}


/*static*/ bool
Model::IORequest::TimeLess(const IORequest* a, const IORequest* b)
{
	return a->scheduledEvent->time < b->scheduledEvent->time;
}


/*static*/ bool
Model::IORequest::SchedulerTimeLess(const IORequest* a, const IORequest* b)
{
	int32 cmp = a->scheduledEvent->scheduler - b->scheduledEvent->scheduler;
	if (cmp != 0)
		return cmp < 0;

	return a->scheduledEvent->time < b->scheduledEvent->time;
}


/*static*/ int
Model::IORequest::CompareSchedulerTime(const IORequest* a, const IORequest* b)
{
	int32 cmp = a->scheduledEvent->scheduler - b->scheduledEvent->scheduler;
	if (cmp != 0)
		return cmp < 0;

	nanotime_t timeCmp = a->scheduledEvent->time - b->scheduledEvent->time;
	if (timeCmp == 0)
		return 0;
	return timeCmp < 0 ? -1 : 1;
}


// #pragma mark - IOScheduler


int32
Model::IOScheduler::ID() const
{
	return fAddedEvent->scheduler;
}


const char*
Model::IOScheduler::Name() const
{
	return fAddedEvent->name;
}


int32
Model::IOScheduler::Index() const
{
	return fIndex;
}


// #pragma mark - WaitObject


uint32
Model::WaitObject::Type() const
{
	return fEvent->type;
}


addr_t
Model::WaitObject::Object() const
{
	return fEvent->object;
}


const char*
Model::WaitObject::Name() const
{
	return fEvent->name;
}


addr_t
Model::WaitObject::ReferencedObject()
{
	return fEvent->referenced_object;
}


int64
Model::WaitObject::Waits() const
{
	return fWaits;
}


nanotime_t
Model::WaitObject::TotalWaitTime() const
{
	return fTotalWaitTime;
}


/*static*/ int
Model::WaitObject::CompareByTypeObject(const WaitObject* a, const WaitObject* b)
{
	type_and_object key;
	key.type = a->Type();
	key.object = a->Object();

	return CompareWithTypeObject(&key, b);
}


/*static*/ int
Model::WaitObject::CompareWithTypeObject(const type_and_object* key,
	const WaitObject* object)
{
	if (key->type == object->Type()) {
		if (key->object == object->Object())
			return 0;
		return key->object < object->Object() ? -1 : 1;
	}

	return key->type < object->Type() ? -1 : 1;
}


// #pragma mark - WaitObjectGroup


uint32
Model::WaitObjectGroup::Type() const
{
	return MostRecentWaitObject()->Type();
}


addr_t
Model::WaitObjectGroup::Object() const
{
	return MostRecentWaitObject()->Object();
}


const char*
Model::WaitObjectGroup::Name() const
{
	return MostRecentWaitObject()->Name();
}


Model::WaitObject*
Model::WaitObjectGroup::MostRecentWaitObject() const
{
	return fWaitObjects.ItemAt(fWaitObjects.CountItems() - 1);
}


int32
Model::WaitObjectGroup::CountWaitObjects() const
{
	return fWaitObjects.CountItems();
}


Model::WaitObject*
Model::WaitObjectGroup::WaitObjectAt(int32 index) const
{
	return fWaitObjects.ItemAt(index);
}


void
Model::WaitObjectGroup::AddWaitObject(WaitObject* waitObject)
{
	fWaitObjects.AddItem(waitObject);
}


/*static*/ int
Model::WaitObjectGroup::CompareByTypeObject(
	const WaitObjectGroup* a, const WaitObjectGroup* b)
{
	return WaitObject::CompareByTypeObject(a->MostRecentWaitObject(),
		b->MostRecentWaitObject());
}


/*static*/ int
Model::WaitObjectGroup::CompareWithTypeObject(const type_and_object* key,
	const WaitObjectGroup* group)
{
	return WaitObject::CompareWithTypeObject(key,
		group->MostRecentWaitObject());
}


// #pragma mark - ThreadWaitObject


Model::WaitObject*
Model::ThreadWaitObject::GetWaitObject() const
{
	return fWaitObject;
}


uint32
Model::ThreadWaitObject::Type() const
{
	return fWaitObject->Type();
}


addr_t
Model::ThreadWaitObject::Object() const
{
	return fWaitObject->Object();
}


const char*
Model::ThreadWaitObject::Name() const
{
	return fWaitObject->Name();
}


addr_t
Model::ThreadWaitObject::ReferencedObject()
{
	return fWaitObject->ReferencedObject();
}


int64
Model::ThreadWaitObject::Waits() const
{
	return fWaits;
}


nanotime_t
Model::ThreadWaitObject::TotalWaitTime() const
{
	return fTotalWaitTime;
}


// #pragma mark - ThreadWaitObjectGroup


uint32
Model::ThreadWaitObjectGroup::Type() const
{
	return MostRecentThreadWaitObject()->Type();
}


addr_t
Model::ThreadWaitObjectGroup::Object() const
{
	return MostRecentThreadWaitObject()->Object();
}


const char*
Model::ThreadWaitObjectGroup::Name() const
{
	return MostRecentThreadWaitObject()->Name();
}


Model::ThreadWaitObject*
Model::ThreadWaitObjectGroup::MostRecentThreadWaitObject() const
{
	return fWaitObjects.Head();
}


Model::WaitObject*
Model::ThreadWaitObjectGroup::MostRecentWaitObject() const
{
	return MostRecentThreadWaitObject()->GetWaitObject();
}


void
Model::ThreadWaitObjectGroup::AddWaitObject(ThreadWaitObject* threadWaitObject)
{
	fWaitObjects.Add(threadWaitObject);
}


/*static*/ int
Model::ThreadWaitObjectGroup::CompareByTypeObject(
	const ThreadWaitObjectGroup* a, const ThreadWaitObjectGroup* b)
{
	return WaitObject::CompareByTypeObject(a->MostRecentWaitObject(),
		b->MostRecentWaitObject());
}


/*static*/ int
Model::ThreadWaitObjectGroup::CompareWithTypeObject(const type_and_object* key,
	const ThreadWaitObjectGroup* group)
{
	return WaitObject::CompareWithTypeObject(key,
		group->MostRecentWaitObject());
}


// #pragma mark - Team


team_id
Model::Team::ID() const
{
	return fCreationEvent->team;
}


const char*
Model::Team::Name() const
{
	return fCreationEvent->name;
		// TODO: We should probably return the last exec name!
}


nanotime_t
Model::Team::CreationTime() const
{
	return fCreationTime;
}


nanotime_t
Model::Team::DeletionTime() const
{
	return fDeletionTime;
}


void
Model::Team::SetDeletionTime(nanotime_t time)
{
	fDeletionTime = time;
}


/*static*/ int
Model::Team::CompareByID(const Team* a, const Team* b)
{
	return a->ID() - b->ID();
}


/*static*/ int
Model::Team::CompareWithID(const team_id* id, const Team* team)
{
	return *id - team->ID();
}


// #pragma mark - Thread


thread_id
Model::Thread::ID() const
{
	return fCreationEvent->thread;
}


const char*
Model::Thread::Name() const
{
	return fCreationEvent->name;
}


Model::Team*
Model::Thread::GetTeam() const
{
	return fTeam;
}


nanotime_t
Model::Thread::CreationTime() const
{
	return fCreationTime;
}


nanotime_t
Model::Thread::DeletionTime() const
{
	return fDeletionTime;
}


int32
Model::Thread::Index() const
{
	return fIndex;
}


void
Model::Thread::SetIndex(int32 index)
{
	fIndex = index;
}


system_profiler_event_header**
Model::Thread::Events() const
{
	return fEvents;
}


size_t
Model::Thread::CountEvents() const
{
	return fEventCount;
}


Model::IORequest**
Model::Thread::IORequests() const
{
	return fIORequests;
}


size_t
Model::Thread::CountIORequests() const
{
	return fIORequestCount;
}


int64
Model::Thread::Runs() const
{
	return fRuns;
}


nanotime_t
Model::Thread::TotalRunTime() const
{
	return fTotalRunTime;
}


int64
Model::Thread::Reruns() const
{
	return fReruns;
}


nanotime_t
Model::Thread::TotalRerunTime() const
{
	return fTotalRerunTime;
}


int64
Model::Thread::Latencies() const
{
	return fLatencies;
}


nanotime_t
Model::Thread::TotalLatency() const
{
	return fTotalLatency;
}


int64
Model::Thread::Preemptions() const
{
	return fPreemptions;
}


int64
Model::Thread::Waits() const
{
	return fWaits;
}


nanotime_t
Model::Thread::TotalWaitTime() const
{
	return fTotalWaitTime;
}


nanotime_t
Model::Thread::UnspecifiedWaitTime() const
{
	return fUnspecifiedWaitTime;
}


int64
Model::Thread::IOCount() const
{
	return fIOCount;
}


nanotime_t
Model::Thread::IOTime() const
{
	return fIOTime;
}


int32
Model::Thread::CountThreadWaitObjectGroups() const
{
	return fWaitObjectGroups.CountItems();
}


Model::ThreadWaitObjectGroup*
Model::Thread::ThreadWaitObjectGroupAt(int32 index) const
{
	return fWaitObjectGroups.ItemAt(index);
}


void
Model::Thread::SetDeletionTime(nanotime_t time)
{
	fDeletionTime = time;
}


/*static*/ int
Model::Thread::CompareByID(const Thread* a, const Thread* b)
{
	return a->ID() - b->ID();
}


/*static*/ int
Model::Thread::CompareWithID(const thread_id* id, const Thread* thread)
{
	return *id - thread->ID();
}


/*static*/ int
Model::Thread::CompareByCreationTimeID(const Thread* a, const Thread* b)
{
	creation_time_id key;
	key.time = a->fCreationTime;
	key.id = a->ID();
	return CompareWithCreationTimeID(&key, b);
}


/*static*/ int
Model::Thread::CompareWithCreationTimeID(const creation_time_id* key,
	const Thread* thread)
{
	nanotime_t cmp = key->time - thread->fCreationTime;
	if (cmp == 0)
		return key->id - thread->ID();
	return cmp < 0 ? -1 : 1;
}


// #pragma mark - CompactThreadSchedulingState


Model::CompactThreadSchedulingState&
Model::CompactThreadSchedulingState::operator=(
	const CompactThreadSchedulingState& other)
{
	lastTime = other.lastTime;
	thread = other.thread;
	waitObject = other.waitObject;
	state = other.state;
	priority = other.priority;
	return *this;
}


// #pragma mark - ThreadSchedulingState


Model::ThreadSchedulingState::ThreadSchedulingState(
	const CompactThreadSchedulingState& other)
{
	this->CompactThreadSchedulingState::operator=(other);
}


Model::ThreadSchedulingState::ThreadSchedulingState(Thread* thread)
{
	lastTime = 0;
	this->thread = thread;
	waitObject = NULL;
	state = UNKNOWN;
}


// #pragma mark - SchedulingState


Model::SchedulingState::SchedulingState()
	:
	fLastEventTime(-1)
{
}


void
Model::SchedulingState::SetLastEventTime(nanotime_t time)
{
	fLastEventTime = time;
}


Model::ThreadSchedulingState*
Model::SchedulingState::LookupThread(thread_id threadID) const
{
	return fThreadStates.Lookup(threadID);
}


void
Model::SchedulingState::InsertThread(ThreadSchedulingState* thread)
{
	fThreadStates.Insert(thread);
}


void
Model::SchedulingState::RemoveThread(ThreadSchedulingState* thread)
{
	fThreadStates.Remove(thread);
}


const Model::ThreadSchedulingStateTable&
Model::SchedulingState::ThreadStates() const
{
	return fThreadStates;
}


// #pragma mark - CompactSchedulingState


off_t
Model::CompactSchedulingState::EventOffset() const
{
	return fEventOffset;
}


nanotime_t
Model::CompactSchedulingState::LastEventTime() const
{
	return fLastEventTime;
}


int32
Model::CompactSchedulingState::CountThreadsStates() const
{
	return fThreadCount;
}


const Model::CompactThreadSchedulingState*
Model::CompactSchedulingState::ThreadStateAt(int32 index) const
{
	return index >= 0 && index < fThreadCount ? &fThreadStates[index] : NULL;
}


#endif	// MODEL_H
