/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "AttributeWindow.h"
#include "ProbeView.h"
#include "TypeEditors.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
#include <TabView.h>
#include <Volume.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AttributeWindow"


static const uint32 kMsgRemoveAttribute = 'rmat';


class EditorTabView : public BTabView {
	public:
		EditorTabView(BRect frame, const char *name, 
			button_width width = B_WIDTH_AS_USUAL,
			uint32 resizingMode = B_FOLLOW_ALL, 
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);

		virtual void FrameResized(float width, float height);
		virtual void Select(int32 tab);

		void AddRawEditorTab(BView *view);
		void SetTypeEditorTab(BView *view);

	private:
		BView		*fRawEditorView;
		BView		*fTypeEditorView;
		BStringView	*fNoEditorView;
		int32		fRawTab;
};


//-----------------


EditorTabView::EditorTabView(BRect frame, const char *name, button_width width,
	uint32 resizingMode, uint32 flags)
	: BTabView(frame, name, width, resizingMode, flags),
	fRawEditorView(NULL),
	fRawTab(-1)
{
	ContainerView()->MoveBy(-ContainerView()->Frame().left,
		TabHeight() + 1 - ContainerView()->Frame().top);
	fNoEditorView = new BStringView(ContainerView()->Bounds(), 
		B_TRANSLATE("Type editor"), B_TRANSLATE("No type editor available"),
		B_FOLLOW_NONE);
	fNoEditorView->ResizeToPreferred();
	fNoEditorView->SetAlignment(B_ALIGN_CENTER);
	fTypeEditorView = fNoEditorView;

	FrameResized(0, 0);

	SetTypeEditorTab(NULL);
}


void
EditorTabView::FrameResized(float width, float height)
{
	BRect rect = Bounds();
	rect.top = ContainerView()->Frame().top;

	ContainerView()->ResizeTo(rect.Width(), rect.Height());

	BView *view = fTypeEditorView;
	if (view == NULL)
		view = fNoEditorView;

	BPoint point = view->Frame().LeftTop();
	if ((view->ResizingMode() & B_FOLLOW_RIGHT) == 0)
		point.x = (rect.Width() - view->Bounds().Width()) / 2;
	if ((view->ResizingMode() & B_FOLLOW_BOTTOM) == 0)
		point.y = (rect.Height() - view->Bounds().Height()) / 2;

	view->MoveTo(point);
}


void
EditorTabView::Select(int32 tab)
{
	if (tab != fRawTab && fRawEditorView != NULL && !fRawEditorView->IsHidden(fRawEditorView))
		fRawEditorView->Hide();

	BTabView::Select(tab);

	BView *view;
	if (tab == fRawTab && fRawEditorView != NULL) {
		if (fRawEditorView->IsHidden(fRawEditorView))
			fRawEditorView->Show();
		view = fRawEditorView;
	} else
		view = ViewForTab(Selection());

	if (view != NULL && (view->ResizingMode() & (B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM)) != 0) {
		BRect rect = ContainerView()->Bounds();

		BRect frame = view->Frame();
		rect.left = frame.left;
		rect.top = frame.top;
		if ((view->ResizingMode() & B_FOLLOW_RIGHT) == 0)
			rect.right = frame.right;
		if ((view->ResizingMode() & B_FOLLOW_BOTTOM) == 0)
			rect.bottom = frame.bottom;

		view->ResizeTo(rect.Width(), rect.Height());
	}
}


void
EditorTabView::AddRawEditorTab(BView *view)
{
	fRawEditorView = view;
	if (view != NULL)
		ContainerView()->AddChild(view);

	fRawTab = CountTabs();

	view = new BView(BRect(0, 0, 5, 5), B_TRANSLATE("Raw editor"), B_FOLLOW_NONE, 0);
	view->SetViewColor(ViewColor());
	AddTab(view);
}


