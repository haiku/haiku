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


#include "GeneralInfoView.h"

#include <algorithm>

#include <Application.h>
#include <Catalog.h>
#include <Locale.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <Roster.h>
#include <Screen.h>
#include <StringFormat.h>
#include <SymLink.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Window.h>

#include "Attributes.h"
#include "Commands.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "InfoWindow.h"
#include "Model.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "StringForSize.h"
#include "Tracker.h"
#include "WidgetAttributeText.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InfoWindow"


namespace BPrivate {

class TrackingView : public BControl {
public:
	TrackingView(BRect, const char* str, BMessage* message);

	virtual void MouseDown(BPoint);
	virtual void MouseMoved(BPoint where, uint32 transit, const BMessage*);
	virtual void MouseUp(BPoint);
	virtual void Draw(BRect);

private:
	bool fMouseDown;
	bool fMouseInView;
};

}	// namespace BPrivate


const float kBorderMargin = 15.0f;
const float kDrawMargin = 3.0f;


const uint32 kSetPreferredApp = 'setp';
const uint32 kSelectNewSymTarget = 'snew';
const uint32 kOpenLinkSource = 'opls';
const uint32 kOpenLinkTarget = 'oplt';


static void
OpenParentAndSelectOriginal(const entry_ref* ref)
{
	BEntry entry(ref);
	node_ref node;
	entry.GetNodeRef(&node);

	BEntry parent;
	entry.GetParent(&parent);
	entry_ref parentRef;
	parent.GetRef(&parentRef);

	BMessage message(B_REFS_RECEIVED);
	message.AddRef("refs", &parentRef);
	message.AddData("nodeRefToSelect", B_RAW_TYPE, &node, sizeof(node_ref));

	be_app->PostMessage(&message);
}


static BWindow*
OpenToolTipWindow(BScreen& screen, BRect rect, const char* name,
	const char* string, BMessenger target, BMessage* message)
{
	font_height fontHeight;
	be_plain_font->GetHeight(&fontHeight);
	float height = ceilf(fontHeight.ascent + fontHeight.descent);
	rect.top = floorf(rect.top + (rect.Height() - height) / 2.0f);
	rect.bottom = rect.top + height;

	rect.right = rect.left + ceilf(be_plain_font->StringWidth(string)) + 4;
	if (rect.left < 0)
		rect.OffsetBy(-rect.left, 0);
	else if (rect.right > screen.Frame().right)
		rect.OffsetBy(screen.Frame().right - rect.right, 0);

	BWindow* window = new BWindow(rect, name, B_BORDERED_WINDOW_LOOK,
		B_FLOATING_ALL_WINDOW_FEEL,
		B_NOT_MOVABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE | B_AVOID_FOCUS | B_NO_WORKSPACE_ACTIVATION
		| B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS);

	TrackingView* trackingView = new TrackingView(window->Bounds(),
		string, message);
	trackingView->SetTarget(target);
	window->AddChild(trackingView);

	window->Sync();
	window->Show();

	return window;
}


