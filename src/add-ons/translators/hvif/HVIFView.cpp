/*
 * Copyright 2009, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */

#include "HVIFView.h"
#include "HVIFTranslator.h"

#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <String.h>
#include <StringView.h>

#include <stdio.h>

#define HVIF_SETTING_RENDER_SIZE_CHANGED	'rsch'

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HVIFView"


HVIFView::HVIFView(const char* name, uint32 flags, TranslatorSettings *settings)
	:
	BView(name, flags, new BGroupLayout(B_VERTICAL)),
	fSettings(settings)
{
	BAlignment labelAlignment(B_ALIGN_LEFT, B_ALIGN_NO_VERTICAL);

	BStringView* title= new BStringView("title",
		B_TRANSLATE("Native Haiku icon format translator"));
	title->SetFont(be_bold_font);
	title->SetExplicitAlignment(labelAlignment);

	char versionString[256];
	snprintf(versionString, sizeof(versionString), 
		B_TRANSLATE("Version %d.%d.%d, %s"),
		int(B_TRANSLATION_MAJOR_VERSION(HVIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(HVIF_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(HVIF_TRANSLATOR_VERSION)),
		__DATE__);
	BStringView* version = new BStringView("version", versionString);
	version->SetExplicitAlignment(labelAlignment);

	BStringView* copyright = new BStringView("copyright",
		B_UTF8_COPYRIGHT"2009 Haiku Inc.");
	copyright->SetExplicitAlignment(labelAlignment);


	int32 renderSize = fSettings->SetGetInt32(HVIF_SETTING_RENDER_SIZE);
	BString label = B_TRANSLATE("Render size:");
	label << " " << renderSize;

	fRenderSize = new BSlider("renderSize", label.String(),
		NULL, 1, 32, B_HORIZONTAL);
	fRenderSize->SetValue(renderSize / 8);
	fRenderSize->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fRenderSize->SetHashMarkCount(16);
	fRenderSize->SetModificationMessage(
		new BMessage(HVIF_SETTING_RENDER_SIZE_CHANGED));
	fRenderSize->SetExplicitAlignment(labelAlignment);

	float padding = 5.0f;
	AddChild(BGroupLayoutBuilder(B_VERTICAL, padding)
		.Add(title)
		.Add(version)
		.Add(copyright)
		.Add(fRenderSize)
		.AddGlue()
		.SetInsets(padding, padding, padding, padding)
	);
	
 	BFont font;
 	GetFont(&font);
 	SetExplicitPreferredSize(
		BSize((font.Size() * 270) / 12, (font.Size() * 100) / 12));
}


HVIFView::~HVIFView()
{
	fSettings->Release();
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

			BString newLabel = B_TRANSLATE("Render size:");
			newLabel << " " << value;
			fRenderSize->SetLabel(newLabel.String());
			return;
		}
	}

	BView::MessageReceived(message);
}
