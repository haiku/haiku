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
#include <GroupView.h>
#include <LayoutBuilder.h>
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
		EditorTabView(const char *name,
			button_width width = B_WIDTH_FROM_WIDEST,
			uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS);

		void AddRawEditorTab(BView *view);
		void SetTypeEditorTab(BView *view);
};


// -----------------


EditorTabView::EditorTabView(const char *name, button_width width, uint32 flags)
	: BTabView(name, width, flags)
{
	SetBorder(B_NO_BORDER);
}


void
EditorTabView::AddRawEditorTab(BView *view)
{
	BTab* tab = new BTab(view);
	tab->SetLabel(B_TRANSLATE("Raw editor"));
	AddTab(view, tab);
}


void
EditorTabView::SetTypeEditorTab(BView *view)
{
	if (view == NULL) {
		view = new BStringView(B_TRANSLATE("Type editor"),
			B_TRANSLATE("No type editor available"));
		((BStringView*)view)->SetAlignment(B_ALIGN_CENTER);
	}

	BGroupView* group = new BGroupView(B_VERTICAL);
		BLayoutBuilder::Group<>(group, B_VERTICAL)
		.SetInsets(B_USE_WINDOW_SPACING, 0, B_USE_WINDOW_SPACING, 0)
		.Add(view)
		.AddGlue(25.0f);

	group->SetName(view->Name());

	AddTab(group);
	Select(0);
}


//	#pragma mark -


AttributeWindow::AttributeWindow(BRect _rect, entry_ref *ref,
	const char *attribute, const BMessage *settings)
	: ProbeWindow(_rect, ref),
	fAttribute(strdup(attribute))
{
	// Set alternative window title for devices
	char name[B_FILE_NAME_LENGTH];
	strlcpy(name, ref->name, sizeof(name));

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

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);

	// add the menu

	BMenuBar *menuBar = new BMenuBar("");
	layout->AddView(menuBar, 0);

	BMenu *menu = new BMenu(B_TRANSLATE("Attribute"));

	// the ProbeView save menu items will be inserted here
	menu->AddItem(new BMenuItem(B_TRANSLATE("Remove from file"),
		new BMessage(kMsgRemoveAttribute)));
	menu->AddSeparatorItem();

	// the ProbeView print menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_CLOSE_REQUESTED), 'W', B_COMMAND_KEY));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// add our interface widgets

	EditorTabView *tabView = new EditorTabView("tabView");
	layout->AddView(tabView, 999);

	BRect rect = tabView->ContainerView()->Bounds();
	rect.top += 3;

	fProbeView = new ProbeView(ref, attribute, settings);
	fProbeView->AddSaveMenuItems(menu, 0);
	fProbeView->AddPrintMenuItems(menu, menu->CountItems() - 2);

	fTypeEditorView = GetTypeEditorFor(rect, fProbeView->Editor());

	tabView->SetTypeEditorTab(fTypeEditorView);
	tabView->AddRawEditorTab(fProbeView);

	if (fTypeEditorView == NULL) {
		// show the raw editor if we don't have a specialised type editor
		tabView->Select(1);
	}
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
				B_TRANSLATE("Do you really want to remove the attribute \"%s\" "
				"from the file \"%s\"?\n\nYou cannot undo this action."),
				fAttribute, Ref().name);

			BAlert* alert = new BAlert(B_TRANSLATE("DiskProbe request"),
				buffer, B_TRANSLATE("Cancel"), B_TRANSLATE("Remove"), NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			int32 chosen = alert->Go();

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