GeneralInfoView::GeneralInfoView(Model* model)
	:
	BGroupView(B_VERTICAL),
	fDivider(0),
	fPreferredAppMenu(NULL),
	fModel(model),
	fMouseDown(false),
	fTrackingState(no_track),
	fPathWindow(NULL),
	fLinkWindow(NULL),
	fDescWindow(NULL),
	fCurrentLinkColorWhich(B_LINK_TEXT_COLOR),
	fCurrentPathColorWhich(fCurrentLinkColorWhich)
{
	const char* fieldNames[] = {
		B_TRANSLATE("Description:"),
		B_TRANSLATE("Location:"),
		B_TRANSLATE("Opens with:"),
		B_TRANSLATE("Capacity:"),
		B_TRANSLATE("Size:"),
		B_TRANSLATE("Created:"),
		B_TRANSLATE("Modified:"),
		B_TRANSLATE("Kind:"),
		B_TRANSLATE("Link to:"),
		B_TRANSLATE("Version:"),
		NULL
	};


	SetFlags(Flags() | B_WILL_DRAW | B_PULSE_NEEDED | B_FRAME_EVENTS);
	SetName(B_TRANSLATE("Information"));
	// Set view color to standard background grey
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetFont(be_plain_font);
	fFreeBytes = -1;
	fSizeString = "";
	fSizeRect.Set(0, 0, 0, 0);

	// Find offset for attributes, might be overiden below if there
	// is a prefered handle menu displayed
	BFont currentFont;
	GetFont(&currentFont);
	currentFont.SetSize(currentFont.Size() - 2);

	// The widest string depends on the locale. We should check them all, this
	// is only an approximation that works for English and French.
	float width = 0;
	for (int i = 0; fieldNames[i] != 0; i++)
		width = std::max(width, StringWidth(fieldNames[i]));
	fDivider = width + kBorderMargin + 1;

	// Keep some free space for the stuff we print ourselves
	float lineHeight = CurrentFontHeight();
	int lineCount = 7;
	if (model->IsSymLink())
		lineCount += 1; // Add space for "Link to" line
	if (model->IsExecutable())
		lineCount += 2; // Add space for "Version" and "Description" lines
	GroupLayout()->SetInsets(kBorderMargin, lineHeight * lineCount,
		B_USE_WINDOW_SPACING, B_USE_WINDOW_SPACING);

	// Add a preferred handler pop-up menu if this item
	// is a file...This goes in place of the Link To:
	// string...
	if (model->IsFile()) {
		BMimeType mime(fModel->MimeType());
		BNodeInfo nodeInfo(fModel->Node());

		// But don't add the menu if the file is executable
		if (!fModel->IsExecutable()) {
			fPreferredAppMenu = new BMenuField("", "", new BPopUpMenu(""));
			currentFont.SetSize(currentFont.Size() + 2);
			fDivider = currentFont.StringWidth(B_TRANSLATE("Opens with:"))
				+ 5;
			fPreferredAppMenu->SetDivider(fDivider);
			fDivider += (kBorderMargin - 2);
			fPreferredAppMenu->SetFont(&currentFont);
			fPreferredAppMenu->SetHighUIColor(B_PANEL_TEXT_COLOR);
			fPreferredAppMenu->SetLabel(B_TRANSLATE("Opens with:"));

			char prefSignature[B_MIME_TYPE_LENGTH];
			nodeInfo.GetPreferredApp(prefSignature);

			BMessage supportingAppList;
			mime.GetSupportingApps(&supportingAppList);

			// Add the default menu item and set it to marked
			BMenuItem* result;
			result = new BMenuItem(B_TRANSLATE("Default application"),
				new BMessage(kSetPreferredApp));
			result->SetTarget(this);
			fPreferredAppMenu->Menu()->AddItem(result);
			result->SetMarked(true);

			for (int32 index = 0; ; index++) {
				const char* signature;
				if (supportingAppList.FindString("applications", index,
						&signature) != B_OK) {
					break;
				}

				// Only add separator item if there are more items
				if (index == 0)
					fPreferredAppMenu->Menu()->AddSeparatorItem();

				BMessage* itemMessage = new BMessage(kSetPreferredApp);
				itemMessage->AddString("signature", signature);

				status_t err = B_ERROR;
				entry_ref entry;

				if (signature && signature[0])
					err = be_roster->FindApp(signature, &entry);

				if (err != B_OK)
					result = new BMenuItem(signature, itemMessage);
				else
					result = new BMenuItem(entry.name, itemMessage);

				result->SetTarget(this);
				fPreferredAppMenu->Menu()->AddItem(result);
				if (strcmp(signature, prefSignature) == 0)
					result->SetMarked(true);
			}

			AddChild(fPreferredAppMenu);
		}
	}
}


GeneralInfoView::~GeneralInfoView()
{
	if (fPathWindow->Lock())
		fPathWindow->Quit();

	if (fLinkWindow->Lock())
		fLinkWindow->Quit();

	if (fDescWindow->Lock())
		fDescWindow->Quit();
}


