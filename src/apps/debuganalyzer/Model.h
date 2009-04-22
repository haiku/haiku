/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <ObjectList.h>
#include <OS.h>

#include <system_profiler_defs.h>


class Model {
public:
			struct creation_time_id;
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

private:
			typedef BObjectList<Team> TeamList;
			typedef BObjectList<Thread> ThreadList;

private:
			void*				fEventData;
			size_t				fEventDataSize;
			TeamList			fTeams;		// sorted by ID
			ThreadList			fThreads;	// sorted by ID
};


struct Model::creation_time_id {
	bigtime_t	time;
	thread_id	id;
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

	inline	void				SetDeletionTime(bigtime_t time);

	static inline int			CompareByID(const Thread* a, const Thread* b);
	static inline int			CompareWithID(const thread_id* id,
									const Thread* thread);

	static inline int			CompareByCreationTimeID(const Thread* a,
									const Thread* b);
	static inline int			CompareWithCreationTimeID(
									const creation_time_id* key,
									const Thread* thread);

private:
			Team*				fTeam;
			const system_profiler_thread_added* fCreationEvent;
			bigtime_t			fCreationTime;
			bigtime_t			fDeletionTime;
};


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
