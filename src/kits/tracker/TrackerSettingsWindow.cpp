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


#include "SettingsViews.h"
#include "TrackerSettings.h"
#include "TrackerSettingsWindow.h"

#include <CheckBox.h>


const BPoint kSettingsWindowOffset(30, 30);
const float kSettingsWindowsWidth = 370;
const float kSettingsWindowsHeight = 270;

const uint32 kSettingsViewChanged = 'Svch';
const uint32 kDefaultsButtonPressed = 'Apbp';
const uint32 kRevertButtonPressed = 'Rebp';


TrackerSettingsWindow::TrackerSettingsWindow()
	: BWindow(BRect(kSettingsWindowOffset.x, kSettingsWindowOffset.y,
		kSettingsWindowOffset.x + kSettingsWindowsWidth,
		kSettingsWindowOffset.y + kSettingsWindowsHeight),
		"Tracker Settings", B_TITLED_WINDOW, B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
		| B_NO_WORKSPACE_ACTIVATION | B_NOT_ANCHORED_ON_ACTIVATE
		| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BView *backgroundView = new BView(Bounds(), "Background", B_FOLLOW_ALL_SIDES, 0);

	backgroundView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(backgroundView);

	const float kBorderDistance = 12;
	const float kListViewWidth = 90;
	const float kListViewHeight = kSettingsWindowsHeight - 2 * kBorderDistance;
	const float kBoxWidth = kSettingsWindowsWidth - kListViewWidth - 3 * (kBorderDistance - 1);
	const float kBoxHeight = kListViewHeight - 30;

	BRect listViewRect(kBorderDistance, kBorderDistance, kBorderDistance + kListViewWidth,
		kBorderDistance + kListViewHeight);

	BBox *borderBox = new BBox(listViewRect.InsetByCopy(-2, -2));

	backgroundView->AddChild(borderBox);

	listViewRect.OffsetTo(2, 2);
	listViewRect.right -= 1;

	fSettingsTypeListView = new BListView(listViewRect, "List View");

	borderBox->AddChild(fSettingsTypeListView);

	fSettingsContainerBox = new BBox(BRect(kBorderDistance + kListViewWidth + kBorderDistance,
		kBorderDistance, kBorderDistance + kListViewWidth + kBorderDistance + kBoxWidth,
		kBorderDistance + kBoxHeight));

	backgroundView->AddChild(fSettingsContainerBox);

	const float kButtonTop = fSettingsContainerBox->Frame().bottom + kBorderDistance;
	const float kDefaultsButtonLeft = fSettingsContainerBox->Frame().left;
	const float kButtonWidth = 45;
	const float kButtonHeight = 20;

	fDefaultsButton = new BButton(BRect(kDefaultsButtonLeft, kButtonTop,
		 kDefaultsButtonLeft + kButtonWidth, kButtonTop + kButtonHeight),
		 "Defaults", "Defaults", new BMessage(kDefaultsButtonPressed));

	backgroundView->AddChild(fDefaultsButton);

	fDefaultsButton->ResizeToPreferred();
	fDefaultsButton->SetEnabled(true);

	fRevertButton = new BButton(BRect(fDefaultsButton->Frame().right + kBorderDistance,
		kButtonTop, fDefaultsButton->Frame().right + kBorderDistance + kButtonWidth, kButtonTop
		+ kButtonHeight), "Revert", "Revert", new BMessage(kRevertButtonPressed));

	fRevertButton->SetEnabled(false);
	fRevertButton->ResizeToPreferred();
	backgroundView->AddChild(fRevertButton);

	BRect SettingsViewSize = fSettingsContainerBox->Bounds().InsetByCopy(5, 5);

	SettingsViewSize.top += 10;

	fSettingsTypeListView->AddItem(new SettingsItem("Desktop",
		new DesktopSettingsView(SettingsViewSize)));
	fSettingsTypeListView->AddItem(new SettingsItem("Windows",
		new WindowsSettingsView(SettingsViewSize)));
	fSettingsTypeListView->AddItem(new SettingsItem("File Panel",
		new FilePanelSettingsView(SettingsViewSize)));
	fSettingsTypeListView->AddItem(new SettingsItem("Time Format",
		new TimeFormatSettingsView(SettingsViewSize)));
	fSettingsTypeListView->AddItem(new SettingsItem("Trash",
		new TrashSettingsView(SettingsViewSize)));
	fSettingsTypeListView->AddItem(new SettingsItem("Volume Icons",
		new SpaceBarSettingsView(SettingsViewSize)));

	fSettingsTypeListView->SetSelectionMessage(new BMessage(kSettingsViewChanged));

	fSettingsTypeListView->Select(0);
}


