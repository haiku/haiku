/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ConfigView.h"
#include "PSDTranslator.h"

#include <StringView.h>
#include <SpaceLayoutItem.h>
#include <ControlLook.h>
#include <PopUpMenu.h>
#include <MenuItem.h>

#include <stdio.h>

#include "PSDLoader.h"


ConfigView::ConfigView(TranslatorSettings *settings)
	: BGroupView("PSDTranslator Settings", B_VERTICAL, 0)
{
	fSettings = settings;

	BPopUpMenu* compressionPopupMenu = new BPopUpMenu("popup_compression");

	uint32 currentCompression = 
		fSettings->SetGetInt32(PSD_SETTING_COMPRESSION);

	_AddItemToMenu(compressionPopupMenu, "Uncompressed",
		MSG_COMPRESSION_CHANGED, PSD_COMPRESSED_RAW, currentCompression);
	_AddItemToMenu(compressionPopupMenu, "RLE",
		MSG_COMPRESSION_CHANGED, PSD_COMPRESSED_RLE, currentCompression);

	fCompressionField = new BMenuField("compression",
		"Compression: ", compressionPopupMenu);

	BPopUpMenu* versionPopupMenu = new BPopUpMenu("popup_version");

	uint32 currentVersion = 
		fSettings->SetGetInt32(PSD_SETTING_VERSION);

	_AddItemToMenu(versionPopupMenu, "Photoshop Document (PSD File)",
		MSG_VERSION_CHANGED, PSD_FILE, currentVersion);
	_AddItemToMenu(versionPopupMenu, "Photoshop Big Document (PSB File)",
		MSG_VERSION_CHANGED, PSB_FILE, currentVersion);

	fVersionField = new BMenuField("version",
		"Format: ", versionPopupMenu);

	BAlignment leftAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET);

	BStringView *stringView = new BStringView("title",
		"Photoshop image translator");
	stringView->SetFont(be_bold_font);
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	float spacing = be_control_look->DefaultItemSpacing();
	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	char version[256];
	sprintf(version, "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(PSD_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(PSD_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(PSD_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView("version", version);
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2005-2013 Haiku Inc.");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("my_copyright",
		B_UTF8_COPYRIGHT "2012-2013 Gerasim Troeglazov <3dEyes@gmail.com>");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);
	
	AddChild(fVersionField);

	AddChild(fCompressionField);

	AddChild(BSpaceLayoutItem::CreateGlue());
	GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	SetExplicitPreferredSize(GroupLayout()->MinSize());
}


ConfigView::~ConfigView()
{
	fSettings->Release();
}


void
ConfigView::AllAttached()
{
	fCompressionField->Menu()->SetTargetForItems(this);
	fVersionField->Menu()->SetTargetForItems(this);
}


void
ConfigView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_COMPRESSION_CHANGED: {
			int32 value;
			if (message->FindInt32("value", &value) >= B_OK) {
				fSettings->SetGetInt32(PSD_SETTING_COMPRESSION, &value);
				fSettings->SaveSettings();
			}
			break;
		}
		case MSG_VERSION_CHANGED: {
			int32 value;
			if (message->FindInt32("value", &value) >= B_OK) {
				fSettings->SetGetInt32(PSD_SETTING_VERSION, &value);
				fSettings->SaveSettings();
			}
			break;
		}		
		default:
			BView::MessageReceived(message);
	}
}


void
ConfigView::_AddItemToMenu(BMenu* menu, const char* label,
	uint32 mess, uint32 value, uint32 current_value)
{
	BMessage* message = new BMessage(mess);
	message->AddInt32("value", value);
	BMenuItem* item = new BMenuItem(label, message);
	item->SetMarked(value == current_value);
	menu->AddItem(item);
}
