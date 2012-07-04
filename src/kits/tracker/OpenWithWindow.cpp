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
#include "AutoLock.h"
#include "Commands.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "OpenWithWindow.h"
#include "MimeTypes.h"
#include "StopWatch.h"
#include "Tracker.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Locale.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


const char *kDefaultOpenWithTemplate = "OpenWithSettings";

// ToDo:
// filter out trash
// allow column configuring
// make SaveState/RestoreState save the current window setting for
// other windows

const float kMaxMenuWidth = 150;

const int32 kDocumentKnobWidth = 16;
const int32 kOpenAndMakeDefault = 'OpDf';
const rgb_color kOpenWithDefaultColor = { 0xFF, 0xFF, 0xCC, 255};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OpenWithWindow"

OpenWithContainerWindow::OpenWithContainerWindow(BMessage *entriesToOpen,
		LockingList<BWindow> *windowList, window_look look, window_feel feel,
		uint32 flags, uint32 workspace)
	: BContainerWindow(windowList, 0, look, feel, flags, workspace),
	fEntriesToOpen(entriesToOpen)
{
	AutoLock<BWindow> lock(this);

	BRect windowRect(85, 50, 718, 296);
	MoveTo(windowRect.LeftTop());
	ResizeTo(windowRect.Width(), windowRect.Height());

	// add a background view; use the standard BackgroundView here, the same
	// as the file panel is using
	BRect rect(Bounds());
	BackgroundView *backgroundView = new BackgroundView(rect);
	AddChild(backgroundView);

	rect = Bounds();

	// add buttons

	fLaunchButton = new BButton(rect, "ok",	B_TRANSLATE("Open"),
		new BMessage(kDefaultButton), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fLaunchButton->ResizeToPreferred();
	fLaunchButton->MoveTo(rect.right - 10 - kDocumentKnobWidth
		- fLaunchButton->Bounds().Width(),
		rect.bottom - 10 - fLaunchButton->Bounds().Height());
	backgroundView->AddChild(fLaunchButton);

	BRect buttonRect = fLaunchButton->Frame();
	fLaunchAndMakeDefaultButton = new BButton(buttonRect, "make default",
		B_TRANSLATE("Open and make preferred"),	new BMessage(kOpenAndMakeDefault),
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	// wide button, have to resize to fit text
	fLaunchAndMakeDefaultButton->ResizeToPreferred();
	fLaunchAndMakeDefaultButton->MoveBy(
		- 10 - fLaunchAndMakeDefaultButton->Bounds().Width(), 0);
	backgroundView->AddChild(fLaunchAndMakeDefaultButton);
	fLaunchAndMakeDefaultButton->SetEnabled(false);

	buttonRect = fLaunchAndMakeDefaultButton->Frame();
	BButton *button = new BButton(buttonRect, "cancel",	B_TRANSLATE("Cancel"),
		new BMessage(kCancelButton), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveBy(- 10 - button->Bounds().Width(), 0);
	backgroundView->AddChild(button);

	fMinimalWidth = button->Bounds().Width() + fLaunchButton->Bounds().Width()
		+ fLaunchAndMakeDefaultButton->Bounds().Width() + kDocumentKnobWidth + 40;

	fLaunchButton->MakeDefault(true);

	// add pose view

	rect.OffsetTo(10, 10);
	rect.bottom = buttonRect.top - 15;

	rect.right -= B_V_SCROLL_BAR_WIDTH + 20;
	rect.bottom -= B_H_SCROLL_BAR_HEIGHT;
		// make room for scrollbars and a margin
	fPoseView = NewPoseView(0, rect, kListMode);
	backgroundView->AddChild(fPoseView);

	fPoseView->SetFlags(fPoseView->Flags() | B_NAVIGABLE);
	fPoseView->SetPoseEditing(false);

	// set the window title
	if (CountRefs(fEntriesToOpen) == 1) {
		// if opening just one file, use it in the title
		entry_ref ref;
		fEntriesToOpen->FindRef("refs", &ref);
		BString buffer(B_TRANSLATE("Open %name with:"));
		buffer.ReplaceFirst("%name", ref.name);

		SetTitle(buffer.String());
	} else
		// use generic title
		SetTitle(B_TRANSLATE("Open selection with:"));

	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, &OpenWithContainerWindow::KeyDownFilter));
}


OpenWithContainerWindow::~OpenWithContainerWindow()
{
	delete fEntriesToOpen;
}


BPoseView *
OpenWithContainerWindow::NewPoseView(Model *, BRect rect, uint32)
{
	return new OpenWithPoseView(rect);
}


OpenWithPoseView *
OpenWithContainerWindow::PoseView() const
{
	ASSERT(dynamic_cast<OpenWithPoseView *>(fPoseView));
	return static_cast<OpenWithPoseView *>(fPoseView);
}


const BMessage *
OpenWithContainerWindow::EntryList() const
{
	return fEntriesToOpen;
}


void
OpenWithContainerWindow::OpenWithSelection()
{
	int32 count = PoseView()->SelectionList()->CountItems();
	ASSERT(count == 1);
	if (!count)
		return;

	PoseView()->OpenSelection(PoseView()->SelectionList()->FirstItem(), 0);
}


static const BString *
FindOne(const BString *element, void *castToString)
{
	if (strcasecmp(element->String(), (const char *)castToString) == 0)
		return element;

	return 0;
}


static const entry_ref *
AddOneUniqueDocumentType(const entry_ref *ref, void *castToList)
{
	BObjectList<BString> *list = (BObjectList<BString> *)castToList;

	BEntry entry(ref, true);
		// traverse symlinks

	// get this documents type
	char type[B_MIME_TYPE_LENGTH];
	BFile file(&entry, O_RDONLY);
	if (file.InitCheck() != B_OK)
		return 0;

	BNodeInfo info(&file);
	if (info.GetType(type) != B_OK)
		return 0;

	if (list->EachElement(FindOne, &type))
		// type already in list, bail
		return 0;

	// add type to list
	list->AddItem(new BString(type));
	return 0;
}


static const BString *
SetDefaultAppForOneType(const BString *element, void *castToEntryRef)
{
	const entry_ref *appRef = (const entry_ref *)castToEntryRef;

	// set entry as default handler for one mime string
	BMimeType mime(element->String());
	if (!mime.IsInstalled())
		return 0;

	// first set it's app signature as the preferred type
	BFile appFile(appRef, O_RDONLY);
	if (appFile.InitCheck() != B_OK)
		return 0;

	char appSignature[B_MIME_TYPE_LENGTH];
	if (GetAppSignatureFromAttr(&appFile, appSignature) != B_OK)
		return 0;

	if (mime.SetPreferredApp(appSignature) != B_OK)
		return 0;

	// set the app hint on the metamime for this signature
	mime.SetTo(appSignature);
#if xDEBUG
	status_t result =
#endif
	mime.SetAppHint(appRef);

#if xDEBUG
	BEntry debugEntry(appRef);
	BPath debugPath;
	debugEntry.GetPath(&debugPath);

	PRINT(("setting %s, sig %s as default app for %s, result %s\n",
		debugPath.Path(), appSignature, element->String(), strerror(result)));
#endif

	return 0;
}


void
OpenWithContainerWindow::MakeDefaultAndOpen()
{
	int32 count = PoseView()->SelectionList()->CountItems();
	ASSERT(count == 1);
	if (!count)
		return;

	BPose *selectedAppPose = PoseView()->SelectionList()->FirstItem();
	ASSERT(selectedAppPose);
	if (!selectedAppPose)
		return;

	// collect all the types of all the opened documents into a list
	BObjectList<BString> openedFileTypes(10, true);
	EachEntryRef(EntryList(), AddOneUniqueDocumentType, &openedFileTypes, 100);

	// set the default application to be the selected pose for all the
	// mime types in the list
	openedFileTypes.EachElement(SetDefaultAppForOneType,
		(void *)selectedAppPose->TargetModel()->EntryRef());

	// done setting the default application, now launch the app with the
	// documents
	OpenWithSelection();
}


void
OpenWithContainerWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kDefaultButton:
			OpenWithSelection();
			PostMessage(B_QUIT_REQUESTED);
			return;

		case kOpenAndMakeDefault:
			MakeDefaultAndOpen();
			PostMessage(B_QUIT_REQUESTED);
			return;

		case kCancelButton:
			PostMessage(B_QUIT_REQUESTED);
			return;

		case B_OBSERVER_NOTICE_CHANGE:
			return;

		case kResizeToFit:
			ResizeToFit();
			break;
	}
	_inherited::MessageReceived(message);
}


