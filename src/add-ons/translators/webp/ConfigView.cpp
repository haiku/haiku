/*
 * Copyright 2010-2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */


#include "ConfigView.h"

#include <stdio.h>
#include <string.h>

#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <StringView.h>
#include <TextView.h>

#include "webp/encode.h"

#include "TranslatorSettings.h"
#include "WebPTranslator.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


static const uint32 kMsgQuality	= 'qlty';
static const uint32 kMsgPreset	= 'prst';
static const uint32 kMsgMethod	= 'metd';
static const uint32 kMsgPreprocessing = 'pprc';

static const struct preset_name {
	const char*	name;
	WebPPreset	id;
} kPresetNames[] = {
	{ B_TRANSLATE("Default"), 	WEBP_PRESET_DEFAULT },
	{ B_TRANSLATE("Picture"), 	WEBP_PRESET_PICTURE },
	{ B_TRANSLATE("Photo"), 	WEBP_PRESET_PHOTO },
	{ B_TRANSLATE("Drawing"), 	WEBP_PRESET_DRAWING },
	{ B_TRANSLATE("Icon"), 		WEBP_PRESET_ICON },
	{ B_TRANSLATE("Text"), 		WEBP_PRESET_TEXT },
	{ NULL },
};


ConfigView::ConfigView(TranslatorSettings* settings, uint32 flags)
	: BView(B_TRANSLATE("WebPTranslator Settings"), flags),
	fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BStringView* title = new BStringView("title",
		B_TRANSLATE("WebP image translator"));
	title->SetFont(be_bold_font);

	char versionString[256];
	sprintf(versionString, "v%d.%d.%d",
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(WEBP_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(WEBP_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			WEBP_TRANSLATOR_VERSION)));

	BStringView* version = new BStringView("version", versionString);

	BString copyrightsText;
	copyrightsText << B_TRANSLATE(B_UTF8_COPYRIGHT "2010-2011 Haiku Inc.")
		<< "\n" << B_TRANSLATE("Based on libwebp v0.1,"	)
		<< "\n" << B_TRANSLATE(B_UTF8_COPYRIGHT "2010-2011 Google Inc.");

	BTextView* copyrights = new BTextView("copyrights");
	copyrights->SetText(copyrightsText);
	copyrights->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	copyrights->MakeEditable(false);

	// output parameters

	fPresetsMenu = new BPopUpMenu(B_TRANSLATE("Preset"));
	const struct preset_name* preset = kPresetNames;
	while (preset->name != NULL) {
		BMessage* msg = new BMessage(kMsgPreset);
		msg->AddInt32("value", preset->id);

		BMenuItem* item = new BMenuItem(preset->name, msg);
		if (fSettings->SetGetInt32(WEBP_SETTING_PRESET) == preset->id)
			item->SetMarked(true);
		fPresetsMenu->AddItem(item);

		preset++;
	}
	BMenuField* presetsField = new BMenuField(B_TRANSLATE("Output preset:"),
		fPresetsMenu);

	fQualitySlider = new BSlider("quality", B_TRANSLATE("Output quality:"),
		new BMessage(kMsgQuality), 0, 100, B_HORIZONTAL, B_BLOCK_THUMB);
	fQualitySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQualitySlider->SetHashMarkCount(10);
	fQualitySlider->SetLimitLabels(B_TRANSLATE("Low"), B_TRANSLATE("High"));
	fQualitySlider->SetValue(fSettings->SetGetInt32(WEBP_SETTING_QUALITY));

	fMethodSlider = new BSlider("method", B_TRANSLATE("Compression method:"),
		new BMessage(kMsgMethod), 0, 6, B_HORIZONTAL, B_BLOCK_THUMB);
	fMethodSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fMethodSlider->SetHashMarkCount(7);
	fMethodSlider->SetLimitLabels(B_TRANSLATE("Fast"),
		B_TRANSLATE("Slower but better"));
	fMethodSlider->SetValue(fSettings->SetGetInt32(WEBP_SETTING_METHOD));

	fPreprocessingCheckBox = new BCheckBox("preprocessing",
		B_TRANSLATE("Preprocessing filter"), new BMessage(kMsgPreprocessing));
	if (fSettings->SetGetBool(WEBP_SETTING_PREPROCESSING))
		fPreprocessingCheckBox->SetValue(B_CONTROL_ON);

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(5)
		.AddGroup(B_HORIZONTAL)
			.Add(title)
			.Add(version)
			.AddGlue()
		.End()
		.Add(copyrights)
		.AddGlue()

		.AddGrid()
			.Add(presetsField->CreateLabelLayoutItem(), 0, 0)
			.Add(presetsField->CreateMenuBarLayoutItem(), 1, 0)
		.End()
		.Add(fQualitySlider)
		.Add(fMethodSlider)
		.Add(fPreprocessingCheckBox);
}


ConfigView::~ConfigView()
{
	fSettings->Release();
}


void
ConfigView::AttachedToWindow()
{
	BView::AttachedToWindow();

	fPresetsMenu->SetTargetForItems(this);

	fQualitySlider->SetTarget(this);
	fMethodSlider->SetTarget(this);
	fPreprocessingCheckBox->SetTarget(this);
}


void
ConfigView::MessageReceived(BMessage* message)
{
	struct {
		const char*		name;
		uint32			what;
		TranSettingType	type;
	} maps[] = {
		{ WEBP_SETTING_PRESET, kMsgPreset, TRAN_SETTING_INT32 },
		{ WEBP_SETTING_QUALITY, kMsgQuality, TRAN_SETTING_INT32 },
		{ WEBP_SETTING_METHOD, kMsgMethod, TRAN_SETTING_INT32 },
		{ WEBP_SETTING_PREPROCESSING, kMsgPreprocessing, TRAN_SETTING_BOOL },
		{ NULL }
	};

	int i;
	for (i = 0; maps[i].name != NULL; i++) {
		if (maps[i].what == message->what)
			break;
	}

	if (maps[i].name == NULL) {
		BView::MessageReceived(message);
		return;
	}

	int32 value;
	if (message->FindInt32("value", &value) == B_OK
		|| message->FindInt32("be:value", &value) == B_OK) {
		switch(maps[i].type) {
			case TRAN_SETTING_BOOL:
			{
				bool boolValue = value;
				fSettings->SetGetBool(maps[i].name, &boolValue);
				break;
			}
			case TRAN_SETTING_INT32:
				fSettings->SetGetInt32(maps[i].name, &value);
				break;
		}
		fSettings->SaveSettings();
	}
}
