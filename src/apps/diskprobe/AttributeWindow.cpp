/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "AttributeWindow.h"
#include "ProbeView.h"

#include <MenuBar.h>
#include <MenuItem.h>
#include <Alert.h>

#include <stdio.h>


static const uint32 kMsgRemoveAttribute = 'rmat';


AttributeWindow::AttributeWindow(BRect rect, entry_ref *ref, const char *attribute,
	const BMessage *settings)
	: ProbeWindow(rect, ref),
	fAttribute(strdup(attribute))
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%s: %s", ref->name, attribute);
	SetTitle(buffer);

	// add the menu

	BMenuBar *menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu *menu = new BMenu("Attribute");

	menu->AddItem(new BMenuItem("Save", NULL, 'S', B_COMMAND_KEY));
	menu->AddItem(new BMenuItem("Remove from File", new BMessage(kMsgRemoveAttribute)));
	menu->AddSeparatorItem();

	// the ProbeView file menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W', B_COMMAND_KEY));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// add our interface widgets

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;
	ProbeView *probeView = new ProbeView(rect, ref, attribute, settings);
	probeView->AddFileMenuItems(menu, menu->CountItems() - 2);
	AddChild(probeView);

	probeView->UpdateSizeLimits();
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
				"Do you really want to remove the attribute \"%s\" from the file \"%s\"?\n\n"
				"The contents of the attribute will get lost if you click on \"Remove\".",
				fAttribute, Ref().name);

			int32 chosen = (new BAlert("DiskProbe request",
				buffer, "Remove", "Cancel", NULL,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
			if (chosen == 0) {
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
AttributeWindow::Contains(const entry_ref &ref, const char *attribute)
{
	return ref == Ref() && attribute != NULL && !strcmp(attribute, fAttribute);
}

