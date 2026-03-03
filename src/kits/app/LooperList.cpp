/*
 * Copyright 2001-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */


//! Maintains a global list of all loopers in a given team.


#include "LooperList.h"

#include <Looper.h>

#include <algorithm>
#include <string.h>


using std::vector;

namespace BPrivate {

BLooperList gLooperList;


BLooperList::BLooperList()
{
	rw_lock_init(&fLock, "BLooperList rwlock");
}


bool
BLooperList::Lock()
{
	return rw_lock_write_lock(&fLock) == B_OK;
}


void
BLooperList::Unlock()
{
	rw_lock_write_unlock(&fLock);
}


bool
BLooperList::IsLocked()
{
	return fLock.holder == find_thread(NULL);
}


void
BLooperList::AddLooper(BLooper* looper)
{
	WriteLocker locker(fLock);
	AssertLocked();
	if (!IsLooperValid(looper)) {
		LooperDataIterator i
			= find_if(fData.begin(), fData.end(), EmptySlotPred);
		if (i == fData.end()) {
			fData.push_back(LooperData(looper));
			looper->Lock();
		} else {
			i->looper = looper;
			looper->Lock();
		}
	}
}


bool
BLooperList::IsLooperValid(const BLooper* looper)
{
	ReadLocker locker(fLock);
	return find_if(fData.begin(), fData.end(),
		FindLooperPred(looper)) != fData.end();
}


bool
BLooperList::RemoveLooper(BLooper* looper)
{
	WriteLocker locker(fLock);
	AssertLocked();

	LooperDataIterator i = find_if(fData.begin(), fData.end(),
		FindLooperPred(looper));
	if (i != fData.end()) {
		i->looper = NULL;
		return true;
	}

	return false;
}


void
BLooperList::GetLooperList(BList* list)
{
	ReadLocker locker(fLock);

	for (uint32 i = 0; i < fData.size(); ++i) {
		if (fData[i].looper)
			list->AddItem(fData[i].looper);
	}
}


int32
BLooperList::CountLoopers()
{
	ReadLocker locker(fLock);
	return (int32)fData.size();
}


BLooper*
BLooperList::LooperAt(int32 index)
{
	ReadLocker locker(fLock);

	BLooper* looper = NULL;
	if (index < (int32)fData.size())
		looper = fData[(uint32)index].looper;

	return looper;
}


BLooper*
BLooperList::LooperForThread(thread_id thread)
{
	ReadLocker locker(fLock);

	BLooper* looper = NULL;
	LooperDataIterator i
		= find_if(fData.begin(), fData.end(), FindThreadPred(thread));
	if (i != fData.end())
		looper = i->looper;

	return looper;
}


BLooper*
BLooperList::LooperForName(const char* name)
{
	ReadLocker locker(fLock);

	BLooper* looper = NULL;
	LooperDataIterator i
		= find_if(fData.begin(), fData.end(), FindNamePred(name));
	if (i != fData.end())
		looper = i->looper;

	return looper;
}


BLooper*
BLooperList::LooperForPort(port_id port)
{
	ReadLocker locker(fLock);

	BLooper* looper = NULL;
	LooperDataIterator i
		= find_if(fData.begin(), fData.end(), FindPortPred(port));
	if (i != fData.end())
		looper = i->looper;

	return looper;
}


void
BLooperList::InitAfterFork()
{
	rw_lock_init(&fLock, "BLooperList lock");
	fData.clear();
}


bool
BLooperList::EmptySlotPred(LooperData& data)
{
	return data.looper == NULL;
}


void
BLooperList::AssertLocked()
{
	if (!IsLocked())
		debugger("looperlist is not locked; proceed at great risk!");
}


//	#pragma mark - BLooperList::LooperData


BLooperList::LooperData::LooperData()
	:
	looper(NULL)
{
}


BLooperList::LooperData::LooperData(BLooper* looper)
	:
	looper(looper)
{
}


BLooperList::LooperData::LooperData(const LooperData& other)
{
	*this = other;
}


BLooperList::LooperData&
BLooperList::LooperData::operator=(const LooperData& other)
{
	if (this != &other)
		looper = other.looper;

	return *this;
}


bool
BLooperList::FindLooperPred::operator()(BLooperList::LooperData& data)
{
	return data.looper && looper == data.looper;
}


bool
BLooperList::FindThreadPred::operator()(LooperData& data)
{
	return data.looper && thread == data.looper->Thread();
}


bool
BLooperList::FindNamePred::operator()(LooperData& data)
{
	return data.looper && !strcmp(name, data.looper->Name());
}


bool
BLooperList::FindPortPred::operator()(LooperData& data)
{
	return data.looper && port == _get_looper_port_(data.looper);
}

}	// namespace BPrivate

