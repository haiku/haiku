//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
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
//	File Name:		WatchingService.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Features everything needed to provide a watching service.
//------------------------------------------------------------------------------

#ifndef WATCHING_SERVICE_H
#define WATCHING_SERVICE_H

#include <map>

#include <Messenger.h>

class Watcher;
class WatcherFilter;

class WatchingService {
public:
	WatchingService();
	virtual ~WatchingService();

	bool AddWatcher(Watcher *watcher);
	bool AddWatcher(const BMessenger &target);
	bool RemoveWatcher(Watcher *watcher, bool deleteWatcher = true);
	bool RemoveWatcher(const BMessenger &target, bool deleteWatcher = true);

	void NotifyWatchers(BMessage *message, WatcherFilter *filter = NULL);

private:
	typedef std::map<BMessenger,Watcher*> watcher_map;

private:
	watcher_map	fWatchers;
};

#endif	// WATCHING_SERVICE_H
