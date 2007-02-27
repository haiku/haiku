/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jerome Duval (jerome.duval@free.fr)
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "BackgroundsView.h"
#include "ImageFilePanel.h"

#include <OS.h>
#include <MenuField.h>
#include <stdio.h>
#include <stdlib.h>
#include <StorageKit.h>
#include <Background.h>
#include <Window.h>
#include <Messenger.h>
#include <Bitmap.h>
#include <Point.h>
#include <TranslationKit.h>
#include <PopUpMenu.h>


static const uint32 kMsgApplySettings = 'aply';
static const uint32 kMsgRevertSettings = 'rvrt';

static const uint32 kMsgUpdateColor = 'upcl';
static const uint32 kMsgAllWorkspaces = 'alwk';
static const uint32 kMsgCurrentWorkspace = 'crwk';
static const uint32 kMsgDefaultFolder = 'dffl';
static const uint32 kMsgOtherFolder = 'otfl';
static const uint32 kMsgNoImage = 'noim';
static const uint32 kMsgOtherImage = 'otim';
static const uint32 kMsgImageSelected = 'imsl';
static const uint32 kMsgFolderSelected = 'flsl';

static const uint32 kMsgCenterPlacement = 'cnpl';
static const uint32 kMsgManualPlacement = 'mnpl';
static const uint32 kMsgScalePlacement = 'scpl';
static const uint32 kMsgTilePlacement = 'tlpl';
static const uint32 kMsgIconLabelBackground = 'ilcb';

static const uint32 kMsgImagePlacement = 'xypl';
static const uint32 kMsgUpdatePreviewPlacement = 'pvpl';


const uint8 kHandCursorData[68] = {
	16, 1, 2, 2,

	0, 0,		// 0000000000000000
	7, 128,		// 0000011110000000
	61, 112,		// 0011110101110000
	37, 40,		// 0010010100101000
	36, 168,		// 0010010010101000
	18, 148,		// 0001001010010100
	18, 84,		// 0001001001010100
	9, 42,		// 0000100100101010
	8, 1,		// 0000100000000001
	60, 1,		// 0011110000000001
	76, 1,		// 0100110000000001
	66, 1,		// 0100001000000001
	48, 1,		// 0011000000000001
	12, 1,		// 0000110000000001
	2, 0,		// 0000001000000000
	1, 0,		// 0000000100000000

	0, 0,		// 0000000000000000
	7, 128,		// 0000011110000000
	63, 240,		// 0011111111110000
	63, 248,		// 0011111111111000
	63, 248,		// 0011111111111000
	31, 252,		// 0001111111111100
	31, 252,		// 0001111111111100
	15, 254,		// 0000111111111110
	15, 255,		// 0000111111111111
	63, 255,		// 0011111111111111
	127, 255,	// 0111111111111111
	127, 255,	// 0111111111111111
	63, 255,		// 0011111111111111
	15, 255,		// 0000111111111111
	3, 254,		// 0000001111111110
	1, 248		// 0000000111111000
};


