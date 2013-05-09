/*
 * Copyright 2001-2013, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */


#include "TermViewStates.h"

#include <MessageRunner.h>
#include <ScrollBar.h>
#include <UTF8.h>
#include <Window.h>

#include "Shell.h"
#include "TermConst.h"
#include "VTkeymap.h"
#include "VTKeyTbl.h"


// selection granularity
enum {
	SELECT_CHARS,
	SELECT_WORDS,
	SELECT_LINES
};

static const uint32 kAutoScroll = 'AScr';


// #pragma mark - State


TermView::State::State(TermView* view)
	:
	fView(view)
{
}


TermView::State::~State()
{
}


void
TermView::State::Entered()
{
}


void
TermView::State::Exited()
{
}


bool
TermView::State::MessageReceived(BMessage* message)
{
	return false;
}


void
TermView::State::KeyDown(const char* bytes, int32 numBytes)
{
}


void
TermView::State::MouseDown(BPoint where, int32 buttons, int32 modifiers)
{
}


void
TermView::State::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
}


void
TermView::State::MouseUp(BPoint where, int32 buttons)
{
}


// #pragma mark - StandardBaseState


TermView::StandardBaseState::StandardBaseState(TermView* view)
	:
	State(view)
{
}


bool
TermView::StandardBaseState::_StandardMouseMoved(BPoint where)
{
	if (!fView->fReportAnyMouseEvent && !fView->fReportButtonMouseEvent)
		return false;

	int32 modifier;
	fView->Window()->CurrentMessage()->FindInt32("modifiers", &modifier);

	TermPos clickPos = fView->_ConvertToTerminal(where);

	if (fView->fReportButtonMouseEvent) {
		if (fView->fPrevPos.x != clickPos.x
			|| fView->fPrevPos.y != clickPos.y) {
			fView->_SendMouseEvent(fView->fMouseButtons, modifier,
				clickPos.x, clickPos.y, true);
		}
		fView->fPrevPos = clickPos;
	} else {
		fView->_SendMouseEvent(fView->fMouseButtons, modifier, clickPos.x,
			clickPos.y, true);
	}

	return true;
}


// #pragma mark - DefaultState


TermView::DefaultState::DefaultState(TermView* view)
	:
	StandardBaseState(view)
{
}


