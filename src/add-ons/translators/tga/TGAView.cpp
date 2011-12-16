/*****************************************************************************/
// TGAView
// Written by Michael Wilber, OBOS Translation Kit Team
// Use of Layout API added by Maxime Simon, maxime.simon@gmail.com, 2009.
//
// TGAView.cpp
//
// This BView based object displays information about the TGATranslator.
//
//
// Copyright (c) 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <Catalog.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <SpaceLayoutItem.h>

#include <stdio.h>
#include <string.h>

#include "TGAView.h"
#include "TGATranslator.h"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "TGAView"


TGAView::TGAView(const char *name, uint32 flags, TranslatorSettings *settings)
	:
	BView(name, flags),
	fSettings(settings)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ViewColor());

 	fTitle = new BStringView("title", B_TRANSLATE("TGA Image Translator"));
 	fTitle->SetFont(be_bold_font);

 	char detail[100];
 	sprintf(detail, B_TRANSLATE("Version %d.%d.%d %s"),
 		static_cast<int>(B_TRANSLATION_MAJOR_VERSION(TGA_TRANSLATOR_VERSION)),
 		static_cast<int>(B_TRANSLATION_MINOR_VERSION(TGA_TRANSLATOR_VERSION)),
 		static_cast<int>(B_TRANSLATION_REVISION_VERSION(
 			TGA_TRANSLATOR_VERSION)), __DATE__);
 	fDetail = new BStringView("detail", detail);
 	fWrittenBy = new BStringView("writtenby",
 		B_TRANSLATE("Written by the Haiku Translation Kit Team"));

 	fpchkIgnoreAlpha = new BCheckBox(B_TRANSLATE("Ignore TGA alpha channel"),
		new BMessage(CHANGE_IGNORE_ALPHA));
 	int32 val = (fSettings->SetGetBool(TGA_SETTING_IGNORE_ALPHA)) ? 1 : 0;
 	fpchkIgnoreAlpha->SetValue(val);
 	fpchkIgnoreAlpha->SetViewColor(ViewColor());

 	fpchkRLE = new BCheckBox(B_TRANSLATE("Save with RLE Compression"),
		new BMessage(CHANGE_RLE));
 	val = (fSettings->SetGetBool(TGA_SETTING_RLE)) ? 1 : 0;
 	fpchkRLE->SetValue(val);
 	fpchkRLE->SetViewColor(ViewColor());

 	// Build the layout
 	SetLayout(new BGroupLayout(B_HORIZONTAL));

 	AddChild(BGroupLayoutBuilder(B_VERTICAL, 7)
 		.Add(fTitle)
 		.Add(fDetail)
 		.AddGlue()
 		.Add(fpchkIgnoreAlpha)
 		.Add(fpchkRLE)
 		.AddGlue()
 		.Add(fWrittenBy)
 		.AddGlue()
 		.SetInsets(5, 5, 5, 5)
 	);

 	BFont font;
 	GetFont(&font);
 	SetExplicitPreferredSize(BSize((font.Size() * 333)/12,
 		(font.Size() * 200)/12));
}


TGAView::~TGAView()
{
	fSettings->Release();
}


void
TGAView::AllAttached()
{
	fpchkIgnoreAlpha->SetTarget(this);
	fpchkRLE->SetTarget(this);
}


void
TGAView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case CHANGE_IGNORE_ALPHA:
		{
			bool ignoreAlpha = fpchkIgnoreAlpha->Value() == B_CONTROL_ON;
			fSettings->SetGetBool(TGA_SETTING_IGNORE_ALPHA, &ignoreAlpha);
			fSettings->SaveSettings();
		}	break;

		case CHANGE_RLE:
		{
			bool saveWithRLE = fpchkRLE->Value() == B_CONTROL_ON;
			fSettings->SetGetBool(TGA_SETTING_RLE, &saveWithRLE);
			fSettings->SaveSettings();
		}	break;

		default:
			BView::MessageReceived(message);
			break;
	}
}

