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


#include "Attributes.h"
#include "AttributeStream.h"
#include "AutoLock.h"
#include "Commands.h"
#include "DesktopPoseView.h"
#include "DirMenu.h"
#include "FavoritesMenu.h"
#include "FilePanelPriv.h"
#include "FSUtils.h"
#include "FSClipboard.h"
#include "IconMenuItem.h"
#include "MimeTypes.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "Tracker.h"
#include "tracker_private.h"
#include "Utilities.h"

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <FindDirectory.h>
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

#include <string.h>


const char *kDefaultFilePanelTemplate = "FilePanelSettings";


static uint32
GetLinkFlavor(const Model *model, bool resolve = true)
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
key_down_filter(BMessage *message, BHandler **handler, BMessageFilter *filter)
{
	TFilePanel *panel = dynamic_cast<TFilePanel *>(filter->Looper());
	ASSERT(panel);
	BPoseView *view = panel->PoseView();

	if (panel->TrackingMenu())
		return B_DISPATCH_MESSAGE;

	uchar key;
	if (message->FindInt8("byte", (int8 *)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	int32 modifier = 0;
	message->FindInt32("modifiers", &modifier);
	if (!modifier && key == B_ESCAPE) {
		if (view->ActivePose())
			view->CommitActivePose(false);
		else if (view->IsFiltering())
			filter->Looper()->PostMessage(B_CANCEL, *handler);
		else
			filter->Looper()->PostMessage(kCancelButton);
		return B_SKIP_MESSAGE;
	}

	if (key == B_RETURN && view->ActivePose()) {
		view->CommitActivePose();
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark -


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FilePanelPriv"

TFilePanel::TFilePanel(file_panel_mode mode, BMessenger *target,
		const BEntry *startDir, uint32 nodeFlavors, bool multipleSelection,
		BMessage *message, BRefFilter *filter, uint32 containerWindowFlags,
		window_look look, window_feel feel, bool hideWhenDone)
	: BContainerWindow(0, containerWindowFlags, look, feel, 0, B_CURRENT_WORKSPACE),
	fDirMenu(NULL),
	fDirMenuField(NULL),
	fTextControl(NULL),
	fClientObject(NULL),
	fSelectionIterator(0),
	fMessage(NULL),
	fHideWhenDone(hideWhenDone),
	fIsTrackingMenu(false)
{
	InitIconPreloader();

	fIsSavePanel = (mode == B_SAVE_PANEL);

	BRect windRect(85, 50, 510, 296);
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

	gLocalizedNamePreferred
		= BLocaleRoster::Default()->IsFilesystemTranslationPreferred();

	// check for legal starting directory
	Model *model = new Model();
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
	CreatePoseView(model);
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
}


TFilePanel::~TFilePanel()
{
	BMessenger tracker(kTrackerSignature);
	BHandler::StopWatching(tracker, kDesktopFilePanelRootChanged);

	delete fMessage;
}


filter_result
TFilePanel::MessageDropFilter(BMessage *message, BHandler **, BMessageFilter *filter)
{
	TFilePanel *panel = dynamic_cast<TFilePanel *>(filter->Looper());
	if (panel == NULL || !message->WasDropped())
		return B_SKIP_MESSAGE;

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

		panel->fTaskLoop->RunLater(NewMemberFunctionObjectWithResult
			(&TFilePanel::SelectChildInParent, panel,
			const_cast<const entry_ref *>(&ref),
			const_cast<const node_ref *>(&child)),
			ref == *panel->TargetModel()->EntryRef() ? 0 : 100000, 200000, 5000000);
				// if the target directory is already current, we won't
				// delay the initial selection try

		// also set the save name to the dragged in entry
		if (panel->IsSavePanel())
			panel->SetSaveText(path.Leaf());
	}

	panel->SetTo(&ref);

	return B_SKIP_MESSAGE;
}


filter_result
TFilePanel::FSFilter(BMessage *message, BHandler **, BMessageFilter *filter)
{
	switch (message->FindInt32("opcode")) {
		case B_ENTRY_MOVED:
			{
				node_ref itemNode;
				node_ref dirNode;
				TFilePanel *panel = dynamic_cast<TFilePanel *>(filter->Looper());

				message->FindInt32("device", &dirNode.device);
				itemNode.device = dirNode.device;
				message->FindInt64("to directory", (int64 *)&dirNode.node);
				message->FindInt64("node", (int64 *)&itemNode.node);
				const char *name;
				if (message->FindString("name", &name) != B_OK)
					break;

				// if current directory moved, update entry ref and menu
				// but not wind title
				if (*(panel->TargetModel()->NodeRef()) == itemNode) {
					panel->TargetModel()->UpdateEntryRef(&dirNode, name);
					panel->SetTo(panel->TargetModel()->EntryRef());
					return B_SKIP_MESSAGE;
				}
				break;
			}
		case B_ENTRY_REMOVED:
			{
				node_ref itemNode;
				TFilePanel *panel = dynamic_cast<TFilePanel *>(filter->Looper());
				message->FindInt32("device", &itemNode.device);
				message->FindInt64("node", (int64 *)&itemNode.node);

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

					panel->SwitchDirToDesktopIfNeeded(ref);

					panel->SetTo(&ref);
					return B_SKIP_MESSAGE;
				}
			}
			break;
	}
	return B_DISPATCH_MESSAGE;
}


void
TFilePanel::DispatchMessage(BMessage *message, BHandler *handler)
{
	_inherited::DispatchMessage(message, handler);
	if (message->what == B_KEY_DOWN || message->what == B_MOUSE_DOWN)
		AdjustButton();
}


BFilePanelPoseView *
TFilePanel::PoseView() const
{
	ASSERT(dynamic_cast<BFilePanelPoseView *>(fPoseView));
	return static_cast<BFilePanelPoseView *>(fPoseView);
}


bool
TFilePanel::QuitRequested()
{
	// If we have a client object then this window will simply hide
	// itself, to be closed later when the client object itself is
	// destroyed. If we have no client then we must have been started
	// from the "easy" functions which simply instantiate a TFilePanel
	// and expect it to go away by itself

	if (fClientObject) {
		Hide();
		if (fClientObject)
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


BRefFilter *
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
TFilePanel::SetMessage(BMessage *message)
{
	delete fMessage;
	fMessage = new BMessage(*message);
}


void
TFilePanel::SetRefFilter(BRefFilter *filter)
{
	if (!filter)
		return;

	fPoseView->SetRefFilter(filter);
	fPoseView->CommitActivePose();
	fPoseView->Refresh();
	FavoritesMenu* menu = dynamic_cast<FavoritesMenu *>
		(fMenuBar->FindItem(B_TRANSLATE("Favorites"))->Submenu());
	if (menu)
		menu->SetRefFilter(filter);
}


void
TFilePanel::SetTo(const entry_ref *ref)
{
	if (!ref)
		return;

	entry_ref setToRef(*ref);

	bool isDesktop = SwitchDirToDesktopIfNeeded(setToRef);

	BEntry entry(&setToRef);
	if (entry.InitCheck() != B_OK || !entry.IsDirectory())
		return;

	SwitchDirMenuTo(&setToRef);

	PoseView()->SetIsDesktop(isDesktop);
	fPoseView->SwitchDir(&setToRef);

	AddShortcut('H', B_COMMAND_KEY, new BMessage(kSwitchToHome));
		// our shortcut got possibly removed because the home
		// menu item got removed - we shouldn't really have to do
		// this - this is a workaround for a kit bug.
}


void
TFilePanel::Rewind()
{
	fSelectionIterator = 0;
}


void
TFilePanel::SetClientObject(BFilePanel *panel)
{
	fClientObject = panel;
}


void
TFilePanel::AdjustButton()
{
	// adjust button state
	BButton *button = dynamic_cast<BButton *>(FindView("default button"));
	if (!button)
		return;

	BTextControl *textControl = dynamic_cast<BTextControl *>(FindView("text view"));
	BObjectList<BPose> *selectionList = fPoseView->SelectionList();
	BString buttonText = fButtonText;
	bool enabled = false;

	if (fIsSavePanel && textControl) {
		enabled = textControl->Text()[0] != '\0';
		if (fPoseView->IsFocus()) {
			fPoseView->ShowSelection(true);
			if (selectionList->CountItems() == 1) {
				Model *model = selectionList->FirstItem()->TargetModel();
				if (model->ResolveIfLink()->IsDirectory()) {
					enabled = true;
					buttonText = B_TRANSLATE("Open");
				} else {
					// insert the name of the selected model into the text field
					textControl->SetText(model->Name());
					textControl->MakeFocus(true);
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
				Model *model = selectionList->ItemAt(index)->TargetModel();

				uint32 modelFlavor = GetLinkFlavor(model, false);
				uint32 linkFlavor = GetLinkFlavor(model, true);

				// if only one item is selected and we're not in dir
				// selection mode then we don't disable button ever
				if ((modelFlavor == B_DIRECTORY_NODE
						|| linkFlavor == B_DIRECTORY_NODE)
					&& count == 1)
				  break;

				if ((fNodeFlavors & modelFlavor) == 0
					&& (fNodeFlavors & linkFlavor) == 0) {
		    		enabled = false;
					break;
				}
			}
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
TFilePanel::GetNextEntryRef(entry_ref *ref)
{
	if (!ref)
		return B_ERROR;

	BPose *pose = fPoseView->SelectionList()->ItemAt(fSelectionIterator++);
	if (!pose)
		return B_ERROR;

	*ref = *pose->TargetModel()->EntryRef();
	return B_OK;
}


BPoseView *
TFilePanel::NewPoseView(Model *model, BRect rect, uint32)
{
	return new BFilePanelPoseView(model, rect);
}


void
TFilePanel::Init(const BMessage *)
{
	BRect windRect(Bounds());
	AddChild(fBackView = new BackgroundView(windRect));

	// add poseview menu bar
	fMenuBar = new BMenuBar(BRect(0, 0, windRect.Width(), 1), "MenuBar");
	fMenuBar->SetBorder(B_BORDER_FRAME);
	fBackView->AddChild(fMenuBar);

	AddMenus();
	AddContextMenus();

	FavoritesMenu* favorites = new FavoritesMenu(B_TRANSLATE("Favorites"),
		new BMessage(kSwitchDirectory), new BMessage(B_REFS_RECEIVED),
		BMessenger(this), IsSavePanel(), fPoseView->RefFilter());
	favorites->AddItem(new BMenuItem(B_TRANSLATE("Add current folder"),
		new BMessage(kAddCurrentDir)));
	favorites->AddItem(new BMenuItem(
		B_TRANSLATE("Edit favorites"B_UTF8_ELLIPSIS),
		new BMessage(kEditFavorites)));

	fMenuBar->AddItem(favorites);

	// configure menus
	BMenuItem* item = fMenuBar->FindItem(B_TRANSLATE("Window"));
	if (item) {
		fMenuBar->RemoveItem(item);
		delete item;
	}

	item = fMenuBar->FindItem(B_TRANSLATE("File"));
	if (item) {
		BMenu *menu = item->Submenu();
		if (menu) {
			item = menu->FindItem(kOpenSelection);
			if (item && menu->RemoveItem(item))
				delete item;

			item = menu->FindItem(kDuplicateSelection);
			if (item && menu->RemoveItem(item))
				delete item;

			// remove add-ons menu, identifier menu, separator
			item = menu->FindItem(B_TRANSLATE("Add-ons"));
			if (item) {
				int32 index = menu->IndexOf(item);
				delete menu->RemoveItem(index);
				delete menu->RemoveItem(--index);
				delete menu->RemoveItem(--index);
			}

			// remove separator
			item = menu->FindItem(B_CUT);
			if (item) {
				item = menu->ItemAt(menu->IndexOf(item)-1);
				if (item && menu->RemoveItem(item))
					delete item;
			}
		}
	}

	// add directory menu and menufield
	fDirMenu = new BDirMenu(0, this, kSwitchDirectory, "refs");

	font_height ht;
	be_plain_font->GetHeight(&ht);
	float f_height = ht.ascent + ht.descent + ht.leading;

	BRect rect;
	rect.top = fMenuBar->Bounds().Height() + 8;
	rect.left = windRect.left + 8;
	rect.right = rect.left + 300;
	rect.bottom = rect.top + (f_height > 22 ? f_height : 22);

	fDirMenuField = new BMenuField(rect, "DirMenuField", "", fDirMenu);
	fDirMenuField->MenuBar()->SetFont(be_plain_font);
	fDirMenuField->SetDivider(0);

	fDirMenuField->MenuBar()->RemoveItem((int32)0);
	fDirMenu->SetMenuBar(fDirMenuField->MenuBar());
		// the above is a weird call from BDirMenu
		// ToDo: clean up

	BEntry entry(TargetModel()->EntryRef());
	if (entry.InitCheck() == B_OK)
		fDirMenu->Populate(&entry, 0, true, true, false, true);
	else
		fDirMenu->Populate(0, 0, true, true, false, true);

	fBackView->AddChild(fDirMenuField);

	// add file name text view
	if (fIsSavePanel) {
		BRect rect(windRect);
		rect.top = rect.bottom - 35;
		rect.left = 8;
		rect.right = rect.left + 170;
		rect.bottom = rect.top + 13;

		fTextControl = new BTextControl(rect, "text view",
			B_TRANSLATE("save text"), "", NULL,
			B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
		DisallowMetaKeys(fTextControl->TextView());
		DisallowFilenameKeys(fTextControl->TextView());
		fBackView->AddChild(fTextControl);
		fTextControl->SetDivider(0.0f);
		fTextControl->TextView()->SetMaxBytes(B_FILE_NAME_LENGTH - 1);

		fButtonText.SetTo(B_TRANSLATE("Save"));
	} else
		fButtonText.SetTo(B_TRANSLATE("Open"));

	rect = windRect;
	rect.OffsetTo(10, fDirMenuField->Frame().bottom + 10);
	rect.bottom = windRect.bottom - 60;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 20;

	// re-parent the poseview to our backview
	// ToDo:
	// This is terrible, fix it up
	PoseView()->RemoveSelf();
	if (fIsSavePanel)
		fBackView->AddChild(PoseView(), fTextControl);
	else
		fBackView->AddChild(PoseView());

	PoseView()->MoveTo(rect.LeftTop());
	PoseView()->ResizeTo(rect.Width(), rect.Height());
	PoseView()->AddScrollBars();
	PoseView()->SetDragEnabled(false);
	PoseView()->SetDropEnabled(false);
	PoseView()->SetSelectionHandler(this);
	PoseView()->SetSelectionChangedHook(true);
	PoseView()->DisableSaveLocation();
	PoseView()->VScrollBar()->MoveBy(0, -1);
	PoseView()->VScrollBar()->ResizeBy(0, 1);


	AddShortcut('W', B_COMMAND_KEY, new BMessage(kCancelButton));
	AddShortcut('H', B_COMMAND_KEY, new BMessage(kSwitchToHome));
	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(kOpenDir));
	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY | B_OPTION_KEY, new BMessage(kOpenDir));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY, new BMessage(kOpenParentDir));
	AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY, new BMessage(kOpenParentDir));

	// New code to make buttons font sensitive
	rect = windRect;
	rect.top = rect.bottom - 35;
	rect.bottom -= 10;
	rect.right -= 25;
	float default_width = be_plain_font->StringWidth(fButtonText.String()) + 20;
	rect.left = (default_width > 75) ? (rect.right - default_width) : (rect.right - 75);

	BButton *default_button = new BButton(rect, "default button", fButtonText.String(),
		new BMessage(kDefaultButton), B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
	fBackView->AddChild(default_button);

	rect.right = rect.left -= 10;
	float cancel_width = be_plain_font->StringWidth(B_TRANSLATE("Cancel")) + 20;
	rect.left = (cancel_width > 75) ? (rect.right - cancel_width) : (rect.right - 75);

	BButton* cancel_button = new BButton(rect, "cancel button",
		B_TRANSLATE("Cancel"), new BMessage(kCancelButton),
		B_FOLLOW_RIGHT + B_FOLLOW_BOTTOM);
	fBackView->AddChild(cancel_button);

	if (!fIsSavePanel)
		default_button->SetEnabled(false);

	default_button->MakeDefault(true);

	RestoreState();

	PoseView()->ScrollTo(B_ORIGIN);
	PoseView()->UpdateScrollRange();
	PoseView()->ScrollTo(B_ORIGIN);

	if (fTextControl) {
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

	SetSizeLimits(370, 10000, 200, 10000);
}


void
TFilePanel::RestoreState()
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultFilePanelTemplate, &defaultingNode, false)) {
		AttributeStreamFileNode streamNodeSource(&defaultingNode);
		RestoreWindowState(&streamNodeSource);
		PoseView()->Init(&streamNodeSource);
	} else {
		RestoreWindowState(NULL);
		PoseView()->Init(NULL);
	}
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
	}
}


void
TFilePanel::SaveState(BMessage &message) const
{
	_inherited::SaveState(message);
}


void
TFilePanel::RestoreWindowState(AttributeStreamNode *node)
{
	SetSizeLimits(360, 10000, 200, 10000);
	if (!node)
		return;

	const char *rectAttributeName = kAttrWindowFrame;
	BRect frame(Frame());
	if (node->Read(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame)
		== sizeof(BRect)) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
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
TFilePanel::AddFileContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));
	menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
		? B_TRANSLATE("Delete")
		: B_TRANSLATE("Move to Trash"),
		new BMessage(kMoveToTrash), 'T'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
//	menu->AddItem(pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));

	menu->SetTargetForItems(PoseView());
}


void
TFilePanel::AddVolumeContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Cut"), new BMessage(B_CUT), 'X'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
//	menu->AddItem(pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));

	menu->SetTargetForItems(PoseView());
}


