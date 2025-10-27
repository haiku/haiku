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


#include "FilePanelPriv.h"

#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <ControlLook.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <GridView.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <SymLink.h>
#include <ScrollView.h>
#include <String.h>
#include <StopWatch.h>
#include <TextControl.h>
#include <TextView.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "AttributeStream.h"
#include "Attributes.h"
#include "AutoLock.h"
#include "Commands.h"
#include "CountView.h"
#include "DesktopPoseView.h"
#include "DirMenu.h"
#include "FSClipboard.h"
#include "FSUtils.h"
#include "FavoritesMenu.h"
#include "IconMenuItem.h"
#include "LiveMenu.h"
#include "MimeTypes.h"
#include "NavMenu.h"
#include "Shortcuts.h"
#include "Tracker.h"
#include "TrackerDefaults.h"
#include "Utilities.h"

#include "tracker_private.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilePanelPriv"


const char* kDefaultFilePanelTemplate = "FilePanelSettings";


static uint32
GetLinkFlavor(const Model* model, bool resolve = true)
{
	if (model && model->IsSymLink()) {
		if (!resolve)
			return B_SYMLINK_NODE;
		model = model->LinkTo();
	}
	if (!model)
		return 0;

	if (model->IsDirectory())
		return B_DIRECTORY_NODE;

	return B_FILE_NODE;
}