BackgroundsView::BackgroundsView(BRect frame, const char *name, int32 resize,
	int32 flags)
	: BBox(frame, name, resize, flags | B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER),
	fCurrent(NULL),
	fCurrentInfo(NULL),
	fLastImageIndex(-1),
	fPathList(1, true),
	fImageList(1, true)
{
	// we need the "Current Workspace" first to get its height

	BMenuItem *menuItem;
	fWorkspaceMenu = new BPopUpMenu("pick one");
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("All Workspaces",
		new BMessage(kMsgAllWorkspaces)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Current Workspace",
		new BMessage(kMsgCurrentWorkspace)));
	menuItem->SetMarked(true);
	fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(fWorkspaceMenu->FindMarked());
	fWorkspaceMenu->AddSeparatorItem();
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Default folder",
		new BMessage(kMsgDefaultFolder)));
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem("Other folder" B_UTF8_ELLIPSIS,
		new BMessage(kMsgOtherFolder)));

	BMenuField *workspaceMenuField = new BMenuField(BRect(0, 0, 130, 18),
		"workspaceMenuField", NULL, fWorkspaceMenu, true);
	workspaceMenuField->ResizeToPreferred();

	/* the preview box */

	BFont font(be_plain_font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	fPreview = new PreviewBox(BRect(10, 8.0 + workspaceMenuField->Bounds().Height() / 2.0f
		- ceilf(fontHeight.ascent + fontHeight.descent) / 2.0f, 160, 180), "preview");
	fPreview->SetFont(&font);
	fPreview->SetLabel("Preview");
	AddChild(fPreview);

	BRect rect(10, fPreview->Bounds().bottom - 30, 70, fPreview->Bounds().bottom - 10);
	fXPlacementText = new BTextControl(rect, "xPlacementText", "X:", NULL,
		new BMessage(kMsgImagePlacement), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fXPlacementText->SetDivider(fXPlacementText->StringWidth(fXPlacementText->Label()) + 4.0f);
	fXPlacementText->TextView()->SetMaxBytes(5);
	float width, height;
	fXPlacementText->GetPreferredSize(&width, &height);
	float delta = fXPlacementText->Bounds().Height() - height;
	fXPlacementText->MoveBy(0, delta);
	fXPlacementText->ResizeTo(fXPlacementText->Bounds().Width(), height);
	fPreview->AddChild(fXPlacementText);

	rect.OffsetBy(70, delta);
	fYPlacementText = new BTextControl(rect, "yPlacementText", "Y:", NULL,
		new BMessage(kMsgImagePlacement), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fYPlacementText->SetDivider(fYPlacementText->StringWidth(fYPlacementText->Label()) + 4.0f);
	fYPlacementText->TextView()->SetMaxBytes(5);
	fXPlacementText->ResizeTo(fYPlacementText->Bounds().Width(), height);
	fPreview->AddChild(fYPlacementText);

	for (int32 i = 0; i < 256; i++) {
		if ((i < '0' || i > '9') && i != '-') {
			fXPlacementText->TextView()->DisallowChar(i);
			fYPlacementText->TextView()->DisallowChar(i);
		}
	}

	fPreView = new PreView(BRect(15, 25, 135, 115), "preView",
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_SUBPIXEL_PRECISE);
	fPreview->AddChild(fPreView);

	/* the right box */

	BBox *rightbox = new BBox(BRect(fPreview->Frame().right + 10,
		7, Frame().right - 10, fPreview->Frame().bottom),
		"rightbox");
	rightbox->SetLabel(workspaceMenuField);
	AddChild(rightbox);

	float offset = be_plain_font->StringWidth("Placement:") + 5;
	rect.Set(10, 0, rightbox->Bounds().right - 10, 30);
#ifdef __HAIKU__
	rect.top = 8 + rightbox->InnerFrame().top;
#else
	rect.top = 5 + workspaceMenuField->Bounds().Height();
	rect.bottom = rect.top + workspaceMenuField->Bounds().Height();
#endif

	fImageMenu = new BPopUpMenu("pick one");
	fImageMenu->AddItem(new BGImageMenuItem("None", -1, new BMessage(kMsgNoImage)));
	fImageMenu->AddSeparatorItem();
	fImageMenu->AddItem(new BMenuItem("Other" B_UTF8_ELLIPSIS, new BMessage(kMsgOtherImage)));

	BMenuField *imageMenuField = new BMenuField(rect, "imageMenuField",
		"Image:", fImageMenu);
	imageMenuField->SetDivider(offset);
	imageMenuField->SetAlignment(B_ALIGN_RIGHT);
	imageMenuField->ResizeToPreferred();
	rightbox->AddChild(imageMenuField);

	fPlacementMenu = new BPopUpMenu("pick one");
	fPlacementMenu->AddItem(new BMenuItem("Manual",
		new BMessage(kMsgManualPlacement)));
	fPlacementMenu->AddItem(new BMenuItem("Center",
		new BMessage(kMsgCenterPlacement)));
	fPlacementMenu->AddItem(new BMenuItem("Scale to fit",
		new BMessage(kMsgScalePlacement)));
	fPlacementMenu->AddItem(new BMenuItem("Tile",
		new BMessage(kMsgTilePlacement)));

	rect.OffsetBy(0, imageMenuField->Bounds().Height() + 5);
	BMenuField *placementMenuField = new BMenuField(rect, "placementMenuField",
		"Placement:", fPlacementMenu);
	placementMenuField->SetDivider(offset);
	placementMenuField->SetAlignment(B_ALIGN_RIGHT);
	placementMenuField->ResizeToPreferred();
	rightbox->AddChild(placementMenuField);

	rect.OffsetBy(offset, placementMenuField->Bounds().Height() + 5);
	fIconLabelBackground = new BCheckBox(rect, "iconLabelBackground",
		"Icon label background", new BMessage(kMsgIconLabelBackground));
	fIconLabelBackground->SetValue(B_CONTROL_ON);
	fIconLabelBackground->ResizeToPreferred();
	rightbox->AddChild(fIconLabelBackground);

	rect.top += fIconLabelBackground->Bounds().Height() + 15;
	fPicker = new BColorControl(BPoint(10, rect.top), B_CELLS_32x8, 5.0, "Picker",
		new BMessage(kMsgUpdateColor));
	rightbox->AddChild(fPicker);

	float xDelta = max_c(fIconLabelBackground->Frame().right, fPicker->Frame().right)
		+ 10.0f - rightbox->Bounds().Width();
	delta = fPicker->Frame().bottom + 10.0f - rightbox->Bounds().Height();

	rightbox->ResizeBy(xDelta, delta);
	fPreview->ResizeBy(0, delta);

	// we're not yet attached to a view, so we need to move them manually
	fXPlacementText->MoveBy(0, delta);
	fYPlacementText->MoveBy(0, delta);

	fRevert = new BButton(BRect(fPreview->Frame().left, fPreview->Frame().bottom + 10,
		fPreview->Frame().left + 80, Frame().bottom - 10), "RevertButton",
		"Revert", new BMessage(kMsgRevertSettings), B_FOLLOW_LEFT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE);
	fRevert->ResizeToPreferred();
	AddChild(fRevert);

	fApply = new BButton(BRect(rightbox->Frame().right, fPreview->Frame().bottom
		+ 10, rightbox->Frame().right, 110), "ApplyButton", "Apply",
		new BMessage(kMsgApplySettings), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_NAVIGABLE);
	fApply->ResizeToPreferred();
	fApply->MoveBy(-fApply->Bounds().Width(), 0);
	AddChild(fApply);

	ResizeTo(rightbox->Frame().right + 10, fApply->Frame().bottom + 10);
}


BackgroundsView::~BackgroundsView()
{
	delete fPanel;
	delete fFolderPanel;
}


void
BackgroundsView::GetPreferredSize(float* _width, float* _height)
{
	if (_width)
		*_width = fApply->Frame().right + 10;
	if (_height)
		*_height = fApply->Frame().bottom + 10;
}


void
BackgroundsView::AllAttached()
{
	fPlacementMenu->SetTargetForItems(this);
	fImageMenu->SetTargetForItems(this);
	fWorkspaceMenu->SetTargetForItems(this);
	fXPlacementText->SetTarget(this);
	fYPlacementText->SetTarget(this);
	fIconLabelBackground->SetTarget(this);
	fPicker->SetTarget(this);
	fApply->SetTarget(this);
	fRevert->SetTarget(this);

	fPanel = new ImageFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
		B_FILE_NODE, false, NULL, new CustomRefFilter(true));
	fPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");

	fFolderPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL,
		B_DIRECTORY_NODE, false, NULL, new CustomRefFilter(false));
	fFolderPanel->SetButtonLabel(B_DEFAULT_BUTTON, "Select");

	Window()->ResizeTo(Frame().Width(), Frame().Height());

	LoadSettings();
	LoadDesktopFolder();
}


