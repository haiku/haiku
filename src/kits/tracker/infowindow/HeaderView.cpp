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


#include "HeaderView.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "Commands.h"
#include "FSUtils.h"
#include "GeneralInfoView.h"
#include "IconMenuItem.h"
#include "Model.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "Tracker.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InfoWindow"


// Offsets taken from TAlertView::Draw in BAlert.cpp
const float kIconHorizOffset = 18.0f;
const float kIconVertOffset = 6.0f;
const float kBorderWidth = 32.0f;

// Amount you have to move the mouse before a drag starts
const float kDragSlop = 3.0f;


HeaderView::HeaderView(Model* model)
	:
	BView("header", B_WILL_DRAW),
	fModel(model),
	fIconModel(model),
	fTitleEditView(NULL),
	fTrackingState(no_track),
	fMouseDown(false),
	fIsDropTarget(false),
	fDoubleClick(false),
	fDragging(false)
{
	// Create the rect for displaying the icon
	fIconRect.Set(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1);
	// Offset taken from BAlert
	fIconRect.OffsetBy(kIconHorizOffset, kIconVertOffset);
	SetExplicitSize(BSize(B_SIZE_UNSET, B_LARGE_ICON + 2 * kIconVertOffset));

	// The title rect
	// The magic numbers are used to properly calculate the rect so that
	// when the editing text view is displayed, the position of the text
	// does not change.
	BFont currentFont;
	font_height fontMetrics;
	GetFont(&currentFont);
	currentFont.GetHeight(&fontMetrics);

	fTitleRect.left = fIconRect.right + 5;
	fTitleRect.top = 0;
	fTitleRect.bottom = fontMetrics.ascent + 1;
	fTitleRect.right = min_c(
		fTitleRect.left + currentFont.StringWidth(fModel->Name()),
		Bounds().Width() - 5);
	// Offset so that it centers with the icon
	fTitleRect.OffsetBy(0,
		fIconRect.top + ((fIconRect.Height() - fTitleRect.Height()) / 2));
	// Make some room for the border for when we are in edit mode
	// (Negative numbers increase the size of the rect)
	fTitleRect.InsetBy(-1, -2);

	// If the model is a symlink, then we deference the model to
	// get the targets icon
	if (fModel->IsSymLink()) {
		Model* resolvedModel = new Model(model->EntryRef(), true, true);
		if (resolvedModel->InitCheck() == B_OK)
			fIconModel = resolvedModel;
		// broken link, just show the symlink
		else
			delete resolvedModel;
	}

}


HeaderView::~HeaderView()
{
	if (fIconModel != fModel)
		delete fIconModel;
}


void
HeaderView::ModelChanged(Model* model, BMessage* message)
{
	// Update the icon stuff
	if (fIconModel != fModel) {
		delete fIconModel;
		fIconModel = NULL;
	}

	fModel = model;
	if (fModel->IsSymLink()) {
		// if we are looking at a symlink, deference the model and look
		// at the target
		Model* resolvedModel = new Model(model->EntryRef(), true, true);
		if (resolvedModel->InitCheck() == B_OK) {
			if (fIconModel != fModel)
				delete fIconModel;
			fIconModel = resolvedModel;
		} else {
			fIconModel = model;
			delete resolvedModel;
		}
	}

	Invalidate();
}


void
HeaderView::ReLinkTargetModel(Model* model)
{
	fModel = model;
	if (fModel->IsSymLink()) {
		Model* resolvedModel = new Model(model->EntryRef(), true, true);
		if (resolvedModel->InitCheck() == B_OK) {
			if (fIconModel != fModel)
				delete fIconModel;
			fIconModel = resolvedModel;
		} else {
			fIconModel = fModel;
			delete resolvedModel;
		}
	}
	Invalidate();
}