void
TFilePanel::AddWindowContextMenus(BMenu *menu)
{
	BMenuItem* item = new BMenuItem(B_TRANSLATE("New folder"),
		new BMessage(kNewFolder), 'N');
	item->SetTarget(PoseView());
	menu->AddItem(item);
	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Paste"), new BMessage(B_PASTE), 'V');
	item->SetTarget(PoseView());
	menu->AddItem(item);
	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection), 'S');
	item->SetTarget(PoseView());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Go to parent"),
		new BMessage(kOpenParentDir), B_UP_ARROW);
	item->SetTarget(this);
	menu->AddItem(item);
}


void
TFilePanel::AddDropContextMenus(BMenu *)
{
}


void
TFilePanel::MenusBeginning()
{
	int32 count = PoseView()->SelectionList()->CountItems();

	EnableNamedMenuItem(fMenuBar, kNewFolder, !TargetModel()->IsRoot());
	EnableNamedMenuItem(fMenuBar, kMoveToTrash, !TargetModel()->IsRoot() && count);
	EnableNamedMenuItem(fMenuBar, kGetInfo, count != 0);
	EnableNamedMenuItem(fMenuBar, kEditItem, count == 1);

	SetCutItem(fMenuBar);
	SetCopyItem(fMenuBar);
	SetPasteItem(fMenuBar);

	fIsTrackingMenu = true;
}


