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
//	File Name:		WatchingService.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Features everything needed to provide a watching service.
//------------------------------------------------------------------------------

#include <List.h>

#include "Watcher.h"
#include "WatchingService.h"

// constructor
WatchingService::WatchingService()
	: fWatchers()
{
}

// destructor
WatchingService::~WatchingService()
{
	// delete the watchers
	for (watcher_map::iterator it = fWatchers.begin();
		 it != fWatchers.end();
		 ++it) {
		delete it->second;
	}
}

// AddWatcher
bool
WatchingService::AddWatcher(Watcher *watcher)
{
	bool result = (watcher);
	if (result) {
		RemoveWatcher(watcher->Target(), true);
		fWatchers[watcher->Target()] = watcher;
	}
	return result;
}

// AddWatcher
bool
WatchingService::AddWatcher(const BMessenger &target)
{
	return AddWatcher(new(nothrow) Watcher(target));
}

// RemoveWatcher
bool
WatchingService::RemoveWatcher(Watcher *watcher, bool deleteWatcher)
{
	return (watcher && RemoveWatcher(watcher->Target(), deleteWatcher));
}

// RemoveWatcher
bool
WatchingService::RemoveWatcher(const BMessenger &target, bool deleteWatcher)
{
	watcher_map::iterator it = fWatchers.find(target);
	bool result = (it != fWatchers.end());
	if (result) {
		if (deleteWatcher)
			delete it->second;
		fWatchers.erase(it);
	}
	return result;
}

// NotifyWatchers
void
WatchingService::NotifyWatchers(BMessage *message, WatcherFilter *filter)
{
	BList staleWatchers;
	// deliver the message
	for (watcher_map::iterator it = fWatchers.begin();
		 it != fWatchers.end();
		 ++it) {
		Watcher *watcher = it->second;
// TODO: If a watcher is invalid, but the filter never selects it, it will
// not be removed.
		if (!filter || filter->Filter(watcher, message)) {
			status_t error = watcher->SendMessage(message);
			if (error != B_OK && error != B_WOULD_BLOCK)
				staleWatchers.AddItem(watcher);
		}
	}
	// remove the stale watchers
	for (int32 i = 0;
		 Watcher *watcher = (Watcher*)staleWatchers.ItemAt(i);
		 i++) {
		RemoveWatcher(watcher, true);
	}
}

