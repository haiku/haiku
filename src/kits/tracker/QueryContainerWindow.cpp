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


#include <Catalog.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Query.h>

#include "Attributes.h"
#include "Commands.h"
#include "QueryContainerWindow.h"
#include "QueryPoseView.h"
#include "Shortcuts.h"


//	#pragma mark - BQueryContainerWindow


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "QueryContainerWindow"


BQueryContainerWindow::BQueryContainerWindow(LockingList<BWindow>* windowList,
	uint32 openFlags)
	:
	BContainerWindow(windowList, openFlags)
{
}


BPoseView*
BQueryContainerWindow::NewPoseView(Model* model, uint32)
{
	return new BQueryPoseView(model);
}


BQueryPoseView*
BQueryContainerWindow::PoseView() const
{
	return static_cast<BQueryPoseView*>(fPoseView);
}


void
BQueryContainerWindow::CreatePoseView(Model* model)
{
	fPoseView = NewPoseView(model, kListMode);

	fBorderedView->GroupLayout()->AddView(fPoseView);
	fBorderedView->EnableBorderHighlight(false);
	fBorderedView->GroupLayout()->SetInsets(0, 0, 1, 1);
}


void
BQueryContainerWindow::AddWindowMenu(BMenu* menu)
{
	BMenuItem* item;

	item = Shortcuts()->ResizeToFitItem();
	item->SetTarget(this);
	menu->AddItem(item);

	item = Shortcuts()->SelectItem();
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = Shortcuts()->SelectAllItem();
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = Shortcuts()->InvertSelectionItem();
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = Shortcuts()->CloseItem();
	item->SetTarget(this);
	menu->AddItem(item);

	item = Shortcuts()->CloseAllInWorkspaceItem();
	item->SetTarget(this);
	menu->AddItem(item);
}


void
BQueryContainerWindow::AddWindowContextMenus(BMenu* menu)
{
	BMenuItem* resizeItem = Shortcuts()->ResizeToFitItem();
	menu->AddItem(resizeItem);
	menu->AddItem(Shortcuts()->SelectItem());
	menu->AddItem(Shortcuts()->SelectAllItem());
	BMenuItem* closeItem = Shortcuts()->CloseItem();
	menu->AddItem(closeItem);

	// target items as needed
	menu->SetTargetForItems(PoseView());
	closeItem->SetTarget(this);
	resizeItem->SetTarget(this);
}


void
BQueryContainerWindow::SetupDefaultState()
{
	BNode defaultingNode;

	WindowStateNodeOpener opener(this, true);
		// this is our destination node, whatever it is for this window
	if (opener.StreamNode() == NULL)
		return;

	BString defaultStatePath(kQueryTemplates);
	BString sanitizedType(PoseView()->SearchForType());

	defaultStatePath += '/';
	int32 length = sanitizedType.Length();
	char* buf = sanitizedType.LockBuffer(length);
	for (int32 index = length - 1; index >= 0; index--)
		if (buf[index] == '/')
			buf[index] = '_';
	sanitizedType.UnlockBuffer(length);

	defaultStatePath += sanitizedType;

	PRINT(("looking for default query state at %s\n",
		defaultStatePath.String()));

	if (!DefaultStateSourceNode(defaultStatePath.String(), &defaultingNode,
			false)) {
		TRACE();
		return;
	}

	// copy over the attributes

	// set up a filter of the attributes we want copied
	const char* allowAttrs[] = {
		kAttrWindowFrame,
		kAttrViewState,
		kAttrViewStateForeign,
		kAttrColumns,
		kAttrColumnsForeign,
		0
	};

	// do it
	AttributeStreamMemoryNode memoryNode;
	NamesToAcceptAttrFilter filter(allowAttrs);
	AttributeStreamFileNode fileNode(&defaultingNode);
	*opener.StreamNode() << memoryNode << filter << fileNode;
}


bool
BQueryContainerWindow::ShouldHaveEditQueryItem(const entry_ref*)
{
	return true;
}


bool
BQueryContainerWindow::ActiveOnDevice(dev_t device) const
{
	return PoseView()->ActiveOnDevice(device);
}