static filter_result
key_down_filter(BMessage* message, BHandler** handler, BMessageFilter* filter)
{
	if (filter == NULL)
		return B_DISPATCH_MESSAGE;

	TFilePanel* panel = dynamic_cast<TFilePanel*>(filter->Looper());
	if (panel == NULL || panel->TrackingMenu())
		return B_DISPATCH_MESSAGE;

	BPoseView* view = panel->PoseView();
	if (view == NULL)
		return B_DISPATCH_MESSAGE;

	uchar key;
	if (message->FindInt8("byte", (int8*)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	int32 modifiers = message->GetInt32("modifiers", 0);
	modifiers &= B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY | B_CONTROL_KEY | B_MENU_KEY;

	if ((modifiers & B_COMMAND_KEY) != 0) {
		switch (key) {
			case B_UP_ARROW:
				BMessenger(panel).SendMessage(kOpenParentDir);
				return B_SKIP_MESSAGE;

			case 'w':
				BMessenger(panel).SendMessage(kCancelButton);
				return B_SKIP_MESSAGE;

			default:
				break;
		}
	}

	if (modifiers == 0 && key == B_ESCAPE) {
		if (view->ActivePose() != NULL)
			view->CommitActivePose(false);
		else if (view->IsTypeAheadFiltering())
			BMessenger(panel->PoseView()).SendMessage(B_CANCEL, *handler);
		else
			BMessenger(panel).SendMessage(kCancelButton);

		return B_SKIP_MESSAGE;
	}

	if (key == B_RETURN && view->ActivePose() != NULL) {
		view->CommitActivePose();

		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark - TFilePanel


TFilePanel::TFilePanel(file_panel_mode mode, BMessenger* target, const BEntry* startDir,
	uint32 nodeFlavors, bool multipleSelection, BMessage* message, BRefFilter* filter,
	uint32 openFlags, window_look look, window_feel feel, uint32 windowFlags, uint32 workspace,
	bool hideWhenDone)
	:
	BContainerWindow(0, openFlags, look, feel, windowFlags, workspace, false),
	fDirMenu(NULL),
	fDirMenuField(NULL),
	fTextControl(NULL),
	fClientObject(NULL),
	fSelectionIterator(0),
	fMessage(NULL),
	fFavoritesMenu(NULL),
	fHideWhenDone(hideWhenDone),
	fIsTrackingMenu(false),
	fDefaultStateRestored(false)
{
	InitIconPreloader();

	fIsSavePanel = (mode == B_SAVE_PANEL);

	const float labelSpacing = be_control_look->DefaultLabelSpacing();
	// approximately (84, 50, 568, 296) with default sizing
	BRect windRect(labelSpacing * 14.0f, labelSpacing * 8.0f,
		labelSpacing * 95.0f, labelSpacing * 49.0f);
	MoveTo(windRect.LeftTop());
	ResizeTo(windRect.Width(), windRect.Height());

	fNodeFlavors = (nodeFlavors == 0) ? B_FILE_NODE : nodeFlavors;

	if (target)
		fTarget = *target;
	else
		fTarget = BMessenger(be_app);

	if (message)
		SetMessage(message);
	else if (fIsSavePanel)
		fMessage = new BMessage(B_SAVE_REQUESTED);
	else
		fMessage = new BMessage(B_REFS_RECEIVED);

	// no new template menu in file panel
	fNewTemplatesItem = NULL;

	gLocalizedNamePreferred = BLocaleRoster::Default()->IsFilesystemTranslationPreferred();

	// check for legal starting directory
	Model* model = new Model();
	bool useRoot = true;

	if (startDir) {
		if (model->SetTo(startDir) == B_OK && model->IsDirectory())
			useRoot = false;
		else {
			delete model;
			model = new Model();
		}
	}

	if (useRoot) {
		BPath path;
		if (find_directory(B_USER_DIRECTORY, &path) == B_OK) {
			BEntry entry(path.Path(), true);
			if (entry.InitCheck() == B_OK && model->SetTo(&entry) == B_OK)
				useRoot = false;
		}
	}

	if (useRoot) {
		BVolume volume;
		BDirectory root;
		BVolumeRoster volumeRoster;
		volumeRoster.GetBootVolume(&volume);
		volume.GetRootDirectory(&root);

		BEntry entry;
		root.GetEntry(&entry);
		model->SetTo(&entry);
	}

	fTaskLoop = new PiggybackTaskLoop;

	AutoLock<BWindow> lock(this);
	fBorderedView = new BorderedView;
	CreatePoseView(model);
	fBorderedView->GroupLayout()->SetInsets(1);

	fPoseContainer = new BGridView(0.0, 0.0);
	fPoseContainer->GridLayout()->AddView(fBorderedView, 0, 1);

	fCountContainer = new BGroupView(B_HORIZONTAL, 0);
	fPoseContainer->GridLayout()->AddView(fCountContainer, 0, 2);

	fPoseView->SetRefFilter(filter);
	if (!fIsSavePanel)
		fPoseView->SetMultipleSelection(multipleSelection);

	fPoseView->SetFlags(fPoseView->Flags() | B_NAVIGABLE);
	fPoseView->SetPoseEditing(false);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, key_down_filter));
	AddCommonFilter(new BMessageFilter(B_SIMPLE_DATA, TFilePanel::MessageDropFilter));
	AddCommonFilter(new BMessageFilter(B_NODE_MONITOR, TFilePanel::FSFilter));

	// inter-application observing
	BMessenger tracker(kTrackerSignature);
	BHandler::StartWatching(tracker, kDesktopFilePanelRootChanged);

	Init();

	// Avoid the need to save state later just because of changes made
	// during setup. This prevents unnecessary saving by Quit that can
	// overwrite user's changes previously saved from another panel object.
	if (StateNeedsSaving())
		SaveState(false);
}


TFilePanel::~TFilePanel()
{
	BMessenger tracker(kTrackerSignature);
	BHandler::StopWatching(tracker, kDesktopFilePanelRootChanged);

	delete fMessage;
}


filter_result
TFilePanel::MessageDropFilter(BMessage* message, BHandler**, BMessageFilter* filter)
{
	if (message == NULL || !message->WasDropped())
		return B_DISPATCH_MESSAGE;

	ASSERT(filter != NULL);
	if (filter == NULL)
		return B_DISPATCH_MESSAGE;

	TFilePanel* panel = dynamic_cast<TFilePanel*>(filter->Looper());
	ASSERT(panel != NULL);

	if (panel == NULL)
		return B_DISPATCH_MESSAGE;

	uint32 type;
	int32 count;
	if (message->GetInfo("refs", &type, &count) != B_OK)
		return B_SKIP_MESSAGE;

	if (count != 1)
		return B_SKIP_MESSAGE;

	entry_ref ref;
	if (message->FindRef("refs", &ref) != B_OK)
		return B_SKIP_MESSAGE;

	BEntry entry(&ref);
	if (entry.InitCheck() != B_OK)
		return B_SKIP_MESSAGE;

	// if the entry is a symlink
	// resolve it and see if it is a directory
	// pass it on if it is
	if (entry.IsSymLink()) {
		entry_ref resolvedRef;

		entry.GetRef(&resolvedRef);
		BEntry resolvedEntry(&resolvedRef, true);

		if (resolvedEntry.IsDirectory()) {
			// both entry and ref need to be the correct locations
			// for the last setto
			resolvedEntry.GetRef(&ref);
			entry.SetTo(&ref);
		}
	}

	// if not a directory, set to the parent, and select the child
	if (!entry.IsDirectory()) {
		node_ref child;
		if (entry.GetNodeRef(&child) != B_OK)
			return B_SKIP_MESSAGE;

		BPath path(&entry);

		if (entry.GetParent(&entry) != B_OK)
			return B_SKIP_MESSAGE;

		entry.GetRef(&ref);

		// don't delay the initial selection try if the target directory is already current
		panel->fTaskLoop->RunLater(
			NewMemberFunctionObjectWithResult(&TFilePanel::SelectChildInParent, panel,
				const_cast<const entry_ref*>(&ref), const_cast<const node_ref*>(&child)),
			ref == *panel->TargetModel()->EntryRef() ? 0 : 100000, 200000, 5000000);

		// also set the save name to the dragged in entry
		if (panel->IsSavePanel())
			panel->SetSaveText(path.Leaf());
	}

	panel->SwitchDirectory(&ref);

	return B_SKIP_MESSAGE;
}


filter_result
TFilePanel::FSFilter(BMessage* message, BHandler**, BMessageFilter* filter)
{
	if (message == NULL)
		return B_DISPATCH_MESSAGE;

	ASSERT(filter != NULL);
	if (filter == NULL)
		return B_DISPATCH_MESSAGE;

	TFilePanel* panel = dynamic_cast<TFilePanel*>(filter->Looper());
	ASSERT(panel != NULL);

	if (panel == NULL)
		return B_DISPATCH_MESSAGE;

	switch (message->GetInt32("opcode", 0)) {
		case B_ENTRY_MOVED:
		{
			node_ref itemNode;
			message->FindInt64("node", (int64*)&itemNode.node);

			node_ref dirNode;
			message->FindInt32("device", &dirNode.device);
			itemNode.device = dirNode.device;
			message->FindInt64("to directory", (int64*)&dirNode.node);

			const char* name;
			if (message->FindString("name", &name) != B_OK)
				break;

			// if current directory moved, update entry ref and menu
			// but not wind title
			if (*(panel->TargetModel()->NodeRef()) == itemNode) {
				panel->TargetModel()->UpdateEntryRef(&dirNode, name);
				panel->SwitchDirectory(panel->TargetModel()->EntryRef());
				return B_SKIP_MESSAGE;
			}
			break;
		}

		case B_ENTRY_REMOVED:
		{
			node_ref itemNode;
			message->FindInt32("device", &itemNode.device);
			message->FindInt64("node", (int64*)&itemNode.node);

			// if folder we're watching is deleted, switch to root
			// or Desktop
			if (*(panel->TargetModel()->NodeRef()) == itemNode) {
				BVolumeRoster volumeRoster;
				BVolume volume;
				volumeRoster.GetBootVolume(&volume);

				BDirectory root;
				volume.GetRootDirectory(&root);

				BEntry entry;
				entry_ref ref;
				root.GetEntry(&entry);
				entry.GetRef(&ref);

				panel->SwitchDirectory(&ref);
				return B_SKIP_MESSAGE;
			}
			break;
		}
	}

	return B_DISPATCH_MESSAGE;
}


void
TFilePanel::DispatchMessage(BMessage* message, BHandler* handler)
{
	_inherited::DispatchMessage(message, handler);
	if (message->what == B_KEY_DOWN || message->what == B_MOUSE_DOWN)
		AdjustButton();
}


BFilePanelPoseView*
TFilePanel::PoseView() const
{
	ASSERT(dynamic_cast<BFilePanelPoseView*>(fPoseView) != NULL);

	return static_cast<BFilePanelPoseView*>(fPoseView);
}


bool
TFilePanel::QuitRequested()
{
	// If we have a client object then this window will simply hide
	// itself, to be closed later when the client object itself is
	// destroyed. If we have no client then we must have been started
	// from the "easy" functions which simply instantiate a TFilePanel
	// and expect it to go away by itself

	if (fClientObject != NULL) {
		Hide();
		if (fClientObject != NULL)
			fClientObject->WasHidden();

		BMessage message(*fMessage);
		message.what = B_CANCEL;
		message.AddInt32("old_what", (int32)fMessage->what);
		message.AddPointer("source", fClientObject);
		fTarget.SendMessage(&message);

		return false;
	}

	return _inherited::QuitRequested();
}


BRefFilter*
TFilePanel::Filter() const
{
	return fPoseView->RefFilter();
}


void
TFilePanel::SetTarget(BMessenger target)
{
	fTarget = target;
}


void
TFilePanel::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = new BMessage(*message);
}


void
TFilePanel::SetRefFilter(BRefFilter* filter)
{
	ASSERT(filter != NULL);
	if (filter == NULL)
		return;

	fPoseView->SetRefFilter(filter);
	fPoseView->CommitActivePose();
	fPoseView->Refresh();

	if (fMenuBar == NULL)
		return;

	BMenuItem* favoritesItem = fMenuBar->FindItem(B_TRANSLATE("Favorites"));
	if (favoritesItem == NULL)
		return;

	FavoritesMenu* favoritesSubMenu = dynamic_cast<FavoritesMenu*>(favoritesItem->Submenu());
	if (favoritesSubMenu != NULL)
		favoritesSubMenu->SetRefFilter(filter);
}


void
TFilePanel::SwitchDirectory(const entry_ref* ref)
{
	if (ref == NULL)
		return;

	entry_ref setToRef(*ref);
	bool isDesktop = SwitchDirToDesktopIfNeeded(setToRef);
	BEntry entry(&setToRef, true);
	if (entry.InitCheck() != B_OK)
		return;

	if (!entry.Exists())
		return;

	PoseView()->SetIsDesktop(isDesktop);
	_inherited::SwitchDirectory(&setToRef);

	if (PoseView()->IsDesktop())
		PoseView()->AddVolumePoses();

	AddShortcut('D', B_COMMAND_KEY, new BMessage(kSwitchToDesktop));
	AddShortcut('H', B_COMMAND_KEY, new BMessage(kSwitchToHome));
		// our shortcut got possibly removed because the home
		// menu item got removed - we shouldn't really have to do
		// this - this is a workaround for a kit bug.

	// update the menu field
	for (int32 index = fDirMenu->CountItems() - 1; index >= 0; index--)
		delete fDirMenu->RemoveItem(index);

	fDirMenuField->MenuBar()->RemoveItem((int32)0);
	fDirMenu->Populate(&entry, 0, true, true, false, true);

	ModelMenuItem* item = dynamic_cast<ModelMenuItem*>(fDirMenuField->MenuBar()->ItemAt(0));
	ASSERT(item != NULL);

	// set dir menu to the new directory
	item->SetEntry(&entry);
}


void
TFilePanel::Rewind()
{
	fSelectionIterator = 0;
}


void
TFilePanel::SetClientObject(BFilePanel* panel)
{
	fClientObject = panel;
}


void
TFilePanel::AdjustButton()
{
	// adjust button state
	BButton* button = dynamic_cast<BButton*>(FindView("default button"));
	if (button == NULL)
		return;

	BTextControl* textControl
		= dynamic_cast<BTextControl*>(FindView("text view"));
	PoseList* selectionList = fPoseView->SelectionList();
	BString buttonText = fButtonText;
	bool enabled = false;

	if (fIsSavePanel && textControl != NULL) {
		enabled = textControl->Text()[0] != '\0';
		if (fPoseView->IsFocus()) {
			fPoseView->ShowSelection(true);
			if (selectionList->CountItems() == 1) {
				Model* model = selectionList->FirstItem()->TargetModel();
				if (model->ResolveIfLink()->IsDirectory()) {
					enabled = true;
					buttonText = B_TRANSLATE("Open");
				} else {
					// insert the name of the selected model into
					// the text field, do not alter focus
					textControl->SetText(model->Name());
				}
			}
		} else
			fPoseView->ShowSelection(false);
	} else {
		int32 count = selectionList->CountItems();
		if (count) {
			enabled = true;

			// go through selection list looking at content
			for (int32 index = 0; index < count; index++) {
				Model* model = selectionList->ItemAt(index)->TargetModel();

				uint32 modelFlavor = GetLinkFlavor(model, false);
				uint32 linkFlavor = GetLinkFlavor(model, true);

				// if only one item is selected and we're not in dir
				// selection mode then we don't disable button ever
				if ((modelFlavor == B_DIRECTORY_NODE
						|| linkFlavor == B_DIRECTORY_NODE)
					&& count == 1) {
					break;
				}

				if ((fNodeFlavors & modelFlavor) == 0
					&& (fNodeFlavors & linkFlavor) == 0) {
					enabled = false;
					break;
				}
			}
		} else if ((fNodeFlavors & B_DIRECTORY_NODE) != 0) {
			// No selection, but the current directory could be opened.
			enabled = true;
		}
	}

	button->SetLabel(buttonText.String());
	button->SetEnabled(enabled);
}


void
TFilePanel::SelectionChanged()
{
	AdjustButton();

	if (fClientObject)
		fClientObject->SelectionChanged();
}


status_t
TFilePanel::GetNextEntryRef(entry_ref* ref)
{
	if (!ref)
		return B_ERROR;

	BPose* pose = fPoseView->SelectionList()->ItemAt(fSelectionIterator++);
	if (!pose)
		return B_ERROR;

	*ref = *pose->TargetModel()->EntryRef();
	return B_OK;
}


BPoseView*
TFilePanel::NewPoseView(Model* model, uint32)
{
	return new BFilePanelPoseView(model);
}


void
TFilePanel::Init(const BMessage*)
{
	BRect windRect(Bounds());
	fBackView = new BView(Bounds(), "View", B_FOLLOW_ALL, 0);
	fBackView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	// BButton adopts the parent's high color instead of its own,
	// hence this mismatched color set here
	fBackView->SetHighUIColor(B_CONTROL_TEXT_COLOR);

	AddChild(fBackView);

	// add poseview menu bar
	fMenuBar = new BMenuBar(BRect(0, 0, windRect.Width(), 1), "MenuBar");
	fMenuBar->SetBorder(B_BORDER_FRAME);
	fBackView->AddChild(fMenuBar);

	// add directory menu and menufield
	font_height ht;
	be_plain_font->GetHeight(&ht);
	const float f_height = ht.ascent + ht.descent + ht.leading;
	const float spacing = be_control_look->ComposeSpacing(B_USE_SMALL_SPACING);

	BRect rect;
	rect.top = fMenuBar->Bounds().Height() + spacing;
	rect.left = spacing;
	rect.right = rect.left + (spacing * 50);
	rect.bottom = rect.top + (f_height > 22 ? f_height : 22);

	fDirMenuField = new BMenuField(rect, "DirMenuField", "", NULL);
	fDirMenuField->MenuBar()->SetFont(be_plain_font);
	fDirMenuField->SetDivider(0);
	fDirMenuField->MenuBar()->SetMaxContentWidth(rect.Width() - 26.0f);
		// Make room for the icon

	fDirMenu = new BDirMenu(fDirMenuField->MenuBar(), this, kSwitchDirectory, "refs");

	BEntry entry(TargetModel()->EntryRef());
	if (entry.InitCheck() == B_OK)
		fDirMenu->Populate(&entry, 0, true, true, false, true);
	else
		fDirMenu->Populate(0, 0, true, true, false, true);

	fBackView->AddChild(fDirMenuField);

	// add buttons
	fButtonText = fIsSavePanel ? B_TRANSLATE("Save") : B_TRANSLATE("Open");
	BButton* default_button = new BButton(BRect(), "default button",
		fButtonText.String(), new BMessage(kDefaultButton),
		B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
	BSize preferred = default_button->PreferredSize();
	const BRect defaultButtonRect = BRect(BPoint(
		windRect.Width() - (preferred.Width() + spacing + be_control_look->GetScrollBarWidth()),
		windRect.Height() - (preferred.Height() + spacing)),
		preferred);
	default_button->MoveTo(defaultButtonRect.LeftTop());
	default_button->ResizeTo(preferred);
	fBackView->AddChild(default_button);

	BButton* cancel_button = new BButton(BRect(), "cancel button",
		B_TRANSLATE("Cancel"), new BMessage(kCancelButton),
		B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
	preferred = cancel_button->PreferredSize();
	cancel_button->MoveTo(defaultButtonRect.LeftTop()
		- BPoint(preferred.Width() + spacing, 0));
	cancel_button->ResizeTo(preferred);
	fBackView->AddChild(cancel_button);

	// add file name text view
	if (fIsSavePanel) {
		BRect rect(defaultButtonRect);
		rect.left = spacing;
		rect.right = rect.left + spacing * 28;

		fTextControl = new BTextControl(rect, "text view",
			B_TRANSLATE("save text"), "", NULL,
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		DisallowMetaKeys(fTextControl->TextView());
		DisallowFilenameKeys(fTextControl->TextView());
		fBackView->AddChild(fTextControl);
		fTextControl->SetDivider(0.0f);
		fTextControl->TextView()->SetMaxBytes(B_FILE_NAME_LENGTH - 1);
	}

	// Add PoseView
	PoseView()->SetName("ActualPoseView");
	fPoseContainer->SetName("PoseView");
	fPoseContainer->SetResizingMode(B_FOLLOW_ALL);
	fBorderedView->EnableBorderHighlight(true);

	rect.left = spacing;
	rect.top = fDirMenuField->Frame().bottom + spacing;
	rect.right = windRect.Width() - spacing;
	rect.bottom = defaultButtonRect.top - spacing;
	fPoseContainer->MoveTo(rect.LeftTop());
	fPoseContainer->ResizeTo(rect.Size());

	PoseView()->AddScrollBars();
	PoseView()->SetDragEnabled(false);
	PoseView()->SetDropEnabled(false);
	PoseView()->SetSelectionHandler(this);
	PoseView()->SetSelectionChangedHook(true);
	PoseView()->DisableSaveLocation();

	if (fIsSavePanel)
		fBackView->AddChild(fPoseContainer, fTextControl);
	else
		fBackView->AddChild(fPoseContainer);

	fShortcuts = new TShortcuts(this);

	AddShortcut('W', B_COMMAND_KEY, new BMessage(kCancelButton));
	AddShortcut('D', B_COMMAND_KEY, new BMessage(kSwitchToDesktop));
	AddShortcut('H', B_COMMAND_KEY, new BMessage(kSwitchToHome));
	AddShortcut(B_DELETE, B_NO_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kDeleteSelection),
		PoseView());
	AddShortcut(B_DELETE, B_NO_COMMAND_KEY, new BMessage(kMoveSelectionToTrash), PoseView());
	AddShortcut('A', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kShowSelectionWindow));
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), this);
	AddShortcut('S', B_COMMAND_KEY, new BMessage(kInvertSelection), PoseView());
	AddShortcut('Y', B_COMMAND_KEY, new BMessage(kResizeToFit), PoseView());
	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(kOpenDir));
	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY | B_OPTION_KEY, new BMessage(kOpenDir));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(kOpenParentDir));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY, new BMessage(kOpenParentDir));

	if (!fIsSavePanel && (fNodeFlavors & B_DIRECTORY_NODE) == 0)
		default_button->SetEnabled(false);

	default_button->MakeDefault(true);

	RestoreState();

	if (ShouldAddMenus())
		AddMenus();

	AddContextMenus();

	PoseView()->ScrollTo(B_ORIGIN);
	PoseView()->UpdateScrollRange();
	PoseView()->ScrollTo(B_ORIGIN);

	// Focus on text control initially, but do not alter focus afterwords
	// because pose view focus is needed for Cut/Copy/Paste to work.

	if (fIsSavePanel && fTextControl != NULL) {
		fTextControl->MakeFocus();
		fTextControl->TextView()->SelectAll();
	} else
		PoseView()->MakeFocus();

	app_info info;
	BString title;
	if (be_app->GetAppInfo(&info) == B_OK) {
		if (!gLocalizedNamePreferred
			|| BLocaleRoster::Default()->GetLocalizedFileName(
				title, info.ref, false) != B_OK)
			title = info.ref.name;
		title << ": ";
	}
	title << fButtonText;	// Open or Save

	SetTitle(title.String());

	SetSizeLimits(spacing * 60, 10000, spacing * 33, 10000);
}


