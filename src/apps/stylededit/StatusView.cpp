/*
 * Copyright 2002-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vlad Slepukhin
 *		Siarzhuk Zharski
 */


#include "StatusView.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <ControlLook.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Window.h>

#include <tracker_private.h>
#include "DirMenu.h"

#include "Constants.h"


const float kHorzSpacing = 5.f;
#define UTF8_EXPAND_ARROW "\xe2\x96\xbe"

using namespace BPrivate;


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StatusView"


StatusView::StatusView(BScrollView* scrollView)
			:
			BView(BRect(), "statusview",
				B_FOLLOW_BOTTOM | B_FOLLOW_LEFT, B_WILL_DRAW),
			fScrollView(scrollView),
			fPreferredSize(0., 0.),
			fReadOnly(false),
			fCanUnlock(false)
{
	memset(fCellWidth, 0, sizeof(fCellWidth));
}


StatusView::~StatusView()
{
}


void
StatusView::AttachedToWindow()
{
	SetFont(be_plain_font);
	SetFontSize(10.);

	BMessage message(UPDATE_STATUS);
	message.AddInt32("line", 1);
	message.AddInt32("column", 1);
	message.AddString("encoding", "");
	SetStatus(&message);

	BScrollBar* scrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
	MoveTo(0., scrollBar->Frame().top);

	rgb_color color = B_TRANSPARENT_COLOR;
	BView* parent = Parent();
	if (parent != NULL)
		color = parent->ViewColor();

	if (color == B_TRANSPARENT_COLOR)
		color = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(color);

	ResizeToPreferred();
}


void
StatusView::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


void
StatusView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	if (Bounds().Width() > width)
		width = Bounds().Width();

	BView::ResizeTo(width, height);
}


void
StatusView::Draw(BRect updateRect)
{
	if (fPreferredSize.width <= 0)
		return;

	if (be_control_look != NULL) {
		BRect bounds(Bounds());
		be_control_look->DrawMenuBarBackground(this,
			bounds, updateRect,	ViewColor());
	}

	BRect bounds(Bounds());
	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	StrokeLine(bounds.LeftTop(), bounds.RightTop());

	float x = bounds.left;
	for (size_t i = 0; i < kStatusCellCount - 1; i++) {
		x += fCellWidth[i];
		StrokeLine(BPoint(x, bounds.top + 3), BPoint(x, bounds.bottom - 3));
	}

	SetLowColor(ViewColor());
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	x = bounds.left;
	float y = (bounds.bottom + bounds.top
		+ ceilf(fontHeight.ascent) - ceilf(fontHeight.descent)) / 2;

	for (size_t i = 0; i < kStatusCellCount; i++) {
		if (fCellText[i].Length() == 0)
			continue;
		DrawString(fCellText[i], BPoint(x + kHorzSpacing, y));
		x += fCellWidth[i];
	}
}


void
StatusView::MouseDown(BPoint where)
{
	if (where.x < fCellWidth[kPositionCell]) {
		_ShowDirMenu();
		return;
	}

	if (!fReadOnly || !fCanUnlock)
		return;

	float left = fCellWidth[kPositionCell] + fCellWidth[kEncodingCell];
	if (where.x < left)
		return;

	int32 clicks = 0;
	BMessage* message = Window()->CurrentMessage();
	if (message != NULL
		&& message->FindInt32("clicks", &clicks) == B_OK && clicks > 1)
			return;

	BPopUpMenu *menu = new BPopUpMenu(B_EMPTY_STRING, false, false);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Unlock file"),
					new BMessage(UNLOCK_FILE)));
	where.x = left;
	where.y = Bounds().bottom;

	ConvertToScreen(&where);
	menu->SetTargetForItems(this);
	menu->Go(where, true, true,	true);
}


void
StatusView::SetStatus(BMessage* message)
{
	int32 line = 0, column = 0;
	if (B_OK == message->FindInt32("line", &line)
		&& B_OK == message->FindInt32("column", &column))
	{
		char info[256];
		snprintf(info, sizeof(info),
				B_TRANSLATE("line %d, column %d"), line, column);
		fCellText[kPositionCell].SetTo(info);
	}

	if (B_OK == message->FindString("encoding", &fEncoding)) {
		// sometime corresponding Int-32 "encoding" attrib is read as string :(
		if (fEncoding.Length() == 0
			|| fEncoding.Compare("\xff\xff") == 0
			|| fEncoding.Compare("UTF-8") == 0)
		{
			// do not display default UTF-8 encoding
			fCellText[kEncodingCell].Truncate(0);
			fEncoding.Truncate(0);
		} else {
			const BCharacterSet* charset
				= BCharacterSetRoster::FindCharacterSetByName(fEncoding);
			fCellText[kEncodingCell]
				= charset != NULL ? charset->GetPrintName() : "";
		}
	}

	bool modified = false;
	fReadOnly = false;
	fCanUnlock = false;
	if (B_OK == message->FindBool("modified", &modified) && modified) {
		fCellText[kFileStateCell] = B_TRANSLATE("Modified");
	} else if (B_OK == message->FindBool("readOnly", &fReadOnly) && fReadOnly) {
		fCellText[kFileStateCell] = B_TRANSLATE("Read-only");
	    if (B_OK == message->FindBool("canUnlock", &fCanUnlock) && fCanUnlock)
			fCellText[kFileStateCell] << " " UTF8_EXPAND_ARROW;
	} else
		fCellText[kFileStateCell].Truncate(0);

	_ValidatePreferredSize();
	Invalidate();
}


void
StatusView::SetRef(const entry_ref& ref)
{
	fRef = ref;
}


void
StatusView::_ValidatePreferredSize()
{
	float orgWidth = fPreferredSize.width;
	// width
	fPreferredSize.width = 0.f;
	for (size_t i = 0; i < kStatusCellCount; i++) {
		if (fCellText[i].Length() == 0) {
			fCellWidth[i] = 0;
			continue;
		}
		float width = ceilf(StringWidth(fCellText[i]));
		if (width > 0)
			width += kHorzSpacing * 2;
		if (width > fCellWidth[i] || i != kPositionCell)
			fCellWidth[i] = width;
		fPreferredSize.width += fCellWidth[i];
	}

	// height
	font_height fontHeight;
	GetFontHeight(&fontHeight);

	fPreferredSize.height = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);

	if (fPreferredSize.height < B_H_SCROLL_BAR_HEIGHT)
		fPreferredSize.height = B_H_SCROLL_BAR_HEIGHT;

	float delta = fPreferredSize.width - orgWidth;
	ResizeBy(delta, 0);
	BScrollBar* scrollBar = fScrollView->ScrollBar(B_HORIZONTAL);
	scrollBar->ResizeBy(-delta, 0);
	scrollBar->MoveBy(delta, 0);
}


void
StatusView::_ShowDirMenu()
{
	BEntry entry;
	status_t status = entry.SetTo(&fRef);

	if (status != B_OK || !entry.Exists())
		return;

	BPrivate::BDirMenu* menu = new BDirMenu(NULL,
		BMessenger(kTrackerSignature), B_REFS_RECEIVED);

	menu->Populate(&entry, Window(), false, false, true, false, true);

	BPoint point = Bounds().LeftBottom();
	point.y += 3;
	ConvertToScreen(&point);
	BRect clickToOpenRect(Bounds());
	ConvertToScreen(&clickToOpenRect);
	menu->Go(point, true, true, clickToOpenRect);
	delete menu;
}