void
HeaderView::BeginEditingTitle()
{
	if (fTitleEditView != NULL)
		return;

	BFont font(be_plain_font);
	font.SetSize(font.Size() + 2);
	BRect textFrame(fTitleRect);
	textFrame.right = Bounds().Width() - 5;
	BRect textRect(textFrame);
	textRect.OffsetTo(0, 0);
	textRect.InsetBy(1, 1);

	// Just make it some really large size, since we don't do any line
	// wrapping. The text filter will make sure to scroll the cursor
	// into position

	textRect.right = 2000;
	fTitleEditView = new BTextView(textFrame, "text_editor",
		textRect, &font, 0, B_FOLLOW_ALL, B_WILL_DRAW);
	fTitleEditView->SetText(fModel->Name());
	DisallowFilenameKeys(fTitleEditView);

	// Reset the width of the text rect
	textRect = fTitleEditView->TextRect();
	textRect.right = fTitleEditView->LineWidth() + 20;
	fTitleEditView->SetTextRect(textRect);
	fTitleEditView->SetWordWrap(false);
	// Add filter for catching B_RETURN and B_ESCAPE key's
	fTitleEditView->AddFilter(
		new BMessageFilter(B_KEY_DOWN, HeaderView::TextViewFilter));

	BScrollView* scrollView = new BScrollView("BorderView", fTitleEditView,
		0, 0, false, false, B_PLAIN_BORDER);
	AddChild(scrollView);
	fTitleEditView->SelectAll();
	fTitleEditView->MakeFocus();

	Window()->UpdateIfNeeded();
}


void
HeaderView::FinishEditingTitle(bool commit)
{
	if (fTitleEditView == NULL)
		return;

	bool reopen = false;

	const char* text = fTitleEditView->Text();
	uint32 length = strlen(text);
	if (commit && strcmp(text, fModel->Name()) != 0
		&& length < B_FILE_NAME_LENGTH) {
		BEntry entry(fModel->EntryRef());
		BDirectory parent;
		if (entry.InitCheck() == B_OK
			&& entry.GetParent(&parent) == B_OK) {
			if (parent.Contains(text)) {
				BAlert* alert = new BAlert("",
					B_TRANSLATE("That name is already taken. "
					"Please type another one."),
					B_TRANSLATE("OK"),
					0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go();
				reopen = true;
			} else {
				if (fModel->IsVolume()) {
					BVolume	volume(fModel->NodeRef()->device);
					if (volume.InitCheck() == B_OK)
						volume.SetName(text);
				} else
					entry.Rename(text);

				// Adjust the size of the text rect
				BFont currentFont(be_plain_font);
				currentFont.SetSize(currentFont.Size() + 2);
				fTitleRect.right = min_c(fTitleRect.left
						+ currentFont.StringWidth(fTitleEditView->Text()),
					Bounds().Width() - 5);
			}
		}
	} else if (length >= B_FILE_NAME_LENGTH) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("That name is too long. Please type another one."),
			B_TRANSLATE("OK"),
			0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go();
		reopen = true;
	}

	// Remove view
	BView* scrollView = fTitleEditView->Parent();
	RemoveChild(scrollView);
	delete scrollView;
	fTitleEditView = NULL;

	if (reopen)
		BeginEditingTitle();
}


void
HeaderView::Draw(BRect)
{
	// Set the low color for anti-aliasing
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Clear the old contents
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());

	rgb_color labelColor = ui_color(B_PANEL_TEXT_COLOR);

	// Draw the icon, straddling the border
	SetDrawingMode(B_OP_OVER);
	IconCache::sIconCache->Draw(fIconModel, this, fIconRect.LeftTop(),
		kNormalIcon, B_LARGE_ICON, true);
	SetDrawingMode(B_OP_COPY);

	// Font information
	font_height fontMetrics;
	BFont currentFont;
	float lineBase = 0;

	// Draw the main title if the user is not currently editing it
	if (fTitleEditView == NULL) {
		SetFont(be_bold_font);
		SetFontSize(be_bold_font->Size());
		GetFont(&currentFont);
		currentFont.GetHeight(&fontMetrics);
		lineBase = fTitleRect.bottom - fontMetrics.descent;
		SetHighColor(labelColor);
		MovePenTo(BPoint(fIconRect.right + 6, lineBase));

		// Recalculate the rect width
		fTitleRect.right = min_c(
				fTitleRect.left + currentFont.StringWidth(fModel->Name()),
			Bounds().Width() - 5);
		// Check for possible need of truncation
		if (StringWidth(fModel->Name()) > fTitleRect.Width()) {
			BString nameString(fModel->Name());
			TruncateString(&nameString, B_TRUNCATE_END,
				fTitleRect.Width() - 2);
			DrawString(nameString.String());
		} else
			DrawString(fModel->Name());
	}

}


