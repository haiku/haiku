/*
 * Copyright 2002-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jerome Duval, jerome.duval@free.fr
 *		Jonas Sundström, jonas@kirilla.se
 *		John Scipione, jscipione@gmail.com
 *		Brian Hill, supernova@warpmail.net
 */


#include "BackgroundsView.h"

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Cursor.h>
#include <Debug.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuField.h>
#include <Messenger.h>
#include <MimeType.h>
#include <Point.h>
#include <PopUpMenu.h>

#include <be_apps/Tracker/Background.h>
#include <ScreenDefs.h>

#include "ImageFilePanel.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Main View"


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
static const uint32 kMsgIconLabelOutline = 'ilol';

static const uint32 kMsgImagePlacement = 'xypl';
static const uint32 kMsgUpdatePreviewPlacement = 'pvpl';


BackgroundsView::BackgroundsView()
	:
	BBox("BackgroundsView"),
	fCurrent(NULL),
	fCurrentInfo(NULL),
	fLastImageIndex(-1),
	fRecentFoldersLimit(10),
	fPathList(1, true),
	fImageList(1, true),
	fFoundPositionSetting(false)
{
	SetBorder(B_NO_BORDER);

	BBox* previewBox = new BBox("preview");
	previewBox->SetLabel(B_TRANSLATE("Preview"));

	fPreview = new Preview();

	fTopLeft = new FramePart(FRAME_TOP_LEFT);
	fTop = new FramePart(FRAME_TOP);
	fTopRight = new FramePart(FRAME_TOP_RIGHT);
	fLeft = new FramePart(FRAME_LEFT_SIDE);
	fRight = new FramePart(FRAME_RIGHT_SIDE);
	fBottomLeft = new FramePart(FRAME_BOTTOM_LEFT);
	fBottom = new FramePart(FRAME_BOTTOM);
	fBottomRight = new FramePart(FRAME_BOTTOM_RIGHT);

	fXPlacementText = new BTextControl(B_TRANSLATE("X:"), NULL,
		new BMessage(kMsgImagePlacement));
	fYPlacementText = new BTextControl(B_TRANSLATE("Y:"), NULL,
		new BMessage(kMsgImagePlacement));

	// right-align text view
	fXPlacementText->TextView()->SetAlignment(B_ALIGN_RIGHT);
	fYPlacementText->TextView()->SetAlignment(B_ALIGN_RIGHT);

	// max 5 characters allowed
	fXPlacementText->TextView()->SetMaxBytes(5);
	fYPlacementText->TextView()->SetMaxBytes(5);

	// limit to numbers only
	for (int32 i = 0; i < 256; i++) {
		if ((i < '0' || i > '9') && i != '-') {
			fXPlacementText->TextView()->DisallowChar(i);
			fYPlacementText->TextView()->DisallowChar(i);
		}
	}

	previewBox->AddChild(BLayoutBuilder::Group<>()
		.AddGlue()
		.AddGroup(B_VERTICAL, 0)
			.AddGroup(B_HORIZONTAL, 0)
				.AddGlue()
				.AddGrid(0, 0, 1)
					.Add(fTopLeft, 0, 0)
					.Add(fTop, 1, 0)
					.Add(fTopRight, 2, 0)
					.Add(fLeft, 0, 1)
					.Add(fPreview, 1, 1)
					.Add(fRight, 2, 1)
					.Add(fBottomLeft, 0, 2)
					.Add(fBottom, 1, 2)
					.Add(fBottomRight, 2, 2)
					.End()
				.AddGlue()
				.End()
			.AddStrut(be_control_look->DefaultItemSpacing() * 2)
			.AddGroup(B_HORIZONTAL)
				.Add(fXPlacementText)
				.Add(fYPlacementText)
				.End()
			.AddGlue()
			.SetInsets(B_USE_DEFAULT_SPACING)
			.End()
		.AddGlue()
		.View());

	BBox* rightbox = new BBox("rightbox");

	fWorkspaceMenu = new BPopUpMenu(B_TRANSLATE("pick one"));
	fWorkspaceMenu->AddItem(new BMenuItem(B_TRANSLATE("All workspaces"),
		new BMessage(kMsgAllWorkspaces)));
	BMenuItem* menuItem;
	fWorkspaceMenu->AddItem(menuItem = new BMenuItem(
		B_TRANSLATE("Current workspace"),
		new BMessage(kMsgCurrentWorkspace)));
	menuItem->SetMarked(true);
	fWorkspaceMenu->AddSeparatorItem();
	fWorkspaceMenu->AddItem(new BMenuItem(B_TRANSLATE("Default folder"),
		new BMessage(kMsgDefaultFolder)));
	fWorkspaceMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Other folder" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOtherFolder)));

	BMenuField* workspaceMenuField = new BMenuField("workspaceMenuField",
		NULL, fWorkspaceMenu);
	workspaceMenuField->ResizeToPreferred();
	rightbox->SetLabel(workspaceMenuField);

	fImageMenu = new BPopUpMenu(B_TRANSLATE("pick one"));
	fImageMenu->AddItem(new BGImageMenuItem(B_TRANSLATE("None"), -1,
		new BMessage(kMsgNoImage)));
	fImageMenu->AddSeparatorItem();
	fImageMenu->AddItem(new BMenuItem(B_TRANSLATE("Other" B_UTF8_ELLIPSIS),
		new BMessage(kMsgOtherImage)));

	BMenuField* imageMenuField = new BMenuField("image",
		B_TRANSLATE("Image:"), fImageMenu);
	imageMenuField->SetAlignment(B_ALIGN_RIGHT);

	fPlacementMenu = new BPopUpMenu(B_TRANSLATE("pick one"));
	fPlacementMenu->AddItem(new BMenuItem(B_TRANSLATE("Manual"),
		new BMessage(kMsgManualPlacement)));
	fPlacementMenu->AddItem(new BMenuItem(B_TRANSLATE("Center"),
		new BMessage(kMsgCenterPlacement)));
	fPlacementMenu->AddItem(new BMenuItem(B_TRANSLATE("Scale to fit"),
		new BMessage(kMsgScalePlacement)));
	fPlacementMenu->AddItem(new BMenuItem(B_TRANSLATE("Tile"),
		new BMessage(kMsgTilePlacement)));

	BMenuField* placementMenuField = new BMenuField("placement",
		B_TRANSLATE("Placement:"), fPlacementMenu);
	placementMenuField->SetAlignment(B_ALIGN_RIGHT);

	fIconLabelOutline = new BCheckBox(B_TRANSLATE("Icon label outline"),
		new BMessage(kMsgIconLabelOutline));
	fIconLabelOutline->SetValue(B_CONTROL_OFF);

	fPicker = new BColorControl(BPoint(0, 0), B_CELLS_32x8, 8.0, "Picker",
		new BMessage(kMsgUpdateColor));

	rightbox->AddChild(BLayoutBuilder::Group<>(B_VERTICAL)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
				.Add(imageMenuField->CreateLabelLayoutItem(), 0, 0)
				.AddGroup(B_HORIZONTAL, 0.0f, 1, 0)
					.Add(imageMenuField->CreateMenuBarLayoutItem(), 0.0f)
					.AddGlue()
					.End()
				.Add(placementMenuField->CreateLabelLayoutItem(), 0, 1)
				.AddGroup(B_HORIZONTAL, 0.0f, 1, 1)
					.Add(placementMenuField->CreateMenuBarLayoutItem(), 0.0f)
					.AddGlue()
					.End()
				.Add(fIconLabelOutline, 1, 2)
				.End()
			.AddGlue()
			.End()
		.AddGlue()
		.Add(fPicker)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.View());

	fRevert = new BButton(B_TRANSLATE("Revert"),
		new BMessage(kMsgRevertSettings));
	fApply = new BButton(B_TRANSLATE("Apply"),
		new BMessage(kMsgApplySettings));

	fRevert->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_NO_VERTICAL));
	fApply->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
		B_ALIGN_NO_VERTICAL));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_WINDOW_SPACING)
		.AddGroup(B_HORIZONTAL, 0)
			.AddGroup(B_VERTICAL, 0)
				.AddStrut(floorf(rightbox->TopBorderOffset()
					- previewBox->TopBorderOffset()) - 1)
				.Add(previewBox)
				.End()
			.AddStrut(B_USE_DEFAULT_SPACING)
			.Add(rightbox)
			.End()
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fRevert)
			.Add(fApply)
			.SetInsets(0, B_USE_DEFAULT_SPACING, 0,	0)
			.End();

	fApply->MakeDefault(true);
}


