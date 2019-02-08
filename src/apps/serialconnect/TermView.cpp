/*
 * Copyright 2012-2015, Adrien Destugues, pulkomandy@gmail.com
 * Distributed under the terms of the MIT licence.
 */


#include "TermView.h"

#include <stdio.h>

#include <Entry.h>
#include <File.h>
#include <Font.h>
#include <Layout.h>
#include <ScrollBar.h>

#include "SerialApp.h"
#include "libvterm/src/vterm_internal.h"


struct ScrollBufferItem {
	int cols;
	VTermScreenCell cells[];
};


TermView::TermView()
	:
	BView("TermView", B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init();
}


TermView::TermView(BRect r)
	:
	BView(r, "TermView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS)
{
	_Init();
}


TermView::~TermView()
{
	vterm_free(fTerm);
}


void
TermView::AttachedToWindow()
{
	BView::AttachedToWindow();
	MakeFocus();
}


void
TermView::Draw(BRect updateRect)
{
	VTermRect updatedChars = _PixelsToGlyphs(updateRect);

	VTermPos pos;
	font_height height;
	GetFontHeight(&height);

	VTermPos cursorPos;
	vterm_state_get_cursorpos(vterm_obtain_state(fTerm), &cursorPos);

	for (pos.row = updatedChars.start_row; pos.row <= updatedChars.end_row;
			pos.row++) {
		int x = updatedChars.start_col * fFontWidth + kBorderSpacing;
		int y = pos.row * fFontHeight + (int)ceil(height.ascent)
			+ kBorderSpacing;
		MovePenTo(x, y);

		BString string;
		VTermScreenCell cell;
		int width = 0;
		bool isCursor = false;

		pos.col = updatedChars.start_col;
		_GetCell(pos, cell);

		for (pos.col = updatedChars.start_col;
				pos.col <= updatedChars.end_col;) {

			VTermScreenCell newCell;
			_GetCell(pos, newCell);

			// We need to start a new extent if:
			// - The attributes change
			// - The colors change
			// - The end of line is reached
			// - The current cell is under the cursor
			// - The current cell is right of the cursor
			if (*(uint32_t*)&cell.attrs != *(uint32_t*)&newCell.attrs
				|| !vterm_color_equal(cell.fg, newCell.fg)
				|| !vterm_color_equal(cell.bg, newCell.bg)
				|| pos.col >= updatedChars.end_col
				|| (pos.col == cursorPos.col && pos.row == cursorPos.row)
				|| (pos.col == cursorPos.col + 1 && pos.row == cursorPos.row)) {

				rgb_color foreground, background;
				foreground.red = cell.fg.red;
				foreground.green = cell.fg.green;
				foreground.blue = cell.fg.blue;
				foreground.alpha = 255;
				background.red = cell.bg.red;
				background.green = cell.bg.green;
				background.blue = cell.bg.blue;
				background.alpha = 255;

				// Draw the cursor by swapping foreground and background colors
				if (isCursor) {
					SetLowColor(foreground);
					SetViewColor(foreground);
					SetHighColor(background);
				} else {
					SetLowColor(background);
					SetViewColor(background);
					SetHighColor(foreground);
				}

				FillRect(BRect(x, y - ceil(height.ascent) + 1,
					x + width * fFontWidth - 1,
					y + ceil(height.descent) + ceil(height.leading)),
					B_SOLID_LOW);

				BFont font = be_fixed_font;
				if (cell.attrs.bold)
					font.SetFace(B_BOLD_FACE);
				if (cell.attrs.underline)
					font.SetFace(B_UNDERSCORE_FACE);
				if (cell.attrs.italic)
					font.SetFace(B_ITALIC_FACE);
				if (cell.attrs.blink) // FIXME make it actually blink
					font.SetFace(B_OUTLINED_FACE);
				if (cell.attrs.reverse)
					font.SetFace(B_NEGATIVE_FACE);
				if (cell.attrs.strike)
					font.SetFace(B_STRIKEOUT_FACE);

				// TODO handle "font" (alternate fonts), dwl and dhl (double size)

				SetFont(&font);
				DrawString(string);
				x += width * fFontWidth;

				// Prepare for next cell
				cell = newCell;
				string = "";
				width = 0;
			}

			if (pos.col == cursorPos.col && pos.row == cursorPos.row)
				isCursor = true;
			else
				isCursor = false;

			if (newCell.chars[0] == 0) {
				string += " ";
				pos.col ++;
				width += 1;
			} else {
				char buffer[VTERM_MAX_CHARS_PER_CELL];
				wcstombs(buffer, (wchar_t*)newCell.chars,
					VTERM_MAX_CHARS_PER_CELL);
				string += buffer;
				width += newCell.width;
				pos.col += newCell.width;
			}
		}
	}
}


void
TermView::FrameResized(float width, float height)
{
	VTermRect newSize = _PixelsToGlyphs(BRect(0, 0, width - 2 * kBorderSpacing,
				height - 2 * kBorderSpacing));
	vterm_set_size(fTerm, newSize.end_row, newSize.end_col);
}


void
TermView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = kDefaultWidth * fFontWidth + 2 * kBorderSpacing - 1;
	if (height != NULL)
		*height = kDefaultHeight * fFontHeight + 2 * kBorderSpacing - 1;
}


