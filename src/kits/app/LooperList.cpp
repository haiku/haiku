/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */

//! Maintains a global list of all loopers in a given team.


#include "LooperList.h"

#include <Autolock.h>
#include <Looper.h>

#include <algorithm>


using std::vector;

namespace BPrivate {

BLooperList gLooperList;

typedef vector<BLooperList::LooperData>::iterator LooperDataIterator;


BLooperList::BLooperList()
	:
	fLock("BLooperList lock"),
	fLooperID(0)
{
}


bool
BLooperList::Lock()
{
	return fLock.Lock();
}


void
BLooperList::Unlock()
{
	fLock.Unlock();
}


bool
BLooperList::IsLocked()
{
	return fLock.IsLocked();
}


void
BLooperList::AddLooper(BLooper* looper)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	if (!IsLooperValid(looper)) {
		LooperDataIterator i = find_if(fData.begin(), fData.end(), EmptySlotPred);
		if (i == fData.end()) {
			fData.push_back(LooperData(looper, ++fLooperID));
			looper->fLooperID = fLooperID;
			looper->Lock();
		} else {
			i->looper = looper;
			i->id = ++fLooperID;
			looper->fLooperID = fLooperID;
			looper->Lock();
		}
	}
}


bool
BLooperList::IsLooperValid(const BLooper* looper)
{
	BAutolock Listlock(fLock);
	AssertLocked();

	return find_if(fData.begin(), fData.end(),
		FindLooperPred(looper)) != fData.end();
}


bool
BLooperList::RemoveLooper(BLooper* looper)
{
	BAutolock Listlock(fLock);
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
	BAutolock Listlock(fLock);
	AssertLocked();

	for (uint32 i = 0; i < fData.size(); ++i) {
		if (fData[i].looper)
			list->AddItem(fData[i].looper);
	}
}


int32
BLooperList::CountLoopers()
{
	BAutolock Listlock(fLock);
	AssertLocked();
	return (int32)fData.size();
}


BLooper*
BLooperList::LooperAt(int32 index)
{
	BAutolock Listlock(fLock);
	AssertLocked();

	BLooper* looper = NULL;
	if (index < (int32)fData.size())
		looper = fData[(uint32)index].looper;

	return looper;
}


BLooper*
BLooperList::LooperForThread(thread_id thread)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	BLooper* looper = NULL;
	LooperDataIterator i = find_if(fData.begin(), fData.end(), FindThreadPred(thread));
	if (i != fData.end())
		looper = i->looper;

	return looper;
}


BLooper*
BLooperList::LooperForName(const char* name)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	BLooper* looper = NULL;
	LooperDataIterator i = find_if(fData.begin(), fData.end(), FindNamePred(name));
	if (i != fData.end())
		looper = i->looper;

	return looper;
}


BLooper*
BLooperList::LooperForPort(port_id port)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	BLooper* looper = NULL;
	LooperDataIterator i = find_if(fData.begin(), fData.end(), FindPortPred(port));
	if (i != fData.end())
		looper = i->looper;

	return looper;
}


bool
BLooperList::EmptySlotPred(LooperData& Data)
{
	return Data.looper == NULL;
}


void
BLooperList::AssertLocked()
{
	if (!IsLocked())
		debugger("looperlist is not locked; proceed at great risk!");
}


//	#pragma mark - BLooperList::LooperData


BLooperList::LooperData::LooperData()
	: looper(NULL), id(0)
{
}


BLooperList::LooperData::LooperData(BLooper* loop, uint32 i)
	: looper(loop), id(i)
{
}


BLooperList::LooperData::LooperData(const LooperData& other)
{
	*this = other;
}


BLooperList::LooperData&
BLooperList::LooperData::operator=(const LooperData& other)
{
	if (this != &other) {
		looper = other.looper;
		id = other.id;
	}

	return *this;
}


bool
BLooperList::FindLooperPred::operator()(BLooperList::LooperData& Data)
{
	return Data.looper && looper == Data.looper;
}


bool
BLooperList::FindThreadPred::operator()(LooperData& Data)
{
	return Data.looper && thread == Data.looper->Thread();
}


bool
BLooperList::FindNamePred::operator()(LooperData& Data)
{
	return Data.looper && !strcmp(name, Data.looper->Name());
}


bool
BLooperList::FindPortPred::operator()(LooperData& Data)
{
	return Data.looper && port == _get_looper_port_(Data.looper);
}

}	// namespace BPrivate