void
BackgroundsView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;

		case kMsgUpdatePreviewPlacement:
		{
			BString xstring, ystring;
			xstring << (int)fPreView->fPoint.x;
			ystring << (int)fPreView->fPoint.y;
			fXPlacementText->SetText(xstring.String());
			fYPlacementText->SetText(ystring.String());
			UpdatePreview();
			UpdateButtons();
			break;
		}

		case kMsgManualPlacement:
			UpdatePreview();
			UpdateButtons();
			break;

		case kMsgTilePlacement:
		case kMsgScalePlacement:
		case kMsgCenterPlacement:
			UpdatePreview();
			UpdateButtons();
			break;

		case kMsgIconLabelBackground:
			UpdateButtons();
			break;

		case kMsgUpdateColor:
		case kMsgImagePlacement:
			UpdatePreview();
			UpdateButtons();
			break;

		case kMsgCurrentWorkspace:
		case kMsgAllWorkspaces:
			fImageMenu->FindItem(kMsgNoImage)->SetLabel("None");
			fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked());
			if (!fCurrent->IsDesktop()) {
				fPreview->SetDesktop(true);
				LoadDesktopFolder();
			} else
				UpdateButtons();
			break;

		case kMsgDefaultFolder:
			fImageMenu->FindItem(kMsgNoImage)->SetLabel("None");
			fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked());
			fPreview->SetDesktop(false);
			LoadDefaultFolder();
			break;

		case kMsgOtherFolder:
			fFolderPanel->Show();
			break;

		case kMsgOtherImage:
			fPanel->Show();
			break;

		case B_CANCEL:
		{
			PRINT(("cancel received\n"));
			void* pointer;
			msg->FindPointer("source", &pointer);
			if (pointer == fPanel) {
				if (fLastImageIndex >= 0)
					FindImageItem(fLastImageIndex)->SetMarked(true);
				else
					fImageMenu->ItemAt(0)->SetMarked(true);
			} else if (pointer == fFolderPanel) {
				if (fLastWorkspaceIndex >= 0)
					fWorkspaceMenu->ItemAt(fLastWorkspaceIndex)->SetMarked(true);
			}
			break;
		}

		case kMsgImageSelected:
		case kMsgNoImage:
			fLastImageIndex = ((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex();
			UpdatePreview();
			UpdateButtons();
			break;

		case kMsgFolderSelected:
			fImageMenu->FindItem(kMsgNoImage)->SetLabel("Default");
			fLastWorkspaceIndex = fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked());
			fPreview->SetDesktop(false);
			LoadRecentFolder(*fPathList.ItemAt(fWorkspaceMenu->IndexOf(
				fWorkspaceMenu->FindMarked()) - 6));
			break;

		case kMsgApplySettings:
		{
			Save();

			//NotifyServer();
			thread_id notify_thread;
			notify_thread = spawn_thread(BackgroundsView::NotifyThread, "notifyServer",
				B_NORMAL_PRIORITY, this);
			resume_thread(notify_thread);
			UpdateButtons();
			break;
		}
		case kMsgRevertSettings:
			UpdateWithCurrent();
			break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
BackgroundsView::LoadDesktopFolder()
{
	BPath path;
	if (find_directory(B_DESKTOP_DIRECTORY, &path) == B_OK) {
		status_t err;
		err = get_ref_for_path(path.Path(), &fCurrentRef);
		if (err != B_OK)
			printf("error in LoadDesktopSettings\n");
		LoadFolder(true);
	}
}


void
BackgroundsView::LoadDefaultFolder()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BString pathString = path.Path();
		pathString << "/Tracker/DefaultFolderTemplate";
		status_t err;
		err = get_ref_for_path(pathString.String(), &fCurrentRef);
		if (err != B_OK)
			printf("error in LoadDefaultFolderSettings\n");
		LoadFolder(false);
	}
}


void
BackgroundsView::LoadRecentFolder(BPath path)
{
	status_t err;
	err = get_ref_for_path(path.Path(), &fCurrentRef);
	if (err != B_OK)
		printf("error in LoadRecentFolder\n");
	LoadFolder(false);
}


