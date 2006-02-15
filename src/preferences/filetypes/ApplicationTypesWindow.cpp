/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

// TODO: think about adopting Tracker's info window style here (pressable path)

#include "ApplicationTypesWindow.h"
#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "MimeTypeListView.h"
#include "StringView.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <Roster.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdio.h>


const uint32 kMsgTypeSelected = 'typs';
const uint32 kMsgRemoveUninstalled = 'runs';


ApplicationTypesWindow::ApplicationTypesWindow(BRect frame)
	: BWindow(frame, "Application Types", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// Application list

	BRect rect = Bounds();
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BButton* button = new BButton(rect, "remove", "Remove Uninstalled",
		new BMessage(kMsgRemoveUninstalled), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(8.0f, rect.bottom - 8.0f - button->Bounds().Height());
	topView->AddChild(button);

	rect.bottom = button->Frame().top - 10.0f;
	rect.top = 10.0f;
	rect.left = 10.0f;
	rect.right = 170;

	fTypeListView = new MimeTypeListView(rect, "listview", "application", true, true,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fTypeListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));

	BScrollView* scrollView = new BScrollView("scrollview", fTypeListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	// "Description" group

	BFont font(be_bold_font);
	float labelWidth = font.StringWidth("Icon");
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	rect.left = rect.right + 12.0f + B_V_SCROLL_BAR_WIDTH;
	rect.top -= 2.0f;
	rect.right = topView->Bounds().Width() - 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent) + 24.0f;
	BBox* box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Description");
	topView->AddChild(box);

	BRect innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	labelWidth = topView->StringWidth("Signature:") + 4.0f;

	innerRect.right = box->Bounds().Width() - 8.0f;
	innerRect.top += ceilf(fontHeight.ascent);
	fNameView = new StringView(innerRect, "name", "Name:", NULL, B_FOLLOW_LEFT_RIGHT);
	fNameView->SetDivider(labelWidth);
	float width, height;
	fNameView->GetPreferredSize(&width, &height);
	fNameView->ResizeTo(innerRect.Width(), height);
	box->ResizeBy(0, fNameView->Bounds().Height() * 3.0f);
	box->AddChild(fNameView);

	innerRect.OffsetBy(0, fNameView->Bounds().Height() + 5.0f);
	fSignatureView = new StringView(innerRect, "signature", "Signature:", NULL,
		B_FOLLOW_LEFT_RIGHT);
	fSignatureView->SetDivider(labelWidth);
	box->AddChild(fSignatureView);

	innerRect.OffsetBy(0, fNameView->Bounds().Height() + 5.0f);
	fPathView = new StringView(innerRect, "path", "Path:", NULL,
		B_FOLLOW_LEFT_RIGHT);
	fPathView->SetDivider(labelWidth);
	box->AddChild(fPathView);

	// Launch and Tracker buttons

	rect = box->Frame();
	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + 20.0f;
	fTrackerButton = new BButton(rect, "tracker", "Open In Tracker" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	fTrackerButton->ResizeToPreferred();
	fTrackerButton->MoveTo(rect.right - fTrackerButton->Bounds().Width(), rect.top);
	topView->AddChild(fTrackerButton);

	fLaunchButton = new BButton(rect, "launch", "Launch", NULL,
		B_FOLLOW_RIGHT);
	fLaunchButton->ResizeToPreferred();
	fLaunchButton->MoveTo(fTrackerButton->Frame().left - 6.0f
		- fLaunchButton->Bounds().Width(), rect.top);
	topView->AddChild(fLaunchButton);

	SetSizeLimits(scrollView->Frame().right + 22.0f + fTrackerButton->Frame().Width()
		+ fLaunchButton->Frame().Width(), 32767.0f,
		fTrackerButton->Frame().bottom + 8.0f, 32767.0f);

	BMimeType::StartWatching(this);
	_SetType(NULL);
}


ApplicationTypesWindow::~ApplicationTypesWindow()
{
	BMimeType::StopWatching(this);
}


void
ApplicationTypesWindow::_RemoveUninstalled()
{
	// Note: this runs in the looper's thread, which isn't that nice

	int32 removed = 0;

	for (int32 i = fTypeListView->FullListCountItems(); i-- > 0;) {
		MimeTypeItem* item = dynamic_cast<MimeTypeItem*>(fTypeListView->FullListItemAt(i));
		if (item == NULL)
			continue;

		// search for application on all volumes

		bool found = false;

		BVolumeRoster volumeRoster;
		BVolume volume;
		while (volumeRoster.GetNextVolume(&volume) == B_OK) {
			if (!volume.KnowsQuery())
				continue;

			BQuery query;
			query.PushAttr("BEOS:APP_SIG");
			query.PushString(item->Type());
			query.PushOp(B_EQ);

			query.SetVolume(&volume);
			query.Fetch();

			entry_ref ref;
			if (query.GetNextRef(&ref) == B_OK) {
				found = true;
				break;
			}
		}

		if (!found) {
			BMimeType mimeType(item->Type());
			mimeType.Delete();

			removed++;

			// We're blocking the message loop that received the MIME changes,
			// so we dequeue all waiting messages from time to time
			if (removed % 10 == 0)
				UpdateIfNeeded();
		}
	}

	char message[512];
	snprintf(message, sizeof(message), "%ld Application Type%s could be removed.",
		removed, removed == 1 ? "" : "s");
	error_alert(message, B_OK, B_INFO_ALERT);
}


void
ApplicationTypesWindow::_SetType(BMimeType* type, int32 forceUpdate)
{
	bool enabled = type != NULL;
	bool appFound = true;

	// update controls

	if (type != NULL) {
		if (fCurrentType == *type) {
			if (!forceUpdate)
				return;
		} else
			forceUpdate = B_EVERYTHING_CHANGED;

		if (&fCurrentType != type)
			fCurrentType.SetTo(type->Type());

		fSignatureView->SetText(type->Type());

		char description[B_MIME_TYPE_LENGTH];

		if ((forceUpdate & B_SHORT_DESCRIPTION_CHANGED) != 0) {
			if (type->GetShortDescription(description) != B_OK)
				description[0] = '\0';
			fNameView->SetText(description);
		}

		entry_ref ref;
		// TODO: disabled because of somewhat broken BRoster::FindApp() behaviour
		if (/*(forceUpdate & B_APP_HINT_CHANGED) != 0*/
			forceUpdate == B_EVERYTHING_CHANGED
			&& be_roster->FindApp(fCurrentType.Type(), &ref) == B_OK) {
			// Set launch message
			BMessenger tracker("application/x-vnd.Be-TRAK");			
			BMessage* message = new BMessage(B_REFS_RECEIVED);
			message->AddRef("refs", &ref);

			fLaunchButton->SetMessage(message);
			fLaunchButton->SetTarget(tracker);

			// Set path
			BPath path(&ref);
			path.GetParent(&path);
			fPathView->SetText(path.Path());

			// Set "Open In Tracker" message
			BEntry entry(path.Path());
			if (entry.GetRef(&ref) == B_OK) {
				BMessenger tracker("application/x-vnd.Be-TRAK");			
				message = new BMessage(B_REFS_RECEIVED);
				message->AddRef("refs", &ref);

				fTrackerButton->SetMessage(message);
				fTrackerButton->SetTarget(tracker);
			} else {
				fTrackerButton->SetMessage(NULL);
				appFound = false;
			}
		}
	} else {
		fNameView->SetText(NULL);
		fNameView->SetText(NULL);
		fPathView->SetText(NULL);
	}

	fNameView->SetEnabled(enabled);
	fSignatureView->SetEnabled(enabled);
	fPathView->SetEnabled(enabled);

	fTrackerButton->SetEnabled(enabled && appFound);
	fLaunchButton->SetEnabled(enabled && appFound);
}


void
ApplicationTypesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgTypeSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				MimeTypeItem* item = (MimeTypeItem*)fTypeListView->ItemAt(index);
				if (item != NULL) {
					BMimeType type(item->Type());
					_SetType(&type);
				} else
					_SetType(NULL);
			}
			break;
		}

		case kMsgRemoveUninstalled:
			_RemoveUninstalled();
			break;

		case B_META_MIME_CHANGED:
		{
			const char* type;
			int32 which;
			if (message->FindString("be:type", &type) != B_OK
				|| message->FindInt32("be:which", &which) != B_OK)
				break;

			if (fCurrentType.Type() == NULL)
				break;

			if (!strcasecmp(fCurrentType.Type(), type)) {
				if (which != B_MIME_TYPE_DELETED)
					_SetType(&fCurrentType, which);
				else
					_SetType(NULL);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
ApplicationTypesWindow::QuitRequested()
{
	be_app->PostMessage(kMsgApplicationTypesWindowClosed);
	return true;
}


