/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <GroupView.h>
#include <LayoutItem.h>
#include <StringView.h>

#include "StringView.h"


StringView::StringView(const char* label, const char* text)
	:
	fView(NULL),
	fLabel(new BStringView(NULL, label)),
	fLabelItem(NULL),
	fText(new BStringView(NULL, text)),
	fTextItem(NULL)
{
}


void
StringView::SetLabel(const char* label)
{
	fLabel->SetText(label);
}


void
StringView::SetText(const char* text)
{
	fText->SetText(text);
}


BLayoutItem*
StringView::GetLabelLayoutItem()
{
	return fLabelItem;
}


BView*
StringView::LabelView()
{ return fLabel; }


BLayoutItem*
StringView::GetTextLayoutItem()
{
	return fTextItem;
}


BView*
StringView::TextView()
{ return fText; }


void
StringView::SetEnabled(bool enabled)
{
	
	rgb_color color;

	if (!enabled) {
		color = tint_color(
			ui_color(B_PANEL_BACKGROUND_COLOR), B_DISABLED_LABEL_TINT);
	} else
		color = ui_color(B_CONTROL_TEXT_COLOR);

	fLabel->SetHighColor(color);
	fText->SetHighColor(color);
	fLabel->Invalidate();
	fText->Invalidate();
}

	
//cast operator BView*
StringView::operator BView*()
{
	if (fView)
		return fView;
	fView = new BGroupView(B_HORIZONTAL);
	BLayout* layout = fView->GroupLayout();
	fLabelItem = layout->AddView(fLabel);
	fTextItem = layout->AddView(fText);
	return fView;
}


const char*
StringView::Label() const
{
	return fLabel->Text();
}


const char*
StringView::Text() const
{
	return fText->Text();
}

