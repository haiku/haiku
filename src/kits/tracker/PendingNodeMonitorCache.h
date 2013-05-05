/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//	PendingNodeMonitorCache is used to store node monitors that
//	cannot be delivered yet because the corresponding model has not yet been
//  added to a PoseView
//
//	The respective node montior messages are stored in a list and applied
//  later, when their target shows up. They get nuked when they become too
//  old.
#ifndef __PENDING_NODEMONITOR_CACHE_H__
#define __PENDING_NODEMONITOR_CACHE_H__


#include <Message.h>
#include <Node.h>

#include "ObjectList.h"


namespace BPrivate {

class BPoseView;
class BPose;

class PendingNodeMonitorEntry {
public:
	PendingNodeMonitorEntry(const node_ref* node, const BMessage*);
	const BMessage* NodeMonitor() const;
	bool Match(const node_ref*) const;
	bool TooOld(bigtime_t now) const;

private:
	bigtime_t fExpiresAfter;
	BMessage fNodeMonitor;
	node_ref fNode;
};


class PendingNodeMonitorCache {
public:
	PendingNodeMonitorCache();
	~PendingNodeMonitorCache();

	void Add(const BMessage*);
	void RemoveEntries(const node_ref*);
	void RemoveOldEntries();

	void PoseCreatedOrMoved(BPoseView*, const BPose*);

private:
	BObjectList<PendingNodeMonitorEntry> fList;
};

} // namespace BPrivate

using namespace BPrivate;

#endif	// __PENDING_NODEMONITOR_CACHE_H__