void
HeaderView::MakeFocus(bool focus)
{
	if (!focus && fTitleEditView != NULL)
		FinishEditingTitle(true);
}


void
HeaderView::WindowActivated(bool active)
{
	if (active)
		return;

	if (fTitleEditView != NULL)
		FinishEditingTitle(true);
}


void
HeaderView::MouseDown(BPoint where)
{
	// Assume this isn't part of a double click
	fDoubleClick = false;

	BEntry entry;
	fModel->GetEntry(&entry);

	if (fTitleRect.Contains(where)) {
		if (!fModel->HasLocalizedName()
			&& ConfirmChangeIfWellKnownDirectory(&entry, kRename, true)) {
			BeginEditingTitle();
		}
	} else if (fTitleEditView) {
		FinishEditingTitle(true);
	} else if (fIconRect.Contains(where)) {
		uint32 buttons;
		Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
		if (SecondaryMouseButtonDown(modifiers(), buttons)) {
			// Show contextual menu
			BPopUpMenu* contextMenu
				= new BPopUpMenu("FileContext", false, false);
			if (contextMenu) {
				BuildContextMenu(contextMenu);
				contextMenu->SetAsyncAutoDestruct(true);
				contextMenu->Go(ConvertToScreen(where), true, true,
					ConvertToScreen(fIconRect));
			}
		} else {
			// Check to see if the point is actually on part of the icon,
			// versus just in the container rect. The icons are always
			// the large version
			BPoint offsetPoint;
			offsetPoint.x = where.x - fIconRect.left;
			offsetPoint.y = where.y - fIconRect.top;
			if (IconCache::sIconCache->IconHitTest(offsetPoint, fIconModel,
					kNormalIcon, B_LARGE_ICON)) {
				// Can't drag the trash anywhere..
				fTrackingState = fModel->IsTrash()
					? open_only_track : icon_track;

				// Check for possible double click
				if (abs((int32)(fClickPoint.x - where.x)) < kDragSlop
					&& abs((int32)(fClickPoint.y - where.y)) < kDragSlop) {
					int32 clickCount;
					Window()->CurrentMessage()->FindInt32("clicks",
						&clickCount);

					// This checks the* previous* click point
					if (clickCount == 2) {
						offsetPoint.x = fClickPoint.x - fIconRect.left;
						offsetPoint.y = fClickPoint.y - fIconRect.top;
						fDoubleClick
							= IconCache::sIconCache->IconHitTest(offsetPoint,
							fIconModel, kNormalIcon, B_LARGE_ICON);
					}
				}
			}
		}
	}

	fClickPoint = where;
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
HeaderView::MouseMoved(BPoint where, uint32, const BMessage* dragMessage)
{
	if (dragMessage != NULL && dragMessage->ReturnAddress() != BMessenger(this)
		&& dragMessage->what == B_SIMPLE_DATA
		&& BPoseView::CanHandleDragSelection(fModel, dragMessage,
			(modifiers() & B_CONTROL_KEY) != 0)) {
		// highlight drag target
		bool overTarget = fIconRect.Contains(where);
		SetDrawingMode(B_OP_OVER);
		if (overTarget != fIsDropTarget) {
			IconCache::sIconCache->Draw(fIconModel, this, fIconRect.LeftTop(),
				overTarget ? kSelectedIcon : kNormalIcon, B_LARGE_ICON, true);
			fIsDropTarget = overTarget;
		}
	}

	switch (fTrackingState) {
		case icon_track:
			if (fMouseDown && !fDragging
				&& (abs((int32)(where.x - fClickPoint.x)) > kDragSlop
					|| abs((int32)(where.y - fClickPoint.y)) > kDragSlop)) {
				// Find the required height
				BFont font;
				GetFont(&font);

				float height = CurrentFontHeight()
					+ fIconRect.Height() + 8;
				BRect rect(0, 0, min_c(fIconRect.Width()
						+ font.StringWidth(fModel->Name()) + 4,
					fIconRect.Width() * 3), height);
				BBitmap* dragBitmap = new BBitmap(rect, B_RGBA32, true);
				dragBitmap->Lock();
				BView* view = new BView(dragBitmap->Bounds(), "",
					B_FOLLOW_NONE, 0);
				dragBitmap->AddChild(view);
				view->SetOrigin(0, 0);
				BRect clipRect(view->Bounds());
				BRegion newClip;
				newClip.Set(clipRect);
				view->ConstrainClippingRegion(&newClip);

				// Transparent draw magic
				view->SetHighColor(0, 0, 0, 0);
				view->FillRect(view->Bounds());
				view->SetDrawingMode(B_OP_ALPHA);
				rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
				textColor.alpha = 128;
					// set transparency by value
				view->SetHighColor(textColor);
				view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);

				// Draw the icon
				float hIconOffset = (rect.Width() - fIconRect.Width()) / 2;
				IconCache::sIconCache->Draw(fIconModel, view,
					BPoint(hIconOffset, 0), kNormalIcon, B_LARGE_ICON, true);

				// See if we need to truncate the string
				BString nameString(fModel->Name());
				if (view->StringWidth(fModel->Name()) > rect.Width()) {
					view->TruncateString(&nameString, B_TRUNCATE_END,
						rect.Width() - 5);
				}

				// Draw the label
				font_height fontHeight;
				font.GetHeight(&fontHeight);
				float leftText = (view->StringWidth(nameString.String())
					- fIconRect.Width()) / 2;
				view->MovePenTo(BPoint(hIconOffset - leftText + 2,
					fIconRect.Height() + (fontHeight.ascent + 2)));
				view->DrawString(nameString.String());

				view->Sync();
				dragBitmap->Unlock();

				BMessage dragMessage(B_REFS_RECEIVED);
				dragMessage.AddPoint("click_pt", fClickPoint);
				BPoint tmpLoc;
				uint32 button;
				GetMouse(&tmpLoc, &button);
				if (button)
					dragMessage.AddInt32("buttons", (int32)button);

				dragMessage.AddInt32("be:actions",
					(modifiers() & B_OPTION_KEY) != 0
						? B_COPY_TARGET : B_MOVE_TARGET);
				dragMessage.AddRef("refs", fModel->EntryRef());
				DragMessage(&dragMessage, dragBitmap, B_OP_ALPHA,
					BPoint((fClickPoint.x - fIconRect.left)
					+ hIconOffset, fClickPoint.y - fIconRect.top), this);
				fDragging = true;
			}
			break;

		case open_only_track :
			// Special type of entry that can't be renamed or drag and dropped
			// It can only be opened by double clicking on the icon
			break;

		case no_track:
			// No mouse tracking, do nothing
			break;
	}
}


