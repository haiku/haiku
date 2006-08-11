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

//#include <CheckBox.h>
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


TrackerSettingsWindow::TrackerSettingsWindow()
	: BWindow(BRect(80, 80, 450, 350), "Tracker Settings", B_TITLED_WINDOW,
		B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_NO_WORKSPACE_ACTIVATION
		| B_NOT_ANCHORED_ON_ACTIVATE | B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE)
{
	BRect rect = Bounds();
	BView *topView = new BView(rect, "Background", B_FOLLOW_ALL, 0);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	rect.InsetBy(10, 10);
	rect.right = rect.left + be_plain_font->StringWidth("Volume Icons")
		+ (float)B_V_SCROLL_BAR_WIDTH + 40.0f;
	fSettingsTypeListView = new BListView(rect, "List View", B_SINGLE_SELECTION_LIST,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	BScrollView* scrollView = new BScrollView("scrollview", fSettingsTypeListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	rect = scrollView->Frame();
	rect.left = rect.right + 10;
	rect.top = rect.bottom;
	fDefaultsButton = new BButton(rect, "Defaults", "Defaults",
		new BMessage(kDefaultsButtonPressed), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fDefaultsButton->ResizeToPreferred();
	fDefaultsButton->MoveBy(0, -fDefaultsButton->Bounds().Height());
	topView->AddChild(fDefaultsButton);

	rect = fDefaultsButton->Frame();
	rect.left = rect.right + 10;
	fRevertButton = new BButton(rect, "Revert", "Revert",
		new BMessage(kRevertButtonPressed), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fRevertButton->SetEnabled(false);
	fRevertButton->ResizeToPreferred();
	topView->AddChild(fRevertButton);

	rect = scrollView->Frame();
	rect.left = rect.right + 10;
	rect.right = Bounds().right - 10;
	rect.bottom = fDefaultsButton->Frame().top - 10;
	fSettingsContainerBox = new BBox(rect, NULL, B_FOLLOW_ALL);
	topView->AddChild(fSettingsContainerBox);

	rect = _SettingsFrame();

	fSettingsTypeListView->AddItem(new SettingsItem("Desktop",
		new DesktopSettingsView(rect)));
	fSettingsTypeListView->AddItem(new SettingsItem("Windows",
		new WindowsSettingsView(rect)));
	fSettingsTypeListView->AddItem(new SettingsItem("Time Format",
		new TimeFormatSettingsView(rect)));
	fSettingsTypeListView->AddItem(new SettingsItem("Trash",
		new TrashSettingsView(rect)));
	fSettingsTypeListView->AddItem(new SettingsItem("Volume Icons",
		new SpaceBarSettingsView(rect)));

	// compute preferred view size

	float minWidth = 0, minHeight = 0;

	for (int32 i = 0; i < fSettingsTypeListView->CountItems(); i++) {
		SettingsView* view = ((SettingsItem *)fSettingsTypeListView->ItemAt(i))->View();

		float width, height;
		view->GetPreferredSize(&width, &height);

		if (minWidth < width)
			minWidth = width;
		if (minHeight < height)
			minHeight = height;
	}

	ResizeBy(max_c(minWidth - rect.Width(), 0), max_c(minHeight - rect.Height(), 0));
		// make sure window is large enough to contain all views

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


BRect
TrackerSettingsWindow::_SettingsFrame()
{
	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);

	BRect rect = fSettingsContainerBox->Bounds().InsetByCopy(8, 8);
	rect.top += ceilf(fontHeight.ascent + fontHeight.descent);

	return rect;
}


void
TrackerSettingsWindow::_HandleChangedContents()
{
	int32 itemCount = fSettingsTypeListView->CountItems();

	bool revertable = false;

	for (int32 i = 0; i < itemCount; i++) {
		revertable |= _ViewAt(i)->IsRevertable();
	}

	fSettingsTypeListView->Invalidate();	
	fRevertButton->SetEnabled(revertable);

	TrackerSettings().SaveSettings(false);
}


void
TrackerSettingsWindow::_HandlePressedDefaultsButton()
{
	int32 itemCount = fSettingsTypeListView->CountItems();
	
	for (int32 i = 0; i < itemCount; i++)
		_ViewAt(i)->SetDefaults();

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

		// Resize view after it has been attached to the window, so that
		// it's resizing modes are respected
		BRect rect = _SettingsFrame();
		view->ResizeTo(rect.Width(), rect.Height());
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
