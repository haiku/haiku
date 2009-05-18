/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include "HVIFView.h"
#include "HVIFTranslator.h"

#include <String.h>
#include <StringView.h>

#include <stdio.h>

#define HVIF_SETTING_RENDER_SIZE_CHANGED	'rsch'


HVIFView::HVIFView(const BRect &frame, const char *name, uint32 resizeMode,
	uint32 flags, TranslatorSettings *settings)
	:	BView(frame, name, resizeMode, flags),
		fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	float height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	BRect rect(10, 10, 200, 10 + height);
	BStringView *stringView = new BStringView(rect, "title",
		"Native Haiku Icon Format Translator");
	stringView->SetFont(be_bold_font);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect.OffsetBy(0, height + 10);
	char version[256];
	snprintf(version, sizeof(version), "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(HVIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(HVIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(HVIF_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView(rect, "version", version);
	stringView->ResizeToPreferred();
	AddChild(stringView);

	GetFontHeight(&fontHeight);
	height = fontHeight.descent + fontHeight.ascent + fontHeight.leading;

	rect.OffsetBy(0, height + 5);
	stringView = new BStringView(rect, "copyright",
		B_UTF8_COPYRIGHT"2009 Haiku Inc.");
	stringView->ResizeToPreferred();
	AddChild(stringView);

	rect.OffsetBy(0, height + 5);
	int32 renderSize = fSettings->SetGetInt32(HVIF_SETTING_RENDER_SIZE);
	BString label = "Render Size: ";
	label << renderSize;
	fRenderSize = new BSlider(rect, "renderSize", label.String(), NULL, 1, 32);

	fRenderSize->ResizeToPreferred();
	fRenderSize->SetValue(renderSize / 8);
	fRenderSize->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRenderSize->SetHashMarkCount(16);
	fRenderSize->SetModificationMessage(
		new BMessage(HVIF_SETTING_RENDER_SIZE_CHANGED));
	AddChild(fRenderSize);
}


void
HVIFView::AttachedToWindow()
{
	fRenderSize->SetTarget(this);
}


void
HVIFView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case HVIF_SETTING_RENDER_SIZE_CHANGED:
		{
			int32 value = fRenderSize->Value();
			if (value <= 0 || value > 32)
				break;

			value *= 8;
			fSettings->SetGetInt32(HVIF_SETTING_RENDER_SIZE, &value);
			fSettings->SaveSettings();

			BString newLabel = "Render Size: ";
			newLabel << value;
			fRenderSize->SetLabel(newLabel.String());
			return;
		}
	}

	BView::MessageReceived(message);
}
