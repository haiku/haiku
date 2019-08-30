/*
 * Console.cpp - Mimicing the console driver
 * Based on the console driver.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the MIT License.
 *
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "Console.h"
#include "ViewBuffer.h"
#include <stdio.h>

Console::Console(ViewBuffer *output)
	:	fState(CONSOLE_STATE_NORMAL),
		fOutput(output)
{
	fOutput->GetSize(&fColumns, &fLines);
	fOutput->SetResizeCallback(&ResizeCallback, this);
	ResetConsole();
	GotoXY(0, 0);
	SaveCursor(true);
	fOutput->Clear(0x0f);
}


Console::~Console()
{
}


void
Console::ResetConsole()
{
	fAttr = 0x0f;
	fScrollTop = 0;
	fScrollBottom = fLines - 1;
	fBrightAttr = true;
	fReverseAttr = false;
}


void
Console::ResizeCallback(int32 width, int32 height, void *data)
{
	Console *console = (Console *)data;
	
	console->fColumns = width;
	console->fLines = height;
	console->SetScrollRegion(console->fScrollTop, height - 1);
}


void
Console::SetScrollRegion(int top, int bottom)
{
	if (top < 0)
		top = 0;
	if (bottom >= fLines)
		bottom = fLines - 1;
	if (top > bottom)
		return;
	
	fScrollTop = top;
	fScrollBottom = bottom;
}


void
Console::ScrollUp()
{
	// see if cursor is outside of scroll region
	if (fY < fScrollTop || fY > fScrollBottom)
		return;
	
	if (fY - fScrollTop > 1) {
		// move the screen up one
		fOutput->Blit(0, fScrollTop + 1, fColumns, fY - fScrollTop, 0, fScrollTop);
	}
	
	// clear the bottom line
	fOutput->FillGlyph(0, fY, fColumns, 1, ' ', fAttr);
}


void
Console::ScrollDown()
{
	// see if cursor is outside of scroll region
	if (fY < fScrollTop || fY > fScrollBottom)
		return;
	
	if (fScrollBottom - fY > 1) {
		// move the screen down one
		fOutput->Blit(0, fY, fColumns, fScrollBottom - fY, 0, fY + 1);
	}
	
	// clear the top line
	fOutput->FillGlyph(0, fY, fColumns, 1, ' ', fAttr);
}


void
Console::LineFeed()
{
	if (fY == fScrollBottom) {
 		// we hit the bottom of our scroll region
 		ScrollUp();
	} else if (fY < fScrollBottom) {
		fY++;
	}
}


void
Console::RLineFeed()
{
	if (fY == fScrollTop) {
 		// we hit the top of our scroll region
 		ScrollDown();
	} else if (fY > fScrollTop) {
		fY--;
	}
}


void
Console::CariageReturn()
{
	fX = 0;
}


void
Console::Delete()
{
	if (fX > 0) {
		fX--;
	} else if (fY > 0) {
        fY--;
        fX = fColumns - 1;
    } else {
        ScrollDown();
        fY--;
        fX = fColumns - 1;
        return;
    }
	
	fOutput->PutGlyph(fX, fY, ' ', fAttr);
}


void
Console::Tab()
{
	fX = (fX + TAB_SIZE) & ~TAB_MASK;
	if (fX >= fColumns) {
		fX -= fColumns;
		LineFeed();
	}
}


void
Console::EraseLine(erase_line_mode mode)
{
	switch (mode) {
		case LINE_ERASE_WHOLE:
			fOutput->FillGlyph(0, fY, fColumns, 1, ' ', fAttr);
			break;
		case LINE_ERASE_LEFT:
			fOutput->FillGlyph(0, fY, fX + 1, 1, ' ', fAttr);
			break;
		case LINE_ERASE_RIGHT:
			fOutput->FillGlyph(fX, fY, fColumns - fX, 1, ' ', fAttr);
			break;
		default:
			return;
	}
}


void
Console::EraseScreen(erase_screen_mode mode)
{
	switch (mode) {
		case SCREEN_ERASE_WHOLE:
			fOutput->Clear(fAttr);
			break;
		case SCREEN_ERASE_UP:
			fOutput->FillGlyph(0, 0, fColumns, fY + 1, ' ', fAttr);
			break;
		case SCREEN_ERASE_DOWN:
			fOutput->FillGlyph(fY, 0, fColumns, fLines - fY, ' ', fAttr);
			break;
		default:
			return;
	}
}


void
Console::SaveCursor(bool save_attrs)
{
	fSavedX = fX;
	fSavedY = fY;
	
	if (save_attrs)
		fSavedAttr = fAttr;
}


void
Console::RestoreCursor(bool restore_attrs)
{
	fX = fSavedX;
	fY = fSavedY;
	
	if (restore_attrs)
		fAttr = fSavedAttr;
}


void
Console::UpdateCursor(int x, int y)
{
	fOutput->MoveCursor(x, y);
}


void
Console::GotoXY(int new_x, int new_y)
{
	if (new_x >= fColumns)
		new_x = fColumns - 1;
	if (new_x < 0)
		new_x = 0;
	if (new_y >= fLines)
		new_y = fLines - 1;
	if (new_y < 0)
		new_y = 0;
	
	fX = new_x;
	fY = new_y;
}


void
Console::PutChar(const char c)
{
	fOutput->PutGlyph(fX, fY, c, fAttr);
	if (++fX >= fColumns) {
		CariageReturn();
		LineFeed();
	}
}


void
Console::SetVT100Attributes(int32 *args, int32 argCount)
{
	if (argCount == 0) {
		// that's the default (attributes off)
		argCount++;
		args[0] = 0;
	}
	
	for (int32 i = 0; i < argCount; i++) {
		switch (args[i]) {
			case 0: // reset
				fAttr = 0x0f;
				fBrightAttr = true;
				fReverseAttr = false;
				break;
			case 1: // bright
				fBrightAttr = true;
				fAttr |= 0x08; // set the bright bit
				break;
			case 2: // dim
				fBrightAttr = false;
				fAttr &= ~0x08; // unset the bright bit
				break;
			case 4: // underscore we can't do
				break;
			case 5: // blink
				fAttr |= 0x80; // set the blink bit
				break;
			case 7: // reverse
				fReverseAttr = true;
				fAttr = ((fAttr & BMASK) >> 4) | ((fAttr & FMASK) << 4);
				if (fBrightAttr)
					fAttr |= 0x08;
				break;
			case 8: // hidden?
				break;
			
			/* foreground colors */
			case 30: fAttr = (fAttr & ~FMASK) | 0 | (fBrightAttr ? 0x08 : 0); break; // black
			case 31: fAttr = (fAttr & ~FMASK) | 4 | (fBrightAttr ? 0x08 : 0); break; // red
			case 32: fAttr = (fAttr & ~FMASK) | 2 | (fBrightAttr ? 0x08 : 0); break; // green
			case 33: fAttr = (fAttr & ~FMASK) | 6 | (fBrightAttr ? 0x08 : 0); break; // yellow
			case 34: fAttr = (fAttr & ~FMASK) | 1 | (fBrightAttr ? 0x08 : 0); break; // blue
			case 35: fAttr = (fAttr & ~FMASK) | 5 | (fBrightAttr ? 0x08 : 0); break; // magenta
			case 36: fAttr = (fAttr & ~FMASK) | 3 | (fBrightAttr ? 0x08 : 0); break; // cyan
			case 37: fAttr = (fAttr & ~FMASK) | 7 | (fBrightAttr ? 0x08 : 0); break; // white
			
			/* background colors */
			case 40: fAttr = (fAttr & ~BMASK) | (0 << 4); break; // black
			case 41: fAttr = (fAttr & ~BMASK) | (4 << 4); break; // red
			case 42: fAttr = (fAttr & ~BMASK) | (2 << 4); break; // green
			case 43: fAttr = (fAttr & ~BMASK) | (6 << 4); break; // yellow
			case 44: fAttr = (fAttr & ~BMASK) | (1 << 4); break; // blue
			case 45: fAttr = (fAttr & ~BMASK) | (5 << 4); break; // magenta
			case 46: fAttr = (fAttr & ~BMASK) | (3 << 4); break; // cyan
			case 47: fAttr = (fAttr & ~BMASK) | (7 << 4); break; // white
		}
	}
}


