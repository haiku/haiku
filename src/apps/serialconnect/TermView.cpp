/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "TermView.h"

#include <stdio.h>

#include <Layout.h>

#include "SerialApp.h"


TermView::TermView()
	:
	BView("TermView", B_WILL_DRAW)
{
	font_height height;
	GetFontHeight(&height);
	fFontHeight = height.ascent + height.descent + height.leading;
	fFontWidth = be_fixed_font->StringWidth("X");
	fTerm = vterm_new(kDefaultWidth, kDefaultHeight);

	vterm_parser_set_utf8(fTerm, 1);

	fTermScreen = vterm_obtain_screen(fTerm);
	vterm_screen_set_callbacks(fTermScreen, &sScreenCallbacks, this);
	vterm_screen_reset(fTermScreen, 1);

	SetFont(be_fixed_font);
}


TermView::~TermView()
{
	vterm_free(fTerm);
}


void TermView::AttachedToWindow()
{
	MakeFocus();
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


void TermView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = kDefaultWidth * fFontWidth;
	if (height != NULL)
		*height = kDefaultHeight * fFontHeight;
}


void TermView::KeyDown(const char* bytes, int32 numBytes)
{
	BMessage* keyEvent = new BMessage(kMsgDataWrite);
	keyEvent->AddData("data", B_RAW_TYPE, bytes, numBytes);
	be_app_messenger.SendMessage(keyEvent);
}


void TermView::PushBytes(const char* bytes, size_t length)
{
	vterm_push_bytes(fTerm, bytes, length);
}


VTermRect TermView::PixelsToGlyphs(BRect pixels) const
{
	pixels.OffsetBy(-kBorderSpacing, -kBorderSpacing);

	VTermRect rect;
	rect.start_col = (int)floor(pixels.left / fFontWidth);
	rect.end_col = (int)ceil(pixels.right / fFontWidth);
	rect.start_row = (int)floor(pixels.top / fFontHeight);
	rect.end_row = (int)ceil(pixels.bottom / fFontHeight);

	printf(
		"TOP %d ch < %f px\n"
		"BTM %d ch < %f px\n"
		"LFT %d ch < %f px\n"
		"RGH %d ch < %f px\n",
		rect.start_row, pixels.top,
		rect.end_row, pixels.bottom,
		rect.start_col, pixels.left,
		rect.end_col, pixels.right
	);

	return rect;
}


BRect TermView::GlyphsToPixels(const VTermRect& glyphs) const
{
	BRect rect;
	rect.top = glyphs.start_row * fFontHeight;
	rect.bottom = glyphs.end_row * fFontHeight;
	rect.left = glyphs.start_col * fFontWidth;
	rect.right = glyphs.end_col * fFontWidth;

	rect.OffsetBy(kBorderSpacing, kBorderSpacing);

	printf(
		"TOP %d ch > %f px (%f)\n"
		"BTM %d ch > %f px\n"
		"LFT %d ch > %f px (%f)\n"
		"RGH %d ch > %f px\n",
		glyphs.start_row, rect.top, fFontHeight,
		glyphs.end_row, rect.bottom,
		glyphs.start_col, rect.left, fFontWidth,
		glyphs.end_col, rect.right
	);

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


void TermView::Damage(VTermRect rect)
{
	Invalidate();
//	Invalidate(GlyphsToPixels(rect));
}


/* static */
int TermView::Damage(VTermRect rect, void* user)
{
	TermView* view = (TermView*)user;
	view->Damage(rect);

	return 0;
}


const VTermScreenCallbacks TermView::sScreenCallbacks = {
	&TermView::Damage,
	/*.moverect =*/ NULL,
	/*.movecursor =*/ NULL,
	/*.settermprop =*/ NULL,
	/*.setmousefunc =*/ NULL,
	/*.bell =*/ NULL,
	/*.resize =*/ NULL,
};
