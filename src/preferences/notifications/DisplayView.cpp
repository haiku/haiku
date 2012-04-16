/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2009, Pier Luigi Fiorini.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Catalog.h>
#include <Directory.h>
#include <Message.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GridLayoutBuilder.h>
#include <SpaceLayoutItem.h>
#include <TextControl.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Mime.h>
#include <Node.h>
#include <notification/Notifications.h>
#include <Path.h>

#include "DisplayView.h"
#include "SettingsHost.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "DisplayView"


DisplayView::DisplayView(SettingsHost* host)
	:
	SettingsPane("display", host)
{
	// Window width
	fWindowWidth = new BTextControl(B_TRANSLATE("Window width:"), NULL,
		new BMessage(kSettingChanged));

	// Icon size
	fIconSize = new BMenu("iconSize");
	fIconSize->AddItem(new BMenuItem(B_TRANSLATE("Mini icon"),
		new BMessage(kSettingChanged)));
	fIconSize->AddItem(new BMenuItem(B_TRANSLATE("Large icon"),
		new BMessage(kSettingChanged)));
	fIconSize->SetLabelFromMarked(true);
	fIconSizeField = new BMenuField(B_TRANSLATE("Icon size:"), fIconSize);

	// Load settings
	Load();

	// Calculate inset
	float inset = ceilf(be_plain_font->Size() * 0.7f);

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGridLayoutBuilder(inset, inset)
		.Add(fWindowWidth->CreateLabelLayoutItem(), 0, 0)
		.Add(fWindowWidth->CreateTextViewLayoutItem(), 1, 0)
		.Add(fIconSizeField->CreateLabelLayoutItem(), 0, 1)
		.Add(fIconSizeField->CreateMenuBarLayoutItem(), 1, 1)
		.Add(BSpaceLayoutItem::CreateGlue(), 0, 2, 2, 1)
	);
}


void
DisplayView::AttachedToWindow()
{
	fWindowWidth->SetTarget(this);
	fIconSize->SetTargetForItems(this);
}


void
DisplayView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kSettingChanged:
			SettingsPane::MessageReceived(msg);
			break;
		default:
			BView::MessageReceived(msg);
	}
}


status_t
DisplayView::Load()
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append(kSettingsDirectory);

	if (create_directory(path.Path(), 0755) != B_OK) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("There was a problem saving the preferences.\n"
				"It's possible you don't have write access to the "
				"settings directory."), B_TRANSLATE("OK"), NULL, NULL,
			B_WIDTH_AS_USUAL, B_STOP_ALERT);
		(void)alert->Go();
	}

	path.Append(kDisplaySettings);

	BFile file(path.Path(), B_READ_ONLY);
	BMessage settings;
	settings.Unflatten(&file);

	char buffer[255];
	int32 setting;
	BMenuItem* item = NULL;

	float width;
	if (settings.FindFloat(kWidthName, &width) != B_OK)
		width = kDefaultWidth;
	(void)sprintf(buffer, "%.2f", width);
	fWindowWidth->SetText(buffer);

	icon_size iconSize;
	if (settings.FindInt32(kIconSizeName, &setting) != B_OK)
		iconSize = kDefaultIconSize;
	else
		iconSize = (icon_size)setting;
	if (iconSize == B_MINI_ICON)
		item = fIconSize->ItemAt(0);
	else
		item = fIconSize->ItemAt(1);
	if (item)
		item->SetMarked(true);

	return B_OK;
}


status_t
DisplayView::Save()
{
	BPath path;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append(kSettingsDirectory);
	path.Append(kDisplaySettings);

	BMessage settings;

	float width = atof(fWindowWidth->Text());
	settings.AddFloat(kWidthName, width);

	icon_size iconSize = kDefaultIconSize;
	switch (fIconSize->IndexOf(fIconSize->FindMarked())) {
		case 0:
			iconSize = B_MINI_ICON;
			break;
		default:
			iconSize = B_LARGE_ICON;
	}
	settings.AddInt32(kIconSizeName, (int32)iconSize);

	// Save settings file
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t ret = settings.Flatten(&file);
	if (ret != B_OK) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("Can't save preferenes, you probably don't have "
				"write access to the settings directory or the disk is full."),
				B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		(void)alert->Go();
		return ret;
	}

	return B_OK;
}


status_t
DisplayView::Revert()
{
	return B_ERROR;
}