filter_result
OpenWithContainerWindow::KeyDownFilter(BMessage *message, BHandler **,
	BMessageFilter *filter)
{
	uchar key;
	if (message->FindInt8("byte", (int8 *)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	int32 modifier=0;
	message->FindInt32("modifiers", &modifier);
	if (!modifier && key == B_ESCAPE) {
		filter->Looper()->PostMessage(kCancelButton);
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


bool
OpenWithContainerWindow::ShouldAddMenus() const
{
	return false;
}


void
OpenWithContainerWindow::ShowContextMenu(BPoint, const entry_ref *, BView *)
{
}


void
OpenWithContainerWindow::AddShortcuts()
{
	AddShortcut('I', B_COMMAND_KEY, new BMessage(kGetInfo), PoseView());
	AddShortcut('Y', B_COMMAND_KEY, new BMessage(kResizeToFit), PoseView());
}


void
OpenWithContainerWindow::NewAttributeMenu(BMenu *menu)
{
	_inherited::NewAttributeMenu(menu);
	BMessage *message = new BMessage(kAttributeItem);
	message->AddString("attr_name", kAttrOpenWithRelation);
	message->AddInt32("attr_type", B_STRING_TYPE);
	message->AddInt32("attr_hash", (int32)AttrHashString(kAttrOpenWithRelation, B_STRING_TYPE));
	message->AddFloat("attr_width", 180);
	message->AddInt32("attr_align", B_ALIGN_LEFT);
	message->AddBool("attr_editable", false);
	message->AddBool("attr_statfield", false);
	BMenuItem *item = new BMenuItem(B_TRANSLATE("Relation"), message);
	menu->AddItem(item);
	message = new BMessage(kAttributeItem);
	message->AddString("attr_name", kAttrAppVersion);
	message->AddInt32("attr_type", B_STRING_TYPE);
	message->AddInt32("attr_hash", (int32)AttrHashString(kAttrAppVersion, B_STRING_TYPE));
	message->AddFloat("attr_width", 70);
	message->AddInt32("attr_align", B_ALIGN_LEFT);
	message->AddBool("attr_editable", false);
	message->AddBool("attr_statfield", false);
	item = new BMenuItem(B_TRANSLATE("Version"), message);
	menu->AddItem(item);
}


void
OpenWithContainerWindow::SaveState(bool)
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultOpenWithTemplate, &defaultingNode,
			true, false)) {
		AttributeStreamFileNode streamNodeDestination(&defaultingNode);
		SaveWindowState(&streamNodeDestination);
		fPoseView->SaveState(&streamNodeDestination);
	}
}


void
OpenWithContainerWindow::SaveState(BMessage &message) const
{
	_inherited::SaveState(message);
}


void
OpenWithContainerWindow::Init(const BMessage *message)
{
	_inherited::Init(message);
}


void
OpenWithContainerWindow::RestoreState()
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultOpenWithTemplate, &defaultingNode, false)) {
		AttributeStreamFileNode streamNodeSource(&defaultingNode);
		RestoreWindowState(&streamNodeSource);
		fPoseView->Init(&streamNodeSource);
	} else {
		RestoreWindowState(NULL);
		fPoseView->Init(NULL);
	}
}


