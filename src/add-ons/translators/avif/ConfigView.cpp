/*
 * Copyright 2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Emmanuel Gil Peyrot
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

#include "avif/avif.h"

#include "TranslatorSettings.h"
#include "AVIFTranslator.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConfigView"


static const uint32 kMsgLossless = 'losl';
static const uint32 kMsgPixelFormat = 'pfmt';
static const uint32 kMsgQuality	= 'qlty';
static const uint32 kMsgSpeed = 'sped';
static const uint32 kMsgTilesHorizontal = 'tilh';
static const uint32 kMsgTilesVertical = 'tilv';


ConfigView::ConfigView(TranslatorSettings* settings)
	:
	BGroupView(B_TRANSLATE("AVIFTranslator Settings"), B_VERTICAL),
	fSettings(settings)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BStringView* title = new BStringView("title",
		B_TRANSLATE("AVIF image translator"));
	title->SetFont(be_bold_font);

	char versionString[256];
	sprintf(versionString, "v%d.%d.%d, %s",
		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(AVIF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_MINOR_VERSION(AVIF_TRANSLATOR_VERSION)),
		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
			AVIF_TRANSLATOR_VERSION)),
		__DATE__);

	BStringView* version = new BStringView("version", versionString);

	BString copyrightsText;
	BStringView *copyrightView = new BStringView("Copyright",
		B_TRANSLATE(B_UTF8_COPYRIGHT "2021 Emmanuel Gil Peyrot"));

	BString libavifInfo = B_TRANSLATE(
		"Based on libavif %version%");
	libavifInfo.ReplaceAll("%version%", avifVersion());

	BStringView *copyright2View = new BStringView("Copyright2",
		libavifInfo.String());
	BStringView *copyright3View = new BStringView("Copyright3",
		B_TRANSLATE(B_UTF8_COPYRIGHT "2019 Joe Drago. All rights reserved."));

	// output parameters

	fLosslessCheckBox = new BCheckBox("lossless",
		B_TRANSLATE("Lossless"), new BMessage(kMsgLossless));
	bool lossless;
	fSettings->SetGetBool(AVIF_SETTING_LOSSLESS, &lossless);
	if (lossless)
		fLosslessCheckBox->SetValue(B_CONTROL_ON);

	fPixelFormatMenu = new BPopUpMenu(B_TRANSLATE("Pixel format"));

	static const avifPixelFormat pixelFormats[4] = {
		AVIF_PIXEL_FORMAT_YUV444,
		AVIF_PIXEL_FORMAT_YUV420,
		AVIF_PIXEL_FORMAT_YUV400,
		AVIF_PIXEL_FORMAT_YUV422,
	};
	for (size_t i = 0; i < 4; ++i) {
		BMessage* msg = new BMessage(kMsgPixelFormat);
		msg->AddInt32("value", pixelFormats[i]);

		BMenuItem* item = new BMenuItem(
			avifPixelFormatToString(pixelFormats[i]), msg);
		if (fSettings->SetGetInt32(AVIF_SETTING_PIXEL_FORMAT) == pixelFormats[i])
			item->SetMarked(true);
		fPixelFormatMenu->AddItem(item);
	}

	BMenuField* pixelFormatField = new BMenuField(B_TRANSLATE("Pixel format:"),
		fPixelFormatMenu);

	rgb_color barColor = { 0, 0, 229, 255 };

	fQualitySlider = new BSlider("quality", B_TRANSLATE("Output quality:"),
		new BMessage(kMsgQuality), AVIF_QUANTIZER_BEST_QUALITY,
		AVIF_QUANTIZER_WORST_QUALITY, B_HORIZONTAL, B_BLOCK_THUMB);
	fQualitySlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fQualitySlider->SetHashMarkCount(8);
	fQualitySlider->SetLimitLabels(B_TRANSLATE("Best"), B_TRANSLATE("Worst"));
	fQualitySlider->UseFillColor(true, &barColor);
	fQualitySlider->SetValue(fSettings->SetGetInt32(AVIF_SETTING_QUALITY));
	fQualitySlider->SetEnabled(!lossless);

	fSpeedSlider = new BSlider("speed", B_TRANSLATE("Compression speed:"),
		new BMessage(kMsgSpeed), AVIF_SPEED_SLOWEST,
		AVIF_SPEED_FASTEST, B_HORIZONTAL, B_BLOCK_THUMB);
	fSpeedSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSpeedSlider->SetHashMarkCount(11);
	fSpeedSlider->SetLimitLabels(B_TRANSLATE("Slow"),
		B_TRANSLATE("Faster but worse quality"));
	fSpeedSlider->UseFillColor(true, &barColor);
	fSpeedSlider->SetValue(fSettings->SetGetInt32(AVIF_SETTING_SPEED));

	fHTilesSlider = new BSlider("htiles", B_TRANSLATE("Horizontal tiles:"),
		new BMessage(kMsgTilesHorizontal), 1, 6, B_HORIZONTAL,
		B_BLOCK_THUMB);
	fHTilesSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fHTilesSlider->SetHashMarkCount(6);
	fHTilesSlider->SetLimitLabels(B_TRANSLATE("1"),
		B_TRANSLATE("2⁶"));
	fHTilesSlider->UseFillColor(true, &barColor);
	fHTilesSlider->SetValue(
		fSettings->SetGetInt32(AVIF_SETTING_TILES_HORIZONTAL));

	fVTilesSlider = new BSlider("vtiles", B_TRANSLATE("Vertical tiles:"),
		new BMessage(kMsgTilesVertical), 1, 6, B_HORIZONTAL,
		B_BLOCK_THUMB);
	fVTilesSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fVTilesSlider->SetHashMarkCount(6);
	fVTilesSlider->SetLimitLabels(B_TRANSLATE("1"),
		B_TRANSLATE("2⁶"));
	fVTilesSlider->UseFillColor(true, &barColor);
	fVTilesSlider->SetValue(
		fSettings->SetGetInt32(AVIF_SETTING_TILES_VERTICAL));

	// Build the layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(B_USE_DEFAULT_SPACING)
		.Add(title)
		.Add(version)
		.Add(copyrightView)
		.AddGlue()
		.Add(fLosslessCheckBox)
		.AddGrid(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
		        .Add(pixelFormatField->CreateLabelLayoutItem(), 0, 0)
			.AddGroup(B_HORIZONTAL, 0.0f, 1, 0)
				.Add(pixelFormatField->CreateMenuBarLayoutItem(), 0.0f)
				.AddGlue()
				.End()
			.End()
		.Add(fQualitySlider)
		.Add(fSpeedSlider)
		.Add(fHTilesSlider)
		.Add(fVTilesSlider)
		.AddGlue()
		.Add(copyright2View)
		.Add(copyright3View);

	BFont font;
	GetFont(&font);
	SetExplicitPreferredSize(BSize((font.Size() * 250) / 12,
		(font.Size() * 350) / 12));
}


ConfigView::~ConfigView()
{
	fSettings->Release();
}


void
ConfigView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fQualitySlider->SetTarget(this);
	fSpeedSlider->SetTarget(this);
	fHTilesSlider->SetTarget(this);
	fVTilesSlider->SetTarget(this);

	if (Parent() == NULL && Window()->GetLayout() == NULL) {
		Window()->SetLayout(new BGroupLayout(B_VERTICAL));
		Window()->ResizeTo(PreferredSize().Width(),
			PreferredSize().Height());
	}
}


void
ConfigView::MessageReceived(BMessage* message)
{
	struct {
		const char*		name;
		uint32			what;
		TranSettingType	type;
	} maps[] = {
		{ AVIF_SETTING_LOSSLESS, kMsgLossless, TRAN_SETTING_BOOL },
		{ AVIF_SETTING_PIXEL_FORMAT, kMsgPixelFormat,
			TRAN_SETTING_INT32 },
		{ AVIF_SETTING_QUALITY, kMsgQuality, TRAN_SETTING_INT32 },
		{ AVIF_SETTING_SPEED, kMsgSpeed, TRAN_SETTING_INT32 },
		{ AVIF_SETTING_TILES_HORIZONTAL, kMsgTilesHorizontal,
			TRAN_SETTING_INT32 },
		{ AVIF_SETTING_TILES_VERTICAL, kMsgTilesVertical,
			TRAN_SETTING_INT32 },
		{ NULL }
	};

	int i;
	for (i = 0; maps[i].name != NULL; i++) {
		if (maps[i].what == message->what)
			break;
	}

	if (maps[i].name == NULL) {
		BGroupView::MessageReceived(message);
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
				if (maps[i].what == kMsgLossless)
					fSpeedSlider->SetEnabled(!boolValue);
				break;
			}
			case TRAN_SETTING_INT32:
				fSettings->SetGetInt32(maps[i].name, &value);
				break;
		}
		fSettings->SaveSettings();
	}
}
