//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		LooperList.cpp
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	Maintains a global list of all loopers in a given team.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algorithm>

// System Includes -------------------------------------------------------------
#include <Autolock.h>
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LooperList.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
using std::vector;

namespace BPrivate {

BLooperList gLooperList;

typedef vector<BLooperList::LooperData>::iterator LDIter;

//------------------------------------------------------------------------------
BLooperList::BLooperList()
	:	fLooperID(0)
{
}
//------------------------------------------------------------------------------
bool BLooperList::Lock()
{
	return fLock.Lock();
}
//------------------------------------------------------------------------------
void BLooperList::Unlock()
{
	fLock.Unlock();
}
//------------------------------------------------------------------------------
bool BLooperList::IsLocked()
{
	return fLock.IsLocked();
}
//------------------------------------------------------------------------------
void BLooperList::AddLooper(BLooper* looper)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	if (!IsLooperValid(looper))
	{
		LDIter i = find_if(fData.begin(), fData.end(), EmptySlotPred);
		if (i == fData.end())
		{
			fData.push_back(LooperData(looper, ++fLooperID));
			looper->fLooperID = fLooperID;
			looper->Lock();
		}
		else
		{
			i->looper = looper;
			i->id = ++fLooperID;
			looper->fLooperID = fLooperID;
			looper->Lock();
		}
	}
}
//------------------------------------------------------------------------------
bool BLooperList::IsLooperValid(const BLooper* looper)
{
	BAutolock Listlock(fLock);
	AssertLocked();

	return find_if(fData.begin(), fData.end(), FindLooperPred(looper)) !=
			fData.end();
}
//------------------------------------------------------------------------------
bool BLooperList::RemoveLooper(BLooper* looper)
{
	BAutolock Listlock(fLock);
	AssertLocked();

	LDIter i = find_if(fData.begin(), fData.end(), FindLooperPred(looper));
	if (i != fData.end())
	{
		i->looper = NULL;
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------
void BLooperList::GetLooperList(BList* list)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	for (uint32 i = 0; i < fData.size(); ++i)
	{
		if (fData[i].looper)
			list->AddItem(fData[i].looper);
	}
}
//------------------------------------------------------------------------------
int32 BLooperList::CountLoopers()
{
	BAutolock Listlock(fLock);
	AssertLocked();
	return (int32)fData.size();
}
//------------------------------------------------------------------------------
BLooper* BLooperList::LooperAt(int32 index)
{
	BAutolock Listlock(fLock);
	AssertLocked();

	BLooper* Looper = NULL;
	if (index < (int32)fData.size())
	{
		Looper = fData[(uint32)index].looper;
	}

	return Looper;
}
//------------------------------------------------------------------------------
BLooper* BLooperList::LooperForThread(thread_id tid)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	BLooper* looper = NULL;
	LDIter i = find_if(fData.begin(), fData.end(), FindThreadPred(tid));
	if (i != fData.end())
	{
		looper = i->looper;
	}

	return looper;
}
//------------------------------------------------------------------------------
BLooper* BLooperList::LooperForName(const char* name)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	BLooper* looper = NULL;
	LDIter i = find_if(fData.begin(), fData.end(), FindNamePred(name));
	if (i != fData.end())
	{
		looper = i->looper;
	}

	return looper;
}
//------------------------------------------------------------------------------
BLooper* BLooperList::LooperForPort(port_id port)
{
	BAutolock Listlock(fLock);
	AssertLocked();
	BLooper* looper = NULL;
	LDIter i = find_if(fData.begin(), fData.end(), FindPortPred(port));
	if (i != fData.end())
	{
		looper = i->looper;
	}

	return looper;
}
//------------------------------------------------------------------------------
bool BLooperList::EmptySlotPred(LooperData& Data)
{
	return Data.looper == NULL;
}
//------------------------------------------------------------------------------
void BLooperList::AssertLocked()
{
	if (!IsLocked())
	{
		debugger("looperlist is not locked; proceed at great risk!");
	}
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//	#pragma mark -
//	#pragma mark BLooperList::LooperData
//	#pragma mark -
//------------------------------------------------------------------------------
BLooperList::LooperData::LooperData()
	:	looper(NULL), id(0)
{
}
//------------------------------------------------------------------------------
BLooperList::LooperData::LooperData(BLooper* loop, uint32 i)
	:	looper(loop), id(i)
{
	;
}
//------------------------------------------------------------------------------
BLooperList::LooperData::LooperData(const LooperData& rhs)
{
	*this = rhs;
}
//------------------------------------------------------------------------------
BLooperList::LooperData&
BLooperList::LooperData::operator=(const LooperData& rhs)
{
	if (this != &rhs)
	{
		looper = rhs.looper;
		id = rhs.id;
	}

	return *this;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
bool BLooperList::FindLooperPred::operator()(BLooperList::LooperData& Data)
{
	return Data.looper && (looper == Data.looper);
}
//------------------------------------------------------------------------------
bool BLooperList::FindThreadPred::operator()(LooperData& Data)
{
	return Data.looper && (thread == Data.looper->Thread());
}
//------------------------------------------------------------------------------
bool BLooperList::FindNamePred::operator()(LooperData& Data)
{
	return Data.looper && (strcmp(name, Data.looper->Name()) == 0);
}
//------------------------------------------------------------------------------
bool BLooperList::FindPortPred::operator()(LooperData& Data)
{
	return Data.looper && (port == _get_looper_port_(Data.looper));
}
//------------------------------------------------------------------------------

}	// namespace BPrivate

/*
 * $Log $
 *
 * $Id  $
 *
 */

