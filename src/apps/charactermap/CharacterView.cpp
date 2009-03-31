/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "CharacterView.h"

#include <stdio.h>

#include <LayoutUtils.h>
#include <ScrollBar.h>

#include "UnicodeBlocks.h"


void
unicode_to_utf8(uint32 c, char** out)
{
	char* s = *out;

	if (c < 0x80)
		*(s++) = c;
	else if (c < 0x800) {
		*(s++) = 0xc0 | (c >> 6);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
		*(s++) = 0xe0 | (c >> 12);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c <= 0x10ffff) {
		*(s++) = 0xf0 | (c >> 18);
		*(s++) = 0x80 | ((c >> 12) & 0x3f);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	*out = s;
}


//	#pragma mark -


CharacterView::CharacterView(const char* name)
	: BView(name, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_FRAME_EVENTS),
	fShowPrivateBlocks(false),
	fShowContainedBlocksOnly(false)
{
	fTitleTops = new int32[kNumUnicodeBlocks];
	fCharacterFont.SetSize(fCharacterFont.Size() * 1.5f);
}


CharacterView::~CharacterView()
{
	delete[] fTitleTops;
}


void
CharacterView::SetCharacterFont(const BFont& font)
{
	fCharacterFont = font;

	InvalidateLayout();
}


void
CharacterView::ShowPrivateBlocks(bool show)
{
	if (fShowPrivateBlocks == show)
		return;

	fShowPrivateBlocks = show;
	InvalidateLayout();
}


void
CharacterView::ShowContainedBlocksOnly(bool show)
{
	if (fShowContainedBlocksOnly == show)
		return;

	fShowContainedBlocksOnly = show;
	InvalidateLayout();
}


bool
CharacterView::IsShowingBlock(int32 blockIndex) const
{
	if (!fShowPrivateBlocks && kUnicodeBlocks[blockIndex].private_block)
		return false;

	if (fShowContainedBlocksOnly
		&& !fCharacterFont.Blocks().Includes(
				kUnicodeBlocks[blockIndex].block)) {
		return false;
	}

	return true;
}


void
CharacterView::ScrollTo(int32 blockIndex)
{
	if (blockIndex < 0)
		blockIndex = 0;
	else if (blockIndex >= (int32)kNumUnicodeBlocks)
		blockIndex = kNumUnicodeBlocks - 1;

	BView::ScrollTo(0.0f, fTitleTops[blockIndex]);
}


void
CharacterView::AttachedToWindow()
{
}


void
CharacterView::DetachedFromWindow()
{
}


BSize
CharacterView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		BSize(fCharacterHeight, fCharacterHeight + fTitleHeight));
}


void
CharacterView::FrameResized(float width, float height)
{
	puts("hey");
	_UpdateSize();
}


void
CharacterView::MouseDown(BPoint where)
{
}


void
CharacterView::MouseUp(BPoint where)
{
}


void
CharacterView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
}


void
CharacterView::MessageReceived(BMessage* message)
{
}


void
CharacterView::Draw(BRect updateRect)
{
	const int32 kStartX = fGap / 2;

	BFont font;
	GetFont(&font);
	int32 y = 2;

	// TODO: jump to the correct start
	for (uint32 i = 0; i < kNumUnicodeBlocks; i++) {
		if (!IsShowingBlock(i))
			continue;
		if (fTitleTops[i] > updateRect.bottom)
			break;

		SetHighColor(0, 0, 0);
		DrawString(kUnicodeBlocks[i].name, BPoint(3, y + fTitleBase));

		y += fTitleHeight;
		int32 x = kStartX;
		SetFont(&fCharacterFont);

		char character[16];
		for (uint32 c = kUnicodeBlocks[i].start; c <= kUnicodeBlocks[i].end;
				c++) {
			if (y + fCharacterHeight > updateRect.top
				&& y < updateRect.bottom) {
#if 0
				// Stroke frame around the charater
				SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
				StrokeRect(BRect(x, y, x + fCharacterWidth,
					y + fCharacterHeight - fGap));

				SetHighColor(0, 0, 0);
#endif

				// Draw character
				char* end = character;
				unicode_to_utf8(c, &end);
				end[0] = '\0';

				DrawString(character,
					BPoint(x + (fCharacterWidth - StringWidth(character)) / 2,
						y + fCharacterBase));
			}

			x += fCharacterWidth + fGap;
			if (x + fCharacterWidth + fGap / 2 >= fDataRect.right) {
				y += fCharacterHeight;
				x = kStartX;
			}
		}

		if (x != kStartX)
			y += fCharacterHeight;
		y += fTitleGap;

		SetFont(&font);
	}
}


void
CharacterView::DoLayout()
{
	_UpdateSize();
}


void
CharacterView::_UpdateSize()
{
	// Compute data rect

	BRect bounds = Bounds();

	font_height fontHeight;
	GetFontHeight(&fontHeight);
	fTitleHeight = (int32)ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);
	fTitleBase = (int32)ceilf(fontHeight.ascent);

	// Find widest character
	fCharacterWidth = (int32)ceilf(fCharacterFont.StringWidth("W"));

	if (fCharacterFont.IsFullAndHalfFixed()) {
		// TODO: improve this!
		fCharacterWidth *= 2;
	}

	fCharacterFont.GetHeight(&fontHeight);
	fCharacterHeight = (int32)ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);
	fCharacterBase = (int32)ceilf(fontHeight.ascent);

	fGap = (int32)roundf(fCharacterHeight / 8.0);
	if (fGap < 3)
		fGap = 3;

	fCharacterHeight += fGap;
	fTitleGap = fGap * 3;

	fDataRect.right = bounds.Width();
	fDataRect.bottom = 0;

	int32 charactersPerLine = int32(bounds.Width() / (fGap + fCharacterWidth));
	if (charactersPerLine == 0)
		charactersPerLine = 1;

	for (uint32 i = 0; i < kNumUnicodeBlocks; i++) {
		fTitleTops[i] = (int32)ceilf(fDataRect.bottom);

		if (!IsShowingBlock(i))
			continue;

		int32 lines = (kUnicodeBlocks[i].Count() + charactersPerLine - 1)
			/ charactersPerLine;
		fDataRect.bottom += lines * fCharacterHeight + fTitleHeight + fTitleGap;
	}

	// Update scroll bars

	BScrollBar* scroller = ScrollBar(B_VERTICAL);
	if (scroller == NULL)
		return;

	if (bounds.Height() > fDataRect.Height()) {
		// no scrolling
		scroller->SetRange(0.0f, 0.0f);
		scroller->SetValue(0.0f);
	} else {
		scroller->SetRange(0.0f, fDataRect.Height() - bounds.Height() - 1.0f);
		scroller->SetProportion(bounds.Height () / fDataRect.Height());
		// scroll up if there is empty room on bottom
		if (fDataRect.Height() < bounds.bottom) {
			ScrollBy(0.0f, bounds.bottom - fDataRect.Height());
		}
	}

	Invalidate();
}