void
GeneralInfoView::InitStrings(const Model* model)
{
	BMimeType mime;
	char kind[B_MIME_TYPE_LENGTH];

	ASSERT(model->IsNodeOpen());

	BRect drawBounds(Bounds());
	drawBounds.left = fDivider;

	// We'll do our own truncation later on in Draw()
	WidgetAttributeText::AttrAsString(model, &fCreatedStr, kAttrStatCreated,
		B_TIME_TYPE, drawBounds.Width() - kBorderMargin, this);
	WidgetAttributeText::AttrAsString(model, &fModifiedStr, kAttrStatModified,
		B_TIME_TYPE, drawBounds.Width() - kBorderMargin, this);
	WidgetAttributeText::AttrAsString(model, &fPathStr, kAttrPath,
		B_STRING_TYPE, 0, this);

	// Use the same method as used to resolve fIconModel, which handles
	// both absolute and relative symlinks. if the link is broken, try to
	// get a little more information.
	if (model->IsSymLink()) {
		bool linked = false;

		Model resolvedModel(model->EntryRef(), true, true);
		if (resolvedModel.InitCheck() == B_OK) {
			// Get the path of the link
			BPath traversedPath;
			resolvedModel.GetPath(&traversedPath);

			// If the BPath is initialized, then check the file for existence
			if (traversedPath.InitCheck() == B_OK) {
				BEntry entry(traversedPath.Path(), false);
					// look at the target itself
				if (entry.InitCheck() == B_OK && entry.Exists())
					linked = true;
			}
		}

		// always show the target as it is: absolute or relative!
		BSymLink symLink(model->EntryRef());
		char linkToPath[B_PATH_NAME_LENGTH];
		symLink.ReadLink(linkToPath, B_PATH_NAME_LENGTH);
		fLinkToStr = linkToPath;
		if (!linked) {
			// link points to missing object
			fLinkToStr += B_TRANSLATE(" (broken)");
		}
	} else if (model->IsExecutable()) {
		if (((Model*)model)->GetLongVersionString(fDescStr,
				B_APP_VERSION_KIND) == B_OK) {
			// we want a flat string, so replace all newlines and tabs
			// with spaces
			fDescStr.ReplaceAll('\n', ' ');
			fDescStr.ReplaceAll('\t', ' ');
		} else
			fDescStr = "-";
	}

	if (mime.SetType(model->MimeType()) == B_OK
		&& mime.GetShortDescription(kind) == B_OK)
		fKindStr = kind;

	if (fKindStr.Length() == 0)
		fKindStr = model->MimeType();
}


void
GeneralInfoView::AttachedToWindow()
{
	BFont font(be_plain_font);

	font.SetSpacing(B_BITMAP_SPACING);
	SetFont(&font);

	CheckAndSetSize();
	if (fPreferredAppMenu)
		fPreferredAppMenu->Menu()->SetTargetForItems(this);

	_inherited::AttachedToWindow();
}


void
GeneralInfoView::Pulse()
{
	CheckAndSetSize();
	_inherited::Pulse();
}