void
HeaderView::MouseUp(BPoint where)
{
	if ((fTrackingState == icon_track
			|| fTrackingState == open_only_track)
		&& fIconRect.Contains(where)) {
		// If it was a double click, then tell Tracker to open the item
		// The CurrentMessage() here does* not* have a "clicks" field,
		// which is why we are tracking the clicks with this temp var
		if (fDoubleClick) {
			// Double click, launch.
			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", fModel->EntryRef());

			// add a messenger to the launch message that will be used to
			// dispatch scripting calls from apps to the PoseView
			message.AddMessenger("TrackerViewToken", BMessenger(this));
			be_app->PostMessage(&message);
			fDoubleClick = false;
		}
	}

	// End mouse tracking
	fMouseDown = false;
	fDragging = false;
	fTrackingState = no_track;
}


void
HeaderView::MessageReceived(BMessage* message)
{
	if (message->WasDropped()
		&& message->what == B_SIMPLE_DATA
		&& message->ReturnAddress() != BMessenger(this)
		&& fIconRect.Contains(ConvertFromScreen(message->DropPoint()))
		&& BPoseView::CanHandleDragSelection(fModel, message,
			(modifiers() & B_CONTROL_KEY) != 0)) {
		BPoseView::HandleDropCommon(message, fModel, 0, this,
			message->DropPoint());
		Invalidate(fIconRect);
		return;
	}

	BView::MessageReceived(message);
}



