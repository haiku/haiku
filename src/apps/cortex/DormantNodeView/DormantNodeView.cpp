/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// DormantNodeView.cpp

#include "DormantNodeView.h"
// DormantNodeView
#include "DormantNodeWindow.h"
#include "DormantNodeListItem.h"
// InfoWindow
#include "InfoWindowManager.h"

// Interface Kit
#include <Deskbar.h>
#include <Region.h>
#include <Screen.h>
#include <ScrollBar.h>
// Media Kit
#include <MediaRoster.h>
// Storage Kit
#include <Mime.h>

__USE_CORTEX_NAMESPACE

#include <Debug.h>
#define D_ALLOC(x) //PRINT (x)		// ctor/dtor
#define D_HOOK(x) //PRINT (x)		// BListView impl.
#define D_MESSAGE(x) //PRINT (x)	// MessageReceived()
#define D_INTERNAL(x) //PRINT (x)	// internal operations

// -------------------------------------------------------- //
// ctor/dtor (public)
// -------------------------------------------------------- //

DormantNodeView::DormantNodeView(
	BRect frame,
	const char *name,
	uint32 resizeMode)
	: BListView(frame, name, B_SINGLE_SELECTION_LIST, resizeMode),
	  m_lastItemUnder(0) {
	D_ALLOC(("DormantNodeView::DormantNodeView()\n"));

}

DormantNodeView::~DormantNodeView() {
	D_ALLOC(("DormantNodeView::~DormantNodeView()\n"));

}

// -------------------------------------------------------- //
// BListView impl. (public)
// -------------------------------------------------------- //

void DormantNodeView::AttachedToWindow() {
	D_HOOK(("DormantNodeView::AttachedToWindow()\n"));

	// populate the list
	_populateList();

	// Start watching the MediaRoster for flavor changes
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster) {
		BMessenger messenger(this, Window());
		roster->StartWatching(messenger, B_MEDIA_FLAVORS_CHANGED);
	}
}

void DormantNodeView::DetachedFromWindow() {
	D_HOOK(("DormantNodeView::DetachedFromWindow()\n"));

	// delete the lists contents
	_freeList();

	// Stop watching the MediaRoster for flavor changes
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	if (roster) {
		BMessenger messenger(this, Window());
		roster->StopWatching(messenger, B_MEDIA_FLAVORS_CHANGED);
	}
}

void DormantNodeView::GetPreferredSize(
	float* width,
	float* height) {
	D_HOOK(("DormantNodeView::GetPreferredSize()\n"));

	// calculate the accumulated size of all list items
	*width = 0;
	*height = 0;
	for (int32 i = 0; i < CountItems(); i++) {
		DormantNodeListItem *item;
		item = dynamic_cast<DormantNodeListItem *>(ItemAt(i));
		if (item) {
			BRect r = item->getRealFrame(be_plain_font);
			if (r.Width() > *width) {
				*width = r.Width();
			}
			*height += r.Height() + 1.0;
		}
	}
}

void DormantNodeView::MessageReceived(
	BMessage *message) {
	D_MESSAGE(("DormantNodeView::MessageReceived()\n"));

	switch (message->what) {
		case B_MEDIA_FLAVORS_CHANGED: {
			D_MESSAGE((" -> B_MEDIA_FLAVORS_CHANGED\n"));

			// init & re-populate the list
			int32 addOnID = 0;
			if (message->FindInt32("be:addon_id", &addOnID) != B_OK) {
				D_MESSAGE((" -> messages doesn't contain 'be:addon_id'!\n"));
				return;
			}
			_updateList(addOnID);
			break;
		}
		case InfoWindowManager::M_INFO_WINDOW_REQUESTED: {
			D_MESSAGE((" -> InfoWindowManager::M_INFO_WINDOW_REQUESTED)\n"));

			DormantNodeListItem *item;
			item = dynamic_cast<DormantNodeListItem *>(ItemAt(CurrentSelection()));
			if (item) {
				InfoWindowManager *manager = InfoWindowManager::Instance();
				if (manager && manager->Lock()) {
					manager->openWindowFor(item->info());
					manager->Unlock();
				}
			}
			break;
		}
		default: {
			_inherited::MessageReceived(message);
		}
	}
}

void DormantNodeView::MouseDown(
	BPoint point) {
	D_HOOK(("DormantNodeView::MouseDown()\n"));

	BMessage* message = Window()->CurrentMessage();
	int32 buttons = message->FindInt32("buttons");
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		int32 index;
		if ((index = IndexOf(point)) >= 0) {
			DormantNodeListItem *item = dynamic_cast<DormantNodeListItem *>(ItemAt(index));
			if (item) {
				Select(index);
				BRect r = item->getRealFrame(be_plain_font);
				if (r.Contains(point)) {
					item->showContextMenu(point, this);
				}
			}
		}
	}

	_inherited::MouseDown(point);
}