void
TFilePanel::AddMenus()
{
	// File

	fFileMenu = new TLiveFileMenu(B_TRANSLATE("File"), this);
	AddFileMenu(fFileMenu);
	fMenuBar->AddItem(fFileMenu);

	// Favorites

	fFavoritesMenu = new FavoritesMenu(B_TRANSLATE("Favorites"), new BMessage(kSwitchDirectory),
		new BMessage(B_REFS_RECEIVED), BMessenger(this), IsSavePanel(), Filter());
	AddFavoritesMenu(fFavoritesMenu);
	fMenuBar->AddItem(fFavoritesMenu);
}


void
TFilePanel::AddFileMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->NewFolderItem());
	menu->AddItem(new BSeparatorItem());

	menu->AddItem(Shortcuts()->GetInfoItem());
	menu->AddItem(Shortcuts()->EditNameItem());
	if (TargetModel()->IsTrash() || TargetModel()->InTrash()) {
		menu->AddItem(Shortcuts()->DeleteItem());
		menu->AddItem(Shortcuts()->RestoreItem());
	} else {
		menu->AddItem(Shortcuts()->DuplicateItem());
		menu->AddItem(Shortcuts()->MoveToTrashItem());
	}

	if (!TargetModel()->IsPrintersDir() || TargetModel()->IsRoot() || TargetModel()->IsTrash()
		|| TargetModel()->InTrash()) {
		menu->AddSeparatorItem();
		menu->AddItem(Shortcuts()->CutItem());
		menu->AddItem(Shortcuts()->CopyItem());
		menu->AddItem(Shortcuts()->PasteItem());
	}
}


