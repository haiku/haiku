/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "TermView.h"

#include <stdio.h>

#include <Layout.h>


TermView::TermView()
	:
	BView("TermView", B_WILL_DRAW)
{
	fTerm = vterm_new(kDefaultWidth, kDefaultHeight);
	vterm_parser_set_utf8(fTerm, 1);

	fTermScreen = vterm_obtain_screen(fTerm);
	vterm_screen_set_callbacks(fTermScreen, &sScreenCallbacks, this);
	vterm_screen_reset(fTermScreen, 1);

	SetFont(be_fixed_font);

	font_height height;
	GetFontHeight(&height);
	fFontHeight = height.ascent + height.descent + height.leading;
	fFontWidth = be_fixed_font->StringWidth("X");

	// TEST
	//vterm_push_bytes(fTerm,"Hello World!",11);
}


TermView::~TermView()
{
	vterm_free(fTerm);
}


void TermView::Draw(BRect updateRect)
{
	VTermRect updatedChars = PixelsToGlyphs(updateRect);
	
	VTermPos pos;
	font_height height;
	GetFontHeight(&height);
	MovePenTo(kBorderSpacing, height.ascent + kBorderSpacing);
	for (pos.row = updatedChars.start_row; pos.row < updatedChars.end_row;
			pos.row++)
	{
		for (pos.col = updatedChars.start_col;
				pos.col < updatedChars.end_col; pos.col++)
		{
			VTermScreenCell cell;
			vterm_screen_get_cell(fTermScreen, pos, &cell);

			char buffer[6];
			wcstombs(buffer, (wchar_t*)cell.chars, 6);

			DrawString(buffer);
		}
	}
}


VTermRect TermView::PixelsToGlyphs(BRect pixels) const
{
	pixels.OffsetBy(-kBorderSpacing, -kBorderSpacing);

	VTermRect rect;
	rect.start_col = (int)floor(pixels.left / fFontWidth);
	rect.end_col = (int)ceil(pixels.right / fFontWidth);
	rect.start_row = (int)floor(pixels.top / fFontHeight);
	rect.end_row = (int)ceil(pixels.bottom / fFontHeight);

#if 0
	printf("pixels:\t%d\t%d\t%d\t%d\n"
		"glyps:\t%d\t%d\t%d\t%d\n",
		(int)pixels.top, (int)pixels.bottom, (int)pixels.left, (int)pixels.right,
		rect.start_row, rect.end_row, rect.start_col, rect.end_col);
#endif
	return rect;
}


BRect TermView::GlyphsToPixels(const VTermRect& glyphs) const
{
	BRect rect;
	rect.top = glyphs.start_row * fFontHeight;
	rect.bottom = glyphs.end_row * fFontHeight;
	rect.left = glyphs.start_col * fFontWidth;
	rect.right = glyphs.end_col * fFontWidth;

	return rect;
}


BRect TermView::GlyphsToPixels(const int width, const int height) const
{
	VTermRect rect;
	rect.start_row = 0;
	rect.start_col = 0;
	rect.end_row = height;
	rect.end_col = width;
	return GlyphsToPixels(rect);
}


const VTermScreenCallbacks TermView::sScreenCallbacks = {
	/*.damage =*/ NULL,
	/*.moverect =*/ NULL,
	/*.movecursor =*/ NULL,
	/*.settermprop =*/ NULL,
	/*.setmousefunc =*/ NULL,
	/*.bell =*/ NULL,
	/*.resize =*/ NULL,
};