void
GeneralInfoView::ModelChanged(Model* model, BMessage* message)
{
	BRect drawBounds(Bounds());
	drawBounds.left = fDivider;

	switch (message->FindInt32("opcode")) {
		case B_ENTRY_MOVED:
		{
			node_ref dirNode;
			node_ref itemNode;
			dirNode.device = itemNode.device = message->FindInt32("device");
			message->FindInt64("to directory", &dirNode.node);
			message->FindInt64("node", &itemNode.node);

			const char* name;
			if (message->FindString("name", &name) != B_OK)
				return;

			// ensure notification is for us
			if (*model->NodeRef() == itemNode
				// For volumes, the device ID is obviously not handled in a
				// consistent way; the node monitor sends us the ID of the
				// parent device, while the model is set to the device of the
				// volume directly - this hack works for volumes that are
				// mounted in the root directory
				|| (model->IsVolume()
					&& itemNode.device == 1
					&& itemNode.node == model->NodeRef()->node)) {
				model->UpdateEntryRef(&dirNode, name);
				BString title;
				title.SetToFormat(B_TRANSLATE_COMMENT("%s info",
					"window title"), name);
				Window()->SetTitle(title.String());
				WidgetAttributeText::AttrAsString(model, &fPathStr, kAttrPath,
					B_STRING_TYPE, 0, this);
				Invalidate();
			}
			break;
		}

		case B_STAT_CHANGED:
			if (model->OpenNode() == B_OK) {
				WidgetAttributeText::AttrAsString(model, &fCreatedStr,
					kAttrStatCreated, B_TIME_TYPE, drawBounds.Width()
					- kBorderMargin, this);
				WidgetAttributeText::AttrAsString(model, &fModifiedStr,
					kAttrStatModified, B_TIME_TYPE, drawBounds.Width()
					- kBorderMargin, this);

				// don't change the size if it's a directory
				if (!model->IsDirectory()) {
					fLastSize = model->StatBuf()->st_size;
					fSizeString = "";
					BInfoWindow::GetSizeString(fSizeString, fLastSize, 0);
				}
				model->CloseNode();
			}
			break;

		case B_ATTR_CHANGED:
		{
			// watch for icon updates
			const char* attrName;
			if (message->FindString("attr", &attrName) == B_OK) {
				if (strcmp(attrName, kAttrLargeIcon) == 0
					|| strcmp(attrName, kAttrIcon) == 0) {
					IconCache::sIconCache->IconChanged(model->ResolveIfLink());
					Invalidate();
				} else if (strcmp(attrName, kAttrMIMEType) == 0) {
					if (model->OpenNode() == B_OK) {
						model->AttrChanged(attrName);
						InitStrings(model);
						model->CloseNode();
					}
					Invalidate();
				}
			}
			break;
		}

		default:
			break;
	}

	fModel = model;
	if (fModel->IsSymLink()) {
		InitStrings(model);
		Invalidate();
	}

	drawBounds.left = fDivider;
	Invalidate(drawBounds);
}


// This only applies to symlinks. If the target of the symlink
// was changed, then we have to update the entire model.
// (Since in order to re-target a symlink, we had to delete
// the old model and create a new one; BSymLink::SetTarget(),
// would be nice)

void
GeneralInfoView::ReLinkTargetModel(Model* model)
{
	fModel = model;
	InitStrings(model);
	Invalidate(Bounds());
}


void
GeneralInfoView::MouseDown(BPoint where)
{
	// Start tracking the mouse if we are in any of the hotspots
	if (fLinkRect.Contains(where)) {
		InvertRect(fLinkRect);
		fTrackingState = link_track;
	} else if (fPathRect.Contains(where)) {
		InvertRect(fPathRect);
		fTrackingState = path_track;
	} else if (fSizeRect.Contains(where)) {
		if (fModel->IsDirectory() && !fModel->IsVolume()
			&& !fModel->IsRoot()) {
			InvertRect(fSizeRect);
			fTrackingState = size_track;
		} else
			fTrackingState = no_track;
	}

	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
}