void DormantNodeView::MouseMoved(
	BPoint point,
	uint32 transit,
	const BMessage *message) {
	D_HOOK(("DormantNodeView::MouseMoved()\n"));

	int32 index;
	if (!message && ((index = IndexOf(point)) >= 0)) {
		DormantNodeListItem *item =
			dynamic_cast<DormantNodeListItem *>(ItemAt(index));
		DormantNodeListItem *last =
			dynamic_cast<DormantNodeListItem *>(m_lastItemUnder);
		if (item != NULL) {
			BRect r = item->getRealFrame(be_plain_font);
			if (r.Contains(point)) {
				if (item != last) {
					if (last != NULL)
						last->MouseOver(this, point, B_EXITED_VIEW);
					item->MouseOver(this, point, B_ENTERED_VIEW);
					m_lastItemUnder = item;
				}
				else {
					item->MouseOver(this, point, B_INSIDE_VIEW);
				}
			}
		}
		else if (last != NULL) {
			last->MouseOver(this, point, B_EXITED_VIEW);
		}
	}

	_inherited::MouseMoved(point, transit, message);
}

bool DormantNodeView::InitiateDrag(
	BPoint point,
	int32 index,
	bool wasSelected) {
	D_HOOK(("DormantNodeView::InitiateDrag()\n"));

	DormantNodeListItem *item = dynamic_cast<DormantNodeListItem *>(ItemAt(CurrentSelection()));
	if (item) {
		BMessage dragMsg(M_INSTANTIATE_NODE);
		dragMsg.AddData("which", B_RAW_TYPE,
						reinterpret_cast<const void *>(&item->info()),
						sizeof(item->info()));
		point -= ItemFrame(index).LeftTop();
		DragMessage(&dragMsg, item->getDragBitmap(), B_OP_ALPHA, point);
		return true;
	}
	return false;
}

// -------------------------------------------------------- //
// internal operations (private)
// -------------------------------------------------------- //

void DormantNodeView::_populateList() {
	D_INTERNAL(("DormantNodeView::_populateList()\n"));

	// init the resizable node-info buffer
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	const int32 bufferInc = 64;
	int32 bufferSize = bufferInc;
	dormant_node_info *infoBuffer = new dormant_node_info[bufferSize];
	int32 numNodes;

	// fill the buffer
	while (true) {
		numNodes = bufferSize;
		status_t error = roster->GetDormantNodes(infoBuffer, &numNodes);
		if (error) {
			return;
		}
		if (numNodes < bufferSize) {
			break;
		}

		// reallocate buffer & try again
		delete [] infoBuffer;
		bufferSize += bufferInc;
		infoBuffer = new dormant_node_info[bufferSize];
	}

	// populate the list
	for (int32 i = 0; i < numNodes; i++) {
		DormantNodeListItem *item = new DormantNodeListItem(infoBuffer[i]);
		AddItem(item);
	}
	SortItems(compareName);
}

void DormantNodeView::_freeList() {
	D_HOOK(("DormantNodeView::_freeList()\n"));

	// remove and delete all items in the list
	while (CountItems() > 0) {
		BListItem *item = ItemAt(0);
		if (RemoveItem(item)) {
			delete item;
		}
	}
}

void DormantNodeView::_updateList(
	int32 addOnID) {
	D_INTERNAL(("DormantNodeView::_updateList(%ld)\n", addOnID));

	// init the resizable node-info buffer
	BMediaRoster *roster = BMediaRoster::CurrentRoster();
	const int32 bufferInc = 64;
	int32 bufferSize = bufferInc;
	dormant_node_info *infoBuffer = new dormant_node_info[bufferSize];
	int32 numNodes;

	// fill the buffer
	while (true) {
		numNodes = bufferSize;
		status_t error = roster->GetDormantNodes(infoBuffer, &numNodes);
		if (error) {
			return;
		}
		if (numNodes < bufferSize) {
			break;
		}

		// reallocate buffer & try again
		delete [] infoBuffer;
		bufferSize += bufferInc;
		infoBuffer = new dormant_node_info[bufferSize];
	}

	// sort the list by add-on id to avoid multiple searches through
	// the list
	SortItems(compareAddOnID);

	// Remove all nodes managed by this add-on
	int32 start;
	for (start = 0; start < CountItems(); start++) {
		DormantNodeListItem *item = dynamic_cast<DormantNodeListItem *>(ItemAt(start));
		if (item && (item->info().addon == addOnID)) {
			break;
		}
	}
	int32 count = 0;
	for (int32 i = start; start < CountItems(); i++) {
		DormantNodeListItem *item = dynamic_cast<DormantNodeListItem *>(ItemAt(i));
		if (!item || (item->info().addon != addOnID)) {
			break;
		}
		count++;
	}
	RemoveItems(start, count);

	// add the items
	for (int32 i = 0; i < numNodes; i++) {
		if (infoBuffer[i].addon != addOnID) {
			continue;
		}
		AddItem(new DormantNodeListItem(infoBuffer[i]));
	}

	SortItems(compareName);
}

// END -- DormantNodeView.cpp --
