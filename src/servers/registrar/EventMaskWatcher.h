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
//	File Name:		EventMaskWatcher.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	EventMaskWatcher is a Watcher extended by an event mask.
//					EventMaskWatcherFilter filters EventMaskWatchers with
//					respect to their event mask and a specific event.
//------------------------------------------------------------------------------

#ifndef EVENT_MASK_WATCHER_H
#define EVENT_MASK_WATCHER_H

#include "Watcher.h"

// EventMaskWatcher
class EventMaskWatcher : public Watcher {
public:
	EventMaskWatcher(const BMessenger &target, uint32 eventMask);

	uint32 EventMask() const;

private:
	uint32	fEventMask;
};

// EventMaskWatcherFilter
class EventMaskWatcherFilter : public WatcherFilter {
public:
	EventMaskWatcherFilter(uint32 event);

	virtual bool Filter(Watcher *watcher, BMessage *message);

private:
	uint32	fEvent;
};

#endif	// EVENT_MASK_WATCHER_H