void
OpenWithContainerWindow::RestoreState(const BMessage &message)
{
	_inherited::RestoreState(message);
}


void
OpenWithContainerWindow::RestoreWindowState(AttributeStreamNode *node)
{
	SetSizeLimits(fMinimalWidth, 10000, 160, 10000);
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
OpenWithContainerWindow::RestoreWindowState(const BMessage &message)
{
	_inherited::RestoreWindowState(message);
}


bool
OpenWithContainerWindow::NeedsDefaultStateSetup()
{
	return true;
}


void
OpenWithContainerWindow::SetUpDefaultState()
{
}


bool
OpenWithContainerWindow::IsShowing(const node_ref *) const
{
	return false;
}


bool
OpenWithContainerWindow::IsShowing(const entry_ref *) const
{
	return false;
}


void
OpenWithContainerWindow::SetCanSetAppAsDefault(bool on)
{
	fLaunchAndMakeDefaultButton->SetEnabled(on);
}


void
OpenWithContainerWindow::SetCanOpen(bool on)
{
	fLaunchButton->SetEnabled(on);
}


//	#pragma mark -


OpenWithPoseView::OpenWithPoseView(BRect frame, uint32 resizeMask)
	: BPoseView(0, frame, kListMode, resizeMask),
	fHaveCommonPreferredApp(false),
	fIterator(NULL)
{
	fSavePoseLocations = false;
	fMultipleSelection = false;
	fDragEnabled = false;
}


OpenWithContainerWindow *
OpenWithPoseView::ContainerWindow() const
{
	ASSERT(dynamic_cast<OpenWithContainerWindow *>(Window()));
	return static_cast<OpenWithContainerWindow *>(Window());
}


void
OpenWithPoseView::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	SetViewColor(kOpenWithDefaultColor);
	SetLowColor(kOpenWithDefaultColor);
}


bool
OpenWithPoseView::CanHandleDragSelection(const Model *, const BMessage *, bool)
{
	return false;
}


static void
AddSupportingAppForTypeToQuery(SearchForSignatureEntryList *queryIterator,
	const char *type)
{
	// get supporting apps for type
	BMimeType mime(type);
	if (!mime.IsInstalled())
		return;

	BMessage message;
	mime.GetSupportingApps(&message);

	// push each of the supporting apps signature uniquely

	const char *signature;
	for (int32 index = 0; message.FindString("applications", index,
			&signature) == B_OK; index++) {
		queryIterator->PushUniqueSignature(signature);
	}
}


static const entry_ref *
AddOneRefSignatures(const entry_ref *ref, void *castToIterator)
{
	// TODO: resolve cases where each entry has a different type and
	// their supporting apps are disjoint sets

	SearchForSignatureEntryList *queryIterator =
		(SearchForSignatureEntryList *)castToIterator;

	Model model(ref, true, true);
	if (model.InitCheck() != B_OK)
		return NULL;

	BString mimeType(model.MimeType());

	if (!mimeType.Length() || mimeType.ICompare(B_FILE_MIMETYPE) == 0)
		// if model is of unknown type, try mimeseting it first
		model.Mimeset(true);

	entry_ref preferredRef;

	// add preferred app for file, if any
	if (model.PreferredAppSignature()[0]) {
		// got one, mark it as preferred for this node
		if (be_roster->FindApp(model.PreferredAppSignature(), &preferredRef) == B_OK) {
			queryIterator->PushUniqueSignature(model.PreferredAppSignature());
			queryIterator->TrySettingPreferredAppForFile(&preferredRef);
		}
	}

	mimeType = model.MimeType();
	mimeType.ToLower();

	if (mimeType.Length() && !mimeType.ICompare(B_FILE_MIMETYPE) == 0)
		queryIterator->NonGenericFileFound();

	// get supporting apps for type
	AddSupportingAppForTypeToQuery(queryIterator, mimeType.String());

	// find the preferred app for this type
	if (be_roster->FindApp(mimeType.String(), &preferredRef) == B_OK)
		queryIterator->TrySettingPreferredApp(&preferredRef);

	return NULL;
}


EntryListBase *
OpenWithPoseView::InitDirentIterator(const entry_ref *)
{
	OpenWithContainerWindow *window = ContainerWindow();

	const BMessage *entryList = window->EntryList();

	fIterator = new SearchForSignatureEntryList(true);

	// push all the supporting apps from all the entries into the
	// search for signature iterator
	EachEntryRef(entryList, AddOneRefSignatures, fIterator, 100);

	// push superhandlers
	AddSupportingAppForTypeToQuery(fIterator, B_FILE_MIMETYPE);
	fHaveCommonPreferredApp = fIterator->GetPreferredApp(&fPreferredRef);

	if (fIterator->Rewind() != B_OK) {
		delete fIterator;
		fIterator = NULL;
		HideBarberPole();
		return NULL;
	}
	return fIterator;
}


