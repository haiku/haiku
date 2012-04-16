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
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>

#include "SettingsViews.h"
#include "TrackerSettings.h"
#include "TrackerSettingsWindow.h"

#include <ScrollView.h>


namespace BPrivate {

class SettingsItem : public BStringItem {
	public:
		SettingsItem(const char *label, SettingsView *view);

		void DrawItem(BView *owner, BRect rect, bool drawEverything);

		SettingsView *View();

	private:
		SettingsView *fSettingsView;
};

}	// namespace BPrivate


const uint32 kSettingsViewChanged = 'Svch';
const uint32 kDefaultsButtonPressed = 'Apbp';
const uint32 kRevertButtonPressed = 'Rebp';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "TrackerSettingsWindow"

TrackerSettingsWindow::TrackerSettingsWindow()
	:
	BWindow(BRect(80, 80, 450, 350), B_TRANSLATE("Tracker preferences"),
		B_TITLED_WINDOW, B_NOT_MINIMIZABLE | B_NOT_RESIZABLE
		| B_NO_WORKSPACE_ACTIVATION	| B_NOT_ANCHORED_ON_ACTIVATE
		| B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fSettingsTypeListView = new BListView("List View", B_SINGLE_SELECTION_LIST);

	BScrollView* scrollView = new BScrollView("scrollview",
		fSettingsTypeListView, B_FRAME_EVENTS | B_WILL_DRAW, false, true);

	fDefaultsButton = new BButton("Defaults", B_TRANSLATE("Defaults"),
		new BMessage(kDefaultsButtonPressed));
	fDefaultsButton->SetEnabled(false);

	fRevertButton = new BButton("Revert", B_TRANSLATE("Revert"),
		new BMessage(kRevertButtonPressed));
	fRevertButton->SetEnabled(false);

	fSettingsContainerBox = new BBox("SettingsContainerBox");
	
	const float spacing = be_control_look->DefaultItemSpacing();

	BLayoutBuilder::Group<>(this)
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(scrollView)
			.AddGroup(B_VERTICAL, spacing)
				.Add(fSettingsContainerBox)
				.AddGroup(B_HORIZONTAL, spacing)
					.Add(fDefaultsButton)
					.Add(fRevertButton)
					.AddGlue()
				.End()
			.End()
		.SetInsets(spacing, spacing, spacing, spacing)
		.End();

	fSettingsTypeListView->AddItem(new SettingsItem(B_TRANSLATE("Desktop"),
		new DesktopSettingsView()));
	fSettingsTypeListView->AddItem(new SettingsItem(B_TRANSLATE("Windows"),
		new WindowsSettingsView()));
	fSettingsTypeListView->AddItem(new SettingsItem(B_TRANSLATE("Trash"),
		new TrashSettingsView()));
	fSettingsTypeListView->AddItem(new SettingsItem(B_TRANSLATE("Volume icons"),
		new SpaceBarSettingsView()));

	// constraint the listview width so that the longest item fits
	float width = 0;
	fSettingsTypeListView->GetPreferredSize(&width, NULL);
	width += B_V_SCROLL_BAR_WIDTH;
	fSettingsTypeListView->SetExplicitMinSize(BSize(width, 0));
	fSettingsTypeListView->SetExplicitMaxSize(BSize(width, B_SIZE_UNLIMITED));

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
			_HandleChangedContents();
			break;

		case kDefaultsButtonPressed:
			_HandlePressedDefaultsButton();
			break;

		case kRevertButtonPressed:
			_HandlePressedRevertButton();
			break;

		case kSettingsViewChanged:
			_HandleChangedSettingsView();
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
			_ViewAt(i)->RecordRevertSettings();
			_ViewAt(i)->ShowCurrentSettings();
		}

		fSettingsTypeListView->Invalidate();

		_UpdateButtons();

		Unlock();
	}
	_inherited::Show();
}


SettingsView *
TrackerSettingsWindow::_ViewAt(int32 i)
{
	if (!Lock())
		return NULL;
		
	SettingsItem *item = dynamic_cast<SettingsItem*>(fSettingsTypeListView->ItemAt(i));
	
	Unlock();
	
	return item->View();
}


void
TrackerSettingsWindow::_HandleChangedContents()
{
	fSettingsTypeListView->Invalidate();	
	_UpdateButtons();

	TrackerSettings().SaveSettings(false);
}


void
TrackerSettingsWindow::_UpdateButtons()
{
	int32 itemCount = fSettingsTypeListView->CountItems();

	bool defaultable = false;
	bool revertable = false;

	for (int32 i = 0; i < itemCount; i++) {
		defaultable |= _ViewAt(i)->IsDefaultable();
		revertable |= _ViewAt(i)->IsRevertable();
	}
	
	fDefaultsButton->SetEnabled(defaultable);
	fRevertButton->SetEnabled(revertable);
}


void
TrackerSettingsWindow::_HandlePressedDefaultsButton()
{
	int32 itemCount = fSettingsTypeListView->CountItems();
	
	for (int32 i = 0; i < itemCount; i++) {
		if (_ViewAt(i)->IsDefaultable())
			_ViewAt(i)->SetDefaults();
	}

	_HandleChangedContents();
}


void
TrackerSettingsWindow::_HandlePressedRevertButton()
{
	int32 itemCount = fSettingsTypeListView->CountItems();

	for (int32 i = 0; i < itemCount; i++) {
		if (_ViewAt(i)->IsRevertable())
			_ViewAt(i)->Revert();
	}

	_HandleChangedContents();
}


void
TrackerSettingsWindow::_HandleChangedSettingsView()
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

		BView *view = selectedItem->View();
		view->SetViewColor(fSettingsContainerBox->ViewColor());
		view->Hide();
		fSettingsContainerBox->AddChild(view);

		view->Show();
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

		font_height fheight;
		owner->GetFontHeight(&fheight);

		owner->DrawString(Text(), BPoint(rect.left + 4, rect.top 
			+ fheight.ascent + 2 + floorf(fheight.leading / 2)));

		owner->SetHighColor(kBlack);
		owner->SetLowColor(owner->ViewColor());
	}
}


SettingsView *
SettingsItem::View()
{
	return fSettingsView;
}