void
GeneralInfoView::MouseMoved(BPoint where, uint32, const BMessage* dragMessage)
{
	fCurrentLinkColorWhich = B_LINK_TEXT_COLOR;
	fCurrentPathColorWhich = fCurrentLinkColorWhich;

	switch (fTrackingState) {
		case link_track:
			if (fLinkRect.Contains(where) != fMouseDown) {
				fMouseDown = !fMouseDown;
				InvertRect(fLinkRect);
			}
			break;

		case path_track:
			if (fPathRect.Contains(where) != fMouseDown) {
				fMouseDown = !fMouseDown;
				InvertRect(fPathRect);
			}
			break;

		case size_track:
			if (fSizeRect.Contains(where) != fMouseDown) {
				fMouseDown = !fMouseDown;
				InvertRect(fSizeRect);
			}
			break;

		default:
		{
			// Only consider this if the window is the active window.
			// We have to manually get the mouse here in the event that the
			// mouse is over a pop-up window
			uint32 buttons;
			BPoint point;
			GetMouse(&point, &buttons);
			if (Window()->IsActive() && !buttons) {
				// If we are down here, then that means that we're tracking
				// the mouse but not from a mouse down. In this case, we're
				// just interested in knowing whether or not we need to
				// display the "pop-up" version of the path or link text.
				BScreen screen(Window());
				BFont font;
				GetFont(&font);
				float maxWidth = (Bounds().Width()
					- (fDivider + kBorderMargin));

				if (fPathRect.Contains(point)) {
					if (fCurrentPathColorWhich != B_LINK_HOVER_COLOR)
						fCurrentPathColorWhich = B_LINK_HOVER_COLOR;

					if (font.StringWidth(fPathStr.String()) > maxWidth) {
						fTrackingState = no_track;
						BRect rect = ConvertToScreen(fPathRect);

						if (fPathWindow == NULL
							|| BMessenger(fPathWindow).IsValid() == false) {
							fPathWindow = OpenToolTipWindow(screen, rect,
								"fPathWindow", fPathStr.String(),
								BMessenger(this),
								new BMessage(kOpenLinkSource));
						}
					}
				} else if (fLinkRect.Contains(point)) {

					if (fCurrentLinkColorWhich != B_LINK_HOVER_COLOR)
						fCurrentLinkColorWhich = B_LINK_HOVER_COLOR;

					if (font.StringWidth(fLinkToStr.String()) > maxWidth) {
						fTrackingState = no_track;
						BRect rect = ConvertToScreen(fLinkRect);

						if (!fLinkWindow
							|| BMessenger(fLinkWindow).IsValid() == false) {
							fLinkWindow = OpenToolTipWindow(screen, rect,
								"fLinkWindow", fLinkToStr.String(),
								BMessenger(this),
								new BMessage(kOpenLinkTarget));
						}
					}
				} else if (fDescRect.Contains(point)
					&& font.StringWidth(fDescStr.String()) > maxWidth) {
					fTrackingState = no_track;
					BRect rect = ConvertToScreen(fDescRect);

					if (!fDescWindow
						|| BMessenger(fDescWindow).IsValid() == false) {
						fDescWindow = OpenToolTipWindow(screen, rect,
							"fDescWindow", fDescStr.String(),
							BMessenger(this), NULL);
					}
				}
			}
			break;
		}
	}

	DelayedInvalidate(16666, fPathRect);
	DelayedInvalidate(16666, fLinkRect);
}


void
GeneralInfoView::OpenLinkSource()
{
	OpenParentAndSelectOriginal(fModel->EntryRef());
}


void
GeneralInfoView::OpenLinkTarget()
{
	Model resolvedModel(fModel->EntryRef(), true, true);
	BEntry entry;
	if (resolvedModel.InitCheck() == B_OK) {
		// Get the path of the link
		BPath traversedPath;
		resolvedModel.GetPath(&traversedPath);

		// If the BPath is initialized, then check the file for existence
		if (traversedPath.InitCheck() == B_OK)
			entry.SetTo(traversedPath.Path());
	}
	if (entry.InitCheck() != B_OK || !entry.Exists()) {
		// Open a file dialog panel to allow the user to relink.
		BInfoWindow* window = dynamic_cast<BInfoWindow*>(Window());
		if (window != NULL)
			window->OpenFilePanel(fModel->EntryRef());
	} else {
		entry_ref ref;
		entry.GetRef(&ref);
		BPath path(&ref);
		printf("Opening link target: %s\n", path.Path());
		OpenParentAndSelectOriginal(&ref);
	}
}


void
GeneralInfoView::MouseUp(BPoint where)
{
	// Are we in the link rect?
	if (fTrackingState == link_track && fLinkRect.Contains(where)) {
		InvertRect(fLinkRect);
		OpenLinkTarget();
	} else if (fTrackingState == path_track && fPathRect.Contains(where)) {
		InvertRect(fPathRect);
		OpenLinkSource();
	} else if (fTrackingState == size_track && fSizeRect.Contains(where)) {
		// Recalculate size
		Window()->PostMessage(kRecalculateSize);
	}

	// End mouse tracking
	fMouseDown = false;
	fTrackingState = no_track;
}


