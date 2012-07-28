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
#include <Debug.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Commands.h"
#include "VolumeWindow.h"
#include "PoseView.h"
#include "MountMenu.h"



#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "VolumeWindow"

BVolumeWindow::BVolumeWindow(LockingList<BWindow>* windowList, uint32 openFlags)
	:	BContainerWindow(windowList, openFlags)
{
}


void
BVolumeWindow::MenusBeginning()
{
	_inherited::MenusBeginning();

	if (!fMenuBar)
		return;

	BVolume boot;
	BVolumeRoster().GetBootVolume(&boot);

	bool ejectableVolumeSelected = false;

	int32 count = PoseView()->SelectionList()->CountItems();
	for (int32 index = 0; index < count; index++) {
		Model* model = PoseView()->SelectionList()->ItemAt(index)->TargetModel();
		if (model->IsVolume()) {
			BVolume volume;
			volume.SetTo(model->NodeRef()->device);
			if (volume != boot) {
				ejectableVolumeSelected = true;
				break;
			}
		}
	}

	BMenuItem* item = fMenuBar->FindItem(kUnmountVolume);
	if (item)
		item->SetEnabled(ejectableVolumeSelected);
}


void
BVolumeWindow::AddFileMenu(BMenu* menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Find"B_UTF8_ELLIPSIS),
		new BMessage(kFindButton), 'F'));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	BMenuItem* item = new BMenuItem(B_TRANSLATE("Unmount"),
		new BMessage(kUnmountVolume), 'U');
	item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddItem(new BMenuItem(B_TRANSLATE("Mount settings" B_UTF8_ELLIPSIS),
		new BMessage(kRunAutomounterSettings)));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenu(B_TRANSLATE("Add-ons")));
	menu->SetTargetForItems(PoseView());
}


void
BVolumeWindow::AddWindowContextMenus(BMenu* menu)
{
	if (fPoseView != NULL && fPoseView->TargetModel() != NULL
		&& !fPoseView->TargetModel()->IsRoot()) {
		_inherited::AddWindowContextMenus(menu);
		return;
	}

	menu->AddItem(new BMenuItem(B_TRANSLATE("Icon view"),
		new BMessage(kIconMode)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Mini icon view"),
		new BMessage(kMiniIconMode)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("List view"),
		new BMessage(kListMode)));
	menu->AddSeparatorItem();

	BMenuItem* resizeItem = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	menu->AddItem(resizeItem);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Clean up"),
		new BMessage(kCleanup), 'K'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection), 'S'));

	BMenuItem* closeItem = new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W');
	menu->AddItem(closeItem);
	menu->AddSeparatorItem();

	menu->AddItem(new MountMenu(B_TRANSLATE("Mount")));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenu(B_TRANSLATE("Add-ons")));

	// target items as needed
	menu->SetTargetForItems(PoseView());
	closeItem->SetTarget(this);
	resizeItem->SetTarget(this);
}