void
OpenWithPoseView::OpenSelection(BPose *pose, int32 *)
{
	OpenWithContainerWindow *window = ContainerWindow();

	int32 count = fSelectionList->CountItems();
	if (!count)
		return;

	if (!pose)
		pose = fSelectionList->FirstItem();

	ASSERT(pose);

	BEntry entry(pose->TargetModel()->EntryRef());
	if (entry.InitCheck() != B_OK) {
		BString errorString(
			B_TRANSLATE("Could not find application \"%appname\""));
		errorString.ReplaceFirst("%appname", pose->TargetModel()->Name());

		(new BAlert("", errorString.String(), B_TRANSLATE("OK"), 0, 0,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return;
	}

	if (OpenWithRelation(pose->TargetModel()) == kNoRelation) {
		if (!fIterator->GenericFilesOnly()) {
			BString warning(B_TRANSLATE(
				"The application \"%appname\" does not support the type of "
				"document you are about to open.\nAre you sure you want to "
				"proceed?\n\nIf you know that the application supports the "
				"document type, you should contact the publisher of the "
				"application and ask them to update their application to list "
				"the type of your document as supported."));
			warning.ReplaceFirst("%appname", pose->TargetModel()->Name());

			BAlert* alert = new BAlert("", warning.String(),
				B_TRANSLATE("Cancel"), B_TRANSLATE("Open"),	0, B_WIDTH_AS_USUAL,
				B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 0)
				return;
		}
		// else - once we have an extensible sniffer, tell users to ask
		// publishers to fix up sniffers
	}

	BMessage message(*window->EntryList());
		// make a clone to send
	message.RemoveName("launchUsingSelector");
		// make sure the old selector is not in the message
	message.AddRef("handler", pose->TargetModel()->EntryRef());
		// add ref of the selected handler

	ASSERT(fSelectionHandler);

	if (fSelectionHandler)
		fSelectionHandler->PostMessage(&message);

	window->PostMessage(B_QUIT_REQUESTED);
}


void
OpenWithPoseView::Pulse()
{
	// disable the Open and make default button if the default
	// app matches the selected app
	//
	// disable the Open button if no apps selected

	OpenWithContainerWindow *window = ContainerWindow();

	if (!fSelectionList->CountItems()) {
		window->SetCanSetAppAsDefault(false);
		window->SetCanOpen(false);
		_inherited::Pulse();
		return;
	}

	// if we selected a non-handling application, don't allow setting
	// it as preferred
	Model *firstSelected = fSelectionList->FirstItem()->TargetModel();
	if (OpenWithRelation(firstSelected) == kNoRelation) {
		window->SetCanSetAppAsDefault(false);
		window->SetCanOpen(true);
		_inherited::Pulse();
		return;
	}

	// make the open button enabled, because we have na app selected
	window->SetCanOpen(true);
	if (!fHaveCommonPreferredApp) {
		window->SetCanSetAppAsDefault(true);
		_inherited::Pulse();
		return;
	}

	ASSERT(fSelectionList->CountItems() == 1);

	// enable the Open and make default if selected application different
	// from preferred app ref
	window->SetCanSetAppAsDefault((*fSelectionList->FirstItem()->
		TargetModel()->EntryRef()) != fPreferredRef);

	_inherited::Pulse();
}


void
OpenWithPoseView::SetUpDefaultColumnsIfNeeded()
{
	// in case there were errors getting some columns
	if (fColumnList->CountItems() != 0)
		return;

	BColumn *nameColumn = new BColumn(B_TRANSLATE("Name"), kColumnStart, 125,
		B_ALIGN_LEFT, kAttrStatName, B_STRING_TYPE, true, true);
	fColumnList->AddItem(nameColumn);
	BColumn *relationColumn = new BColumn(B_TRANSLATE("Relation"), 180, 100,
		B_ALIGN_LEFT, kAttrOpenWithRelation, B_STRING_TYPE, false, false);
	fColumnList->AddItem(relationColumn);
	fColumnList->AddItem(new BColumn(B_TRANSLATE("Location"), 290, 225,
		B_ALIGN_LEFT, kAttrPath, B_STRING_TYPE, true, false));
	fColumnList->AddItem(new BColumn(B_TRANSLATE("Version"), 525, 70,
		B_ALIGN_LEFT, kAttrAppVersion, B_STRING_TYPE, false, false));

	// sort by relation and by name
	SetPrimarySort(relationColumn->AttrHash());
	SetSecondarySort(nameColumn->AttrHash());
}


bool
OpenWithPoseView::AddPosesThreadValid(const entry_ref *) const
{
	return true;
}


void
OpenWithPoseView::CreatePoses(Model **models, PoseInfo *poseInfoArray, int32 count,
	BPose **resultingPoses, bool insertionSort,	int32 *lastPoseIndexPtr,
	BRect *boundsPtr, bool forceDraw)
{
	// overridden to try to select the preferred handling app
	_inherited::CreatePoses(models, poseInfoArray, count, resultingPoses, insertionSort,
		lastPoseIndexPtr, boundsPtr, forceDraw);

	if (resultingPoses) {
		for (int32 index = 0; index < count; index++) {
			if (resultingPoses[index] && fHaveCommonPreferredApp
				&& *(models[index]->EntryRef()) == fPreferredRef) {
				// this is our preferred app, select it's pose
				SelectPose(resultingPoses[index], IndexOfPose(resultingPoses[index]));
			}
		}
	}
}


void
OpenWithPoseView::KeyDown(const char *bytes, int32 count)
{
	if (bytes[0] == B_TAB) {
		// just shift the focus, don't tab to the next pose
		BView::KeyDown(bytes, count);
	} else
		_inherited::KeyDown(bytes, count);
}


void
OpenWithPoseView::SaveState(AttributeStreamNode *node)
{
	_inherited::SaveState(node);
}


void
OpenWithPoseView::RestoreState(AttributeStreamNode *node)
{
	_inherited::RestoreState(node);
	fViewState->SetViewMode(kListMode);
}


void
OpenWithPoseView::SaveState(BMessage &message) const
{
	_inherited::SaveState(message);
}


void
OpenWithPoseView::RestoreState(const BMessage &message)
{
	_inherited::RestoreState(message);
	fViewState->SetViewMode(kListMode);
}


void
OpenWithPoseView::SavePoseLocations(BRect *)
{
	// do nothing
}


void
OpenWithPoseView::MoveSelectionToTrash(bool)
{
}


void
OpenWithPoseView::MoveSelectionTo(BPoint, BPoint, BContainerWindow *)
{
}


void
OpenWithPoseView::MoveSelectionInto(Model *, BContainerWindow *, bool, bool)
{
}


bool
OpenWithPoseView::Represents(const node_ref *) const
{
	return false;
}


bool
OpenWithPoseView::Represents(const entry_ref *) const
{
	return false;
}


bool
OpenWithPoseView::HandleMessageDropped(BMessage *DEBUG_ONLY(message))
{
#if DEBUG
	// in debug mode allow tweaking the colors
	const rgb_color *color;
	int32 size;
	// handle roColour-style color drops
	if (message->FindData("RGBColor", 'RGBC', (const void **)&color, &size) == B_OK) {
		SetViewColor(*color);
		SetLowColor(*color);
		Invalidate();
		return true;
	}
#endif
	return false;
}


int32
OpenWithPoseView::OpenWithRelation(const Model *model) const
{
	OpenWithContainerWindow *window = ContainerWindow();

	return SearchForSignatureEntryList::Relation(window->EntryList(),
		model, fHaveCommonPreferredApp ? &fPreferredRef : 0, 0);
}


void
OpenWithPoseView::OpenWithRelationDescription(const Model *model,
	BString *description) const
{
	OpenWithContainerWindow *window = ContainerWindow();

	SearchForSignatureEntryList::RelationDescription(window->EntryList(),
		model, description, fHaveCommonPreferredApp ? &fPreferredRef : 0, 0);
}


bool
OpenWithPoseView::ShouldShowPose(const Model *model, const PoseInfo *poseInfo)
{
	OpenWithContainerWindow *window = ContainerWindow();
	// filter for add_poses
	if (!fIterator->CanOpenWithFilter(model, window->EntryList(),
		fHaveCommonPreferredApp ? &fPreferredRef : 0))
		return false;

	return _inherited::ShouldShowPose(model, poseInfo);
}


//	#pragma mark -


RelationCachingModelProxy::RelationCachingModelProxy(Model *model)
	:
	fModel(model),
	fRelation(kUnknownRelation)
{
}


RelationCachingModelProxy::~RelationCachingModelProxy()
{
	delete fModel;
}


int32
RelationCachingModelProxy::Relation(SearchForSignatureEntryList *iterator,
	BMessage *entries) const
{
	if (fRelation == kUnknownRelation)
		fRelation = iterator->Relation(entries, fModel);

	return fRelation;
}


//	#pragma mark -


OpenWithMenu::OpenWithMenu(const char *label, const BMessage *entriesToOpen,
		BWindow *parentWindow, BHandler *target)
	: BSlowMenu(label),
	fEntriesToOpen(*entriesToOpen),
	target(target),
	fIterator(NULL),
	fSupportingAppList(NULL),
	fParentWindow(parentWindow)
{
	InitIconPreloader();

	SetFont(be_plain_font);

	// too long to have triggers
	SetTriggersEnabled(false);
}


OpenWithMenu::OpenWithMenu(const char *label, const BMessage *entriesToOpen,
		BWindow *parentWindow, const BMessenger &messenger)
	: BSlowMenu(label),
	fEntriesToOpen(*entriesToOpen),
	target(NULL),
	fMessenger(messenger),
	fIterator(NULL),
	fSupportingAppList(NULL),
	fParentWindow(parentWindow)
{
	InitIconPreloader();

	SetFont(be_plain_font);

	// too long to have triggers
	SetTriggersEnabled(false);
}


namespace BPrivate {

int
SortByRelationAndName(const RelationCachingModelProxy *model1,
	const RelationCachingModelProxy *model2, void *castToMenu)
{
	OpenWithMenu *menu = (OpenWithMenu *)castToMenu;

	// find out the relations of app models to the opened entries
	int32 relation1 = model1->Relation(menu->fIterator, &menu->fEntriesToOpen);
	int32 relation2 = model2->Relation(menu->fIterator, &menu->fEntriesToOpen);

	if (relation1 < relation2) {
		// relation with the lowest number goes first
		return 1;
	} else if (relation1 > relation2)
		return -1;

	// if relations match, sort by app name
	return strcmp(model1->fModel->Name(), model2->fModel->Name());
}

} // namespace BPrivate


bool
OpenWithMenu::StartBuildingItemList()
{
	fIterator = new SearchForSignatureEntryList(false);
	// push all the supporting apps from all the entries into the
	// search for signature iterator
	EachEntryRef(&fEntriesToOpen, AddOneRefSignatures, fIterator, 100);
	// add superhandlers
	AddSupportingAppForTypeToQuery(fIterator, B_FILE_MIMETYPE);

	fHaveCommonPreferredApp = fIterator->GetPreferredApp(&fPreferredRef);
	status_t error = fIterator->Rewind();
	if (error != B_OK) {
		PRINT(("failed to initialize iterator %s\n", strerror(error)));
		return false;
	}

	fSupportingAppList = new BObjectList<RelationCachingModelProxy>(20, true);

	//queryRetrieval = new BStopWatch("get next entry on BQuery");
	return true;
}


bool
OpenWithMenu::AddNextItem()
{
	BEntry entry;
	if (fIterator->GetNextEntry(&entry) != B_OK)
		return false;

	Model *model = new Model(&entry, true);
	if (model->InitCheck() != B_OK
		|| !fIterator->CanOpenWithFilter(model, &fEntriesToOpen,
				fHaveCommonPreferredApp ? &fPreferredRef : 0)) {
		// only allow executables, filter out multiple copies of the
		// Tracker, filter out version that don't list the correct types,
		// etc.
		delete model;
	} else
		fSupportingAppList->AddItem(new RelationCachingModelProxy(model));

	return true;
}


void
OpenWithMenu::DoneBuildingItemList()
{
	// sort by app name
	fSupportingAppList->SortItems(SortByRelationAndName, this);

	// check if each app is unique
	bool unique = true;
	int32 count = fSupportingAppList->CountItems();
	for (int32 index = 0; index < count - 1; index++) {
		// the list is sorted, just compare two adjacent models
		if (strcmp(fSupportingAppList->ItemAt(index)->fModel->Name(),
			fSupportingAppList->ItemAt(index + 1)->fModel->Name()) == 0) {
			unique = false;
			break;
		}
	}

	// add apps as menu items
	BFont font;
	GetFont(&font);

	int32 lastRelation = -1;
	for (int32 index = 0; index < count ; index++) {
		RelationCachingModelProxy *modelProxy = fSupportingAppList->ItemAt(index);
		Model *model = modelProxy->fModel;
		BMessage *message = new BMessage(fEntriesToOpen);
		message->AddRef("handler", model->EntryRef());
		BContainerWindow *window = dynamic_cast<BContainerWindow *>(fParentWindow);
		if (window)
			message->AddData("nodeRefsToClose", B_RAW_TYPE, window->TargetModel()->NodeRef(),
				sizeof (node_ref));

		BString result;
		if (unique) {
			// just use the app name
			result = model->Name();
		} else {
			// get a truncated full path
			BPath path;
			BEntry entry(model->EntryRef());
			if (entry.GetPath(&path) != B_OK) {
				PRINT(("stale entry ref %s\n", model->Name()));
				delete message;
				continue;
			}
			result = path.Path();
			font.TruncateString(&result, B_TRUNCATE_MIDDLE, kMaxMenuWidth);
		}
#if DEBUG
		BString relationDescription;
		fIterator->RelationDescription(&fEntriesToOpen, model, &relationDescription);
		result += " (";
		result += relationDescription;
		result += ")";
#endif

		// divide different relations of opening with a separator
		int32 relation = modelProxy->Relation(fIterator, &fEntriesToOpen);
		if (lastRelation != -1 && relation != lastRelation)
			AddSeparatorItem();
		lastRelation = relation;

		ModelMenuItem *item = new ModelMenuItem(model, result.String(), message);
		AddItem(item);
		// mark item if it represents the preferred app
		if (fHaveCommonPreferredApp && *(model->EntryRef()) == fPreferredRef) {
			//PRINT(("marking item for % as preferred", model->Name()));
			item->SetMarked(true);
		}
	}

	// target the menu
	if (target)
		SetTargetForItems(target);
	else
		SetTargetForItems(fMessenger);

	if (!CountItems()) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("no supporting apps"), 0);
		item->SetEnabled(false);
		AddItem(item);
	}
}