void
TFilePanel::AddWindowMenu(BMenu* menu)
{
	// no window menu on file panel
}


void
TFilePanel::AddFavoritesMenu(BMenu* menu)
{
	const char* name = B_TRANSLATE("Add current folder");
	menu->AddItem(new BMenuItem(name, new BMessage(kAddCurrentDir)));
	name = B_TRANSLATE("Edit favorites" B_UTF8_ELLIPSIS);
	menu->AddItem(new BMenuItem(name, new BMessage(kEditFavorites)));
}


void
TFilePanel::RestoreState()
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultFilePanelTemplate, &defaultingNode,
			false)) {
		AttributeStreamFileNode streamNodeSource(&defaultingNode);
		RestoreWindowState(&streamNodeSource);
		PoseView()->Init(&streamNodeSource);
		fDefaultStateRestored = true;
	} else {
		RestoreWindowState(NULL);
		PoseView()->Init(NULL);
		fDefaultStateRestored = false;
	}

	// Finish UI creation now that the PoseView is initialized
	InitLayout();
}


void
TFilePanel::SaveState(bool)
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultFilePanelTemplate, &defaultingNode,
		true, false)) {
		AttributeStreamFileNode streamNodeDestination(&defaultingNode);
		SaveWindowState(&streamNodeDestination);
		PoseView()->SaveState(&streamNodeDestination);
		fStateNeedsSaving = false;
	}
}