BackgroundsView::~BackgroundsView()
{
	// The order matter. The last panel saves the state
	delete fFolderPanel;
	delete fPanel;
}


void
BackgroundsView::AllAttached()
{
	fPlacementMenu->SetTargetForItems(this);
	fImageMenu->SetTargetForItems(this);
	fWorkspaceMenu->SetTargetForItems(this);
	fXPlacementText->SetTarget(this);
	fYPlacementText->SetTarget(this);
	fIconLabelOutline->SetTarget(this);
	fPicker->SetTarget(this);
	fApply->SetTarget(this);
	fRevert->SetTarget(this);

	BPath path;
	entry_ref ref;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) == B_OK) {
		path.Append("artwork");
		get_ref_for_path(path.Path(), &ref);
	}

	BMessenger messenger(this);
	fPanel = new ImageFilePanel(B_OPEN_PANEL, &messenger, &ref,
		B_FILE_NODE, false, NULL, new CustomRefFilter(true));
	fPanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Select"));

	fFolderPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL,
		B_DIRECTORY_NODE, false, NULL, new CustomRefFilter(false));
	fFolderPanel->SetButtonLabel(B_DEFAULT_BUTTON, B_TRANSLATE("Select"));

	_LoadSettings();
	_LoadDesktopFolder();

	BPoint point;
	if (fSettings.FindPoint("pos", &point) == B_OK) {
		fFoundPositionSetting = true;
		Window()->MoveTo(point);
	}

	fApply->SetEnabled(false);
	fRevert->SetEnabled(false);
}


