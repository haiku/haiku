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

#include "PendingNodeMonitorCache.h"
#include "PoseView.h"

const bigtime_t kDelayedNodeMonitorLifetime = 10000000;
	// after this much the pending node monitor gets discarded as
	// too old

PendingNodeMonitorEntry::PendingNodeMonitorEntry(const node_ref *node,
	const BMessage *nodeMonitor)
	:	fExpiresAfter(system_time() + kDelayedNodeMonitorLifetime),
		fNodeMonitor(*nodeMonitor),
		fNode(*node)
{
}

const BMessage *
PendingNodeMonitorEntry::NodeMonitor() const
{
	return &fNodeMonitor;
}

bool
PendingNodeMonitorEntry::Match(const node_ref *node) const
{
	return fNode == *node;
}

bool
PendingNodeMonitorEntry::TooOld(bigtime_t now) const
{
	return now > fExpiresAfter;
}


PendingNodeMonitorCache::PendingNodeMonitorCache()
	:	fList(10, true)
{
}


PendingNodeMonitorCache::~PendingNodeMonitorCache()
{
}

void
PendingNodeMonitorCache::Add(const BMessage *message)
{
#if xDEBUG
	PRINT(("adding pending node monitor\n"));
	message->PrintToStream();
#endif
	node_ref node;
	if (message->FindInt32("device", &node.device) != B_OK
		|| message->FindInt64("node", (int64 *)&node.node) != B_OK)
		return;

	fList.AddItem(new PendingNodeMonitorEntry(&node, message));
}

void
PendingNodeMonitorCache::RemoveEntries(const node_ref *nodeRef)
{
	int32 count = fList.CountItems();
	for (int32 index = count - 1; index >= 0; index--)
		if (fList.ItemAt(index)->Match(nodeRef))
			delete fList.RemoveItemAt(index);
}

void
PendingNodeMonitorCache::RemoveOldEntries()
{
	bigtime_t now = system_time();
	int32 count = fList.CountItems();
	for (int32 index = count - 1; index >= 0; index--)
		if (fList.ItemAt(index)->TooOld(now)) {
			PRINT(("removing old entry from pending node monitor cache\n"));
			delete fList.RemoveItemAt(index);
		}
}

void
PendingNodeMonitorCache::PoseCreatedOrMoved(BPoseView *poseView, const BPose *pose)
{
	bigtime_t now = system_time();
	for (int32 index = 0; index < fList.CountItems();) {
		PendingNodeMonitorEntry *item = fList.ItemAt(index);
		if (item->TooOld(now)) {
			PRINT(("removing old entry from pending node monitor cache\n"));
			delete fList.RemoveItemAt(index);
		} else if (item->Match(pose->TargetModel()->NodeRef())) {
			fList.RemoveItemAt(index);
#if DEBUG
			PRINT(("reapplying node monitor for model:\n"));
			pose->TargetModel()->PrintToStream();
			item->NodeMonitor()->PrintToStream();
			bool result =
#endif
			poseView->FSNotification(item->NodeMonitor());
			ASSERT(result);
			delete item;
		} else
			index++;
	}
}