void
TermView::KeyDown(const char* bytes, int32 numBytes)
{
	// Translate some keys to more usual VT100 escape codes
	switch (bytes[0]) {
		case B_UP_ARROW:
			numBytes = 3;
			bytes = "\x1B[A";
			break;
		case B_DOWN_ARROW:
			numBytes = 3;
			bytes = "\x1B[B";
			break;
		case B_RIGHT_ARROW:
			numBytes = 3;
			bytes = "\x1B[C";
			break;
		case B_LEFT_ARROW:
			numBytes = 3;
			bytes = "\x1B[D";
			break;
		case B_BACKSPACE:
			numBytes = 1;
			bytes = "\x7F";
			break;
		case '\n':
			numBytes = fLineTerminator.Length();
			bytes = fLineTerminator.String();
			break;
	}

	// Send the bytes to the serial port
	BMessage* keyEvent = new BMessage(kMsgDataWrite);
	keyEvent->AddData("data", B_RAW_TYPE, bytes, numBytes);
	be_app_messenger.SendMessage(keyEvent);
}


void
TermView::MessageReceived(BMessage* message)
{
	switch (message->what)
	{
		case 'DATA':
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK)
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


void
TermView::SetLineTerminator(BString terminator)
{
	fLineTerminator = terminator;
}


void
TermView::PushBytes(const char* bytes, size_t length)
{
	vterm_push_bytes(fTerm, bytes, length);
}


// #pragma mark -


void
TermView::_Init()
{
	SetFont(be_fixed_font);

	font_height height;
	GetFontHeight(&height);
	fFontHeight = (int)(ceilf(height.ascent) + ceilf(height.descent)
		+ ceilf(height.leading));
	fFontWidth = (int)be_fixed_font->StringWidth("X");
	fTerm = vterm_new(kDefaultHeight, kDefaultWidth);

	fTermScreen = vterm_obtain_screen(fTerm);
	vterm_screen_set_callbacks(fTermScreen, &sScreenCallbacks, this);
	vterm_screen_reset(fTermScreen, 1);

	vterm_parser_set_utf8(fTerm, 1);

	VTermScreenCell cell;
	VTermPos firstPos;
	firstPos.row = 0;
	firstPos.col = 0;
	_GetCell(firstPos, cell);

	rgb_color background;
	background.red = cell.bg.red;
	background.green = cell.bg.green;
	background.blue = cell.bg.blue;
	background.alpha = 255;

	SetViewColor(background);
	SetLineTerminator("\n");
}


VTermRect
TermView::_PixelsToGlyphs(BRect pixels) const
{
	pixels.OffsetBy(-kBorderSpacing, -kBorderSpacing);

	VTermRect rect;
	rect.start_col = (int)floor(pixels.left / fFontWidth);
	rect.end_col = (int)ceil(pixels.right / fFontWidth);
	rect.start_row = (int)floor(pixels.top / fFontHeight);
	rect.end_row = (int)ceil(pixels.bottom / fFontHeight);
#if 0
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
#endif
	return rect;
}


BRect TermView::_GlyphsToPixels(const VTermRect& glyphs) const
{
	BRect rect;
	rect.top = glyphs.start_row * fFontHeight;
	rect.bottom = glyphs.end_row * fFontHeight;
	rect.left = glyphs.start_col * fFontWidth;
	rect.right = glyphs.end_col * fFontWidth;

	rect.OffsetBy(kBorderSpacing, kBorderSpacing);
#if 0
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
#endif
	return rect;
}


BRect
TermView::_GlyphsToPixels(const int width, const int height) const
{
	VTermRect rect;
	rect.start_row = 0;
	rect.start_col = 0;
	rect.end_row = height;
	rect.end_col = width;
	return _GlyphsToPixels(rect);
}


void
TermView::_GetCell(VTermPos pos, VTermScreenCell& cell)
{
	// First handle cells from the normal screen
	if (vterm_screen_get_cell(fTermScreen, pos, &cell) != 0)
		return;

	// Try the scroll-back buffer
	if (pos.row < 0 && pos.col >= 0) {
		int offset = - pos.row - 1;
		ScrollBufferItem* line
			= (ScrollBufferItem*)fScrollBuffer.ItemAt(offset);
		if (line != NULL && pos.col < line->cols) {
			cell = line->cells[pos.col];
			return;
		}
	}

	// All cells outside the used terminal area are drawn with the same
	// background color as the top-left one.
	// TODO should they use the attributes of the closest neighbor instead?
	VTermPos firstPos;
	firstPos.row = 0;
	firstPos.col = 0;
	vterm_screen_get_cell(fTermScreen, firstPos, &cell);
	cell.chars[0] = 0;
	cell.width = 1;
}


void
TermView::_Damage(VTermRect rect)
{
	Invalidate(_GlyphsToPixels(rect));
}


void
TermView::_MoveCursor(VTermPos pos, VTermPos oldPos, int visible)
{
	VTermRect r;

	// We need to erase the cursor from its old position
	r.start_row = oldPos.row;
	r.start_col = oldPos.col;
	r.end_col = oldPos.col + 1;
	r.end_row = oldPos.row + 1;
	Invalidate(_GlyphsToPixels(r));

	// And we need to draw it at the new one
	r.start_row = pos.row;
	r.start_col = pos.col;
	r.end_col = pos.col + 1;
	r.end_row = pos.row + 1;
	Invalidate(_GlyphsToPixels(r));
}


void
TermView::_PushLine(int cols, const VTermScreenCell* cells)
{
	ScrollBufferItem* item = (ScrollBufferItem*)malloc(sizeof(int)
		+ cols * sizeof(VTermScreenCell));
	item->cols = cols;
	memcpy(item->cells, cells, cols * sizeof(VTermScreenCell));

	fScrollBuffer.AddItem(item, 0);

	free(fScrollBuffer.RemoveItem(kScrollBackSize));

	int availableRows, availableCols;
	vterm_get_size(fTerm, &availableRows, &availableCols);

	VTermRect dirty;
	dirty.start_col = 0;
	dirty.end_col = availableCols;
	dirty.end_row = 0;
	dirty.start_row = -fScrollBuffer.CountItems();
	// FIXME we should rather use CopyRect if possible, and only invalidate the
	// newly exposed area here.
	Invalidate(_GlyphsToPixels(dirty));

	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar != NULL) {
		float range = (fScrollBuffer.CountItems() + availableRows)
			* fFontHeight;
		scrollBar->SetRange(availableRows * fFontHeight - range, 0.0f);
		// TODO we need to adjust this in FrameResized, as availableRows can
		// change
		scrollBar->SetProportion(availableRows * fFontHeight / range);
		scrollBar->SetSteps(fFontHeight, fFontHeight * 3);
	}
}


/* static */ int
TermView::_Damage(VTermRect rect, void* user)
{
	TermView* view = (TermView*)user;
	view->_Damage(rect);

	return 0;
}


/* static */ int
TermView::_MoveCursor(VTermPos pos, VTermPos oldPos, int visible, void* user)
{
	TermView* view = (TermView*)user;
	view->_MoveCursor(pos, oldPos, visible);

	return 0;
}


/* static */ int
TermView::_PushLine(int cols, const VTermScreenCell* cells, void* user)
{
	TermView* view = (TermView*)user;
	view->_PushLine(cols, cells);

	return 0;
}


const
VTermScreenCallbacks TermView::sScreenCallbacks = {
	&TermView::_Damage,
	/*.moverect =*/ NULL,
	&TermView::_MoveCursor,
	/*.settermprop =*/ NULL,
	/*.setmousefunc =*/ NULL,
	/*.bell =*/ NULL,
	/*.resize =*/ NULL,
	&TermView::_PushLine,
	/*.sb_popline =*/ NULL,
};