void
OpenWithMenu::ClearMenuBuildingState()
{
	delete fIterator;
	fIterator = NULL;
	delete fSupportingAppList;
	fSupportingAppList = NULL;
}


//	#pragma mark -


SearchForSignatureEntryList::SearchForSignatureEntryList(bool canAddAllApps)
	:
	fIteratorList(NULL),
	fSignatures(20, true),
	fPreferredAppCount(0),
	fPreferredAppForFileCount(0),
	fGenericFilesOnly(true),
	fCanAddAllApps(canAddAllApps),
	fFoundOneNonSuperHandler(false)
{
}


SearchForSignatureEntryList::~SearchForSignatureEntryList()
{
	delete fIteratorList;
}


void
SearchForSignatureEntryList::PushUniqueSignature(const char *str)
{
	// do a unique add
	if (fSignatures.EachElement(FindOne, (void *)str))
		return;

	fSignatures.AddItem(new BString(str));
}


status_t
SearchForSignatureEntryList::GetNextEntry(BEntry *entry, bool)
{
	return fIteratorList->GetNextEntry(entry);
}


status_t
SearchForSignatureEntryList::GetNextRef(entry_ref *ref)
{
	return fIteratorList->GetNextRef(ref);
}


int32
SearchForSignatureEntryList::GetNextDirents(struct dirent *buffer,
	size_t length, int32 count)
{
	return fIteratorList->GetNextDirents(buffer, length, count);
}