bool
Console::ProcessVT100Command(const char c, bool seen_bracket, int32 *args, int32 argCount)
{
	bool ret = true;
	
	if (seen_bracket) {
		switch(c) {
			case 'H': /* set cursor position */
			case 'f': {
				int32 row = argCount > 0 ? args[0] : 1;
				int32 col = argCount > 1 ? args[1] : 1;
				if (row > 0)
					row--;
				if (col > 0)
					col--;
				GotoXY(col, row);
				break;
			}
			case 'A': { /* move up */
				int32 deltay = argCount > 0 ? -args[0] : -1;
				if (deltay == 0)
					deltay = -1;
				GotoXY(fX, fY + deltay);
				break;
			}
			case 'e':
			case 'B': { /* move down */
				int32 deltay = argCount > 0 ? args[0] : 1;
				if (deltay == 0)
					deltay = 1;
				GotoXY(fX, fY + deltay);
				break;
			}
			case 'D': { /* move left */
				int32 deltax = argCount > 0 ? -args[0] : -1;
				if (deltax == 0)
					deltax = -1;
				GotoXY(fX + deltax, fY);
				break;
			}
			case 'a':
			case 'C': { /* move right */
				int32 deltax = argCount > 0 ? args[0] : 1;
				if (deltax == 0)
					deltax = 1;
				GotoXY(fX + deltax, fY);
				break;
			}
			case '`':
			case 'G': { /* set X position */
				int32 newx = argCount > 0 ? args[0] : 1;
				if (newx > 0)
					newx--;
				GotoXY(newx, fY);
				break;
			}
			case 'd': { /* set y position */
				int32 newy = argCount > 0 ? args[0] : 1;
				if (newy > 0)
					newy--;
				GotoXY(fX, newy);
				break;
			}
			case 's': /* save current cursor */
				SaveCursor(false);
				break;
			case 'u': /* restore cursor */
				RestoreCursor(false);
				break;
			case 'r': { /* set scroll region */
				int32 low = argCount > 0 ? args[0] : 1;
				int32 high = argCount > 1 ? args[1] : fLines;
				if (low <= high)
					SetScrollRegion(low - 1, high - 1);
				break;
			}
			case 'L': { /* scroll virtual down at cursor */
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					ScrollDown();
					lines--;
				}
				break;
			}
			case 'M': { /* scroll virtual up at cursor */
				int32 lines = argCount > 0 ? args[0] : 1;
				while (lines > 0) {
					ScrollUp();
					lines--;
				}
				break;
			}
			case 'K':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of line
					EraseLine(LINE_ERASE_RIGHT);
				} else if (argCount > 0) {
					if (args[0] == 1)
						EraseLine(LINE_ERASE_LEFT);
					else if (args[0] == 2)
						EraseLine(LINE_ERASE_WHOLE);
				}
				break;
			case 'J':
				if (argCount == 0 || args[0] == 0) {
					// erase to end of screen
					EraseScreen(SCREEN_ERASE_DOWN);
				} else if (argCount > 0) {
					if (args[0] == 1)
						EraseScreen(SCREEN_ERASE_UP);
					else if (args[0] == 2)
						EraseScreen(SCREEN_ERASE_WHOLE);
				}
				break;
			case 'm':
				if (argCount >= 0)
					SetVT100Attributes(args, argCount);
				break;
			default:
				ret = false;
		}
	} else {
		switch (c) {
			case 'c':
				ResetConsole();
				break;
			case 'D':
				RLineFeed();
				break;
			case 'M':
				LineFeed();
				break;
			case '7':
				SaveCursor(true);
				break;
			case '8':
				RestoreCursor(true);
				break;
			default:
				ret = false;
		}
	}
	
	return ret;
}


