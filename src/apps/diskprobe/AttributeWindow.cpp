/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "AttributeWindow.h"
#include "ProbeView.h"

#include <MenuBar.h>
#include <MenuItem.h>

#include <stdio.h>


AttributeWindow::AttributeWindow(BRect rect, entry_ref *ref, const char *attribute)
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

	// the ProbeView file menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W', B_COMMAND_KEY));
	menu->SetTargetForItems(this);
	menuBar->AddItem(menu);

	// add our interface widgets

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;
	ProbeView *probeView = new ProbeView(rect, ref, attribute);
	probeView->AddFileMenuItems(menu, 0);
	AddChild(probeView);

	probeView->UpdateSizeLimits();
}


AttributeWindow::~AttributeWindow()
{
	free(fAttribute);
}


bool
AttributeWindow::Contains(const entry_ref &ref, const char *attribute)
{
	return ref == Ref() && attribute != NULL && !strcmp(attribute, fAttribute);
}

