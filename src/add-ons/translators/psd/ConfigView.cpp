/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ConfigView.h"

#include <Catalog.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include <stdio.h>

#include "PSDLoader.h"
#include "PSDTranslator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PSDConfig"


ConfigView::ConfigView(TranslatorSettings *settings)
	: BGroupView(B_TRANSLATE("PSDTranslator Settings"), B_VERTICAL, 0)
{
	fSettings = settings;

	BPopUpMenu* compressionPopupMenu = new BPopUpMenu("popup_compression");

	uint32 currentCompression = 
		fSettings->SetGetInt32(PSD_SETTING_COMPRESSION);

	_AddItemToMenu(compressionPopupMenu, B_TRANSLATE("Uncompressed"),
		MSG_COMPRESSION_CHANGED, PSD_COMPRESSED_RAW, currentCompression);
	_AddItemToMenu(compressionPopupMenu, B_TRANSLATE("RLE"),
		MSG_COMPRESSION_CHANGED, PSD_COMPRESSED_RLE, currentCompression);

	fCompressionField = new BMenuField("compression",
		B_TRANSLATE("Compression: "), compressionPopupMenu);
	fCompressionField->SetAlignment(B_ALIGN_RIGHT);

	BPopUpMenu* versionPopupMenu = new BPopUpMenu("popup_version");

	uint32 currentVersion = 
		fSettings->SetGetInt32(PSD_SETTING_VERSION);

	_AddItemToMenu(versionPopupMenu,
		B_TRANSLATE("Photoshop Document (PSD file)"), MSG_VERSION_CHANGED,
		PSD_FILE, currentVersion);
	_AddItemToMenu(versionPopupMenu,
		B_TRANSLATE("Photoshop Big Document (PSB file)"), MSG_VERSION_CHANGED,
		PSB_FILE, currentVersion);

	fVersionField = new BMenuField("version",
		B_TRANSLATE("Format: "), versionPopupMenu);
	fVersionField->SetAlignment(B_ALIGN_RIGHT);

	BStringView *titleView = new BStringView("title",
		B_TRANSLATE("Photoshop image translator"));
	titleView->SetFont(be_bold_font);

	char version[256];
	sprintf(version, B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(PSD_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(PSD_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(PSD_TRANSLATOR_VERSION)),
		__DATE__);

	BStringView *versionView = new BStringView("version", version);
	BStringView *copyrightView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2005-2013 Haiku Inc.");
	BStringView *copyright2View = new BStringView("my_copyright",
		B_UTF8_COPYRIGHT "2012-2013 Gerasim Troeglazov <3dEyes@gmail.com>");
	
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(titleView)
		.Add(versionView)
		.Add(copyrightView)
		.AddGlue()
		.AddGrid(10.0f, 5.0f)
			.Add(fVersionField->CreateLabelLayoutItem(), 0, 0)
			.Add(fVersionField->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fCompressionField->CreateLabelLayoutItem(), 0, 1)
			.Add(fCompressionField->CreateMenuBarLayoutItem(), 1, 1)
		.End()
		.AddGlue()
		.Add(copyright2View);


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
