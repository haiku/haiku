/*
 * Copyright 2013-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */
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


#include "VirtualDirectoryWindow.h"

#include <Catalog.h>
#include <Locale.h>

#include <AutoLocker.h>

#include "Commands.h"
#include "VirtualDirectoryManager.h"
#include "VirtualDirectoryPoseView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "VirtualDirectoryWindow"


namespace BPrivate {

//	#pragma mark - VirtualDirectoryWindow


VirtualDirectoryWindow::VirtualDirectoryWindow(LockingList<BWindow>* windowList,
	uint32 containerWindowFlags, window_look look, window_feel feel,
	uint32 flags, uint32 workspace)
	:
	BContainerWindow(windowList, containerWindowFlags, look, feel, flags,
		workspace)
{
}


void
VirtualDirectoryWindow::CreatePoseView(Model* model)
{
	fPoseView = NewPoseView(model, kListMode);
	if (fPoseView == NULL)
		return;

	fBorderedView->GroupLayout()->AddView(fPoseView);
	fBorderedView->EnableBorderHighlight(false);
	fBorderedView->GroupLayout()->SetInsets(0, 0, 1, 1);
}


BPoseView*
VirtualDirectoryWindow::NewPoseView(Model* model, uint32 viewMode)
{
	// If the model (or rather the entry_ref to it) came from another
	// application, it may refer to a subdirectory we cannot use directly. The
	// manager resolves the given refs to new ones, if necessary.
	VirtualDirectoryManager* manager = VirtualDirectoryManager::Instance();
	if (manager == NULL)
		return NULL;

	{
		AutoLocker<VirtualDirectoryManager> managerLocker(manager);
		BStringList directoryPaths;
		node_ref nodeRef;
		entry_ref entryRef;
		if (manager->ResolveDirectoryPaths(*model->NodeRef(),
				*model->EntryRef(), directoryPaths, &nodeRef, &entryRef)
				!= B_OK) {
			return NULL;
		}

		if (nodeRef != *model->NodeRef()) {
			// Indeed a new file. Create a new model.
			Model* newModel = new(std::nothrow) Model(&entryRef);
			if (newModel == NULL || newModel->InitCheck() != B_OK) {
				delete newModel;
				return NULL;
			}

			delete model;
			model = newModel;
		}
	}

	return new VirtualDirectoryPoseView(model);
}


VirtualDirectoryPoseView*
VirtualDirectoryWindow::PoseView() const
{
	return static_cast<VirtualDirectoryPoseView*>(fPoseView);
}


void
VirtualDirectoryWindow::AddWindowMenu(BMenu* menu)
{
	BMenuItem* item;

	item = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	item->SetTarget(this);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection), 'S');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Open parent"),
		new BMessage(kOpenParentDir), B_UP_ARROW);
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W');
	item->SetTarget(this);
	menu->AddItem(item);
}


void
VirtualDirectoryWindow::AddWindowContextMenus(BMenu* menu)
{
	BMenuItem* resizeItem = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	menu->AddItem(resizeItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select" B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	BMenuItem* closeItem = new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W');
	menu->AddItem(closeItem);
	// target items as needed
	menu->SetTargetForItems(PoseView());
	closeItem->SetTarget(this);
	resizeItem->SetTarget(this);
}

} // namespace BPrivate
