/*
 * Copyright 2012, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "TermView.h"

#include <stdio.h>

#include <Entry.h>
#include <File.h>
#include <Layout.h>

#include "SerialApp.h"


TermView::TermView()
	:
	BView("TermView", B_WILL_DRAW | B_FRAME_EVENTS)
{
	font_height height;
	GetFontHeight(&height);
	fFontHeight = height.ascent + height.descent + height.leading;
	fFontWidth = be_fixed_font->StringWidth("X");
	fTerm = vterm_new(kDefaultHeight, kDefaultWidth);

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

	int availableRows, availableCols;
	vterm_get_size(fTerm, &availableRows, &availableCols);

	for (pos.row = updatedChars.start_row; pos.row <= updatedChars.end_row;
			pos.row++) {
		float x = updatedChars.start_col * fFontWidth + kBorderSpacing;
		float y = pos.row * fFontHeight + height.ascent + kBorderSpacing;
		MovePenTo(x, y);

		for (pos.col = updatedChars.start_col;
				pos.col <= updatedChars.end_col;) {
			if (pos.col < 0 || pos.row < 0 || pos.col >= availableCols 
					|| pos.row >= availableRows) {
				DrawString(" ");
				pos.col ++;
			} else {
				VTermScreenCell cell;
				vterm_screen_get_cell(fTermScreen, pos, &cell);

				rgb_color foreground, background;
				foreground.red = cell.fg.red;
				foreground.green = cell.fg.green;
				foreground.blue = cell.fg.blue;
				background.red = cell.bg.red;
				background.green = cell.bg.green;
				background.blue = cell.bg.blue;

				if(cell.attrs.reverse) {
					SetLowColor(foreground);
					SetViewColor(foreground);
					SetHighColor(background);
				} else {
					SetLowColor(background);
					SetViewColor(background);
					SetHighColor(foreground);
				}

				BPoint penLocation = PenLocation();
				FillRect(BRect(penLocation.x, penLocation.y - height.ascent,
					penLocation.x + cell.width * fFontWidth, penLocation.y), B_SOLID_LOW);

				if (cell.chars[0] == 0) {
					DrawString(" ");
					pos.col ++;
				} else {
					char buffer[VTERM_MAX_CHARS_PER_CELL];
					wcstombs(buffer, (wchar_t*)cell.chars,
						VTERM_MAX_CHARS_PER_CELL);

					DrawString(buffer);
					pos.col += cell.width;
				}
			}
		}
	}
}


void TermView::FrameResized(float width, float height)
{
	VTermRect newSize = PixelsToGlyphs(BRect(0, 0, width - 2 * kBorderSpacing,
		height - 2 * kBorderSpacing));
	vterm_set_size(fTerm, newSize.end_row, newSize.end_col);
}


void TermView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = kDefaultWidth * fFontWidth + 2 * kBorderSpacing;
	if (height != NULL)
		*height = kDefaultHeight * fFontHeight + 2 * kBorderSpacing;
}


void TermView::KeyDown(const char* bytes, int32 numBytes)
{
	BMessage* keyEvent = new BMessage(kMsgDataWrite);
	keyEvent->AddData("data", B_RAW_TYPE, bytes, numBytes);
	be_app_messenger.SendMessage(keyEvent);
}


void TermView::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case 'DATA':
		{
			entry_ref ref;
			if(message->FindRef("refs", &ref) == B_OK)
			{
				// The user just dropped a file on us
				// TODO send it by XMODEM or so
			}
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}


void TermView::PushBytes(const char* bytes, size_t length)
{
	vterm_push_bytes(fTerm, bytes, length);
}


//#pragma mark -


VTermRect TermView::PixelsToGlyphs(BRect pixels) const
{
	pixels.OffsetBy(-kBorderSpacing, -kBorderSpacing);

	VTermRect rect;
	rect.start_col = (int)floor(pixels.left / fFontWidth);
	rect.end_col = (int)ceil(pixels.right / fFontWidth);
	rect.start_row = (int)floor(pixels.top / fFontHeight);
	rect.end_row = (int)ceil(pixels.bottom / fFontHeight);
/*
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
*/
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
/*
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
*/
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
