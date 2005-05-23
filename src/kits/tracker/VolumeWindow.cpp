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

#include <Debug.h>
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


BVolumeWindow::BVolumeWindow(LockingList<BWindow> *windowList, uint32 openFlags)
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
		Model *model = PoseView()->SelectionList()->ItemAt(index)->TargetModel();
		if (model->IsVolume()) {
			BVolume volume;
			volume.SetTo(model->NodeRef()->device);
			if (volume != boot) {
				ejectableVolumeSelected = true;
				break;
			}
		}
	}

	BMenuItem *item = fMenuBar->FindItem("Unmount");
	if (item)
		item->SetEnabled(ejectableVolumeSelected);
}


void
BVolumeWindow::AddFileMenu(BMenu *menu)
{
	menu->AddItem(new BMenuItem("Find"B_UTF8_ELLIPSIS,
		new BMessage(kFindButton), 'F'));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Open", new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem("Get Info", new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem("Edit Name", new BMessage(kEditItem), 'E'));

	BMenuItem *item = new BMenuItem("Unmount", new BMessage(kUnmountVolume), 'U');
	item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddItem(new BMenuItem("Mount Settings" B_UTF8_ELLIPSIS, 
		new BMessage(kRunAutomounterSettings)));

	menu->AddSeparatorItem();
	menu->AddItem(new BMenu(kAddOnsMenuName));
	menu->SetTargetForItems(PoseView());
}


void 
BVolumeWindow::AddWindowContextMenus(BMenu *menu)
{
	if (fPoseView != NULL && fPoseView->TargetModel() != NULL
		&& !fPoseView->TargetModel()->IsRoot()) {
		_inherited::AddWindowContextMenus(menu);
		return;
	}

	menu->AddItem(new BMenuItem("Icon View", new BMessage(kIconMode)));
	menu->AddItem(new BMenuItem("Mini Icon View", new BMessage(kMiniIconMode)));
	menu->AddItem(new BMenuItem("List View", new BMessage(kListMode)));
	menu->AddSeparatorItem();

	BMenuItem *resizeItem = new BMenuItem("Resize to Fit",new BMessage(kResizeToFit), 'Y');
	menu->AddItem(resizeItem);
	menu->AddItem(new BMenuItem("Clean Up", new BMessage(kCleanup), 'K'));
	menu->AddItem(new BMenuItem("Select"B_UTF8_ELLIPSIS, new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem("Select All", new BMessage(B_SELECT_ALL), 'A'));
	menu->AddItem(new BMenuItem("Invert Selection", new BMessage(kInvertSelection), 'S'));

	BMenuItem *closeItem = new BMenuItem("Close",new BMessage(B_QUIT_REQUESTED), 'W');
	menu->AddItem(closeItem);
	menu->AddSeparatorItem();

	menu->AddItem(new MountMenu("Mount"));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenu(kAddOnsMenuName));

	// target items as needed
	menu->SetTargetForItems(PoseView());
	closeItem->SetTarget(this);
	resizeItem->SetTarget(this);
}