void
GeneralInfoView::CheckAndSetSize()
{
	if (fModel->IsVolume() || fModel->IsRoot()) {
		off_t freeBytes = 0;
		off_t capacity = 0;

		if (fModel->IsVolume()) {
			BVolume volume(fModel->NodeRef()->device);
			freeBytes = volume.FreeBytes();
			capacity = volume.Capacity();
		} else {
			// iterate over all volumes
			BVolumeRoster volumeRoster;
			BVolume volume;
			while (volumeRoster.GetNextVolume(&volume) == B_OK) {
				freeBytes += volume.FreeBytes();
				capacity += volume.Capacity();
			}
		}

		if (fFreeBytes == freeBytes)
			return;

		fFreeBytes = freeBytes;

		fSizeString.SetTo(B_TRANSLATE("%capacity (%used used -- %free free)"));

		char sizeStr[128];
		string_for_size(capacity, sizeStr, sizeof(sizeStr));
		fSizeString.ReplaceFirst("%capacity", sizeStr);
		string_for_size(capacity - fFreeBytes, sizeStr, sizeof(sizeStr));
		fSizeString.ReplaceFirst("%used", sizeStr);
		string_for_size(fFreeBytes, sizeStr, sizeof(sizeStr));
		fSizeString.ReplaceFirst("%free", sizeStr);

	} else if (fModel->IsFile()) {
		// poll for size changes because they do not get node monitored
		// until a file gets closed (with the old BFS)
		StatStruct statBuf;
		BModelOpener opener(fModel);

		if (fModel->InitCheck() != B_OK
			|| fModel->Node()->GetStat(&statBuf) != B_OK) {
			return;
		}

		if (fLastSize == statBuf.st_size)
			return;

		fLastSize = statBuf.st_size;
		fSizeString = "";
		BInfoWindow::GetSizeString(fSizeString, fLastSize, 0);
	} else
		return;

	SetSizeString(fSizeString);
}


void
GeneralInfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kSetPreferredApp:
		{
			BNode node(fModel->EntryRef());
			BNodeInfo nodeInfo(&node);

			const char* newSignature;
			if (message->FindString("signature", &newSignature) != B_OK)
				newSignature = NULL;

			fModel->SetPreferredAppSignature(newSignature);
			nodeInfo.SetPreferredApp(newSignature);
			break;
		}

		case kOpenLinkSource:
			OpenLinkSource();
			break;

		case kOpenLinkTarget:
			OpenLinkTarget();
			break;

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
GeneralInfoView::FrameResized(float, float)
{
	BModelOpener opener(fModel);

	// Truncate the strings according to the new width
	InitStrings(fModel);
}