void
TFilePanel::SaveState(BMessage &message) const
{
	_inherited::SaveState(message);
}


void
TFilePanel::RestoreWindowState(AttributeStreamNode* node)
{
	SetSizeLimits(360, 10000, 200, 10000);
	if (!node)
		return;

	const char* rectAttributeName = kAttrWindowFrame;
	BRect frame(Frame());
	if (node->Read(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame) == sizeof(BRect)) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
	fStateNeedsSaving = false;
}


void
TFilePanel::RestoreState(const BMessage &message)
{
	_inherited::RestoreState(message);
}


void
TFilePanel::RestoreWindowState(const BMessage &message)
{
	_inherited::RestoreWindowState(message);
}


void
TFilePanel::AddPoseContextMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->GetInfoItem());
	menu->AddItem(Shortcuts()->EditNameItem());
	if (TargetModel()->InTrash()) {
		menu->AddItem(Shortcuts()->DeleteItem());
		menu->AddItem(Shortcuts()->RestoreItem());
	} else {
		menu->AddItem(Shortcuts()->DuplicateItem());
		menu->AddItem(Shortcuts()->MoveToTrashItem());
	}
	menu->AddSeparatorItem();

	menu->AddItem(Shortcuts()->CutItem());
	menu->AddItem(Shortcuts()->CopyItem());
	menu->AddItem(Shortcuts()->PasteItem());
}


void
TFilePanel::AddVolumeContextMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->OpenItem());
	menu->AddItem(Shortcuts()->GetInfoItem());
	menu->AddItem(Shortcuts()->EditNameItem());

	menu->AddSeparatorItem();
	menu->AddItem(Shortcuts()->PasteItem());
}


void
TFilePanel::AddWindowContextMenu(BMenu* menu)
{
	menu->AddItem(Shortcuts()->NewFolderItem());
	menu->AddItem(new BSeparatorItem());

	menu->AddItem(Shortcuts()->PasteItem());
	menu->AddSeparatorItem();

	menu->AddItem(Shortcuts()->SelectItem());
	menu->AddItem(Shortcuts()->SelectAllItem());
	menu->AddItem(Shortcuts()->InvertSelectionItem());
	menu->AddItem(Shortcuts()->OpenParentItem());
}


void
TFilePanel::AddTrashContextMenu(BMenu* menu)
{
	// use default window context menu on Trash
	AddWindowContextMenu(menu);
}


void
TFilePanel::AddDropContextMenu(BMenu*)
{
	// do nothing here so drop context menu doesn't get added
}


void
TFilePanel::MenusBeginning()
{
	if (fMenuBar == NULL)
		return;

	if (CurrentMessage() != NULL && CurrentMessage()->what == B_MOUSE_DOWN) {
		// don't commit active pose if only a keyboard shortcut is
		// invoked - this would prevent Cut/Copy/Paste from working
		PoseView()->CommitActivePose();
	}

	UpdateMenu(fFileMenu, kFileMenuContext);

	fIsTrackingMenu = true;
}


void
TFilePanel::MenusEnded()
{
	fIsTrackingMenu = false;
}


void
TFilePanel::DetachSubmenus()
{
	// no submenus to detatch in file panel
}


void
TFilePanel::UpdateFileMenu(BMenu*)
{
	// nothing more to do
}


void
TFilePanel::UpdateFileMenuOrPoseContextMenu(BMenu*, MenuContext, const entry_ref*)
{
	// nothing more to do
}


void
TFilePanel::UpdateWindowMenu(BMenu*)
{
	// no window menu on file panel
}


void
TFilePanel::UpdateWindowContextMenu(BMenu*)
{
	// nothing more to do
}


void
TFilePanel::UpdateWindowMenuOrWindowContextMenu(BMenu*, MenuContext)
{
	// nothing more to do
}


void
TFilePanel::RepopulateMenus()
{
	if (fMenuBar != NULL && fFileMenu != NULL) {
		fMenuBar->RemoveItem(fFileMenu);
		delete fFileMenu;
		if (ShouldAddMenus()) {
			fFileMenu = new TLiveFileMenu(B_TRANSLATE("File"), this);
			AddFileMenu(fFileMenu);
			fMenuBar->AddItem(fFileMenu, 0);
		}
	}

	delete fPoseContextMenu;
	fPoseContextMenu = new TLivePosePopUpMenu("PoseContext", this, false, false);
	fPoseContextMenu->SetFont(be_plain_font);
	TFilePanel::AddPoseContextMenu(fPoseContextMenu);

	delete fWindowContextMenu;
	fWindowContextMenu = new TLiveWindowPopUpMenu("WindowContext", this, false, false);
	fWindowContextMenu->SetFont(be_plain_font);
	TFilePanel::AddWindowContextMenu(fWindowContextMenu);
}


void
TFilePanel::SetupNavigationMenu(BMenu*, const entry_ref*)
{
	// do nothing here so nav menu doesn't get added
}


