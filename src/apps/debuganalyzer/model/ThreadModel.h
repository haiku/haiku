/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef THREAD_MODEL_H
#define THREAD_MODEL_H

#include <string.h>

#include "Model.h"


class ThreadModel {
public:
			struct type_and_name;
			class WaitObjectGroup;

public:
								ThreadModel(Model* model,
									Model::Thread* thread);
								~ThreadModel();

			Model*				GetModel() const	{ return fModel; }
			Model::Thread*		GetThread() const	{ return fThread; }

			WaitObjectGroup*	AddWaitObjectGroup(
									const BObjectList<Model::ThreadWaitObject>&
										waitObjects,
									int32 start, int32 end);
	inline	int32				CountWaitObjectGroups() const;
	inline	WaitObjectGroup*	WaitObjectGroupAt(int32 index) const;

private:
			typedef BObjectList<WaitObjectGroup> WaitObjectGroupList;

private:
			Model*				fModel;
			Model::Thread*		fThread;
			WaitObjectGroupList	fWaitObjectGroups;
};


struct ThreadModel::type_and_name {
	uint32		type;
	const char*	name;
};


class ThreadModel::WaitObjectGroup {
public:
								WaitObjectGroup(
									Model::ThreadWaitObject** waitObjects,
									int32 count);
								~WaitObjectGroup();

	inline	uint32				Type() const;
	inline	const char*			Name() const;

	inline	int64				Waits() const;
	inline	bigtime_t			TotalWaitTime() const;

	inline	int32				CountWaitObjects() const;
	inline	Model::ThreadWaitObject* WaitObjectAt(int32 index) const;

	static inline int			CompareByTypeName(const WaitObjectGroup* a,
									const WaitObjectGroup* b);
	static inline int			CompareWithTypeName(
									const type_and_name* key,
									const WaitObjectGroup* group);

private:
			Model::ThreadWaitObject** fWaitObjects;
			int32				fCount;
			int64				fWaits;
			bigtime_t			fTotalWaitTime;
};


// #pragma mark - ThreadModel


int32
ThreadModel::CountWaitObjectGroups() const
{
	return fWaitObjectGroups.CountItems();
}


ThreadModel::WaitObjectGroup*
ThreadModel::WaitObjectGroupAt(int32 index) const
{
	return fWaitObjectGroups.ItemAt(index);
}


// #pragma mark - WaitObjectGroup


uint32
ThreadModel::WaitObjectGroup::Type() const
{
	return fWaitObjects[0]->Type();
}


const char*
ThreadModel::WaitObjectGroup::Name() const
{
	return fWaitObjects[0]->Name();
}


int64
ThreadModel::WaitObjectGroup::Waits() const
{
	return fWaits;
}


bigtime_t
ThreadModel::WaitObjectGroup::TotalWaitTime() const
{
	return fTotalWaitTime;
}


int32
ThreadModel::WaitObjectGroup::CountWaitObjects() const
{
	return fCount;
}


Model::ThreadWaitObject*
ThreadModel::WaitObjectGroup::WaitObjectAt(int32 index) const
{
	return index >= 0 && index < fCount ? fWaitObjects[index] : NULL;
}


/*static*/ int
ThreadModel::WaitObjectGroup::CompareByTypeName(const WaitObjectGroup* a,
	const WaitObjectGroup* b)
{
	type_and_name key;
	key.type = a->Type();
	key.name = a->Name();
	return CompareWithTypeName(&key, b);
}


/*static*/ int
ThreadModel::WaitObjectGroup::CompareWithTypeName(const type_and_name* key,
	const WaitObjectGroup* group)
{
	if (key->type != group->Type())
		return key->type < group->Type() ? -1 : 1;

	return strcmp(key->name, group->Name());
}


#endif	// THREAD_MODEL_h