void
BackgroundsView::LoadFolder(bool isDesktop)
{
	if (fCurrent) {
		delete fCurrent;
		fCurrent = NULL;
	}

	BNode node(&fCurrentRef);
	if (node.InitCheck() == B_OK)
		fCurrent = BackgroundImage::GetBackgroundImage(&node, isDesktop, this);

	UpdateWithCurrent();
}


void
BackgroundsView::UpdateWithCurrent(void)
{
	if (fCurrent == NULL)
		return;

	fPlacementMenu->FindItem(kMsgScalePlacement)->SetEnabled(fCurrent->IsDesktop());
	fPlacementMenu->FindItem(kMsgCenterPlacement)->SetEnabled(fCurrent->IsDesktop());

	if (fWorkspaceMenu->IndexOf(fWorkspaceMenu->FindMarked()) > 5)
		fImageMenu->FindItem(kMsgNoImage)->SetLabel("Default");
	else
		fImageMenu->FindItem(kMsgNoImage)->SetLabel("None");

	for (int32 i = fImageMenu->CountItems() - 5; i >= 0; i--) {
		fImageMenu->RemoveItem(2);
	}

	for (int32 i = fImageList.CountItems() - 1; i >= 0; i--) {
		BMessage *message = new BMessage(kMsgImageSelected);
		AddItem(new BGImageMenuItem(GetImage(i)->GetName(), i, message));
	}

	fImageMenu->SetTargetForItems(this);

	fCurrentInfo = fCurrent->ImageInfoForWorkspace(current_workspace());

	if (!fCurrentInfo) {
		fImageMenu->FindItem(kMsgNoImage)->SetMarked(true);
		fPlacementMenu->FindItem(kMsgManualPlacement)->SetMarked(true);
	} else {
		fIconLabelBackground->SetValue(fCurrentInfo->fEraseTextWidgetBackground
			? B_CONTROL_ON : B_CONTROL_OFF);

		BString xtext, ytext;
		int32 cmd = 0;
		switch (fCurrentInfo->fMode) {
			case BackgroundImage::kCentered:
				cmd = kMsgCenterPlacement;
				break;
			case BackgroundImage::kScaledToFit:
				cmd = kMsgScalePlacement;
				break;
			case BackgroundImage::kAtOffset:
				cmd = kMsgManualPlacement;
				xtext << (int)fCurrentInfo->fOffset.x;
				ytext << (int)fCurrentInfo->fOffset.y;
				break;
			case BackgroundImage::kTiled:
				cmd = kMsgTilePlacement;
				break;
		}

		if (cmd != 0)
			fPlacementMenu->FindItem(cmd)->SetMarked(true);

		fXPlacementText->SetText(xtext.String());
		fYPlacementText->SetText(ytext.String());

		fLastImageIndex = fCurrentInfo->fImageIndex;
		FindImageItem(fLastImageIndex)->SetMarked(true);
	}

	rgb_color color = {255, 255, 255, 255};
	if (fCurrent->IsDesktop()) {
		color = BScreen().DesktopColor();
		fPicker->SetEnabled(true);
	} else
		fPicker->SetEnabled(false);

	fPicker->SetValue(color);

	UpdatePreview();
	UpdateButtons();
}


void
BackgroundsView::Save()
{
	bool eraseTextWidgetBackground =
		fIconLabelBackground->Value() == B_CONTROL_ON;
	BackgroundImage::Mode mode = FindPlacementMode();
	BPoint offset(atoi(fXPlacementText->Text()), atoi(fYPlacementText->Text()));

	if (!fCurrent->IsDesktop()) {
		if (fCurrentInfo == NULL) {
			fCurrentInfo = new BackgroundImage::BackgroundImageInfo(B_ALL_WORKSPACES,
				fLastImageIndex, mode, offset, eraseTextWidgetBackground, 0, 0);
			fCurrent->Add(fCurrentInfo);
		} else {
			fCurrentInfo->fEraseTextWidgetBackground = eraseTextWidgetBackground;
			fCurrentInfo->fMode = mode;
			if (fCurrentInfo->fMode == BackgroundImage::kAtOffset)
				fCurrentInfo->fOffset = offset;
			fCurrentInfo->fImageIndex = fLastImageIndex;
		}
	} else {
		uint32 workspaceMask = 1;
		int32 workspace = current_workspace();
		for (; workspace; workspace--)
			workspaceMask *= 2;

		if (fCurrentInfo != NULL) {
			if (fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->IsMarked()) {
				if (fCurrentInfo->fWorkspace != workspaceMask) {
					fCurrentInfo->fWorkspace = fCurrentInfo->fWorkspace
						^ workspaceMask;
					if (fLastImageIndex > -1) {
						fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
							workspaceMask, fLastImageIndex, mode, offset,
							eraseTextWidgetBackground, fCurrentInfo->fImageSet,
							fCurrentInfo->fCacheMode);
						fCurrent->Add(fCurrentInfo);
					}
				} else {
					if (fLastImageIndex > -1) {
						fCurrentInfo->fEraseTextWidgetBackground =
							eraseTextWidgetBackground;
						fCurrentInfo->fMode = mode;
						if (fCurrentInfo->fMode == BackgroundImage::kAtOffset)
							fCurrentInfo->fOffset = offset;

						fCurrentInfo->fImageIndex = fLastImageIndex;
					} else {
						fCurrent->Remove(fCurrentInfo);
						fCurrentInfo = NULL;
					}
				}
			} else if (fLastImageIndex > -1) {
				fCurrent->RemoveAll();
				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					B_ALL_WORKSPACES, fLastImageIndex, mode, offset,
					eraseTextWidgetBackground, fCurrent->GetShowingImageSet(),
					fCurrentInfo->fCacheMode);
				fCurrent->Add(fCurrentInfo);
			}
		} else if (fLastImageIndex > -1) {
			if (fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->IsMarked()) {
				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					workspaceMask, fLastImageIndex, mode, offset,
					eraseTextWidgetBackground, fCurrent->GetShowingImageSet(), 0);
			} else {
				fCurrent->RemoveAll();
				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					B_ALL_WORKSPACES, fLastImageIndex, mode, offset,
					eraseTextWidgetBackground, fCurrent->GetShowingImageSet(), 0);
			}
			fCurrent->Add(fCurrentInfo);
		}

		if (!fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->IsMarked()) {
			for (int32 i = 0; i < count_workspaces(); i++) {
				BScreen().SetDesktopColor(fPicker->ValueAsColor(), i, true);
			}
		} else
			BScreen().SetDesktopColor(fPicker->ValueAsColor(), true);
	}

	BNode node(&fCurrentRef);

	status_t status = fCurrent->SetBackgroundImage(&node);
	if (status != B_OK) {
		// TODO: this should be a BAlert!
		printf("setting background image failed: %s\n", strerror(status));
	}
}