void
TFilePanel::MenusEnded()
{
	fIsTrackingMenu = false;
}


void
TFilePanel::ShowContextMenu(BPoint point, const entry_ref *ref, BView *view)
{
	EnableNamedMenuItem(fWindowContextMenu, kNewFolder, !TargetModel()->IsRoot());
	EnableNamedMenuItem(fWindowContextMenu, kOpenParentDir, !TargetModel()->IsRoot());
	EnableNamedMenuItem(fWindowContextMenu, kMoveToTrash, !TargetModel()->IsRoot());

	_inherited::ShowContextMenu(point, ref, view);
}


void
TFilePanel::SetupNavigationMenu(const entry_ref *, BMenu *)
{
	// do nothing here so nav menu doesn't get added
}


void
TFilePanel::SetButtonLabel(file_panel_button selector, const char *text)
{
	switch (selector) {
		case B_CANCEL_BUTTON:
			{
				BButton *button = dynamic_cast<BButton *>(FindView("cancel button"));
				if (!button)
					break;

				float old_width = button->StringWidth(button->Label());
				button->SetLabel(text);
				float delta = old_width - button->StringWidth(text);
				if (delta) {
					button->MoveBy(delta, 0);
					button->ResizeBy(-delta, 0);
				}
			}
			break;

		case B_DEFAULT_BUTTON:
			{
				fButtonText = text;
				float delta = 0;
				BButton *button = dynamic_cast<BButton *>(FindView("default button"));
				if (button) {
					float old_width = button->StringWidth(button->Label());
					button->SetLabel(text);
					delta = old_width - button->StringWidth(text);
					if (delta) {
						button->MoveBy(delta, 0);
						button->ResizeBy(-delta, 0);
					}
				}

				// now must move cancel button
				button = dynamic_cast<BButton *>(FindView("cancel button"));
				if (button)
					button->MoveBy(delta, 0);
			}
			break;
	}
}