void
TFilePanel::SetButtonLabel(file_panel_button selector, const char* text)
{
	switch (selector) {
		case B_CANCEL_BUTTON:
		{
			BButton* button = dynamic_cast<BButton*>(FindView("cancel button"));
			if (button == NULL)
				break;

			float old_width = button->StringWidth(button->Label());
			button->SetLabel(text);
			float delta = old_width - button->StringWidth(text);
			if (delta) {
				button->MoveBy(delta, 0);
				button->ResizeBy(-delta, 0);
			}
			break;
		}

		case B_DEFAULT_BUTTON:
		{
			fButtonText = text;
			float delta = 0;
			BButton* button = dynamic_cast<BButton*>(FindView("default button"));
			if (button != NULL) {
				float old_width = button->StringWidth(button->Label());
				button->SetLabel(text);
				delta = old_width - button->StringWidth(text);
				if (delta) {
					button->MoveBy(delta, 0);
					button->ResizeBy(-delta, 0);
				}
			}

			// now must move cancel button
			button = dynamic_cast<BButton*>(FindView("cancel button"));
			if (button != NULL)
				button->MoveBy(delta, 0);

			break;
		}
	}
}


void
TFilePanel::SetSaveText(const char* text)
{
	if (text == NULL)
		return;

	BTextControl* textControl = dynamic_cast<BTextControl*>(FindView("text view"));
	if (textControl != NULL) {
		textControl->SetText(text);
		if (textControl->TextView() != NULL)
			textControl->TextView()->SelectAll();
	}
}


void
TFilePanel::MessageReceived(BMessage* message)
{
	entry_ref ref;

	switch (message->what) {
		case B_REFS_RECEIVED:
		{
			// item was double clicked in file panel (PoseView) or from the favorites menu
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BEntry entry(&ref, true);
			if (entry.InitCheck() != B_OK)
				break;

			// Double-click on dir or link-to-dir ALWAYS opens the dir.
			// If more than one dir is selected the first one is opened.
			if (entry.IsDirectory()) {
				SwitchDirectory(&ref);
			} else {
				// Otherwise, we have a file or a link to a file.
				// AdjustButton has already tested the flavor if it comes from the file
				// panel; all we have to do is see if the button is enabled.
				// In other cases, however, we can't rely on that. So first check for
				// TrackerViewToken in the message to see if it's coming from the pose view
				if (message->HasMessenger("TrackerViewToken")) {
					BButton* button = dynamic_cast<BButton*>(FindView("default button"));
					if (button == NULL || !button->IsEnabled())
						break;
				}

				if (IsSavePanel()) {
					int32 count = 0;
					type_code type;
					message->GetInfo("refs", &type, &count);

					// Don't allow saves of multiple files
					if (count > 1) {
						const char* sorry
							= B_TRANSLATE("Sorry, saving more than one item is not allowed.");
						ShowCenteredAlert(sorry, B_TRANSLATE("Cancel"));
					} else {
						// if we are a savepanel, set up the
						// filepanel correctly then pass control
						// so we follow the same path as if the user
						// clicked the save button

						// set the 'name' fld to the current ref's
						// name notify the panel that the default
						// button should be enabled
						SetSaveText(ref.name);
						SelectionChanged();

						HandleSaveButton();
					}
					break;
				}

				// send handler a message and close
				BMessage openMessage(*fMessage);
				for (int32 index = 0;; index++) {
					if (message->FindRef("refs", index, &ref) != B_OK)
						break;
					openMessage.AddRef("refs", &ref);
				}
				OpenSelectionCommon(&openMessage);
			}
			break;
		}

		case kSwitchDirectory:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			SwitchDirectory(&ref);
			break;
		}

		case kSwitchToDesktop:
		{
			if (PoseView() != NULL && PoseView()->IsFocus()
				&& PoseView()->CanMoveToTrashOrDuplicate()) {
				// duplicate selection instead
				message->what = kDuplicateSelection;
				PostMessage(message, PoseView());
				break;
			}

			BPath path;
			entry_ref ref;
			if (find_directory(B_DESKTOP_DIRECTORY, &path) != B_OK
				|| get_ref_for_path(path.Path(), &ref) != B_OK) {
				break;
			}

			SwitchDirectory(&ref);
			break;
		}

		case kSwitchToHome:
		{
			BPath homePath;
			entry_ref ref;
			if (find_directory(B_USER_DIRECTORY, &homePath) != B_OK
				|| get_ref_for_path(homePath.Path(), &ref) != B_OK) {
				break;
			}

			SwitchDirectory(&ref);
			break;
		}

		case kAddCurrentDir:
		{
			BPath path;
			if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true)
					!= B_OK) {
				break;
			}

			path.Append(kGoDirectory);
			BDirectory goDirectory(path.Path());

			if (goDirectory.InitCheck() == B_OK) {
				BEntry entry(TargetModel()->EntryRef());
				entry.GetPath(&path);

				BSymLink link;
				goDirectory.CreateSymLink(TargetModel()->Name(), path.Path(),
					&link);
			}
			break;
		}

		case kEditFavorites:
		{
			BPath path;
			if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true)
					!= B_OK) {
				break;
			}

			path.Append(kGoDirectory);
			BMessenger msgr(kTrackerSignature);
			if (msgr.IsValid()) {
				BMessage message(B_REFS_RECEIVED);
				entry_ref ref;
				if (get_ref_for_path(path.Path(), &ref) == B_OK) {
					message.AddRef("refs", &ref);
					msgr.SendMessage(&message);
				}
			}
			break;
		}

		case kCancelButton:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case kResizeToFit:
			ResizeToFit();
			break;

		case kOpenDir:
			OpenDirectory();
			break;

		case kOpenParentDir:
			OpenParent();
			break;

		case kDefaultButton:
			if (fIsSavePanel) {
				if (PoseView()->IsFocus()
					&& PoseView()->CountSelected() == 1) {
					Model* model = (PoseView()->SelectionList()->FirstItem())->TargetModel();
					if (model->ResolveIfLink()->IsDirectory()) {
						PoseView()->CommitActivePose();
						PoseView()->OpenSelection();
						break;
					}
				}

				HandleSaveButton();
			} else
				HandleOpenButton();

			break;

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 observerWhat;
			if (message->FindInt32("be:observe_change_what", &observerWhat) == B_OK) {
				switch (observerWhat) {
					case kDesktopFilePanelRootChanged:
					{
						bool desktopIsRoot;
						if (message->FindBool("DesktopFilePanelRoot", &desktopIsRoot) == B_OK
							&& TrackerSettings().DesktopFilePanelRoot() != desktopIsRoot) {
							TrackerSettings().SetDesktopFilePanelRoot(desktopIsRoot);
							SwitchDirectory(TargetModel()->EntryRef());
						}
						break;
					}

					default:
						break;
				}
			}
			break;
		}

		default:
			_inherited::MessageReceived(message);
			break;
	}
}