void
BackgroundsView::MessageReceived(BMessage* message)
{
	// Color drop
	if (message->WasDropped()) {
		rgb_color *clr;
		ssize_t out_size;
		if (message->FindData("RGBColor", B_RGB_COLOR_TYPE,
			(const void **)&clr, &out_size) == B_OK) {
			fPicker->SetValue(*clr);
			_UpdatePreview();
			_UpdateButtons();
			return;
		}
	}

	switch (message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			RefsReceived(message);
			break;

		case kMsgUpdatePreviewPlacement:
		{
			BString xstring, ystring;
			xstring << (int)fPreview->fPoint.x;
			ystring << (int)fPreview->fPoint.y;
			fXPlacementText->SetText(xstring.String());
			fYPlacementText->SetText(ystring.String());
			_UpdatePreview();
			_UpdateButtons();
			break;
		}

		case kMsgManualPlacement:
		case kMsgTilePlacement:
		case kMsgScalePlacement:
		case kMsgCenterPlacement:
			_UpdatePreview();
			_UpdateButtons();
			break;

		case kMsgIconLabelOutline:
			_UpdateButtons();
			break;

		case kMsgUpdateColor:
		case kMsgImagePlacement:
			_UpdatePreview();
			_UpdateButtons();
			break;

		case kMsgCurrentWorkspace:
		case kMsgAllWorkspaces:
			fImageMenu->FindItem(kMsgNoImage)->SetLabel(B_TRANSLATE("None"));
			if (fCurrent && fCurrent->IsDesktop()) {
				_UpdateButtons();
			} else {
				_SetDesktop(true);
				_LoadDesktopFolder();
			}
			break;

		case kMsgDefaultFolder:
			fImageMenu->FindItem(kMsgNoImage)->SetLabel(B_TRANSLATE("None"));
			_SetDesktop(false);
			_LoadDefaultFolder();
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
			message->FindPointer("source", &pointer);
			if (pointer == fPanel) {
				if (fLastImageIndex >= 0)
					_FindImageItem(fLastImageIndex)->SetMarked(true);
				else
					fImageMenu->ItemAt(0)->SetMarked(true);
			}
			break;
		}

		case kMsgImageSelected:
		case kMsgNoImage:
			fLastImageIndex = ((BGImageMenuItem*)fImageMenu->FindMarked())
				->ImageIndex();
			_UpdatePreview();
			_UpdateButtons();
			break;

		case kMsgFolderSelected:
		{
			fImageMenu->FindItem(kMsgNoImage)->SetLabel(B_TRANSLATE("Default"));
			_SetDesktop(false);
			BString folderPathStr;
			if (message->FindString("folderPath", &folderPathStr) == B_OK) {
				BPath folderPath(folderPathStr);
				_LoadRecentFolder(folderPath);
			}
			break;
		}
		case kMsgApplySettings:
		{
			_Save();

			// Notify the server and Screen preflet
			thread_id notifyThread;
			notifyThread = spawn_thread(BackgroundsView::_NotifyThread,
				"notifyThread", B_NORMAL_PRIORITY, this);
			resume_thread(notifyThread);
			_UpdateButtons();
			break;
		}
		case kMsgRevertSettings:
			_UpdateWithCurrent();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
BackgroundsView::_LoadDesktopFolder()
{
	BPath path;
	if (find_directory(B_DESKTOP_DIRECTORY, &path) == B_OK) {
		status_t error = get_ref_for_path(path.Path(), &fCurrentRef);
		if (error != B_OK)
			printf("error in LoadDesktopSettings\n");
		_LoadFolder(true);
	}
}


void
BackgroundsView::_LoadDefaultFolder()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BString pathString = path.Path();
		pathString << "/Tracker/DefaultFolderTemplate";
		status_t error = get_ref_for_path(pathString.String(), &fCurrentRef);
		if (error != B_OK)
			printf("error in LoadDefaultFolderSettings\n");
		_LoadFolder(false);
	}
}


void
BackgroundsView::_LoadRecentFolder(BPath path)
{
	status_t error = get_ref_for_path(path.Path(), &fCurrentRef);
	if (error != B_OK)
		printf("error in LoadRecentFolder\n");
	_LoadFolder(false);
}


void
BackgroundsView::_LoadFolder(bool isDesktop)
{
	if (fCurrent) {
		delete fCurrent;
		fCurrent = NULL;
	}

	BNode node(&fCurrentRef);
	if (node.InitCheck() == B_OK)
		fCurrent = BackgroundImage::GetBackgroundImage(&node, isDesktop, this);

	_UpdateWithCurrent();
}


void
BackgroundsView::_UpdateWithCurrent(void)
{
	if (fCurrent == NULL)
		return;

	fPlacementMenu->FindItem(kMsgScalePlacement)
		->SetEnabled(fCurrent->IsDesktop());
	fPlacementMenu->FindItem(kMsgCenterPlacement)
		->SetEnabled(fCurrent->IsDesktop());

	if (fWorkspaceMenu->IndexOf(fWorkspaceMenu->FindMarked()) > 5)
		fImageMenu->FindItem(kMsgNoImage)->SetLabel(B_TRANSLATE("Default"));
	else
		fImageMenu->FindItem(kMsgNoImage)->SetLabel(B_TRANSLATE("None"));

	for (int32 i = fImageMenu->CountItems() - 5; i >= 0; i--) {
		fImageMenu->RemoveItem(2);
	}

	for (int32 i = fImageList.CountItems() - 1; i >= 0; i--) {
		BMessage* message = new BMessage(kMsgImageSelected);
		_AddItem(new BGImageMenuItem(GetImage(i)->GetName(), i, message));
	}

	fImageMenu->SetTargetForItems(this);

	fCurrentInfo = fCurrent->ImageInfoForWorkspace(current_workspace());

	if (!fCurrentInfo) {
		fImageMenu->FindItem(kMsgNoImage)->SetMarked(true);
		fPlacementMenu->FindItem(kMsgManualPlacement)->SetMarked(true);
		fIconLabelOutline->SetValue(B_CONTROL_ON);
	} else {
		fIconLabelOutline->SetValue(fCurrentInfo->fTextWidgetLabelOutline
			? B_CONTROL_ON : B_CONTROL_OFF);

		fLastImageIndex = fCurrentInfo->fImageIndex;
		_FindImageItem(fLastImageIndex)->SetMarked(true);

		if (fLastImageIndex > -1) {

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
		} else {
			fPlacementMenu->FindItem(kMsgManualPlacement)->SetMarked(true);
		}
	}

	rgb_color color = {255, 255, 255, 255};
	if (fCurrent->IsDesktop()) {
		color = BScreen().DesktopColor();
		fPicker->SetEnabled(true);
	} else
		fPicker->SetEnabled(false);

	fPicker->SetValue(color);

	_UpdatePreview();
	_UpdateButtons();
}