bool
TrackerSettingsWindow::QuitRequested()
{
	bool isHidden = false;

	if (Lock()) {
		isHidden = IsHidden();
		Unlock();
	} else
		return true;

	if (isHidden)
		return true;

	Hide();

	return false;
}


void
TrackerSettingsWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kSettingsContentsModified:
			HandleChangedContents();
			break;

		case kDefaultsButtonPressed:
			HandlePressedDefaultsButton();
			break;

		case kRevertButtonPressed:
			HandlePressedRevertButton();
			break;

		case kSettingsViewChanged:
			HandleChangedSettingsView();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}


void
TrackerSettingsWindow::Show()
{
	if (Lock()) {
		int32 itemCount = fSettingsTypeListView->CountItems();

		for (int32 i = 0; i < itemCount; i++) {
			ViewAt(i)->RecordRevertSettings();
			ViewAt(i)->ShowCurrentSettings();
		}

		fSettingsTypeListView->Invalidate();

		Unlock();
	}
	_inherited::Show();
}


SettingsView *
TrackerSettingsWindow::ViewAt(int32 i)
{
	if (!Lock())
		return NULL;
		
	SettingsItem *item = dynamic_cast<SettingsItem*>(fSettingsTypeListView->ItemAt(i));
	
	Unlock();
	
	return item->View();
}


void
TrackerSettingsWindow::HandleChangedContents()
{
	int32 itemCount = fSettingsTypeListView->CountItems();

	bool revertable = false;

	for (int32 i = 0; i < itemCount; i++) {
		revertable |= ViewAt(i)->IsRevertable();
	}

	fSettingsTypeListView->Invalidate();	
	fRevertButton->SetEnabled(revertable);

	TrackerSettings().SaveSettings(false);
}


void
TrackerSettingsWindow::HandlePressedDefaultsButton()
{
	int32 itemCount = fSettingsTypeListView->CountItems();
	
	for (int32 i = 0; i < itemCount; i++)
		ViewAt(i)->SetDefaults();

	HandleChangedContents();
}


void
TrackerSettingsWindow::HandlePressedRevertButton()
{
	int32 itemCount = fSettingsTypeListView->CountItems();

	for (int32 i = 0; i < itemCount; i++) {
		if (ViewAt(i)->IsRevertable())
			ViewAt(i)->Revert();
	}

	HandleChangedContents();
}


void
TrackerSettingsWindow::HandleChangedSettingsView()
{
	int32 currentSelection = fSettingsTypeListView->CurrentSelection();
	if (currentSelection < 0)
		return;

	BView *oldView = fSettingsContainerBox->ChildAt(0);

	if (oldView)
		oldView->RemoveSelf();

	SettingsItem *selectedItem =
		dynamic_cast<SettingsItem*>(fSettingsTypeListView->ItemAt(currentSelection));

	if (selectedItem) {
		fSettingsContainerBox->SetLabel(selectedItem->Text());
		selectedItem->View()->SetViewColor(fSettingsContainerBox->ViewColor());
		fSettingsContainerBox->AddChild(selectedItem->View());
	}
}


//	#pragma mark -


SettingsItem::SettingsItem(const char *label, SettingsView *view)
	: BStringItem(label),
	fSettingsView(view)
{
}


void
SettingsItem::DrawItem(BView *owner, BRect rect, bool drawEverything)
{
	const rgb_color kModifiedColor = {0, 0, 255, 0};
	const rgb_color kBlack = {0, 0, 0, 0};
	const rgb_color kSelectedColor = {140, 140, 140, 0};

	if (fSettingsView) {
		bool isRevertable = fSettingsView->IsRevertable();
		bool isSelected = IsSelected();

		if (isSelected || drawEverything) { 
			rgb_color color; 
			if (isSelected) 
				color = kSelectedColor; 
			else 
				color = owner->ViewColor(); 

			owner->SetHighColor(color); 
			owner->SetLowColor(color); 
			owner->FillRect(rect); 
		}

		if (isRevertable)
			owner->SetHighColor(kModifiedColor);
		else			
			owner->SetHighColor(kBlack);

		owner->MovePenTo(rect.left + 4, rect.bottom - 2);

		owner->DrawString(Text());

		owner->SetHighColor(kBlack);
		owner->SetLowColor(owner->ViewColor());
	}
}


SettingsView *
SettingsItem::View()
{
	return fSettingsView;
}