void
BackgroundsView::NotifyServer()
{
	BMessenger tracker("application/x-vnd.Be-TRAK");

	if (fCurrent->IsDesktop()) {
		tracker.SendMessage(new BMessage(B_RESTORE_BACKGROUND_IMAGE));
	} else {
		int32 i = -1;
		BMessage reply;
		int32 err;
		BEntry currentEntry(&fCurrentRef);
		BPath currentPath(&currentEntry);
		bool isCustomFolder = !fWorkspaceMenu->FindItem(kMsgDefaultFolder)->IsMarked();

		do {
			BMessage msg(B_GET_PROPERTY);
			i++;

			// look at the "Poses" in every Tracker window
			msg.AddSpecifier("Poses");
			msg.AddSpecifier("Window", i);

			reply.MakeEmpty();
			tracker.SendMessage(&msg, &reply);

			// break out of the loop when we're at the end of
			// the windows
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD
				&& reply.FindInt32("error", &err) == B_OK
				&& err == B_BAD_INDEX)
				break;

			// don't stop for windows that don't understand
			// a request for "Poses"; they're not displaying
			// folders
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD
				&& reply.FindInt32("error", &err) == B_OK
				&& err != B_BAD_SCRIPT_SYNTAX)
				continue;

			BMessenger trackerWindow;
			if (reply.FindMessenger("result", &trackerWindow) != B_OK)
				continue;

			if (isCustomFolder) {
				// found a window with poses, ask for its path
				msg.MakeEmpty();
				msg.what = B_GET_PROPERTY;
				msg.AddSpecifier("Path");
				msg.AddSpecifier("Poses");
				msg.AddSpecifier("Window", i);

				reply.MakeEmpty();
				tracker.SendMessage(&msg, &reply);

				// go on with the next if this din't have a path
				if (reply.what == B_MESSAGE_NOT_UNDERSTOOD)
					continue;

				entry_ref ref;
				if (reply.FindRef("result", &ref) == B_OK) {
					BEntry entry(&ref);
					BPath path(&entry);

					// these are not the paths you're looking for
					if (currentPath != path)
						continue;
				}
			}

			trackerWindow.SendMessage(B_RESTORE_BACKGROUND_IMAGE);
		} while (true);
	}
}


int32
BackgroundsView::NotifyThread(void *data)
{
	BackgroundsView *view = (BackgroundsView*)data;

	view->NotifyServer();
	return B_OK;
}


void
BackgroundsView::SaveSettings(void)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);

		BPoint point = Window()->Frame().LeftTop();
		if (fSettings.ReplacePoint("pos", point) != B_OK)
			fSettings.AddPoint("pos", point);

		entry_ref ref;
		BEntry entry;
		BPath path;

		fPanel->GetPanelDirectory(&ref);
		entry.SetTo(&ref);
		entry.GetPath(&path);
		if (fSettings.ReplaceString("paneldir", path.Path()) != B_OK)
			fSettings.AddString("paneldir", path.Path());

		fFolderPanel->GetPanelDirectory(&ref);
		entry.SetTo(&ref);
		entry.GetPath(&path);
		if (fSettings.ReplaceString("folderpaneldir", path.Path()) != B_OK)
			fSettings.AddString("folderpaneldir", path.Path());

		fSettings.RemoveName("recentfolder");
		for (int32 i = 0; i < fPathList.CountItems(); i++) {
			fSettings.AddString("recentfolder", fPathList.ItemAt(i)->Path());
		}

		fSettings.Flatten(&file);
	}
}