void
TFilePanel::OpenDirectory()
{
	PoseList* list = PoseView()->SelectionList();
	if (list->CountItems() != 1)
		return;

	Model* model = list->FirstItem()->TargetModel();
	if (model->ResolveIfLink()->IsDirectory()) {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", model->EntryRef());
		BMessenger(this).SendMessage(&message);
	}
}


void
TFilePanel::OpenParent()
{
	BEntry entry(TargetModel()->EntryRef());
	Model oldModel(*PoseView()->TargetModel());
	const node_ref* oldNode = oldModel.NodeRef();

	BEntry parentEntry;
	if (TrackerSettings().DesktopFilePanelRoot() && FSIsRootDir(&entry)) {
		// open parent on root, set to Desktop
		BDirectory desktopDir;
		if (FSGetDeskDir(&desktopDir) != B_OK || desktopDir.GetEntry(&parentEntry) != B_OK)
			return;
	} else if (FSGetParentVirtualDirectoryAware(entry, parentEntry) != B_OK) {
		return;
	}

	entry_ref setToRef;
	parentEntry.GetRef(&setToRef);
	const entry_ref* parent = &setToRef;
	SwitchDirectory(parent);

	// Make sure the child gets selected in the new view once it shows up.
	fTaskLoop->RunLater(
		NewMemberFunctionObjectWithResult(&TFilePanel::SelectChildInParent, this, parent, oldNode),
		100000, 200000, 5000000);
}


bool
TFilePanel::SwitchDirToDesktopIfNeeded(entry_ref &ref)
{
	// support showing Desktop as root of everything
	// This call implements the worm hole that maps Desktop as
	// a root above the disks
	TrackerSettings settings;
	if (!settings.DesktopFilePanelRoot())
		// Tracker isn't set up that way, just let Disks show
		return false;

	BEntry entry(&ref);

	BDirectory desktopDir;
	FSGetDeskDir(&desktopDir);
	if (FSIsDeskDir(&entry) || (!settings.ShowDisksIcon() && FSIsRootDir(&entry))) {
		// navigated into desktop folder or hit "root" level, switch to Desktop

		desktopDir.GetEntry(&entry);
		entry.GetRef(&ref);
		return true;
	}

	return FSIsDeskDir(&entry);
}


bool
TFilePanel::SelectChildInParent(const entry_ref*, const node_ref* child)
{
	AutoLock<TFilePanel> lock(this);

	if (!IsLocked())
		return false;

	int32 index;
	BPose* pose = PoseView()->FindPose(child, &index);
	if (!pose)
		return false;

	PoseView()->UpdateScrollRange();
		// ToDo: Scroll range should be updated by now, for some
		//	reason sometimes it is not right, force it here
	PoseView()->SelectPose(pose, index, true);
	return true;
}