void
BackgroundsView::_Save()
{
	bool textWidgetLabelOutline = fIconLabelOutline->Value() == B_CONTROL_ON;

	BackgroundImage::Mode mode = _FindPlacementMode();
	BPoint offset(atoi(fXPlacementText->Text()), atoi(fYPlacementText->Text()));

	if (!fCurrent->IsDesktop()) {
		if (fCurrentInfo == NULL) {
			fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
				B_ALL_WORKSPACES, fLastImageIndex, mode, offset,
				textWidgetLabelOutline, 0, 0);
			fCurrent->Add(fCurrentInfo);
		} else {
			fCurrentInfo->fTextWidgetLabelOutline = textWidgetLabelOutline;
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
				if (fCurrentInfo->fWorkspace & workspaceMask
					&& fCurrentInfo->fWorkspace != workspaceMask) {
					fCurrentInfo->fWorkspace = fCurrentInfo->fWorkspace
						^ workspaceMask;
					fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
						workspaceMask, fLastImageIndex, mode, offset,
						textWidgetLabelOutline, fCurrentInfo->fImageSet,
						fCurrentInfo->fCacheMode);
					fCurrent->Add(fCurrentInfo);
				} else if (fCurrentInfo->fWorkspace == workspaceMask) {
					fCurrentInfo->fTextWidgetLabelOutline
						= textWidgetLabelOutline;
					fCurrentInfo->fMode = mode;
					if (fCurrentInfo->fMode == BackgroundImage::kAtOffset)
						fCurrentInfo->fOffset = offset;

					fCurrentInfo->fImageIndex = fLastImageIndex;
				}
			} else {
				fCurrent->RemoveAll();

				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					B_ALL_WORKSPACES, fLastImageIndex, mode, offset,
					textWidgetLabelOutline, fCurrent->GetShowingImageSet(),
					fCurrentInfo->fCacheMode);
				fCurrent->Add(fCurrentInfo);
			}
		} else {
			if (fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->IsMarked()) {
				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					workspaceMask, fLastImageIndex, mode, offset,
					textWidgetLabelOutline, fCurrent->GetShowingImageSet(), 0);
			} else {
				fCurrent->RemoveAll();
				fCurrentInfo = new BackgroundImage::BackgroundImageInfo(
					B_ALL_WORKSPACES, fLastImageIndex, mode, offset,
					textWidgetLabelOutline, fCurrent->GetShowingImageSet(), 0);
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
		BString error(strerror(status));
		BString text(B_TRANSLATE("Setting the background image failed:"));
		text.Append("\n").Append(error);
		BAlert* alert = new BAlert(B_TRANSLATE("Set background image error"),
			text, B_TRANSLATE("OK"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go(NULL);
		printf("setting background image failed: %s\n", error.String());
	}
}


void
BackgroundsView::_NotifyServer()
{
	BMessenger tracker("application/x-vnd.Be-TRAK");

	if (fCurrent->IsDesktop()) {
		tracker.SendMessage(new BMessage(B_RESTORE_BACKGROUND_IMAGE));
	} else {
		int32 i = -1;
		BMessage reply;
		int32 error;
		BEntry currentEntry(&fCurrentRef);
		BPath currentPath(&currentEntry);
		bool isCustomFolder
			= !fWorkspaceMenu->FindItem(kMsgDefaultFolder)->IsMarked();

		do {
			BMessage message(B_GET_PROPERTY);
			i++;

			// look at the "Poses" in every Tracker window
			message.AddSpecifier("Poses");
			message.AddSpecifier("Window", i);

			reply.MakeEmpty();
			tracker.SendMessage(&message, &reply);

			// break out of the loop when we're at the end of
			// the windows
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD
				&& reply.FindInt32("error", &error) == B_OK
				&& error == B_BAD_INDEX)
				break;

			// don't stop for windows that don't understand
			// a request for "Poses"; they're not displaying
			// folders
			if (reply.what == B_MESSAGE_NOT_UNDERSTOOD
				&& reply.FindInt32("error", &error) == B_OK
				&& error != B_BAD_SCRIPT_SYNTAX)
				continue;

			BMessenger trackerWindow;
			if (reply.FindMessenger("result", &trackerWindow) != B_OK)
				continue;

			if (isCustomFolder) {
				// found a window with poses, ask for its path
				message.MakeEmpty();
				message.what = B_GET_PROPERTY;
				message.AddSpecifier("Path");
				message.AddSpecifier("Poses");
				message.AddSpecifier("Window", i);

				reply.MakeEmpty();
				tracker.SendMessage(&message, &reply);

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


void
BackgroundsView::_NotifyScreenPreflet()
{
	BMessenger messenger("application/x-vnd.Haiku-Screen");
	if (messenger.IsValid())
		messenger.SendMessage(UPDATE_DESKTOP_COLOR_MSG);
}


int32
BackgroundsView::_NotifyThread(void* data)
{
	BackgroundsView* view = (BackgroundsView*)data;

	view->_NotifyServer();
	view->_NotifyScreenPreflet();
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
BackgroundsView::_LoadSettings()
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

	BString settingStr;
	if (fSettings.FindString("paneldir", &settingStr) == B_OK)
		fPanel->SetPanelDirectory(settingStr.String());

	if (fSettings.FindString("folderpaneldir", &settingStr) == B_OK)
		fFolderPanel->SetPanelDirectory(settingStr.String());

	int32 index = 0;
	while (fSettings.FindString("recentfolder", index, &settingStr) == B_OK) {
		path.SetTo(settingStr.String());
		_AddRecentFolder(path);
		index++;
	}

	fWorkspaceMenu->ItemAt(1)->SetMarked(true);
	fWorkspaceMenu->SetTargetForItems(this);

	PRINT(("Settings Loaded\n"));
}


void
BackgroundsView::WorkspaceActivated(uint32 oldWorkspaces, bool active)
{
	_UpdateWithCurrent();
}


void
BackgroundsView::_UpdatePreview()
{
	bool imageEnabled = !(fImageMenu->FindItem(kMsgNoImage)->IsMarked());
	if (fPlacementMenu->IsEnabled() ^ imageEnabled)
		fPlacementMenu->SetEnabled(imageEnabled);

	bool textEnabled
		= (fPlacementMenu->FindItem(kMsgManualPlacement)->IsMarked())
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

	fPreview->ClearViewBitmap();

	int32 index = ((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex();
	if (index >= 0) {
		BBitmap* bitmap = GetImage(index)->GetBitmap();
		if (bitmap) {
			BackgroundImage::BackgroundImageInfo* info
				= new BackgroundImage::BackgroundImageInfo(0, index,
					_FindPlacementMode(), BPoint(atoi(fXPlacementText->Text()),
						atoi(fYPlacementText->Text())),
					fIconLabelOutline->Value() == B_CONTROL_ON, 0, 0);
			if (info->fMode == BackgroundImage::kAtOffset) {
				fPreview->SetEnabled(true);
				fPreview->fPoint.x = atoi(fXPlacementText->Text());
				fPreview->fPoint.y = atoi(fYPlacementText->Text());
			} else
				fPreview->SetEnabled(false);

			fPreview->fImageBounds = BRect(bitmap->Bounds());
			fCurrent->Show(info, fPreview);

			delete info;
		}
	} else
		fPreview->SetEnabled(false);

	fPreview->SetViewColor(fPicker->ValueAsColor());
	fPreview->Invalidate();
}


BackgroundImage::Mode
BackgroundsView::_FindPlacementMode()
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


void
BackgroundsView::_UpdateButtons()
{
	bool hasChanged = false;
	if (fPicker->IsEnabled()
		&& fPicker->ValueAsColor() != BScreen().DesktopColor()) {
		hasChanged = true;
	} else if (fCurrentInfo) {
		if ((fIconLabelOutline->Value() == B_CONTROL_ON)
			^ fCurrentInfo->fTextWidgetLabelOutline) {
			hasChanged = true;
		} else if (_FindPlacementMode() != fCurrentInfo->fMode) {
			hasChanged = true;
		} else if (fCurrentInfo->fImageIndex
			!= ((BGImageMenuItem*)fImageMenu->FindMarked())->ImageIndex()) {
			hasChanged = true;
		} else if (fCurrent->IsDesktop()
			&& ((fCurrentInfo->fWorkspace != B_ALL_WORKSPACES)
				^ (fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->IsMarked())))
		{
			hasChanged = true;
		} else if (fCurrentInfo->fImageIndex > -1
			&& fCurrentInfo->fMode == BackgroundImage::kAtOffset) {
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
	} else if (fIconLabelOutline->Value() == B_CONTROL_OFF) {
		hasChanged = true;
	}

	fApply->SetEnabled(hasChanged);
	fRevert->SetEnabled(hasChanged);
}


void
BackgroundsView::RefsReceived(BMessage* message)
{
	if (!message->HasRef("refs") && message->HasRef("dir_ref")) {
		entry_ref dirRef;
		if (message->FindRef("dir_ref", &dirRef) == B_OK)
			message->AddRef("refs", &dirRef);
	}

	entry_ref ref;
	int32 i = 0;
	BMimeType imageType("image");
	BPath desktopPath;
	find_directory(B_DESKTOP_DIRECTORY, &desktopPath);

	while (message->FindRef("refs", i++, &ref) == B_OK) {
		BPath path;
		BEntry entry(&ref, true);
		path.SetTo(&entry);
		BNode node(&entry);

		if (node.IsFile()) {
			BMimeType refType;
			BMimeType::GuessMimeType(&ref, &refType);
			if (!imageType.Contains(&refType))
				continue;

			BGImageMenuItem* item;
			int32 index = AddImage(path);
			if (index >= 0) {
				item = _FindImageItem(index);
				fLastImageIndex = index;
			} else {
				const char* name = GetImage(-index - 1)->GetName();
				item = new BGImageMenuItem(name, -index - 1,
					new BMessage(kMsgImageSelected));
				_AddItem(item);
				item->SetTarget(this);
				fLastImageIndex = -index - 1;
			}

			// An optional placement may have been sent
			int32 placement = 0;
			if (message->FindInt32("placement", &placement) == B_OK) {
				BMenuItem* item = fPlacementMenu->FindItem(placement);
				if (item)
					item->SetMarked(true);
			}
			item->SetMarked(true);
			BMessenger(this).SendMessage(kMsgImageSelected);
		} else if (node.IsDirectory()) {
			if (desktopPath == path) {
				fWorkspaceMenu->FindItem(kMsgCurrentWorkspace)->SetMarked(true);
				BMessenger(this).SendMessage(kMsgCurrentWorkspace);
				break;
			}

			// Add the newly selected path as a recent folder,
			// removing the oldest entry if needed
			_AddRecentFolder(path, true);
		}
	}
}


static BPath*
FindPath(BPath* currentPath, void* newPath)
{
	BPath* pathToCheck = static_cast<BPath*>(newPath);
	int compare = ICompare(currentPath->Path(), pathToCheck->Path());
	return compare == 0 ? currentPath : NULL;
}


void
BackgroundsView::_AddRecentFolder(BPath path, bool notifyApp)
{
	BPath* currentPath = fPathList.EachElement(FindPath, &path);
	BMenuItem* item;
	BMessage* folderSelectedMsg = new BMessage(kMsgFolderSelected);
	folderSelectedMsg->AddString("folderPath", path.Path());

	if (currentPath == NULL) {
		// "All Workspaces", "Current Workspace", "--", "Default folder",
		// "Other folder...", If only these 5 exist,
		// we need a new separator to add path specific recent folders.
		if (fWorkspaceMenu->CountItems() <= 5)
			fWorkspaceMenu->AddSeparatorItem();
		// Maxed out the number of recent folders, remove the oldest entry
		if (fPathList.CountItems() == fRecentFoldersLimit) {
			fPathList.RemoveItemAt(0);
			fWorkspaceMenu->RemoveItem(6);
		}
		// Add the new recent folder
		BString folderMenuText(B_TRANSLATE("Folder: %path"));
		folderMenuText.ReplaceFirst("%path", path.Leaf());
		item = new BMenuItem(folderMenuText.String(), folderSelectedMsg);
		fWorkspaceMenu->AddItem(item);
		fPathList.AddItem(new BPath(path));
		item->SetTarget(this);
	} else {
		int32 itemIndex = fPathList.IndexOf(currentPath);
		item = fWorkspaceMenu->ItemAt(itemIndex + 6);
	}

	item->SetMarked(true);

	if (notifyApp)
		BMessenger(this).SendMessage(folderSelectedMsg);
}


int32
BackgroundsView::AddImage(BPath path)
{
	int32 count = fImageList.CountItems();
	int32 index = 0;
	for (; index < count; index++) {
		Image* image = fImageList.ItemAt(index);
		if (image->GetPath() == path)
			return index;
	}

	fImageList.AddItem(new Image(path));
	return -index - 1;
}


Image*
BackgroundsView::GetImage(int32 imageIndex)
{
	return fImageList.ItemAt(imageIndex);
}


BGImageMenuItem*
BackgroundsView::_FindImageItem(const int32 imageIndex)
{
	if (imageIndex < 0)
		return (BGImageMenuItem*)fImageMenu->ItemAt(0);

	int32 count = fImageMenu->CountItems() - 2;
	int32 index = 2;
	for (; index < count; index++) {
		BGImageMenuItem* image = (BGImageMenuItem*)fImageMenu->ItemAt(index);
		if (image->ImageIndex() == imageIndex)
			return image;
	}
	return NULL;
}


bool
BackgroundsView::_AddItem(BGImageMenuItem* item)
{
	int32 count = fImageMenu->CountItems() - 2;
	int32 index = 2;
	if (count < index) {
		fImageMenu->AddItem(new BSeparatorItem(), 1);
		count = fImageMenu->CountItems() - 2;
	}

	for (; index < count; index++) {
		BGImageMenuItem* image = (BGImageMenuItem*)fImageMenu->ItemAt(index);
		int c = (BString(image->Label()).ICompare(BString(item->Label())));
		if (c > 0)
			break;
	}
	return fImageMenu->AddItem(item, index);
}


void
BackgroundsView::_SetDesktop(bool isDesktop)
{
	fTopLeft->SetDesktop(isDesktop);
	fTop->SetDesktop(isDesktop);
	fTopRight->SetDesktop(isDesktop);
	fLeft->SetDesktop(isDesktop);
	fRight->SetDesktop(isDesktop);
	fBottomLeft->SetDesktop(isDesktop);
	fBottom->SetDesktop(isDesktop);
	fBottomRight->SetDesktop(isDesktop);

	Invalidate();
}


bool
BackgroundsView::FoundPositionSetting()
{
	return fFoundPositionSetting;
}


//	#pragma mark -


Preview::Preview()
	:
	BControl("PreView", NULL, NULL, B_WILL_DRAW | B_SUBPIXEL_PRECISE)
{
	float aspectRatio = BScreen().Frame().Width() / BScreen().Frame().Height();
	float previewWidth = 120.0f * std::max(1.0f, be_plain_font->Size() / 12.0f);
	float previewHeight = ceil(previewWidth / aspectRatio);

	ResizeTo(previewWidth, previewHeight);
	SetExplicitMinSize(BSize(previewWidth, previewHeight));
	SetExplicitMaxSize(BSize(previewWidth, previewHeight));
}


void
Preview::AttachedToWindow()
{
	rgb_color color = ViewColor();
	BControl::AttachedToWindow();
	SetViewColor(color);
}


void
Preview::MouseDown(BPoint point)
{
	if (IsEnabled() && Bounds().Contains(point)) {
		uint32 buttons;
		GetMouse(&point, &buttons);
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			fOldPoint = point;
			SetTracking(true);
			BScreen().GetMode(&fMode);
			fXRatio = Bounds().Width() / fMode.virtual_width;
			fYRatio = Bounds().Height() / fMode.virtual_height;
			SetMouseEventMask(B_POINTER_EVENTS,
				B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);

			BCursor grabbingCursor(B_CURSOR_ID_GRABBING);
			SetViewCursor(&grabbingCursor);
		}
	}
}


void
Preview::MouseUp(BPoint point)
{
	if (IsTracking()) {
		SetTracking(false);
		BCursor grabCursor(B_CURSOR_ID_GRAB);
		SetViewCursor(&grabCursor);
	}
}


void
Preview::MouseMoved(BPoint point, uint32 transit, const BMessage* message)
{
	if (!IsTracking()) {
		BCursor cursor(IsEnabled()
			? B_CURSOR_ID_GRAB : B_CURSOR_ID_SYSTEM_DEFAULT);
		SetViewCursor(&cursor);
	} else {
		float x, y;
		x = fPoint.x + (point.x - fOldPoint.x) / fXRatio;
		y = fPoint.y + (point.y - fOldPoint.y) / fYRatio;
		bool min, max, mustSend = false;
		min = (x > -fImageBounds.Width());
		max = (x < fMode.virtual_width);
		if (min && max) {
			fOldPoint.x = point.x;
			fPoint.x = x;
			mustSend = true;
		} else {
			if (!min && fPoint.x > -fImageBounds.Width()) {
				fPoint.x = -fImageBounds.Width();
				fOldPoint.x = point.x - (x - fPoint.x) * fXRatio;
				mustSend = true;
			}
			if (!max && fPoint.x < fMode.virtual_width) {
				fPoint.x = fMode.virtual_width;
				fOldPoint.x = point.x - (x - fPoint.x) * fXRatio;
				mustSend = true;
			}
		}

		min = (y > -fImageBounds.Height());
		max = (y < fMode.virtual_height);
		if (min && max) {
			fOldPoint.y = point.y;
			fPoint.y = y;
			mustSend = true;
		} else {
			if (!min && fPoint.y > -fImageBounds.Height()) {
				fPoint.y = -fImageBounds.Height();
				fOldPoint.y = point.y - (y - fPoint.y) * fYRatio;
				mustSend = true;
			}
			if (!max && fPoint.y < fMode.virtual_height) {
				fPoint.y = fMode.virtual_height;
				fOldPoint.y = point.y - (y - fPoint.y) * fYRatio;
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


BGImageMenuItem::BGImageMenuItem(const char* label, int32 imageIndex,
	BMessage* message, char shortcut, uint32 modifiers)
	: BMenuItem(label, message, shortcut, modifiers),
	fImageIndex(imageIndex)
{
}


//	#pragma mark -


FramePart::FramePart(int32 part)
	:
	BView(NULL, B_WILL_DRAW | B_FRAME_EVENTS),
	fFramePart(part),
	fIsDesktop(true)
{
	_SetSizeAndAlignment();
}


void
FramePart::Draw(BRect rect)
{
	rgb_color color = HighColor();
	SetDrawingMode(B_OP_COPY);
	SetHighColor(Parent()->ViewColor());

	if (fIsDesktop) {
		switch (fFramePart) {
			case FRAME_TOP_LEFT:
				FillRect(rect);
				SetHighColor(160, 160, 160);
				FillRoundRect(BRect(0, 0, 8, 8), 3, 3);
				SetHighColor(96, 96, 96);
				StrokeRoundRect(BRect(0, 0, 8, 8), 3, 3);
				break;

			case FRAME_TOP:
				SetHighColor(160, 160, 160);
				FillRect(BRect(0, 1, rect.right, 3));
				SetHighColor(96, 96, 96);
				StrokeLine(BPoint(0, 0), BPoint(rect.right, 0));
				SetHighColor(0, 0, 0);
				StrokeLine(BPoint(0, 4), BPoint(rect.right, 4));
				break;

			case FRAME_TOP_RIGHT:
				FillRect(rect);
				SetHighColor(160, 160, 160);
				FillRoundRect(BRect(-4, 0, 4, 8), 3, 3);
				SetHighColor(96, 96, 96);
				StrokeRoundRect(BRect(-4, 0, 4, 8), 3, 3);
				break;

			case FRAME_LEFT_SIDE:
				SetHighColor(160, 160, 160);
				FillRect(BRect(1, 0, 3, rect.bottom));
				SetHighColor(96, 96, 96);
				StrokeLine(BPoint(0, 0), BPoint(0, rect.bottom));
				SetHighColor(0, 0, 0);
				StrokeLine(BPoint(4, 0), BPoint(4, rect.bottom));
				break;

			case FRAME_RIGHT_SIDE:
				SetHighColor(160, 160, 160);
				FillRect(BRect(1, 0, 3, rect.bottom));
				SetHighColor(0, 0, 0);
				StrokeLine(BPoint(0, 0), BPoint(0, rect.bottom));
				SetHighColor(96, 96, 96);
				StrokeLine(BPoint(4, 0), BPoint(4, rect.bottom));
				break;

			case FRAME_BOTTOM_LEFT:
				FillRect(rect);
				SetHighColor(160, 160, 160);
				FillRoundRect(BRect(0, -4, 8, 4), 3, 3);
				SetHighColor(96, 96, 96);
				StrokeRoundRect(BRect(0, -4, 8, 4), 3, 3);
				break;

			case FRAME_BOTTOM:
				SetHighColor(160, 160, 160);
				FillRect(BRect(0, 1, rect.right, 3));
				SetHighColor(0, 0, 0);
				StrokeLine(BPoint(0, 0), BPoint(rect.right, 0));
				SetHighColor(96, 96, 96);
				StrokeLine(BPoint(0, 4), BPoint(rect.right, 4));
				SetHighColor(228, 0, 0);
				StrokeLine(BPoint(5, 2), BPoint(7, 2));
				break;

			case FRAME_BOTTOM_RIGHT:
				FillRect(rect);
				SetHighColor(160, 160, 160);
				FillRoundRect(BRect(-4, -4, 4, 4), 3, 3);
				SetHighColor(96, 96, 96);
				StrokeRoundRect(BRect(-4, -4, 4, 4), 3, 3);
				break;

			default:
				break;
		}
	} else {
		switch (fFramePart) {
			case FRAME_TOP_LEFT:
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 0), BPoint(0, 12));
				StrokeLine(BPoint(0, 0), BPoint(4, 0));
				StrokeLine(BPoint(3, 12), BPoint(3, 12));
				SetHighColor(255, 203, 0);
				FillRect(BRect(1, 1, 3, 9));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(1, 12), BPoint(1, 10));
				StrokeLine(BPoint(2, 10), BPoint(3, 10));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(2, 12), BPoint(2, 11));
				StrokeLine(BPoint(3, 11), BPoint(3, 11));
				break;

			case FRAME_TOP:
				FillRect(BRect(54, 0, rect.right, 8));
				SetHighColor(255, 203, 0);
				FillRect(BRect(0, 1, 52, 9));
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 0), BPoint(53, 0));
				StrokeLine(BPoint(53, 1), BPoint(53, 9));
				StrokeLine(BPoint(54, 9), BPoint(rect.right, 9));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(0, 10), BPoint(rect.right, 10));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(0, 11), BPoint(rect.right, 11));
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 12), BPoint(rect.right, 12));
				break;

			case FRAME_TOP_RIGHT:
				FillRect(BRect(0, 0, 3, 8));
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 12), BPoint(0, 12));
				StrokeLine(BPoint(0, 9), BPoint(3, 9));
				StrokeLine(BPoint(3, 12), BPoint(3, 9));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(0, 10), BPoint(2, 10));
				StrokeLine(BPoint(1, 12), BPoint(1, 12));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(2, 12), BPoint(2, 12));
				StrokeLine(BPoint(0, 11), BPoint(2, 11));
				break;

			case FRAME_LEFT_SIDE:
			case FRAME_RIGHT_SIDE:
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 0), BPoint(0, rect.bottom));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(1, 0), BPoint(1, rect.bottom));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(2, 0), BPoint(2, rect.bottom));
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(3, 0), BPoint(3, rect.bottom));
				break;

			case FRAME_BOTTOM_LEFT:
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 0), BPoint(0, 3));
				StrokeLine(BPoint(0, 3), BPoint(3, 3));
				StrokeLine(BPoint(3, 0), BPoint(3, 0));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(1, 0), BPoint(1, 2));
				StrokeLine(BPoint(3, 1), BPoint(3, 1));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(2, 0), BPoint(2, 2));
				StrokeLine(BPoint(3, 2), BPoint(3, 2));
				break;

			case FRAME_BOTTOM:
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 0), BPoint(rect.right, 0));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(0, 1), BPoint(rect.right, 1));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(0, 2), BPoint(rect.right, 2));
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 3), BPoint(rect.right, 3));
				break;

			case FRAME_BOTTOM_RIGHT:
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(0, 0), BPoint(0, 0));
				SetHighColor(240, 240, 240);
				StrokeLine(BPoint(1, 0), BPoint(1, 1));
				StrokeLine(BPoint(0, 1), BPoint(0, 1));
				SetHighColor(200, 200, 200);
				StrokeLine(BPoint(2, 0), BPoint(2, 2));
				StrokeLine(BPoint(0, 2), BPoint(1, 2));
				SetHighColor(152, 152, 152);
				StrokeLine(BPoint(3, 0), BPoint(3, 3));
				StrokeLine(BPoint(0, 3), BPoint(2, 3));
				break;

			default:
				break;
		}
	}

	SetHighColor(color);
}