status_t
HeaderView::BuildContextMenu(BMenu* parent)
{
	if (parent == NULL)
		return B_BAD_VALUE;

	// Add navigation menu if this is not a symlink
	// Symlink's to directories are OK however!
	BEntry entry(fModel->EntryRef());
	entry_ref ref;
	entry.GetRef(&ref);
	Model model(&entry);
	bool navigate = false;
	if (model.InitCheck() == B_OK) {
		if (model.IsSymLink()) {
			// Check if it's to a directory
			if (entry.SetTo(model.EntryRef(), true) == B_OK) {
				navigate = entry.IsDirectory();
				entry.GetRef(&ref);
			}
		} else if (model.IsDirectory() || model.IsVolume())
			navigate = true;
	}
	ModelMenuItem* navigationItem = NULL;
	if (navigate) {
		navigationItem = new ModelMenuItem(new Model(model),
			new BNavMenu(model.Name(), B_REFS_RECEIVED, be_app, Window()));

		// setup a navigation menu item which will dynamically load items
		// as menu items are traversed
		BNavMenu* navMenu = dynamic_cast<BNavMenu*>(navigationItem->Submenu());
		if (navMenu != NULL)
			navMenu->SetNavDir(&ref);

		navigationItem->SetLabel(model.Name());
		navigationItem->SetEntry(&entry);

		parent->AddItem(navigationItem, 0);
		parent->AddItem(new BSeparatorItem(), 1);

		BMessage* message = new BMessage(B_REFS_RECEIVED);
		message->AddRef("refs", &ref);
		navigationItem->SetMessage(message);
		navigationItem->SetTarget(be_app);
	}

	parent->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));

	if (!model.IsDesktop() && !model.IsRoot() && !model.IsTrash()
		&& !fModel->HasLocalizedName()) {
		parent->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
			new BMessage(kEditItem), 'E'));
		parent->AddSeparatorItem();

		if (fModel->IsVolume()) {
			BMenuItem* item = new BMenuItem(B_TRANSLATE("Unmount"),
				new BMessage(kUnmountVolume), 'U');
			parent->AddItem(item);
			// volume model, enable/disable the Unmount item
			BVolume boot;
			BVolumeRoster().GetBootVolume(&boot);
			BVolume volume;
			volume.SetTo(fModel->NodeRef()->device);
			if (volume == boot)
				item->SetEnabled(false);
		}
	}

	if (!model.IsRoot() && !model.IsVolume() && !model.IsTrash())
		parent->AddItem(new BMenuItem(B_TRANSLATE("Identify"),
			new BMessage(kIdentifyEntry)));

	if (model.IsTrash())
		parent->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
			new BMessage(kEmptyTrash)));

	BMenuItem* sizeItem = NULL;
	if (model.IsDirectory() && !model.IsVolume() && !model.IsRoot())  {
		parent->AddItem(sizeItem
				= new BMenuItem(B_TRANSLATE("Recalculate folder size"),
			new BMessage(kRecalculateSize)));
	}

	if (model.IsSymLink()) {
		parent->AddItem(sizeItem
				= new BMenuItem(B_TRANSLATE("Set new link target"),
			new BMessage(kSetLinkTarget)));
	}

	parent->AddItem(new BSeparatorItem());
	parent->AddItem(new BMenuItem(B_TRANSLATE("Permissions"),
		new BMessage(kPermissionsSelected), 'P'));

	parent->SetFont(be_plain_font);
	parent->SetTargetForItems(this);

	// Reset the nav menu to be_app
	if (navigate)
		navigationItem->SetTarget(be_app);
	if (sizeItem)
		sizeItem->SetTarget(Window());

	return B_OK;
}


filter_result
HeaderView::TextViewFilter(BMessage* message, BHandler**,
	BMessageFilter* filter)
{
	uchar key;
	HeaderView* attribView = static_cast<HeaderView*>(
		static_cast<BWindow*>(filter->Looper())->FindView("header"));

	// Adjust the size of the text rect
	BRect nuRect(attribView->TextView()->TextRect());
	nuRect.right = attribView->TextView()->LineWidth() + 20;
	attribView->TextView()->SetTextRect(nuRect);

	// Make sure the cursor is in view
	attribView->TextView()->ScrollToSelection();
	if (message->FindInt8("byte", (int8*)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	if (key == B_RETURN || key == B_ESCAPE) {
		attribView->FinishEditingTitle(key == B_RETURN);
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


float
HeaderView::CurrentFontHeight()
{
	BFont font;
	GetFont(&font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	return fontHeight.ascent + fontHeight.descent + fontHeight.leading + 2;
}


