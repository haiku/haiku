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
//	File Name:		EventMaskWatcher.cpp
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	EventMaskWatcher is a Watcher extended by an event mask.
//					EventMaskWatcherFilter filters EventMaskWatchers with
//					respect to their event mask and a specific event.
//------------------------------------------------------------------------------

#include "EventMaskWatcher.h"

// EventMaskWatcher

// constructor
EventMaskWatcher::EventMaskWatcher(const BMessenger &target, uint32 eventMask)
	: Watcher(target),
	  fEventMask(eventMask)
{
}

// EventMask
uint32
EventMaskWatcher::EventMask() const
{
	return fEventMask;
}


// EventMaskWatcherFilter

// constructor
EventMaskWatcherFilter::EventMaskWatcherFilter(uint32 event)
	: WatcherFilter(),
	  fEvent(event)
{
}

// Filter
bool
EventMaskWatcherFilter::Filter(Watcher *watcher, BMessage *message)
{
	EventMaskWatcher *emWatcher = dynamic_cast<EventMaskWatcher *>(watcher);
	return (emWatcher && (emWatcher->EventMask() & fEvent));
}

