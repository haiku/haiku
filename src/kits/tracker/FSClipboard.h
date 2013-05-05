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
and shall be included in all copies or substantial portions of the Software..

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
#ifndef FS_CLIPBOARD_H
#define FS_CLIPBOARD_H


#include <Looper.h>
#include "Model.h"
#include "ObjectList.h"
#include "Pose.h"
#include "PoseView.h"


namespace BPrivate {

typedef struct {
	node_ref node;
	uint32 moveMode;
} TClipboardNodeRef;
const int32 T_CLIPBOARD_NODE = 'TCNR';


class BClipboardRefsWatcher : public BLooper {
	public:
		BClipboardRefsWatcher();
		virtual ~BClipboardRefsWatcher();

		void AddToNotifyList(BMessenger target);
		void RemoveFromNotifyList(BMessenger target);
		void AddNode(const node_ref* node);
		void RemoveNode(node_ref* node, bool removeFromClipboard = false);
		void RemoveNodesByDevice(dev_t device);
		void UpdateNode(node_ref* node, entry_ref* ref);
		void Clear();
//		void UpdatePoseViews(bool clearClipboard, const node_ref* node);
		void UpdatePoseViews(BMessage* reportMessage);

	protected:
		virtual void MessageReceived(BMessage*);

	private:
		bool fRefsInClipboard;
		BObjectList<BMessenger> fNotifyList;

		typedef BLooper _inherited;
};

} // namespace BPrivate

using namespace BPrivate;

//bool FSClipboardCheckIntegrity();
bool FSClipboardHasRefs();

void FSClipboardStartWatch(BMessenger target);
void FSClipboardStopWatch(BMessenger target);

void FSClipboardClear();
uint32 FSClipboardAddPoses(const node_ref* directory, PoseList* list,
	uint32 moveMode, bool clearClipboard);
uint32 FSClipboardRemovePoses(const node_ref* directory, PoseList* list);
bool FSClipboardPaste(Model* model, uint32 linksMode = 0);
void FSClipboardRemove(Model* model);
uint32 FSClipboardFindNodeMode(Model* model, bool autoLock,
	bool updateRefIfNeeded);

#endif	// FS_CLIPBOARD_H
