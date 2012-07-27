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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "QueryContainerWindow"

BQueryContainerWindow::BQueryContainerWindow(LockingList<BWindow>* windowList,
	uint32 containerWindowFlags, window_look look,
	window_feel feel, uint32 flags, uint32 workspace)
	:	BContainerWindow(windowList, containerWindowFlags, look, feel,
			flags, workspace)
{
}


BPoseView*
BQueryContainerWindow::NewPoseView(Model* model, BRect rect, uint32)
{
	return new BQueryPoseView(model, rect);
}


BQueryPoseView*
BQueryContainerWindow::PoseView() const
{
	return static_cast<BQueryPoseView*>(fPoseView);
}


void
BQueryContainerWindow::CreatePoseView(Model* model)
{
	BRect rect(Bounds());
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fPoseView = NewPoseView(model, rect, kListMode);

	AddChild(fPoseView);
}


void
BQueryContainerWindow::AddWindowMenu(BMenu* menu)
{
	BMenuItem* item;

	item = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	item->SetTarget(this);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select all"),	new BMessage(B_SELECT_ALL),
		'A');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection), 'S');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED),
		'W');
	item->SetTarget(this);
	menu->AddItem(item);
}


void
BQueryContainerWindow::AddWindowContextMenus(BMenu* menu)
{
	BMenuItem* resizeItem = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	menu->AddItem(resizeItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
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


void
BQueryContainerWindow::SetUpDefaultState()
{
	BNode defaultingNode;

	WindowStateNodeOpener opener(this, true);
		// this is our destination node, whatever it is for this window
	if (!opener.StreamNode())
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

	PRINT(("looking for default query state at %s\n", defaultStatePath.String()));

	if (!DefaultStateSourceNode(defaultStatePath.String(), &defaultingNode, false)) {
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
BQueryContainerWindow::ActiveOnDevice(dev_t device) const
{
	return PoseView()->ActiveOnDevice(device);
}