void
TermView::DefaultState::KeyDown(const char* bytes, int32 numBytes)
{
	int32 key, mod, rawChar;
	BMessage *currentMessage = fView->Looper()->CurrentMessage();
	if (currentMessage == NULL)
		return;

	currentMessage->FindInt32("modifiers", &mod);
	currentMessage->FindInt32("key", &key);
	currentMessage->FindInt32("raw_char", &rawChar);

	fView->_ActivateCursor(true);

	// handle multi-byte chars
	if (numBytes > 1) {
		if (fView->fEncoding != M_UTF8) {
			char destBuffer[16];
			int32 destLen = sizeof(destBuffer);
			int32 state = 0;
			convert_from_utf8(fView->fEncoding, bytes, &numBytes, destBuffer,
				&destLen, &state, '?');
			fView->_ScrollTo(0, true);
			fView->fShell->Write(destBuffer, destLen);
			return;
		}

		fView->_ScrollTo(0, true);
		fView->fShell->Write(bytes, numBytes);
		return;
	}

	// Terminal filters RET, ENTER, F1...F12, and ARROW key code.
	const char *toWrite = NULL;

	switch (*bytes) {
		case B_RETURN:
			if (rawChar == B_RETURN)
				toWrite = "\r";
			break;

		case B_DELETE:
			toWrite = DELETE_KEY_CODE;
			break;

		case B_BACKSPACE:
			// Translate only the actual backspace key to the backspace
			// code. CTRL-H shall just be echoed.
			if (!((mod & B_CONTROL_KEY) && rawChar == 'h'))
				toWrite = BACKSPACE_KEY_CODE;
			break;

		case B_LEFT_ARROW:
			if (rawChar == B_LEFT_ARROW) {
				if ((mod & B_SHIFT_KEY) != 0) {
					if (fView->fListener != NULL)
						fView->fListener->PreviousTermView(fView);
					return;
				}
				if ((mod & B_CONTROL_KEY) || (mod & B_COMMAND_KEY))
					toWrite = CTRL_LEFT_ARROW_KEY_CODE;
				else
					toWrite = LEFT_ARROW_KEY_CODE;
			}
			break;

		case B_RIGHT_ARROW:
			if (rawChar == B_RIGHT_ARROW) {
				if ((mod & B_SHIFT_KEY) != 0) {
					if (fView->fListener != NULL)
						fView->fListener->NextTermView(fView);
					return;
				}
				if ((mod & B_CONTROL_KEY) || (mod & B_COMMAND_KEY))
					toWrite = CTRL_RIGHT_ARROW_KEY_CODE;
				else
					toWrite = RIGHT_ARROW_KEY_CODE;
			}
			break;

		case B_UP_ARROW:
			if (mod & B_SHIFT_KEY) {
				fView->_ScrollTo(fView->fScrollOffset - fView->fFontHeight,
					true);
				return;
			}
			if (rawChar == B_UP_ARROW) {
				if (mod & B_CONTROL_KEY)
					toWrite = CTRL_UP_ARROW_KEY_CODE;
				else
					toWrite = UP_ARROW_KEY_CODE;
			}
			break;

		case B_DOWN_ARROW:
			if (mod & B_SHIFT_KEY) {
				fView->_ScrollTo(fView->fScrollOffset + fView->fFontHeight,
					true);
				return;
			}

			if (rawChar == B_DOWN_ARROW) {
				if (mod & B_CONTROL_KEY)
					toWrite = CTRL_DOWN_ARROW_KEY_CODE;
				else
					toWrite = DOWN_ARROW_KEY_CODE;
			}
			break;

		case B_INSERT:
			if (rawChar == B_INSERT)
				toWrite = INSERT_KEY_CODE;
			break;

		case B_HOME:
			if (rawChar == B_HOME)
				toWrite = HOME_KEY_CODE;
			break;

		case B_END:
			if (rawChar == B_END)
				toWrite = END_KEY_CODE;
			break;

		case B_PAGE_UP:
			if (mod & B_SHIFT_KEY) {
				fView->_ScrollTo(
					fView->fScrollOffset - fView->fFontHeight  * fView->fRows,
					true);
				return;
			}
			if (rawChar == B_PAGE_UP)
				toWrite = PAGE_UP_KEY_CODE;
			break;

		case B_PAGE_DOWN:
			if (mod & B_SHIFT_KEY) {
				fView->_ScrollTo(
					fView->fScrollOffset + fView->fFontHeight * fView->fRows,
					true);
				return;
			}
			if (rawChar == B_PAGE_DOWN)
				toWrite = PAGE_DOWN_KEY_CODE;
			break;

		case B_FUNCTION_KEY:
			for (int32 i = 0; i < 12; i++) {
				if (key == function_keycode_table[i]) {
					toWrite = function_key_char_table[i];
					break;
				}
			}
			break;
	}

	// If the above code proposed an alternative string to write, we get it's
	// length. Otherwise we write exactly the bytes passed to this method.
	size_t toWriteLen;
	if (toWrite != NULL) {
		toWriteLen = strlen(toWrite);
	} else {
		toWrite = bytes;
		toWriteLen = numBytes;
	}

	fView->_ScrollTo(0, true);
	fView->fShell->Write(toWrite, toWriteLen);
}


void
TermView::DefaultState::MouseDown(BPoint where, int32 buttons, int32 modifiers)
{
	if (fView->fReportAnyMouseEvent || fView->fReportButtonMouseEvent
		|| fView->fReportNormalMouseEvent || fView->fReportX10MouseEvent) {
		TermPos clickPos = fView->_ConvertToTerminal(where);
		fView->_SendMouseEvent(buttons, modifiers, clickPos.x, clickPos.y,
			false);
		return;
	}

	// paste button
	if ((buttons & (B_SECONDARY_MOUSE_BUTTON | B_TERTIARY_MOUSE_BUTTON)) != 0) {
		fView->Paste(fView->fMouseClipboard);
		return;
	}

	// Select Region
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		fView->fSelectState->Prepare(where, modifiers);
		fView->_NextState(fView->fSelectState);
	}
}


void
TermView::DefaultState::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	_StandardMouseMoved(where);
}


// #pragma mark - SelectState


TermView::SelectState::SelectState(TermView* view)
	:
	StandardBaseState(view),
	fSelectGranularity(SELECT_CHARS),
	fCheckMouseTracking(false),
	fMouseTracking(false)
{
}