void
BackgroundsView::LoadSettings()
{
	fSettings.MakeEmpty();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append(SETTINGS_FILE);
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return;

	if (fSettings.Unflatten(&file) != B_OK) {
		printf("Error unflattening settings file %s\n", path.Path());
		return;
	}

	PRINT_OBJECT(fSettings);

	BPoint point;
	if (fSettings.FindPoint("pos", &point) == B_OK)
		Window()->MoveTo(point);

	BString string;
	if (fSettings.FindString("paneldir", &string) == B_OK)
		fPanel->SetPanelDirectory(string.String());

	if (fSettings.FindString("folderpaneldir", &string) == B_OK)
		fFolderPanel->SetPanelDirectory(string.String());

	int32 index = 0;
	while (fSettings.FindString("recentfolder", index, &string) == B_OK) {
		if (index == 0)
			fWorkspaceMenu->AddSeparatorItem();

		BPath path(string.String());
		int32 i = AddPath(path);
		BString s;
		s << "Folder: " << path.Leaf();
		BMenuItem *item = new BMenuItem(s.String(),
			new BMessage(kMsgFolderSelected));
		fWorkspaceMenu->AddItem(item, -i - 1 + 6);
		index++;
	}
	fWorkspaceMenu->SetTargetForItems(this);

	PRINT(("Settings Loaded\n"));
}


void
BackgroundsView::WorkspaceActivated(uint32 oldWorkspaces, bool active)
{
	UpdateWithCurrent();
}