void
GeneralInfoView::Draw(BRect)
{
	// Set the low color for anti-aliasing
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Clear the old contents
	SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	FillRect(Bounds());

	rgb_color labelColor = ui_color(B_PANEL_TEXT_COLOR);
	rgb_color attributeColor = mix_color(HighColor(), labelColor, 192);

	// Font information
	font_height fontMetrics;
	float lineHeight = 0;
	float lineBase = 0;
	// Draw the attribute font stuff
	SetFont(be_plain_font);
	GetFontHeight(&fontMetrics);
	lineHeight = CurrentFontHeight() + 5;

	// Starting base line for the first string
	lineBase = lineHeight;

	// Capacity/size
	SetHighColor(labelColor);
	if (fModel->IsVolume() || fModel->IsRoot()) {
		MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Capacity:"))),
			lineBase));
		DrawString(B_TRANSLATE("Capacity:"));
	} else {
		MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Size:"))),
			lineBase));
		fSizeRect.left = fDivider + 2;
		fSizeRect.top = lineBase - fontMetrics.ascent;
		fSizeRect.bottom = lineBase + fontMetrics.descent;
		DrawString(B_TRANSLATE("Size:"));
	}

	MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
	SetHighColor(attributeColor);
	// Check for possible need of truncation
	if (StringWidth(fSizeString.String())
			> (Bounds().Width() - (fDivider + kBorderMargin))) {
		BString tmpString(fSizeString.String());
		TruncateString(&tmpString, B_TRUNCATE_MIDDLE,
			Bounds().Width() - (fDivider + kBorderMargin));
		DrawString(tmpString.String());
		fSizeRect.right = fSizeRect.left + StringWidth(tmpString.String())
			+ 3;
	} else {
		DrawString(fSizeString.String());
		fSizeRect.right = fSizeRect.left + StringWidth(fSizeString.String()) + 3;
	}
	lineBase += lineHeight;

	// Created
	SetHighColor(labelColor);
	MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Created:"))),
		lineBase));
	DrawString(B_TRANSLATE("Created:"));
	MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
	SetHighColor(attributeColor);
	DrawString(fCreatedStr.String());
	lineBase += lineHeight;

	// Modified
	MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Modified:"))),
		lineBase));
	SetHighColor(labelColor);
	DrawString(B_TRANSLATE("Modified:"));
	MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
	SetHighColor(attributeColor);
	DrawString(fModifiedStr.String());
	lineBase += lineHeight;

	// Kind
	MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Kind:"))),
		lineBase));
	SetHighColor(labelColor);
	DrawString(B_TRANSLATE("Kind:"));
	MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
	SetHighColor(attributeColor);
	DrawString(fKindStr.String());
	lineBase += lineHeight;

	BFont normalFont;
	GetFont(&normalFont);

	// Path
	MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Location:"))),
		lineBase));
	SetHighColor(labelColor);
	DrawString(B_TRANSLATE("Location:"));

	MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
	SetHighUIColor(fCurrentPathColorWhich);

	// Check for truncation
	if (StringWidth(fPathStr.String()) > (Bounds().Width()
			- (fDivider + kBorderMargin))) {
		BString nameString(fPathStr.String());
		TruncateString(&nameString, B_TRUNCATE_MIDDLE,
			Bounds().Width() - (fDivider + kBorderMargin));
		DrawString(nameString.String());
	} else
		DrawString(fPathStr.String());

	// Cache the position of the path
	fPathRect.top = lineBase - fontMetrics.ascent;
	fPathRect.bottom = lineBase + fontMetrics.descent;
	fPathRect.left = fDivider + 2;
	fPathRect.right = fPathRect.left + StringWidth(fPathStr.String()) + 3;

	lineBase += lineHeight;

	// Link to/version
	if (fModel->IsSymLink()) {
		MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Link to:"))),
			lineBase));
		SetHighColor(labelColor);
		DrawString(B_TRANSLATE("Link to:"));
		MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
		SetHighUIColor(fCurrentLinkColorWhich);

		// Check for truncation
		if (StringWidth(fLinkToStr.String()) > (Bounds().Width()
				- (fDivider + kBorderMargin))) {
			BString nameString(fLinkToStr.String());
			TruncateString(&nameString, B_TRUNCATE_MIDDLE,
				Bounds().Width() - (fDivider + kBorderMargin));
			DrawString(nameString.String());
		} else
			DrawString(fLinkToStr.String());

		// Cache the position of the link field
		fLinkRect.top = lineBase - fontMetrics.ascent;
		fLinkRect.bottom = lineBase + fontMetrics.descent;
		fLinkRect.left = fDivider + 2;
		fLinkRect.right = fLinkRect.left + StringWidth(fLinkToStr.String())
			+ 3;

		// No description field
		fDescRect = BRect(-1, -1, -1, -1);
	} else if (fModel->IsExecutable()) {
		//Version
		MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Version:"))),
			lineBase));
		SetHighColor(labelColor);
		DrawString(B_TRANSLATE("Version:"));
		MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
		SetHighColor(attributeColor);
		BString nameString;
		if (fModel->GetVersionString(nameString, B_APP_VERSION_KIND) == B_OK)
			DrawString(nameString.String());
		else
			DrawString("-");
		lineBase += lineHeight;

		// Description
		MovePenTo(BPoint(fDivider - (StringWidth(B_TRANSLATE("Description:"))),
			lineBase));
		SetHighColor(labelColor);
		DrawString(B_TRANSLATE("Description:"));
		MovePenTo(BPoint(fDivider + kDrawMargin, lineBase));
		SetHighColor(attributeColor);
		// Check for truncation
		if (StringWidth(fDescStr.String()) > (Bounds().Width()
				- (fDivider + kBorderMargin))) {
			BString nameString(fDescStr.String());
			TruncateString(&nameString, B_TRUNCATE_MIDDLE,
				Bounds().Width() - (fDivider + kBorderMargin));
			DrawString(nameString.String());
		} else
			DrawString(fDescStr.String());

		// Cache the position of the description field
		fDescRect.top = lineBase - fontMetrics.ascent;
		fDescRect.bottom = lineBase + fontMetrics.descent;
		fDescRect.left = fDivider + 2;
		fDescRect.right = fDescRect.left + StringWidth(fDescStr.String()) + 3;

		// No link field
		fLinkRect = BRect(-1, -1, -1, -1);
	}
}