void
TermView::SelectState::Prepare(BPoint where, int32 modifiers)
{
	int32 clicks;
	fView->Window()->CurrentMessage()->FindInt32("clicks", &clicks);

	if (fView->_HasSelection()) {
		TermPos inPos = fView->_ConvertToTerminal(where);
		if (fView->_CheckSelectedRegion(inPos)) {
			if (modifiers & B_CONTROL_KEY) {
				BPoint p;
				uint32 bt;
				do {
					fView->GetMouse(&p, &bt);

					if (bt == 0) {
						fView->_Deselect();
						return;
					}

					snooze(40000);

				} while (abs((int)(where.x - p.x)) < 4
					&& abs((int)(where.y - p.y)) < 4);

				fView->InitiateDrag();
				return;
			}
		}
	}

	// If mouse has moved too much, disable double/triple click.
	if (fView->_MouseDistanceSinceLastClick(where) > 8)
		clicks = 1;

	fView->SetMouseEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS,
		B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);

	TermPos clickPos = fView->_ConvertToTerminal(where);

	if (modifiers & B_SHIFT_KEY) {
		fView->fInitialSelectionStart = clickPos;
		fView->fInitialSelectionEnd = clickPos;
		fView->_ExtendSelection(fView->fInitialSelectionStart, true, false);
	} else {
		fView->_Deselect();
		fView->fInitialSelectionStart = clickPos;
		fView->fInitialSelectionEnd = clickPos;
	}

	// If clicks larger than 3, reset mouse click counter.
	clicks = (clicks - 1) % 3 + 1;

	switch (clicks) {
		case 1:
			fCheckMouseTracking = true;
			fSelectGranularity = SELECT_CHARS;
			break;

		case 2:
			fView->_SelectWord(where, (modifiers & B_SHIFT_KEY) != 0, false);
			fMouseTracking = true;
			fSelectGranularity = SELECT_WORDS;
			break;

		case 3:
			fView->_SelectLine(where, (modifiers & B_SHIFT_KEY) != 0, false);
			fMouseTracking = true;
			fSelectGranularity = SELECT_LINES;
			break;
	}
}


bool
TermView::SelectState::MessageReceived(BMessage* message)
{
	if (message->what == kAutoScroll) {
		_AutoScrollUpdate();
		return true;
	}

	return false;
}


void
TermView::SelectState::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	if (_StandardMouseMoved(where))
		return;

	if (fCheckMouseTracking) {
		if (fView->_MouseDistanceSinceLastClick(where) > 9)
			fMouseTracking = true;
	}
	if (!fMouseTracking)
		return;

	bool doAutoScroll = false;

	if (where.y < 0) {
		doAutoScroll = true;
		fView->fAutoScrollSpeed = where.y;
		where.x = 0;
		where.y = 0;
	}

	BRect bounds(fView->Bounds());
	if (where.y > bounds.bottom) {
		doAutoScroll = true;
		fView->fAutoScrollSpeed = where.y - bounds.bottom;
		where.x = bounds.right;
		where.y = bounds.bottom;
	}

	if (doAutoScroll) {
		if (fView->fAutoScrollRunner == NULL) {
			BMessage message(kAutoScroll);
			fView->fAutoScrollRunner = new (std::nothrow) BMessageRunner(
				BMessenger(fView), &message, 10000);
		}
	} else {
		delete fView->fAutoScrollRunner;
		fView->fAutoScrollRunner = NULL;
	}

	switch (fSelectGranularity) {
		case SELECT_CHARS:
		{
			// If we just start selecting, we first select the initially
			// hit char, so that we get a proper initial selection -- the char
			// in question, which will thus always be selected, regardless of
			// whether selecting forward or backward.
			if (fView->fInitialSelectionStart == fView->fInitialSelectionEnd) {
				fView->_Select(fView->fInitialSelectionStart,
					fView->fInitialSelectionEnd, true, true);
			}

			fView->_ExtendSelection(fView->_ConvertToTerminal(where), true,
				true);
			break;
		}
		case SELECT_WORDS:
			fView->_SelectWord(where, true, true);
			break;
		case SELECT_LINES:
			fView->_SelectLine(where, true, true);
			break;
	}
}


void
TermView::SelectState::MouseUp(BPoint where, int32 buttons)
{
	fCheckMouseTracking = false;
	fMouseTracking = false;

	if (fView->fAutoScrollRunner != NULL) {
		delete fView->fAutoScrollRunner;
		fView->fAutoScrollRunner = NULL;
	}

	// When releasing the first mouse button, we copy the selected text to the
	// clipboard.

	if (fView->fReportAnyMouseEvent || fView->fReportButtonMouseEvent
		|| fView->fReportNormalMouseEvent) {
		TermPos clickPos = fView->_ConvertToTerminal(where);
		fView->_SendMouseEvent(0, 0, clickPos.x, clickPos.y, false);
	} else {
		if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0
			&& (fView->fMouseButtons & B_PRIMARY_MOUSE_BUTTON) != 0) {
			fView->Copy(fView->fMouseClipboard);
		}

	}

	fView->_NextState(fView->fDefaultState);
}


void
TermView::SelectState::_AutoScrollUpdate()
{
	if (fMouseTracking && fView->fAutoScrollRunner != NULL
		&& fView->fScrollBar != NULL) {
		float value = fView->fScrollBar->Value();
		fView->_ScrollTo(value + fView->fAutoScrollSpeed, true);
		if (fView->fAutoScrollSpeed < 0) {
			fView->_ExtendSelection(
				fView->_ConvertToTerminal(BPoint(0, 0)), true, true);
		} else {
			fView->_ExtendSelection(
				fView->_ConvertToTerminal(fView->Bounds().RightBottom()), true,
				true);
		}
	}
}