int32
TFilePanel::ShowCenteredAlert(const char* text, const char* button1,
	const char* button2, const char* button3)
{
	BAlert* alert = new BAlert("", text, button1, button2, button3,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->MoveTo(Frame().left + 10, Frame().top + 10);

#if 0
	if (button1 != NULL && !strncmp(button1, "Cancel", 7))
		alert->SetShortcut(0, B_ESCAPE);
	else if (button2 != NULL && !strncmp(button2, "Cancel", 7))
		alert->SetShortcut(1, B_ESCAPE);
	else if (button3 != NULL && !strncmp(button3, "Cancel", 7))
		alert->SetShortcut(2, B_ESCAPE);
#endif

	return alert->Go();
}


void
TFilePanel::HandleSaveButton()
{
	BDirectory dir;

	if (TargetModel()->IsRoot()) {
		ShowCenteredAlert(
			B_TRANSLATE("Sorry, you can't save things at the root of "
			"your system."),
			B_TRANSLATE("Cancel"));
		return;
	}

	// check for some illegal file names
	if (strcmp(fTextControl->Text(), ".") == 0
		|| strcmp(fTextControl->Text(), "..") == 0) {
		ShowCenteredAlert(
			B_TRANSLATE("The specified name is illegal. Please choose "
			"another name."),
			B_TRANSLATE("Cancel"));
		fTextControl->TextView()->SelectAll();
		return;
	}

	if (dir.SetTo(TargetModel()->EntryRef()) != B_OK) {
		ShowCenteredAlert(
			B_TRANSLATE("There was a problem trying to save in the folder "
			"you specified. Please try another one."),
			B_TRANSLATE("Cancel"));
		return;
	}

	if (dir.Contains(fTextControl->Text())) {
		if (dir.Contains(fTextControl->Text(), B_DIRECTORY_NODE)) {
			ShowCenteredAlert(
				B_TRANSLATE("The specified name is already used as the name "
				"of a folder. Please choose another name."),
				B_TRANSLATE("Cancel"));
			fTextControl->TextView()->SelectAll();
			return;
		} else {
			// if this was invoked by a dbl click, it is an explicit
			// replacement of the file.
			BString str(B_TRANSLATE("The file \"%name\" already exists in "
				"the specified folder. Do you want to replace it?"));
			str.ReplaceFirst("%name", fTextControl->Text());

			if (ShowCenteredAlert(str.String(),	B_TRANSLATE("Cancel"),
					B_TRANSLATE("Replace"))	== 0) {
				// user canceled
				fTextControl->TextView()->SelectAll();
				return;
			}
			// user selected "Replace" - let app deal with it
		}
	}

	BMessage message(*fMessage);
	message.AddRef("directory", TargetModel()->EntryRef());
	message.AddString("name", fTextControl->Text());

	if (fClientObject)
		fClientObject->SendMessage(&fTarget, &message);
	else
		fTarget.SendMessage(&message);

	// close window if we're dealing with standard message
	if (fHideWhenDone)
		PostMessage(B_QUIT_REQUESTED);
}


void
TFilePanel::OpenSelectionCommon(BMessage* openMessage)
{
	if (!openMessage->HasRef("refs"))
		return;

	for (int32 index = 0; ; index++) {
		entry_ref ref;
		if (openMessage->FindRef("refs", index, &ref) != B_OK)
			break;

		BEntry entry(&ref, true);
		if (entry.InitCheck() == B_OK) {
			if (entry.IsDirectory())
				BRoster().AddToRecentFolders(&ref);
			else
				BRoster().AddToRecentDocuments(&ref);
		}
	}

	BRoster().AddToRecentFolders(TargetModel()->EntryRef());

	if (fClientObject)
		fClientObject->SendMessage(&fTarget, openMessage);
	else
		fTarget.SendMessage(openMessage);

	// close window if we're dealing with standard message
	if (fHideWhenDone)
		PostMessage(B_QUIT_REQUESTED);
}


void
TFilePanel::HandleOpenButton()
{
	PoseView()->CommitActivePose();
	PoseList* selection = PoseView()->SelectionList();

	// if we have only one directory and we're not opening dirs, enter.
	if ((fNodeFlavors & B_DIRECTORY_NODE) == 0
		&& selection->CountItems() == 1) {
		Model* model = selection->FirstItem()->TargetModel();

		if (model->IsDirectory()
			|| (model->IsSymLink() && !(fNodeFlavors & B_SYMLINK_NODE)
				&& model->ResolveIfLink()->IsDirectory())) {

			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", model->EntryRef());
			PostMessage(&message);
			return;
		}
	}

	if (selection->CountItems()) {
			// there are items selected
			// message->fMessage->message from here to end
		BMessage message(*fMessage);
		// go through selection and add appropriate items
		for (int32 index = 0; index < selection->CountItems(); index++) {
			Model* model = selection->ItemAt(index)->TargetModel();

			if (((fNodeFlavors & B_DIRECTORY_NODE) != 0
					&& model->ResolveIfLink()->IsDirectory())
				|| ((fNodeFlavors & B_SYMLINK_NODE) != 0 && model->IsSymLink())
				|| ((fNodeFlavors & B_FILE_NODE) != 0
					&& model->ResolveIfLink()->IsFile())) {
				message.AddRef("refs", model->EntryRef());
			}
		}

		OpenSelectionCommon(&message);
	} else if ((fNodeFlavors & B_DIRECTORY_NODE) != 0) {
		// Open the current directory.
		BMessage message(*fMessage);
		message.AddRef("refs", TargetModel()->EntryRef());
		OpenSelectionCommon(&message);
	}
}


void
TFilePanel::WindowActivated(bool active)
{
	// force focus to update properly
	fBackView->Invalidate();
	_inherited::WindowActivated(active);
}


//	#pragma mark -


BFilePanelPoseView::BFilePanelPoseView(Model* model)
	:
	BPoseView(model, kListMode),
	fIsDesktop(model != NULL && model->IsDesktop())
{
}


void
BFilePanelPoseView::StartWatching()
{
	TTracker::WatchNode(0, B_WATCH_MOUNT, this);

	// inter-application observing
	BMessenger tracker(kTrackerSignature);
	BHandler::StartWatching(tracker, kVolumesOnDesktopChanged);
}


void
BFilePanelPoseView::StopWatching()
{
	stop_watching(this);

	// inter-application observing
	BMessenger tracker(kTrackerSignature);
	BHandler::StopWatching(tracker, kVolumesOnDesktopChanged);
}


bool
BFilePanelPoseView::FSNotification(const BMessage* message)
{
	switch (message->GetInt32("opcode", 0)) {
		case B_DEVICE_MOUNTED:
		{
			if (!IsDesktop() && !TargetModel()->IsRoot())
				break;

			dev_t device;
			if (message->FindInt32("new device", &device) != B_OK)
				break;

			ASSERT(TargetModel() != NULL);
			TrackerSettings settings;

			BVolume volume(device);
			if (volume.InitCheck() != B_OK)
				break;

			// place volume icon onto Desktop or Root
			if ((!volume.IsShared() || settings.MountSharedVolumesOntoDesktop())
				&& ((IsVolumesRoot() && settings.MountVolumesOntoDesktop())
					|| TargetModel()->IsRoot())) {
				CreateVolumePose(&volume);
			}
			break;
		}

		case B_DEVICE_UNMOUNTED:
		{
			dev_t device;
			if (message->FindInt32("device", &device) == B_OK) {
				if (TargetModel() != NULL && TargetModel()->NodeRef()->device == device) {
					// Volume currently shown in this file panel
					// disappeared, reset location to home directory
					BMessage message(kSwitchToHome);
					MessageReceived(&message);
				}
			}
			break;
		}
	}
	return _inherited::FSNotification(message);
}


void
BFilePanelPoseView::RestoreState(AttributeStreamNode* node)
{
	_inherited::RestoreState(node);
	fViewState->SetViewMode(kListMode);
}


void
BFilePanelPoseView::RestoreState(const BMessage &message)
{
	_inherited::RestoreState(message);
}


void
BFilePanelPoseView::SavePoseLocations(BRect*)
{
}


EntryListBase*
BFilePanelPoseView::InitDirentIterator(const entry_ref* ref)
{
	if (IsDesktop())
		return DesktopPoseView::InitDesktopDirentIterator(this, ref);

	return _inherited::InitDirentIterator(ref);
}


void
BFilePanelPoseView::AddPosesCompleted()
{
	_inherited::AddPosesCompleted();

	if (IsDesktop())
		CreateTrashPose();

	// the menu that adds these shortcuts may not exist initially
	Window()->AddShortcut('D', B_COMMAND_KEY, new BMessage(kSwitchToDesktop));
	Window()->AddShortcut('H', B_COMMAND_KEY, new BMessage(kSwitchToHome));

	UpdateScrollRange();
}


void
BFilePanelPoseView::AddPoses(Model* model)
{
	if (IsDesktop())
		AddVolumePoses();

	_inherited::AddPoses(model);
}


void
BFilePanelPoseView::AdaptToVolumeChange(BMessage* message)
{
	if (Window() == NULL)
		return;

	bool showDisksIcon = kDefaultShowDisksIcon;
	message->FindBool("ShowDisksIcon", &showDisksIcon);

	BEntry entry("/");
	Model model(&entry);
	if (model.InitCheck() == B_OK) {
		BMessage monitorMsg;
		monitorMsg.what = B_NODE_MONITOR;

		if (showDisksIcon)
			monitorMsg.AddInt32("opcode", B_ENTRY_CREATED);
		else
			monitorMsg.AddInt32("opcode", B_ENTRY_REMOVED);

		monitorMsg.AddInt32("device", model.NodeRef()->device);
		monitorMsg.AddInt64("node", model.NodeRef()->node);
		monitorMsg.AddInt64("directory", model.EntryRef()->directory);
		monitorMsg.AddString("name", model.EntryRef()->name);

		TrackerSettings().SetShowDisksIcon(showDisksIcon);

		Window()->PostMessage(&monitorMsg, this);
	}

	ToggleDisksVolumes();
}


void
BFilePanelPoseView::AdaptToDesktopIntegrationChange(BMessage* message)
{
	ToggleDisksVolumes();
}