void
Console::Write(const void *buf, size_t len)
{
	UpdateCursor(-1, -1); // hide the cursor
	
	const char *c;
	size_t pos = 0;
	
	while (pos < len) {
		c = &((const char *)buf)[pos++];
		
		switch (fState) {
			case CONSOLE_STATE_NORMAL:
				// just output the stuff
				switch (*c) {
					case '\n':
						LineFeed();
						break;
					case '\r':
						CariageReturn();
						break;
					case 0x8: // backspace
						Delete();
						break;
					case '\t':
						Tab();
						break;
					case '\a':
						// beep
						//printf("<BEEP>\n");
						break;
					case '\0':
						break;
					case 0x1b:
						// escape character
						fArgCount = -1;
						fState = CONSOLE_STATE_GOT_ESCAPE;
						break;
					default:
						PutChar(*c);
				}
				break;
			case CONSOLE_STATE_GOT_ESCAPE:
				// look for either commands with no argument, or the '[' character
				switch (*c) {
					case '[':
						fState = CONSOLE_STATE_SEEN_BRACKET;
						break;
					default:
						fArgs[fArgCount] = 0;
						ProcessVT100Command(*c, false, fArgs, fArgCount + 1);
						fState = CONSOLE_STATE_NORMAL;
				}
				break;
			case CONSOLE_STATE_SEEN_BRACKET:
				switch (*c) {
					case '0'...'9':
						fArgCount = 0;
						fArgs[fArgCount] = *c - '0';
						fState = CONSOLE_STATE_PARSING_ARG;
						break;
					case '?':
						// private DEC mode parameter follows - we ignore those anyway
						// ToDo: check if it was really used in combination with a mode command
						break;
					default:
						ProcessVT100Command(*c, true, fArgs, fArgCount + 1);
						fState = CONSOLE_STATE_NORMAL;
				}
				break;
			case CONSOLE_STATE_NEW_ARG:
				switch (*c) {
					case '0'...'9':
						fArgCount++;
						if (fArgCount == MAX_ARGS) {
							fState = CONSOLE_STATE_NORMAL;
							break;
						}
						fArgs[fArgCount] = *c - '0';
						fState = CONSOLE_STATE_PARSING_ARG;
						break;
					default:
						ProcessVT100Command(*c, true, fArgs, fArgCount + 1);
						fState = CONSOLE_STATE_NORMAL;
				}
				break;
			case CONSOLE_STATE_PARSING_ARG:
				// parse args
				switch (*c) {
					case '0'...'9':
						fArgs[fArgCount] *= 10;
						fArgs[fArgCount] += *c - '0';
						break;
					case ';':
						fState = CONSOLE_STATE_NEW_ARG;
						break;
					default:
						ProcessVT100Command(*c, true, fArgs, fArgCount + 1);
						fState = CONSOLE_STATE_NORMAL;
				}
			}
	}
	
	UpdateCursor(fX, fY); // show it again
}