struct AddOneTermParams {
	BString *result;
	bool first;
};

static const BString *
AddOnePredicateTerm(const BString *item, void *castToParams)
{
	AddOneTermParams *params = (AddOneTermParams *)castToParams;
	if (!params->first)
		(*params->result) << " || ";
	(*params->result) << kAttrAppSignature << " = " << item->String();

	params->first = false;

	return 0;
}


status_t
SearchForSignatureEntryList::Rewind()
{
	if (fIteratorList)
		return fIteratorList->Rewind();

	if (!fSignatures.CountItems())
		return ENOENT;

	// build up the iterator
	fIteratorList = new CachedEntryIteratorList(false);
		// We cannot sort the cached inodes, as CanOpenWithFilter() relies
		// on the fact that ConditionalAllAppsIterator results come last.

	// build the predicate string by oring queries for the individual
	// signatures
	BString predicateString;

	AddOneTermParams params;
	params.result = &predicateString;
	params.first = true;

	fSignatures.EachElement(AddOnePredicateTerm, &params);

	ASSERT(predicateString.Length());
// 	PRINT(("query predicate %s\n", predicateString.String()));
	fIteratorList->AddItem(new TWalkerWrapper(
		new BTrackerPrivate::TQueryWalker(predicateString.String())));
	fIteratorList->AddItem(new ConditionalAllAppsIterator(this));

	return fIteratorList->Rewind();
}