void
TFilePanel::SetSaveText(const char *text)
{
	if (!text)
		return;

	BTextControl *textControl = dynamic_cast<BTextControl *>(FindView("text view"));
	textControl->SetText(text);
	textControl->TextView()->SelectAll();
}


void
TFilePanel::MessageReceived(BMessage *message)
{
	entry_ref ref;

	switch (message->what) {
		case B_REFS_RECEIVED:
			// item was double clicked in file panel (PoseView)
			if (message->FindRef("refs", &ref) == B_OK) {
				BEntry entry(&ref, true);
				if (entry.InitCheck() == B_OK) {
					// Double-click on dir or link-to-dir ALWAYS opens the dir.
					// If more than one dir is selected, the
					// first is entered.
					if (entry.IsDirectory()) {
						entry.GetRef(&ref);
						bool isDesktop = SwitchDirToDesktopIfNeeded(ref);

						PoseView()->SetIsDesktop(isDesktop);
						entry.SetTo(&ref);
						PoseView()->SwitchDir(&ref);
						SwitchDirMenuTo(&ref);
					} else {
						// Otherwise, we have a file or a link to a file.
						// AdjustButton has already tested the flavor;
						// all we have to do is see if the button is enabled.
						BButton *button = dynamic_cast<BButton *>(FindView("default button"));
						if (!button)
							break;

						if (IsSavePanel()) {
							int32 count = 0;
							type_code type;
							message->GetInfo("refs", &type, &count);

							// Don't allow saves of multiple files
							if (count > 1) {
								ShowCenteredAlert(
									B_TRANSLATE("Sorry, saving more than one item is not allowed."),
									B_TRANSLATE("Cancel"));
							} else {
								// if we are a savepanel, set up the filepanel correctly
								// then pass control so we follow the same path as if the user
								// clicked the save button

								// set the 'name' fld to the current ref's name
								// notify the panel that the default button should be enabled
								SetSaveText(ref.name);
								SelectionChanged();

								HandleSaveButton();
							}
							break;
						}

					  	// send handler a message and close
						BMessage openMessage(*fMessage);
						for (int32 index = 0; ; index++) {
					  		if (message->FindRef("refs", index, &ref) != B_OK)
								break;
							openMessage.AddRef("refs", &ref);
					  	}
						OpenSelectionCommon(&openMessage);
					}
				}
			}
			break;

		case kSwitchDirectory:
		{
			entry_ref ref;
			// this comes from dir menu or nav menu, so switch directories
			if (message->FindRef("refs", &ref) == B_OK) {
				BEntry entry(&ref, true);
				if (entry.GetRef(&ref) == B_OK)
					SetTo(&ref);
			}
			break;
		}

		case kSwitchToHome:
		{
			BPath homePath;
			entry_ref ref;
			if (find_directory(B_USER_DIRECTORY, &homePath) != B_OK
				|| get_ref_for_path(homePath.Path(), &ref) != B_OK)
				break;

			SetTo(&ref);
			break;
		}

		case kAddCurrentDir:
		{
			BPath path;
			if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
				break;

			path.Append(kGoDirectory);
			BDirectory goDirectory(path.Path());

			if (goDirectory.InitCheck() == B_OK) {
				BEntry entry(TargetModel()->EntryRef());
				entry.GetPath(&path);

				BSymLink link;
				goDirectory.CreateSymLink(TargetModel()->Name(), path.Path(), &link);
			}
			break;
		}

		case kEditFavorites:
		{
			BPath path;
			if (find_directory (B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
				break;

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

		case kOpenDir:
			OpenDirectory();
			break;

		case kOpenParentDir:
			OpenParent();
			break;

		case kDefaultButton:
			if (fIsSavePanel) {
				if (PoseView()->IsFocus()
					&& PoseView()->SelectionList()->CountItems() == 1) {
					Model *model = (PoseView()->SelectionList()->FirstItem())->TargetModel();
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
						bool desktopIsRoot = true;
						if (message->FindBool("DesktopFilePanelRoot", &desktopIsRoot) == B_OK)
							TrackerSettings().SetDesktopFilePanelRoot(desktopIsRoot);
						SetTo(TargetModel()->EntryRef());
						break;
					}
				}
			}
			break;
		}

		default:
			_inherited::MessageReceived(message);
	}
}


void
TFilePanel::OpenDirectory()
{
	BObjectList<BPose> *list = PoseView()->SelectionList();
	if (list->CountItems() != 1)
		return;

	Model *model = list->FirstItem()->TargetModel();
	if (model->ResolveIfLink()->IsDirectory()) {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", model->EntryRef());
		PostMessage(&message);
	}
}


void
TFilePanel::OpenParent()
{
	if (!CanOpenParent())
		return;

	BEntry parentEntry;
	BDirectory dir;

	Model oldModel(*PoseView()->TargetModel());
	BEntry entry(oldModel.EntryRef());

	if (entry.InitCheck() == B_OK
		&& entry.GetParent(&dir) == B_OK
		&& dir.GetEntry(&parentEntry) == B_OK
		&& entry != parentEntry) {

		entry_ref ref;
		parentEntry.GetRef(&ref);

		PoseView()->SetIsDesktop(SwitchDirToDesktopIfNeeded(ref));
		PoseView()->SwitchDir(&ref);
		SwitchDirMenuTo(&ref);

		// make sure the child get's selected in the new view once it
		// shows up
		fTaskLoop->RunLater(NewMemberFunctionObjectWithResult
			(&TFilePanel::SelectChildInParent, this,
			const_cast<const entry_ref *>(&ref),
			oldModel.NodeRef()), 100000, 200000, 5000000);
	}
}


bool
TFilePanel::CanOpenParent() const
{
	if (TrackerSettings().DesktopFilePanelRoot()) {
		// don't allow opening Desktop folder's parent
		if (TargetModel()->IsDesktop())
			return false;
	}

	// block on "/"
	BEntry root("/");
	node_ref rootRef;
	root.GetNodeRef(&rootRef);

	return rootRef != *TargetModel()->NodeRef();
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
	BEntry root("/");

	BDirectory desktopDir;
	FSGetDeskDir(&desktopDir);
	if (FSIsDeskDir(&entry)
		// navigated into non-boot desktop, switch to boot desktop
		|| (entry == root && !settings.ShowDisksIcon())) {
		// hit "/" level, map to desktop

		desktopDir.GetEntry(&entry);
		entry.GetRef(&ref);
		return true;
	}
	return FSIsDeskDir(&entry);
}


bool
TFilePanel::SelectChildInParent(const entry_ref *, const node_ref *child)
{
	AutoLock<TFilePanel> lock(this);

	if (!IsLocked())
		return false;

	int32 index;
	BPose *pose = PoseView()->FindPose(child, &index);
	if (!pose)
		return false;

	PoseView()->UpdateScrollRange();
		// ToDo: Scroll range should be updated by now, for some
		//	reason sometimes it is not right, force it here
	PoseView()->SelectPose(pose, index, true);
	return true;
}


int32
TFilePanel::ShowCenteredAlert(const char *text, const char *button1,
	const char *button2, const char *button3)
{
	BAlert *alert = new BAlert("", text, button1, button2, button3,
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
			// if this was invoked by a dbl click, it is an explicit replacement
			// of the file.
			BString str(B_TRANSLATE("The file \"%name\" already exists in the specified folder. Do you want to replace it?"));
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
TFilePanel::OpenSelectionCommon(BMessage *openMessage)
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
	BObjectList<BPose> *selection = PoseView()->SelectionList();

	// if we have only one directory and we're not opening dirs, enter.
	if ((fNodeFlavors & B_DIRECTORY_NODE) == 0
		&& selection->CountItems() == 1) {
		Model *model = selection->FirstItem()->TargetModel();

		if (model->IsDirectory()
			|| (model->IsSymLink() && !(fNodeFlavors & B_SYMLINK_NODE)
				&& model->ResolveIfLink()->IsDirectory())) {

			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", model->EntryRef());
			PostMessage(&message);
			return;
		}
	}

	// don't do anything unless there are items selected
    // message->fMessage->message from here to end
	if (selection->CountItems()) {
		BMessage message(*fMessage);
		// go through selection and add appropriate items
		for (int32 index = 0; index < selection->CountItems(); index++) {
			Model *model = selection->ItemAt(index)->TargetModel();

			if (((fNodeFlavors & B_DIRECTORY_NODE) != 0
					&& model->ResolveIfLink()->IsDirectory())
				|| ((fNodeFlavors & B_SYMLINK_NODE) != 0 && model->IsSymLink())
				|| ((fNodeFlavors & B_FILE_NODE) != 0 && model->ResolveIfLink()->IsFile()))
				message.AddRef("refs", model->EntryRef());
		}

		OpenSelectionCommon(&message);
	}
}


void
TFilePanel::SwitchDirMenuTo(const entry_ref *ref)
{
	BEntry entry(ref);
	for (int32 index = fDirMenu->CountItems() - 1; index >= 0; index--)
		delete fDirMenu->RemoveItem(index);

	fDirMenuField->MenuBar()->RemoveItem((int32)0);
	fDirMenu->Populate(&entry, 0, true, true, false, true);

	ModelMenuItem *item = dynamic_cast<ModelMenuItem *>(
		fDirMenuField->MenuBar()->ItemAt(0));
	ASSERT(item);
	item->SetEntry(&entry);
}


void
TFilePanel::WindowActivated(bool active)
{
	// force focus to update properly
	fBackView->Invalidate();
	_inherited::WindowActivated(active);
}


//	#pragma mark -


BFilePanelPoseView::BFilePanelPoseView(Model *model, BRect frame, uint32 resizeMask)
	: BPoseView(model, frame, kListMode, resizeMask),
	fIsDesktop(model->IsDesktop())
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
BFilePanelPoseView::FSNotification(const BMessage *message)
{
	if (IsDesktopView()) {
		// Pretty much copied straight from DesktopPoseView.  Would be better
		// if the code could be shared somehow.
		switch (message->FindInt32("opcode")) {
			case B_DEVICE_MOUNTED:
			{
				dev_t device;
				if (message->FindInt32("new device", &device) != B_OK)
					break;

				ASSERT(TargetModel());
				TrackerSettings settings;

				BVolume volume(device);
				if (volume.InitCheck() != B_OK)
					break;

				if (settings.MountVolumesOntoDesktop()
					&& (!volume.IsShared() || settings.MountSharedVolumesOntoDesktop())) {
					// place an icon for the volume onto the desktop
					CreateVolumePose(&volume, true);
				}
			}
			break;
		}
	}
	return _inherited::FSNotification(message);
}


void
BFilePanelPoseView::RestoreState(AttributeStreamNode *node)
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
BFilePanelPoseView::SavePoseLocations(BRect *)
{
}


EntryListBase *
BFilePanelPoseView::InitDirentIterator(const entry_ref *ref)
{
	if (IsDesktopView())
		return DesktopPoseView::InitDesktopDirentIterator(this, ref);

	return _inherited::InitDirentIterator(ref);
}


void
BFilePanelPoseView::AddPosesCompleted()
{
	_inherited::AddPosesCompleted();
	if (IsDesktopView())
		CreateTrashPose();
}


void
BFilePanelPoseView::SetIsDesktop(bool on)
{
	fIsDesktop = on;
}


bool
BFilePanelPoseView::IsDesktopView() const
{
	return fIsDesktop;
}


void
BFilePanelPoseView::ShowVolumes(bool visible, bool showShared)
{
	if (IsDesktopView()) {
		if (!visible)
			RemoveRootPoses();
		else
			AddRootPoses(true, showShared);
	}


	TFilePanel *filepanel = dynamic_cast<TFilePanel*>(Window());
	if (filepanel)
		filepanel->SetTo(TargetModel()->EntryRef());
}


void
BFilePanelPoseView::AdaptToVolumeChange(BMessage *message)
{
	bool showDisksIcon;
	bool mountVolumesOnDesktop;
	bool mountSharedVolumesOntoDesktop;

	message->FindBool("ShowDisksIcon", &showDisksIcon);
	message->FindBool("MountVolumesOntoDesktop", &mountVolumesOnDesktop);
	message->FindBool("MountSharedVolumesOntoDesktop", &mountSharedVolumesOntoDesktop);

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
		if (Window())
			Window()->PostMessage(&monitorMsg, this);
	}

	ShowVolumes(mountVolumesOnDesktop, mountSharedVolumesOntoDesktop);
}


void
BFilePanelPoseView::AdaptToDesktopIntegrationChange(BMessage *message)
{
	bool mountVolumesOnDesktop = true;
	bool mountSharedVolumesOntoDesktop = true;

	message->FindBool("MountVolumesOntoDesktop", &mountVolumesOnDesktop);
	message->FindBool("MountSharedVolumesOntoDesktop", &mountSharedVolumesOntoDesktop);

	ShowVolumes(false, mountSharedVolumesOntoDesktop);
	ShowVolumes(mountVolumesOnDesktop, mountSharedVolumesOntoDesktop);
}

