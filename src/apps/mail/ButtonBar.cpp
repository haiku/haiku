/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2001, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

BeMail(TM), Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include "ButtonBar.h"

#include <stdlib.h>
#include <math.h>

#include <ControlLook.h>


struct BBDivider {
	float 		where;
	float		vmargin;
	BmapButton* button;
};

static const int32 kDividerBlockSize = 8;


ButtonBar::ButtonBar(BRect frame, const char* name, uint8 enabledOffset,
		uint8 disabledOffset, uint8 rollOffset, uint8 pressedOffset,
		float Hmargin, float Vmargin, uint32 resizeMask, int32 flags)
	: BView(frame, name, resizeMask, flags),
	fMaxHeight(0),
	fMaxWidth(0),
	fNextXOffset(Hmargin),
	fHMargin(Hmargin), 
	fVMargin(Vmargin),
	fEnabledOffset(enabledOffset), 
	fDisabledOffset(disabledOffset), 
	fRollOffset(rollOffset), 
	fPressedOffset(pressedOffset), 
	fDividerArray(NULL),
	fDividers(0),
	fShowLabels(true)
{
}


ButtonBar::~ButtonBar()
{
	free(fDividerArray);
}
						

BmapButton*
ButtonBar::AddButton(const char *label, int32 baseID, BMessage *msg,
	int32 position)
{
	BmapButton* button = new BmapButton(BRect(0, 0, 31, 31), label, label,
		baseID + fEnabledOffset, baseID + fDisabledOffset, baseID + fRollOffset,
		baseID + fPressedOffset, fShowLabels, msg,
		B_FOLLOW_LEFT | B_FOLLOW_TOP);

	if (position > 0)
		fButtonList.AddItem(button, position);
	else
		fButtonList.AddItem(button);
	AddChild(button);
	return button;
}


bool
ButtonBar::RemoveButton(BmapButton *button)
{
	if (fButtonList.RemoveItem(button)) {
		RemoveChild(button);
		delete button;
		return true;
	}
	return false;
}


int32
ButtonBar::IndexOf(BmapButton *button)
{
	return fButtonList.IndexOf(button);
}


void
ButtonBar::Arrange(bool fixedWidth)
{
	// Reset Positioning Info
	fNextXOffset = fHMargin;
	fMaxHeight = 0;
	fMaxWidth = 0;

	int32 i;
	float width, height;
	BmapButton *button;

	// Determine Largest button dimensions
	for (i = 0; (button = (BmapButton*)fButtonList.ItemAt(i)) != NULL; i++) {
		button->GetPreferredSize(&width, &height);
		if (height > fMaxHeight)
			fMaxHeight = height;
		if (width > fMaxWidth)
			fMaxWidth = width;
	}

	// Arrange buttons
	for (i = 0; (button = (BmapButton *)fButtonList.ItemAt(i)) != NULL; i++) {
		button->MoveTo(fNextXOffset, fVMargin);
		if (fixedWidth) {
			button->ResizeTo(fMaxWidth, fMaxHeight);
			fNextXOffset += fMaxWidth + fHMargin;
		} else {
			button->GetPreferredSize(&width, &height);
			button->ResizeTo(width, fMaxHeight);
			fNextXOffset += width + fHMargin;
		}
	}

	// Move dividers to match
	for (i = 0; i < fDividers; i++) {
		if (fDividerArray[i].button) {
			fDividerArray[i].where = fDividerArray[i].button->Frame().right
				+ floor(fHMargin/2);
		} else
			fDividerArray[i].where = floor(fHMargin / 2);
	}
}


void
ButtonBar::GetPreferredSize(float* width, float* height)
{
	*width = fNextXOffset + fHMargin;
	*height = fMaxHeight + (2 * fVMargin) + 3;
}


void
ButtonBar::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
ButtonBar::Draw(BRect updateRect)
{
	rgb_color high = tint_color(ViewColor(), 1.1);
	rgb_color low = tint_color(ViewColor(), 0.8);
	BRect bounds = Bounds();

	BeginLineArray(fDividers * 2);

	for (int32 i = 0; i < fDividers; i++) {
		float where = fDividerArray[i].where;
		float vmargin = fDividerArray[i].vmargin;

		AddLine(BPoint(where, fVMargin + vmargin),
			BPoint(where, bounds.bottom - fVMargin - vmargin), high);
		AddLine(BPoint(where + 1, fVMargin + vmargin),
			BPoint(where + 1, bounds.bottom - fVMargin - vmargin), low);
	}
	EndLineArray();

	be_control_look->DrawBorder(this, bounds, updateRect, ViewColor(),
		B_FANCY_BORDER, 0, BControlLook::B_BOTTOM_BORDER);
}


void
ButtonBar::AddDivider(float vmargin)
{
	// Do we need to allocate memory?
	if (fDividers == 0) {
		fDividerArray = (BBDivider*)malloc(sizeof(BBDivider)
			* kDividerBlockSize);
	}
	if ((fDividers % kDividerBlockSize) == 0) {
		fDividerArray = (BBDivider*)realloc(fDividerArray,
			sizeof(BBDivider) * kDividerBlockSize
			* ((fDividers/kDividerBlockSize) + 1));
	}

	// Cache the location and the button which proceeds it
	// The button is stored because we may later wish to change the layout
	fDividerArray[fDividers].vmargin = vmargin;
	fDividerArray[fDividers].where = fNextXOffset + floorf(fHMargin / 2);
	fDividerArray[fDividers].button = (BmapButton*)fButtonList.ItemAt(
		fButtonList.CountItems() - 1);
	fDividers++;
}


void
ButtonBar::ShowLabels(bool show)
{
	BmapButton* button;

	// Set show label flags on buttons
	for (int32 i = 0; (button = (BmapButton*)fButtonList.ItemAt(i)) != NULL;
			i++) {
		button->ShowLabel(show);
	}
	fShowLabels = show;
}