int32
SearchForSignatureEntryList::CountEntries()
{
	return 0;
}


bool
SearchForSignatureEntryList::GetPreferredApp(entry_ref *ref) const
{
	if (fPreferredAppCount == 1)
		*ref = fPreferredRef;

	return fPreferredAppCount == 1;
}


void
SearchForSignatureEntryList::TrySettingPreferredApp(const entry_ref *ref)
{
	if (!fPreferredAppCount) {
		fPreferredRef = *ref;
		fPreferredAppCount++;
	} else if (fPreferredRef != *ref)
		// if more than one, will not return any
		fPreferredAppCount++;
}


void
SearchForSignatureEntryList::TrySettingPreferredAppForFile(const entry_ref *ref)
{
	if (!fPreferredAppForFileCount) {
		fPreferredRefForFile = *ref;
		fPreferredAppForFileCount++;
	} else if (fPreferredRefForFile != *ref) {
		// if more than one, will not return any
		fPreferredAppForFileCount++;
	}
}


void
SearchForSignatureEntryList::NonGenericFileFound()
{
	fGenericFilesOnly = false;
}


bool
SearchForSignatureEntryList::GenericFilesOnly() const
{
	return fGenericFilesOnly;
}


bool
SearchForSignatureEntryList::ShowAllApplications() const
{
	return fCanAddAllApps && !fFoundOneNonSuperHandler;
}


int32
SearchForSignatureEntryList::Relation(const Model *nodeModel,
	const Model *applicationModel)
{
 	switch (applicationModel->SupportsMimeType(nodeModel->MimeType(), 0, true)) {
		case kDoesNotSupportType:
			return kNoRelation;

		case kSuperhandlerModel:
			return kSuperhandler;

		case kModelSupportsSupertype:
			return kSupportsSupertype;

		case kModelSupportsType:
			return kSupportsType;
	}

	TRESPASS();
	return kNoRelation;
}


int32
SearchForSignatureEntryList::Relation(const BMessage *entriesToOpen,
	const Model *model) const
{
	return Relation(entriesToOpen, model,
		fPreferredAppCount == 1 ? &fPreferredRef : 0,
		fPreferredAppForFileCount == 1 ? &fPreferredRefForFile : 0);
}


void
SearchForSignatureEntryList::RelationDescription(const BMessage *entriesToOpen,
	const Model *model, BString *description) const
{
	RelationDescription(entriesToOpen, model, description,
		fPreferredAppCount == 1 ? &fPreferredRef : 0,
		fPreferredAppForFileCount == 1 ? &fPreferredRefForFile : 0);
}


int32
SearchForSignatureEntryList::Relation(const BMessage *entriesToOpen,
	const Model *applicationModel, const entry_ref *preferredApp,
	const entry_ref *preferredAppForFile)
{
	for (int32 index = 0; ; index++) {
		entry_ref ref;
		if (entriesToOpen->FindRef("refs", index, &ref) != B_OK)
			break;

		// need to init a model so that typeless folders etc. will still appear to
		// have a mime type

		Model model(&ref, true, true);
		if (model.InitCheck())
			continue;

		int32 result = Relation(&model, applicationModel);
		if (result != kNoRelation) {
			if (preferredAppForFile
				&& *applicationModel->EntryRef() == *preferredAppForFile)
				return kPreferredForFile;

			if (result == kSupportsType && preferredApp
				&& *applicationModel->EntryRef() == *preferredApp)
				// application matches cached preferred app, we are done
				return kPreferredForType;

			return result;
		}
	}

	return kNoRelation;
}


void
SearchForSignatureEntryList::RelationDescription(const BMessage *entriesToOpen,
	const Model *applicationModel, BString *description, const entry_ref *preferredApp,
	const entry_ref *preferredAppForFile)
{
	for (int32 index = 0; ;index++) {
		entry_ref ref;
		if (entriesToOpen->FindRef("refs", index, &ref) != B_OK)
			break;

		if (preferredAppForFile && ref == *preferredAppForFile) {
			description->SetTo(B_TRANSLATE("Preferred for file"));
			return;
		}

		Model model(&ref, true, true);
		if (model.InitCheck())
			continue;

		BMimeType mimeType;
		int32 result = Relation(&model, applicationModel);
		switch (result) {
			case kDoesNotSupportType:
				continue;

			case kSuperhandler:
				description->SetTo(B_TRANSLATE("Handles any file"));
				return;

			case kSupportsSupertype:
			{
				mimeType.SetTo(model.MimeType());
				// status_t result = mimeType.GetSupertype(&mimeType);

				char *type = (char *)mimeType.Type();
				char *tmp = strchr(type, '/');
				if (tmp)
					*tmp = '\0';

				//PRINT(("getting supertype for %s, result %s, got %s\n",
				//	model.MimeType(), strerror(result), mimeType.Type()));
				description->SetTo(B_TRANSLATE("Handles any %type"));
				// *description += mimeType.Type();
				description->ReplaceFirst("%type", type);
				return;
			}

			case kSupportsType:
			{
				mimeType.SetTo(model.MimeType());

				if (preferredApp && *applicationModel->EntryRef() == *preferredApp)
					// application matches cached preferred app, we are done
					description->SetTo(B_TRANSLATE("Preferred for %type"));
				else
					description->SetTo(B_TRANSLATE("Handles %type"));

				char shortDescription[256];
				if (mimeType.GetShortDescription(shortDescription) == B_OK)
					description->ReplaceFirst("%type", shortDescription);
				else
					description->ReplaceFirst("%type", mimeType.Type());
				return;
			}
		}
	}

	description->SetTo(B_TRANSLATE("Does not handle file"));
}