void
BackgroundsView::UpdatePreview()
{
	bool imageEnabled = !(fImageMenu->FindItem(kMsgNoImage)->IsMarked());
	if (fPlacementMenu->IsEnabled() ^ imageEnabled)
		fPlacementMenu->SetEnabled(imageEnabled);
	if (fIconLabelBackground->IsEnabled() ^ imageEnabled)
		fIconLabelBackground->SetEnabled(imageEnabled);

	bool textEnabled = (fPlacementMenu->FindItem(kMsgManualPlacement)->IsMarked())
		&& imageEnabled;
	if (fXPlacementText->IsEnabled() ^ textEnabled)
		fXPlacementText->SetEnabled(textEnabled);
	if (fYPlacementText->IsEnabled() ^ textEnabled)
		fYPlacementText->SetEnabled(textEnabled);

	if (textEnabled && (strcmp(fXPlacementText->Text(), "") == 0)) {
		fXPlacementText->SetText("0");
		fYPlacementText->SetText("0");
	}
	if (!textEnabled) {
		fXPlacementText->SetText(NULL);
		fYPlacementText->SetText(NULL);
	}

	fXPlacementText->TextView()->MakeSelectable(textEnabled);
	fYPlacementText->TextView()->MakeSelectable(textEnabled);
	fXPlacementText->TextView()->MakeEditable(textEnabled);
	fYPlacementText->TextView()->MakeEditable(textEnabled);

	fPreView->ClearViewBitmap();

	int32 index = ((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex();
	if (index >= 0) {
		BBitmap *bitmap = GetImage(index)->GetBitmap();
		if (bitmap) {
			BackgroundImage::BackgroundImageInfo *info =
				new BackgroundImage::BackgroundImageInfo(0, index,
					FindPlacementMode(), BPoint(atoi(fXPlacementText->Text()),
						atoi(fYPlacementText->Text())),
					fIconLabelBackground->Value() == B_CONTROL_ON, 0, 0);
			if (info->fMode == BackgroundImage::kAtOffset) {
				fPreView->SetEnabled(true);
				fPreView->fPoint.x = atoi(fXPlacementText->Text());
				fPreView->fPoint.y = atoi(fYPlacementText->Text());
			} else
				fPreView->SetEnabled(false);

			fPreView->fImageBounds = BRect(bitmap->Bounds());
			fCurrent->Show(info, fPreView);
		}
	} else
		fPreView->SetEnabled(false);

	fPreView->SetViewColor(fPicker->ValueAsColor());
	fPreView->Invalidate();
}


BackgroundImage::Mode
BackgroundsView::FindPlacementMode()
{
	BackgroundImage::Mode mode = BackgroundImage::kAtOffset;

	if (fPlacementMenu->FindItem(kMsgCenterPlacement)->IsMarked())
		mode = BackgroundImage::kCentered;
	if (fPlacementMenu->FindItem(kMsgScalePlacement)->IsMarked())
		mode = BackgroundImage::kScaledToFit;
	if (fPlacementMenu->FindItem(kMsgManualPlacement)->IsMarked())
		mode = BackgroundImage::kAtOffset;
	if (fPlacementMenu->FindItem(kMsgTilePlacement)->IsMarked())
		mode = BackgroundImage::kTiled;

	return mode;
}


#ifndef __HAIKU__
inline bool operator!=(const rgb_color& x, const rgb_color& y)
{
	return (x.red != y.red || x.blue != y.blue || x.green != y.green);
}
#endif


void
BackgroundsView::UpdateButtons()
{
	bool hasChanged = false;
	if (fPicker->IsEnabled()
		&& fPicker->ValueAsColor() != BScreen().DesktopColor()) {
		hasChanged = true;
	} else if (fCurrentInfo) {
		if ((fIconLabelBackground->Value() == B_CONTROL_ON) ^
			fCurrentInfo->fEraseTextWidgetBackground) {
			hasChanged = true;
		} else if (FindPlacementMode() != fCurrentInfo->fMode) {
			hasChanged = true;
		} else if (fCurrentInfo->fImageIndex !=
			((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex()) {
			hasChanged = true;
		} else if (fCurrent->IsDesktop()
			&& ((fCurrentInfo->fWorkspace != B_ALL_WORKSPACES)
				^ (fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->IsMarked()))) {
			hasChanged = true;
		} else if (fCurrentInfo->fMode == BackgroundImage::kAtOffset) {
			BString oldString, newString;
			oldString << (int)fCurrentInfo->fOffset.x;
			if (oldString != BString(fXPlacementText->Text())) {
				hasChanged = true;
			}
			oldString = "";
			oldString << (int)fCurrentInfo->fOffset.y;
			if (oldString != BString(fYPlacementText->Text())) {
				hasChanged = true;
			}
		}
	} else if (fImageMenu->IndexOf(fImageMenu->FindMarked()) > 0) {
		hasChanged = true;
	}

	fApply->SetEnabled(hasChanged);
	fRevert->SetEnabled(hasChanged);
}


void
BackgroundsView::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i = 0;
	BMimeType imageType("image");
	BPath desktopPath;
	find_directory(B_DESKTOP_DIRECTORY, &desktopPath);

	while (msg->FindRef("refs", i++, &ref) == B_OK) {
		BPath path;
		BEntry entry(&ref, true);
		path.SetTo(&entry);
		BNode node(&entry);

		if (node.IsFile()) {
			BNodeInfo nodeInfo(&node);
			char fileType[B_MIME_TYPE_LENGTH];
			if (nodeInfo.GetType(fileType) != B_OK)
				continue;

			BMimeType refType(fileType);
			if (!imageType.Contains(&refType))
				continue;

			BGImageMenuItem *item;
			int32 index = AddImage(path);
			if (index >= 0) {
				item = FindImageItem(index);
				fLastImageIndex = index;
			} else {
				const char* name = GetImage(-index - 1)->GetName();
				item = new BGImageMenuItem(name, -index - 1,
					new BMessage(kMsgImageSelected));
				AddItem(item);
				item->SetTarget(this);
				fLastImageIndex = -index - 1;
			}

			item->SetMarked(true);
			BMessenger(this).SendMessage(kMsgImageSelected);
		} else if (node.IsDirectory()) {
			if (desktopPath == path) {
				fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->SetMarked(true);
				BMessenger(this).SendMessage(kMsgCurrentWorkspace);
				break;
			}
			BMenuItem *item;
			int32 index = AddPath(path);
			if (index >= 0) {
				item = fWorkspaceMenu->ItemAt(index + 6);
				fLastWorkspaceIndex = index + 6;
			} else {
				if (fWorkspaceMenu->CountItems() <= 5)
					fWorkspaceMenu->AddSeparatorItem();
				BString s;
				s << "Folder: " << path.Leaf();
				item = new BMenuItem(s.String(), new BMessage(kMsgFolderSelected));
				fWorkspaceMenu->AddItem(item, -index - 1 + 6);
				item->SetTarget(this);
				fLastWorkspaceIndex = -index - 1 + 6;
			}

			item->SetMarked(true);
			BMessenger(this).SendMessage(kMsgFolderSelected);
		}
	}
}


int32
BackgroundsView::AddPath(BPath path)
{
	int32 count = fPathList.CountItems();
	int32 index = 0;
	for (; index < count; index++) {
		BPath *p = fPathList.ItemAt(index);
		int c = BString(p->Path()).ICompare(path.Path());
		if (c == 0)
			return index;

		if (c > 0)
			break;
	}
	fPathList.AddItem(new BPath(path), index);
	return -index - 1;
}


int32
BackgroundsView::AddImage(BPath path)
{
	int32 count = fImageList.CountItems();
	int32 index = 0;
	for (; index < count; index++) {
		Image *image = fImageList.ItemAt(index);
		if (image->GetPath() == path)
			return index;
	}

	fImageList.AddItem(new Image(path));
	return -index - 1;
}


void
BackgroundsView::ProcessRefs(entry_ref dir, BMessage* refs)
{
	fWorkspaceMenu->FindItem(kMsgDefaultFolder)->SetMarked(true);
	BMessenger messenger(this);
	messenger.SendMessage(kMsgDefaultFolder);

	if (refs->CountNames(B_REF_TYPE) > 0) {
		messenger.SendMessage(refs);
	} else {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &dir);
		messenger.SendMessage(&message);
	}
}


Image*
BackgroundsView::GetImage(int32 imageIndex)
{
	return fImageList.ItemAt(imageIndex);
}


BGImageMenuItem*
BackgroundsView::FindImageItem(const int32 imageIndex)
{
	if (imageIndex < 0)
		return (BGImageMenuItem *)fImageMenu->ItemAt(0);

	int32 count = fImageMenu->CountItems() - 2;
	int32 index = 2;
	for (; index < count; index++) {
		BGImageMenuItem *image = (BGImageMenuItem *)fImageMenu->ItemAt(index);
		if (image->ImageIndex() == imageIndex)
			return image;
	}
	return NULL;
}


bool
BackgroundsView::AddItem(BGImageMenuItem *item)
{
	int32 count = fImageMenu->CountItems() - 2;
	int32 index = 2;
	if (count < index) {
		fImageMenu->AddItem(new BSeparatorItem(), 1);
		count = fImageMenu->CountItems() - 2;
	}

	for (; index < count; index++) {
		BGImageMenuItem* image = (BGImageMenuItem *)fImageMenu->ItemAt(index);
		int c = (BString(image->Label()).ICompare(BString(item->Label())));
		if (c > 0)
			break;
	}
	return fImageMenu->AddItem(item, index);
}


//	#pragma mark -


PreView::PreView(BRect frame, const char *name, int32 resize, int32 flags)
	: BControl(frame, name, NULL, NULL, resize, flags),
	fMoveHandCursor(kHandCursorData)
{
}


void
PreView::AttachedToWindow()
{
	rgb_color color = ViewColor();
	BControl::AttachedToWindow();
	SetViewColor(color);
}


void
PreView::MouseDown(BPoint point)
{
	if (IsEnabled() && Bounds().Contains(point)) {
		uint32 buttons;
		GetMouse(&point, &buttons);
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			fOldPoint = point;
			SetTracking(true);
			BScreen().GetMode(&mode);
			x_ratio = Bounds().Width() / mode.virtual_width;
			y_ratio = Bounds().Height() / mode.virtual_height;
			SetMouseEventMask(B_POINTER_EVENTS,
				B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
		}
	}
}


void
PreView::MouseUp(BPoint point)
{
	if (IsTracking())
		SetTracking(false);
}


void
PreView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	if (IsEnabled())
		SetViewCursor(&fMoveHandCursor);
	else
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	if (IsTracking()) {
		float x, y;
		x = fPoint.x + (point.x - fOldPoint.x) / x_ratio;
		y = fPoint.y + (point.y - fOldPoint.y) / y_ratio;
		bool min, max, mustSend = false;
		min = (x > -fImageBounds.Width());
		max = (x < mode.virtual_width);
		if (min && max) {
			fOldPoint.x = point.x;
			fPoint.x = x;
			mustSend = true;
		} else {
			if (!min && fPoint.x > -fImageBounds.Width()) {
				fPoint.x = -fImageBounds.Width();
				fOldPoint.x = point.x - (x - fPoint.x) * x_ratio;
				mustSend = true;
			}
			if (!max && fPoint.x < mode.virtual_width) {
				fPoint.x = mode.virtual_width;
				fOldPoint.x = point.x - (x - fPoint.x) * x_ratio;
				mustSend = true;
			}
		}

		min = (y > -fImageBounds.Height());
		max = (y < mode.virtual_height);
		if (min && max) {
			fOldPoint.y = point.y;
			fPoint.y = y;
			mustSend = true;
		} else {
			if (!min && fPoint.y > -fImageBounds.Height()) {
				fPoint.y = -fImageBounds.Height();
				fOldPoint.y = point.y - (y - fPoint.y) * y_ratio;
				mustSend = true;
			}
			if (!max && fPoint.y < mode.virtual_height) {
				fPoint.y = mode.virtual_height;
				fOldPoint.y = point.y - (y - fPoint.y) * y_ratio;
				mustSend = true;
			}
		}

		if (mustSend) {
			BMessenger messenger(Parent());
			messenger.SendMessage(kMsgUpdatePreviewPlacement);
		}
	}
	BControl::MouseMoved(point, transit, message);
}


//	#pragma mark -


PreviewBox::PreviewBox(BRect frame, const char *name)
	: BBox(frame, name)
{
	fIsDesktop = true;
}


void
PreviewBox::Draw(BRect rect)
{
	// TODO: make view size dependent!
	rgb_color color = HighColor();

	SetHighColor(LowColor());
	FillRect(BRect(9, 19, 141, 127));

	if (fIsDesktop) {
		BPoint points[] = {
			BPoint(11, 19), BPoint(139, 19), BPoint(141, 21),
			BPoint(141, 119), BPoint(139, 121), BPoint(118, 121),
			BPoint(118, 126), BPoint(117, 127), BPoint(33, 127),
			BPoint(32, 126), BPoint(32, 121), BPoint(11, 121),
			BPoint(9, 119), BPoint(9, 21), BPoint(11, 19)
		};
		SetHighColor(184, 184, 184);
		FillPolygon(points, 15);
		SetHighColor(96, 96, 96);
		StrokePolygon(points, 15);
		FillRect(BRect(107, 121, 111, 123));
		SetHighColor(0, 0, 0);
		StrokeRect(BRect(14, 24, 136, 116));
		SetHighColor(0, 255, 0);
		FillRect(BRect(101, 122, 103, 123));
	} else {
		SetHighColor(152, 152, 152);
		StrokeLine(BPoint(11, 13), BPoint(67, 13));
		StrokeLine(BPoint(67, 21));
		StrokeLine(BPoint(139, 21));
		StrokeLine(BPoint(139, 119));
		StrokeLine(BPoint(11, 119));
		StrokeLine(BPoint(11, 13));
		StrokeRect(BRect(14, 24, 136, 116));
		SetHighColor(255, 203, 0);
		FillRect(BRect(12, 14, 66, 21));
		SetHighColor(240, 240, 240);
		StrokeRect(BRect(12, 22, 137, 117));
		StrokeLine(BPoint(138, 22), BPoint(138, 22));
		StrokeLine(BPoint(12, 118), BPoint(12, 118));
		SetHighColor(200, 200, 200);
		StrokeRect(BRect(13, 23, 138, 118));
	}

	SetHighColor(color);
	BBox::Draw(rect);
}


void
PreviewBox::SetDesktop(bool isDesktop)
{
	fIsDesktop = isDesktop;
	Invalidate();
}


//	#pragma mark -


BGImageMenuItem::BGImageMenuItem(const char *label, int32 imageIndex,
	BMessage *message, char shortcut, uint32 modifiers)
	: BMenuItem(label, message, shortcut, modifiers),
	fImageIndex(imageIndex)
{
}
