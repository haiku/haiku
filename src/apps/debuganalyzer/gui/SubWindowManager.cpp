/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "SubWindowManager.h"

#include <new>

#include <Message.h>

#include <AutoLocker.h>


SubWindowManager::SubWindowManager(BLooper* parent)
	:
	fParent(parent),
	fSubWindows()
{
}


SubWindowManager::~SubWindowManager()
{
}


status_t
SubWindowManager::Init()
{
	return fSubWindows.Init();
}


bool
SubWindowManager::AddSubWindow(SubWindow* window)
{
	AutoLocker<SubWindowManager> locker(this);

	if (fSubWindows.Lookup(*window->GetSubWindowKey()) != NULL)
		return false;

	fSubWindows.Insert(window);

	return true;
}


bool
SubWindowManager::RemoveSubWindow(SubWindow* window)
{
	AutoLocker<SubWindowManager> locker(this);

	return fSubWindows.Remove(window);
}


SubWindow*
SubWindowManager::LookupSubWindow(const SubWindowKey& key) const
{
	return fSubWindows.Lookup(key);
}


void
SubWindowManager::Broadcast(uint32 messageCode)
{
	BMessage message(messageCode);
	Broadcast(&message);
}


void
SubWindowManager::Broadcast(BMessage* message)
{
	AutoLocker<SubWindowManager> locker(this);

	SubWindowTable::Iterator it = fSubWindows.GetIterator();
	while (SubWindow* window = it.Next())
		window->PostMessage(message);
}
