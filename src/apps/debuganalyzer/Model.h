/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <ObjectList.h>
#include <OS.h>

#include <system_profiler_defs.h>
#include <util/SinglyLinkedList.h>


class Model {
public:
			struct creation_time_id;
			struct type_and_object;
			class WaitObjectGroup;
			class WaitObject;
			class ThreadWaitObject;
			class ThreadWaitObjectGroup;
			class Team;
			class Thread;

public:
								Model(void* eventData, size_t eventDataSize);
								~Model();

			int32				CountTeams() const;
			Team*				TeamAt(int32 index) const;
			Team*				TeamByID(team_id id) const;
			Team*				AddTeam(
									const system_profiler_team_added* event,
									bigtime_t time);

			int32				CountThreads() const;
			Thread*				ThreadAt(int32 index) const;
			Thread*				ThreadByID(thread_id id) const;
			Thread*				AddThread(
									const system_profiler_thread_added* event,
									bigtime_t time);

			WaitObject*			AddWaitObject(
									const system_profiler_wait_object_info*
										event,
									WaitObjectGroup** _waitObjectGroup);
			WaitObjectGroup*	WaitObjectGroupFor(uint32 type,
									addr_t object) const;

			ThreadWaitObject*	AddThreadWaitObject(thread_id threadID,
									WaitObject* waitObject,
									ThreadWaitObjectGroup**
										_threadWaitObjectGroup);
			ThreadWaitObjectGroup* ThreadWaitObjectGroupFor(
									thread_id threadID, uint32 type,
									addr_t object) const;

private:
			typedef BObjectList<Team> TeamList;
			typedef BObjectList<Thread> ThreadList;
			typedef BObjectList<WaitObjectGroup> WaitObjectGroupList;

private:
			void*				fEventData;
			size_t				fEventDataSize;
			TeamList			fTeams;		// sorted by ID
			ThreadList			fThreads;	// sorted by ID
			WaitObjectGroupList	fWaitObjectGroups;
};


struct Model::creation_time_id {
	bigtime_t	time;
	thread_id	id;
};


struct Model::type_and_object {
	uint32		type;
	addr_t		object;
};


class Model::WaitObject : public SinglyLinkedListLinkImpl<WaitObject> {
public:
								WaitObject(
									const system_profiler_wait_object_info*
										event);
								~WaitObject();

	inline	uint32				Type() const;
	inline	addr_t				Object() const;
	inline	const char*			Name() const;
	inline	addr_t				ReferencedObject();

	static inline int			CompareByTypeObject(const WaitObject* a,
									const WaitObject* b);
	static inline int			CompareWithTypeObject(
									const type_and_object* key,
									const WaitObject* object);

private:
			const system_profiler_wait_object_info* fEvent;
};


class Model::WaitObjectGroup {
public:
								WaitObjectGroup(WaitObject* waitObject);
								~WaitObjectGroup();

	inline	uint32				Type() const;
	inline	addr_t				Object() const;
	inline	const char*			Name() const;

	inline	WaitObject*			MostRecentWaitObject() const;

	inline	void				AddWaitObject(WaitObject* waitObject);

	static inline int			CompareByTypeObject(const WaitObjectGroup* a,
									const WaitObjectGroup* b);
	static inline int			CompareWithTypeObject(
									const type_and_object* key,
									const WaitObjectGroup* group);

private:
			typedef SinglyLinkedList<WaitObject> WaitObjectList;

private:
			WaitObjectList		fWaitObjects;
};


class Model::ThreadWaitObject
	: public SinglyLinkedListLinkImpl<ThreadWaitObject> {
public:
								ThreadWaitObject(WaitObject* waitObject);
								~ThreadWaitObject();

	inline	WaitObject*			GetWaitObject() const;

			void				AddWait(bigtime_t waitTime);

private:
			WaitObject*			fWaitObject;
			int64				fWaits;
			bigtime_t			fTotalWaitTime;
};


class Model::ThreadWaitObjectGroup {
public:
								ThreadWaitObjectGroup(
									ThreadWaitObject* threadWaitObject);
								~ThreadWaitObjectGroup();

	inline	ThreadWaitObject*	MostRecentThreadWaitObject() const;
	inline	WaitObject*			MostRecentWaitObject() const;