bool
SearchForSignatureEntryList::CanOpenWithFilter(const Model *appModel,
	const BMessage *entriesToOpen, const entry_ref *preferredApp)
{
	if (!appModel->IsExecutable() || !appModel->Node()) {
		// weed out non-executable
#if xDEBUG
		BPath path;
		BEntry entry(appModel->EntryRef());
		entry.GetPath(&path);
		PRINT(("filtering out %s- not executable \n", path.Path()));
#endif
		return false;
	}

	if (strcasecmp(appModel->MimeType(), B_APP_MIME_TYPE) != 0) {
		// filter out pe containers on PPC etc.
		return false;
	}

	ASSERT(dynamic_cast<BFile *>(appModel->Node()));
	char signature[B_MIME_TYPE_LENGTH];
	status_t result = GetAppSignatureFromAttr(
		dynamic_cast<BFile *>(appModel->Node()), signature);

	if (result == B_OK && strcasecmp(signature, kTrackerSignature) == 0) {
		// special case the Tracker - make sure only the running copy is
		// in the list
		app_info trackerInfo;
		result = be_roster->GetActiveAppInfo(&trackerInfo);
		if (*appModel->EntryRef() != trackerInfo.ref) {
			// this is an inactive copy of the Tracker, remove it

#if xDEBUG
			BPath path, path2;
			BEntry entry(appModel->EntryRef());
			entry.GetPath(&path);

			BEntry entry2(&trackerInfo.ref);
			entry2.GetPath(&path2);

			PRINT(("filtering out %s, sig %s, active Tracker at %s, result %s, refName %s\n",
				path.Path(), signature, path2.Path(), strerror(result),
				trackerInfo.ref.name));
#endif
			return false;
		}
	}

	if (FSInTrashDir(appModel->EntryRef()))
		return false;

	if (ShowAllApplications()) {
		// don't check for these if we didn't look for every single app
		// to not slow filtering down
		uint32 flags;
		BAppFileInfo appFileInfo(dynamic_cast<BFile *>(appModel->Node()));
		if (appFileInfo.GetAppFlags(&flags) != B_OK)
			return false;

		if ((flags & B_BACKGROUND_APP) || (flags & B_ARGV_ONLY))
			return false;

		if (!signature[0])
			// weed out apps with empty signatures
			return false;
	}

	int32 relation = Relation(entriesToOpen, appModel, preferredApp, 0);
	if (relation == kNoRelation && !ShowAllApplications()) {
#if xDEBUG
		BPath path;
		BEntry entry(appModel->EntryRef());
		entry.GetPath(&path);

		PRINT(("filtering out %s, does not handle any of opened files\n",
			path.Path()));
#endif
		return false;
	}

	if (relation != kNoRelation && relation != kSuperhandler && !fGenericFilesOnly) {
		// we hit at least one app that is not a superhandler and
		// handles the document
		fFoundOneNonSuperHandler = true;
	}

	return true;
}


//	#pragma mark -


ConditionalAllAppsIterator::ConditionalAllAppsIterator(
		SearchForSignatureEntryList *parent)
	:
	fParent(parent),
	fWalker(NULL)
{
}


void
ConditionalAllAppsIterator::Instantiate()
{
	if (fWalker)
		return;

	BString lookForAppsPredicate;
	lookForAppsPredicate << "(" << kAttrAppSignature << " = \"*\" ) && ( "
		<< kAttrMIMEType << " = " << B_APP_MIME_TYPE << " ) ";
	fWalker = new BTrackerPrivate::TQueryWalker(lookForAppsPredicate.String());
}


ConditionalAllAppsIterator::~ConditionalAllAppsIterator()
{
	delete fWalker;
}


status_t
ConditionalAllAppsIterator::GetNextEntry(BEntry *entry, bool traverse)
{
	if (!Iterate())
		return B_ENTRY_NOT_FOUND;

	Instantiate();
	return fWalker->GetNextEntry(entry, traverse);
}


status_t
ConditionalAllAppsIterator::GetNextRef(entry_ref *ref)
{
	if (!Iterate())
		return B_ENTRY_NOT_FOUND;

	Instantiate();
	return fWalker->GetNextRef(ref);
}


int32
ConditionalAllAppsIterator::GetNextDirents(struct dirent *buffer, size_t length,
	int32 count)
{
	if (!Iterate())
		return 0;

	Instantiate();
	return fWalker->GetNextDirents(buffer, length, count);
}


status_t
ConditionalAllAppsIterator::Rewind()
{
	if (!Iterate())
		return B_OK;

	Instantiate();
	return fWalker->Rewind();
}


int32
ConditionalAllAppsIterator::CountEntries()
{
	if (!Iterate())
		return 0;

	Instantiate();
	return fWalker->CountEntries();
}


bool
ConditionalAllAppsIterator::Iterate() const
{
	return fParent->ShowAllApplications();
}