void
EditorTabView::SetTypeEditorTab(BView *view)
{
	if (fTypeEditorView == view)
		return;

	BTab *tab = TabAt(0);
	if (tab != NULL)
		tab->SetView(NULL);

	fTypeEditorView = view;

	if (view == NULL)
		view = fNoEditorView;

	if (CountTabs() == 0)
		AddTab(view);
	else
		tab->SetView(view);

	FrameResized(0, 0);

#ifdef HAIKU_TARGET_PLATFORM_BEOS
	if (Window() != NULL) {
		// With R5's BTabView, calling select without being
		// attached to a window crashes...
		Select(0);
	}
#else
	Select(0);
#endif
}


//	#pragma mark -


AttributeWindow::AttributeWindow(BRect _rect, entry_ref *ref, const char *attribute,
	const BMessage *settings)
	: ProbeWindow(_rect, ref),
	fAttribute(strdup(attribute))
{
	// Set alternative window title for devices

	char name[B_FILE_NAME_LENGTH];
#ifdef HAIKU_TARGET_PLATFORM_BEOS
	strncpy(name, ref->name, sizeof(name));
	name[sizeof(name) - 1] = '\0';
#else
	strlcpy(name, ref->name, sizeof(name));
#endif

	BEntry entry(ref);
	if (entry.IsDirectory()) {
		BDirectory directory(&entry);
		if (directory.InitCheck() == B_OK && directory.IsRootDirectory()) {
			// use the volume name for root directories
			BVolume volume;
			if (directory.GetVolume(&volume) == B_OK)
				volume.GetName(name);
		}
	}
	char buffer[B_PATH_NAME_LENGTH];
	snprintf(buffer, sizeof(buffer), "%s: %s", name, attribute);
	SetTitle(buffer);

	// add the menu

	BMenuBar *menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu *menu = new BMenu(B_TRANSLATE("Attribute"));

	// the ProbeView save menu items will be inserted here
	menu->AddItem(new BMenuItem(B_TRANSLATE("Remove from file"), 
		new BMessage(kMsgRemoveAttribute)));
	menu->AddSeparatorItem();

	// the ProbeView print menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_CLOSE_REQUESTED), 
		'W', B_COMMAND_KEY));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// add our interface widgets

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;

	BView *view = new BView(rect, "main", B_FOLLOW_ALL, 0);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect = view->Bounds();
	rect.top += 3;

	EditorTabView *tabView = new EditorTabView(rect, "tabView");

	rect = tabView->ContainerView()->Bounds();
	rect.top += 3;
	fProbeView = new ProbeView(rect, ref, attribute, settings);
	tabView->AddRawEditorTab(fProbeView);

	view->AddChild(tabView);

	fTypeEditorView = GetTypeEditorFor(rect, fProbeView->Editor());
	if (fTypeEditorView != NULL)
		tabView->SetTypeEditorTab(fTypeEditorView);
	else {
		// show the raw editor if we don't have a specialised type editor
		tabView->Select(1);
	}

	fProbeView->AddSaveMenuItems(menu, 0);
	fProbeView->AddPrintMenuItems(menu, menu->CountItems() - 2);

	fProbeView->UpdateSizeLimits();
}


AttributeWindow::~AttributeWindow()
{
	free(fAttribute);
}


void
AttributeWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgRemoveAttribute:
		{
			char buffer[1024];
			snprintf(buffer, sizeof(buffer),
				B_TRANSLATE("Do you really want to remove the attribute \"%s\" from "
				"the file \"%s\"?\n\nYou cannot undo this action."),
				fAttribute, Ref().name);

			int32 chosen = (new BAlert(B_TRANSLATE("DiskProbe request"),
				buffer, B_TRANSLATE("Cancel"), B_TRANSLATE("Remove"), NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			if (chosen == 1) {
				BNode node(&Ref());
				if (node.InitCheck() == B_OK)
					node.RemoveAttr(fAttribute);

				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}

		default:
			ProbeWindow::MessageReceived(message);
			break;
	}
}


bool
AttributeWindow::QuitRequested()
{
	if (fTypeEditorView != NULL)
		fTypeEditorView->CommitChanges();

	bool quit = fProbeView->QuitRequested();
	if (!quit)
		return false;

	return ProbeWindow::QuitRequested();
}


bool
AttributeWindow::Contains(const entry_ref &ref, const char *attribute)
{
	return ref == Ref() && attribute != NULL && !strcmp(attribute, fAttribute);
}