	inline	void				AddWaitObject(
									ThreadWaitObject* threadWaitObject);

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
									bigtime_t time);
								~Team();

	inline	team_id				ID() const;

	inline	bigtime_t			CreationTime() const;
	inline	bigtime_t			DeletionTime() const;

			bool				AddThread(Thread* thread);

	inline	void				SetDeletionTime(bigtime_t time);

	static inline int			CompareByID(const Team* a, const Team* b);
	static inline int			CompareWithID(const team_id* id,
									const Team* team);

private:
			typedef BObjectList<Thread> ThreadList;

private:
			const system_profiler_team_added* fCreationEvent;
			bigtime_t			fCreationTime;
			bigtime_t			fDeletionTime;
			ThreadList			fThreads;	// sorted by creation time, ID
};


class Model::Thread {
public:
								Thread(Team* team,
									const system_profiler_thread_added* event,
									bigtime_t time);
								~Thread();

	inline	thread_id			ID() const;
	inline	const char*			Name() const;

	inline	bigtime_t			CreationTime() const;
	inline	bigtime_t			DeletionTime() const;

	inline	int64				Runs() const;
	inline	bigtime_t			TotalRunTime() const;
	inline	int64				Reruns() const;
	inline	bigtime_t			TotalRerunTime() const;
	inline	int64				Latencies() const;
	inline	bigtime_t			TotalLatency() const;
	inline	int64				Preemptions() const;

			ThreadWaitObjectGroup* ThreadWaitObjectGroupFor(uint32 type,
									addr_t object) const;

	inline	void				SetDeletionTime(bigtime_t time);

			void				AddRun(bigtime_t runTime);
			void				AddRerun(bigtime_t runTime);
			void				AddLatency(bigtime_t latency);
			void				AddPreemption(bigtime_t runTime);
			void				AddUnspecifiedWait(bigtime_t waitTime);

			ThreadWaitObject*	AddThreadWaitObject(WaitObject* waitObject,
									ThreadWaitObjectGroup**
										_threadWaitObjectGroup);

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
			Team*				fTeam;
			const system_profiler_thread_added* fCreationEvent;
			bigtime_t			fCreationTime;
			bigtime_t			fDeletionTime;

			int64				fRuns;
			bigtime_t			fTotalRunTime;
			bigtime_t			fMinRunTime;
			bigtime_t			fMaxRunTime;

			int64				fLatencies;
			bigtime_t			fTotalLatency;
			bigtime_t			fMinLatency;
			bigtime_t			fMaxLatency;

			int64				fReruns;
			bigtime_t			fTotalRerunTime;
			bigtime_t			fMinRerunTime;
			bigtime_t			fMaxRerunTime;

			bigtime_t			fUnspecifiedWaitTime;

			int64				fPreemptions;

			ThreadWaitObjectGroupList fWaitObjectGroups;
};


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
	return fWaitObjects.Head();
}


void
Model::WaitObjectGroup::AddWaitObject(WaitObject* waitObject)
{
	fWaitObjects.Add(waitObject);
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


// #pragma mark - ThreadWaitObjectGroup


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


bigtime_t
Model::Team::CreationTime() const
{
	return fCreationTime;
}


bigtime_t
Model::Team::DeletionTime() const
{
	return fDeletionTime;
}


void
Model::Team::SetDeletionTime(bigtime_t time)
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


bigtime_t
Model::Thread::CreationTime() const
{
	return fCreationTime;
}


bigtime_t
Model::Thread::DeletionTime() const
{
	return fDeletionTime;
}


int64
Model::Thread::Runs() const
{
	return fRuns;
}


bigtime_t
Model::Thread::TotalRunTime() const
{
	return fTotalRunTime;
}


int64
Model::Thread::Reruns() const
{
	return fReruns;
}


bigtime_t
Model::Thread::TotalRerunTime() const
{
	return fTotalRerunTime;
}


int64
Model::Thread::Latencies() const
{
	return fLatencies;
}


bigtime_t
Model::Thread::TotalLatency() const
{
	return fTotalLatency;
}


int64
Model::Thread::Preemptions() const
{
	return fPreemptions;
}


void
Model::Thread::SetDeletionTime(bigtime_t time)
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
	bigtime_t cmp = key->time - thread->fCreationTime;
	if (cmp == 0)
		return key->id - thread->ID();
	return cmp < 0 ? -1 : 1;
}


#endif	// MODEL_H