void
GeneralInfoView::WindowActivated(bool active)
{
	if (active)
		return;

	if (fPathWindow->Lock()) {
		fPathWindow->Quit();
		fPathWindow = NULL;
	}

	if (fLinkWindow->Lock()) {
		fLinkWindow->Quit();
		fLinkWindow = NULL;
	}

	if (fDescWindow->Lock()) {
		fDescWindow->Quit();
		fDescWindow = NULL;
	}
}


float
GeneralInfoView::CurrentFontHeight()
{
	BFont font;
	GetFont(&font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	return fontHeight.ascent + fontHeight.descent + fontHeight.leading + 2;
}


off_t
GeneralInfoView::LastSize() const
{
	return fLastSize;
}


void
GeneralInfoView::SetLastSize(off_t lastSize)
{
	fLastSize = lastSize;
}


void
GeneralInfoView::SetSizeString(const char* sizeString)
{
	fSizeString = sizeString;

	float lineHeight = CurrentFontHeight() + 6;
	BRect bounds(fDivider, 0, Bounds().right, lineHeight);
	Invalidate(bounds);
}


//	#pragma mark -


TrackingView::TrackingView(BRect frame, const char* str, BMessage* message)
	: BControl(frame, "trackingView", str, message, B_FOLLOW_ALL,
		B_WILL_DRAW),
	fMouseDown(false),
	fMouseInView(false)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetEventMask(B_POINTER_EVENTS, 0);
}


void
TrackingView::MouseDown(BPoint)
{
	if (Message() != NULL) {
		fMouseDown = true;
		fMouseInView = true;
		InvertRect(Bounds());
	}
}


void
TrackingView::MouseMoved(BPoint, uint32 transit, const BMessage*)
{
	if ((transit == B_ENTERED_VIEW || transit == B_EXITED_VIEW) && fMouseDown)
		InvertRect(Bounds());

	fMouseInView = (transit == B_ENTERED_VIEW || transit == B_INSIDE_VIEW);
	DelayedInvalidate(16666, Bounds());
	if (!fMouseInView && !fMouseDown)
		Window()->Close();
}


void
TrackingView::MouseUp(BPoint)
{
	if (Message() != NULL) {
		if (fMouseInView)
			Invoke();

		fMouseDown = false;
		Window()->Close();
	}
}


void
TrackingView::Draw(BRect)
{
	if (Message() != NULL)
		SetHighUIColor(fMouseInView ? B_LINK_HOVER_COLOR
			: B_LINK_TEXT_COLOR);
	else
		SetHighUIColor(B_PANEL_TEXT_COLOR);
	SetLowColor(ViewColor());

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	DrawString(Label(), BPoint(3, Bounds().Height() - fontHeight.descent));
}