void
FramePart::SetDesktop(bool isDesktop)
{
	fIsDesktop = isDesktop;

	_SetSizeAndAlignment();
	Invalidate();
}


void
FramePart::_SetSizeAndAlignment()
{
	if (fIsDesktop) {
		switch (fFramePart) {
			case FRAME_TOP_LEFT:
				SetExplicitMinSize(BSize(4, 4));
				SetExplicitMaxSize(BSize(4, 4));
				break;

			case FRAME_TOP:
				SetExplicitMinSize(BSize(1, 4));
				SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 4));
				break;

			case FRAME_TOP_RIGHT:
				SetExplicitMinSize(BSize(4, 4));
				SetExplicitMaxSize(BSize(4, 4));
				break;

			case FRAME_LEFT_SIDE:
				SetExplicitMinSize(BSize(4, 1));
				SetExplicitMaxSize(BSize(4, B_SIZE_UNLIMITED));
				break;

			case FRAME_RIGHT_SIDE:
				SetExplicitMinSize(BSize(4, 1));
				SetExplicitMaxSize(BSize(4, B_SIZE_UNLIMITED));
				break;

			case FRAME_BOTTOM_LEFT:
				SetExplicitMinSize(BSize(4, 4));
				SetExplicitMaxSize(BSize(4, 4));
				break;

			case FRAME_BOTTOM:
				SetExplicitMinSize(BSize(1, 4));
				SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 4));
				break;

			case FRAME_BOTTOM_RIGHT:
				SetExplicitMaxSize(BSize(4, 4));
				SetExplicitMinSize(BSize(4, 4));
				break;

			default:
				break;
		}
	} else {
		switch (fFramePart) {
			case FRAME_TOP_LEFT:
				SetExplicitMinSize(BSize(3, 12));
				SetExplicitMaxSize(BSize(3, 12));
				break;

			case FRAME_TOP:
				SetExplicitMinSize(BSize(1, 12));
				SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 12));
				break;

			case FRAME_TOP_RIGHT:
				SetExplicitMinSize(BSize(3, 12));
				SetExplicitMaxSize(BSize(3, 12));
				break;

			case FRAME_LEFT_SIDE:
				SetExplicitMinSize(BSize(3, 1));
				SetExplicitMaxSize(BSize(3, B_SIZE_UNLIMITED));
				break;

			case FRAME_RIGHT_SIDE:
				SetExplicitMinSize(BSize(3, 1));
				SetExplicitMaxSize(BSize(3, B_SIZE_UNLIMITED));
				break;

			case FRAME_BOTTOM_LEFT:
				SetExplicitMinSize(BSize(3, 3));
				SetExplicitMaxSize(BSize(3, 3));
				break;

			case FRAME_BOTTOM:
				SetExplicitMinSize(BSize(1, 3));
				SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 3));
				break;

			case FRAME_BOTTOM_RIGHT:
				SetExplicitMaxSize(BSize(3, 3));
				SetExplicitMinSize(BSize(3, 3));
				break;

			default:
				break;
		}
	}

	switch (fFramePart) {
		case FRAME_TOP_LEFT:
			SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_BOTTOM));
			break;

		case FRAME_TOP:
			SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_BOTTOM));
			break;

		case FRAME_TOP_RIGHT:
			SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_BOTTOM));
			break;

		case FRAME_LEFT_SIDE:
			SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
			break;

		case FRAME_RIGHT_SIDE:
			SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
			break;

		case FRAME_BOTTOM_LEFT:
			SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_TOP));
			break;

		case FRAME_BOTTOM:
			SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_TOP));
			break;

		case FRAME_BOTTOM_RIGHT:
			SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
			break;

		default:
			break;
	}
}

