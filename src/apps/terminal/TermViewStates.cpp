/*
 * Copyright 2001-2015, Haiku, Inc.
 * Copyright 2003-2004 Kian Duffy, myob@users.sourceforge.net
 * Parts Copyright 1998-1999 Kazuho Okui and Takashi Murai.
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Stefano Ceccherini, stefano.ceccherini@gmail.com
 *		Kian Duffy, myob@users.sourceforge.net
 *		Y.Hayakawa, hida@sawada.riec.tohoku.ac.jp
 *		Simon South, simon@simonsouth.net
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@Clemens-Zeidler.de
 *		Siarzhuk Zharski, zharik@gmx.li
 */


#include "TermViewStates.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <Catalog.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <UTF8.h>
#include <Window.h>

#include <Array.h>

#include "ActiveProcessInfo.h"
#include "Shell.h"
#include "TermConst.h"
#include "TerminalBuffer.h"
#include "VTkeymap.h"
#include "VTKeyTbl.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal TermView"


// selection granularity
enum {
	SELECT_CHARS,
	SELECT_WORDS,
	SELECT_LINES
};

static const uint32 kAutoScroll = 'AScr';

static const uint32 kMessageOpenLink = 'OLnk';
static const uint32 kMessageCopyLink = 'CLnk';
static const uint32 kMessageCopyAbsolutePath = 'CAbs';
static const uint32 kMessageMenuClosed = 'MClo';


static const char* const kKnownURLProtocols = "http:https:ftp:mailto";


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
TermView::State::ModifiersChanged(int32 oldModifiers, int32 modifiers)
{
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
	const BMessage* message, int32 modifiers)
{
}


void
TermView::State::MouseUp(BPoint where, int32 buttons)
{
}


void
TermView::State::WindowActivated(bool active)
{
}


void
TermView::State::VisibleTextBufferChanged()
{
}


// #pragma mark - StandardBaseState


TermView::StandardBaseState::StandardBaseState(TermView* view)
	:
	State(view)
{
}


bool
TermView::StandardBaseState::_StandardMouseMoved(BPoint where, int32 modifiers)
{
	if (!fView->fReportAnyMouseEvent && !fView->fReportButtonMouseEvent)
		return false;

	TermPos clickPos = fView->_ConvertToTerminal(where);

	if (fView->fReportButtonMouseEvent) {
		if (fView->fPrevPos.x != clickPos.x
			|| fView->fPrevPos.y != clickPos.y) {
			fView->_SendMouseEvent(fView->fMouseButtons, modifiers,
				clickPos.x, clickPos.y, true);
		}
		fView->fPrevPos = clickPos;
	} else {
		fView->_SendMouseEvent(fView->fMouseButtons, modifiers, clickPos.x,
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
TermView::DefaultState::ModifiersChanged(int32 oldModifiers, int32 modifiers)
{
	_CheckEnterHyperLinkState(modifiers);
}


void
TermView::DefaultState::KeyDown(const char* bytes, int32 numBytes)
{
	int32 key;
	int32 mod;
	int32 rawChar;
	BMessage* currentMessage = fView->Looper()->CurrentMessage();
	if (currentMessage == NULL)
		return;

	currentMessage->FindInt32("modifiers", &mod);
	currentMessage->FindInt32("key", &key);
	currentMessage->FindInt32("raw_char", &rawChar);

	fView->_ActivateCursor(true);

	// Handle the Option key when used as Meta
	if ((mod & B_LEFT_OPTION_KEY) != 0 && fView->fUseOptionAsMetaKey
		&& (fView->fInterpretMetaKey || fView->fMetaKeySendsEscape)) {
		const char* bytes;
		int8 numBytes;

		// Determine the character produced by the same keypress without the
		// Option key
		mod &= B_SHIFT_KEY | B_CAPS_LOCK | B_CONTROL_KEY;
		const int32 (*keymapTable)[128] = (mod == 0)
			? NULL
			: fView->fKeymapTableForModifiers.Get(mod);
		if (keymapTable == NULL) {
			bytes = (const char*)&rawChar;
			numBytes = 1;
		} else {
			bytes = &fView->fKeymapChars[(*keymapTable)[key]];
			numBytes = *(bytes++);
		}

		if (numBytes <= 0)
			return;

		fView->_ScrollTo(0, true);

		char outputBuffer[2];
		const char* toWrite = bytes;

		if (fView->fMetaKeySendsEscape) {
			fView->fShell->Write("\e", 1);
		} else if (numBytes == 1) {
			char byte = *bytes | 0x80;

			// The eighth bit has special meaning in UTF-8, so if that encoding
			// is in use recode the output (as xterm does)
			if (fView->fEncoding == M_UTF8) {
				outputBuffer[0] = 0xc0 | ((byte >> 6) & 0x03);
				outputBuffer[1] = 0x80 | (byte & 0x3f);
				numBytes = 2;
			} else {
				outputBuffer[0] = byte;
				numBytes = 1;
			}
			toWrite = outputBuffer;
		}

		fView->fShell->Write(toWrite, numBytes);
		return;
	}

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

	// select region
	if (buttons == B_PRIMARY_MOUSE_BUTTON) {
		fView->fSelectState->Prepare(where, modifiers);
		fView->_NextState(fView->fSelectState);
	}
}


void
TermView::DefaultState::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage, int32 modifiers)
{
	if (_CheckEnterHyperLinkState(modifiers))
		return;

	_StandardMouseMoved(where, modifiers);
}


void
TermView::DefaultState::WindowActivated(bool active)
{
	if (active)
		_CheckEnterHyperLinkState(fView->fModifiers);
}


bool
TermView::DefaultState::_CheckEnterHyperLinkState(int32 modifiers)
{
	if ((modifiers & B_COMMAND_KEY) != 0 && fView->Window()->IsActive()) {
		fView->_NextState(fView->fHyperLinkState);
		return true;
	}

	return false;
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
		if (fView->fSelection.RangeContains(inPos)) {
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
	const BMessage* message, int32 modifiers)
{
	if (_StandardMouseMoved(where, modifiers))
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
	} else if ((buttons & B_PRIMARY_MOUSE_BUTTON) == 0
		&& (fView->fMouseButtons & B_PRIMARY_MOUSE_BUTTON) != 0) {
		fView->Copy(fView->fMouseClipboard);
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


// #pragma mark - HyperLinkState


TermView::HyperLinkState::HyperLinkState(TermView* view)
	:
	State(view),
	fURLCharClassifier(kURLAdditionalWordCharacters),
	fPathComponentCharClassifier(
		BString(kDefaultAdditionalWordCharacters).RemoveFirst("/")),
	fCurrentDirectory(),
	fHighlight(),
	fHighlightActive(false)
{
	fHighlight.SetHighlighter(this);
}


void
TermView::HyperLinkState::Entered()
{
	ActiveProcessInfo activeProcessInfo;
	if (fView->GetActiveProcessInfo(activeProcessInfo))
		fCurrentDirectory = activeProcessInfo.CurrentDirectory();
	else
		fCurrentDirectory.Truncate(0);

	_UpdateHighlight();
}


void
TermView::HyperLinkState::Exited()
{
	_DeactivateHighlight();
}


void
TermView::HyperLinkState::ModifiersChanged(int32 oldModifiers, int32 modifiers)
{
	if ((modifiers & B_COMMAND_KEY) == 0)
		fView->_NextState(fView->fDefaultState);
	else
		_UpdateHighlight();
}


void
TermView::HyperLinkState::MouseDown(BPoint where, int32 buttons,
	int32 modifiers)
{
	TermPos start;
	TermPos end;
	HyperLink link;

	bool pathPrefixOnly = (modifiers & B_SHIFT_KEY) != 0;
	if (!_GetHyperLinkAt(where, pathPrefixOnly, link, start, end))
		return;

	if ((buttons & B_PRIMARY_MOUSE_BUTTON) != 0) {
		link.Open();
	} else if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		fView->fHyperLinkMenuState->Prepare(where, link);
		fView->_NextState(fView->fHyperLinkMenuState);
	}
}


void
TermView::HyperLinkState::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message, int32 modifiers)
{
	_UpdateHighlight(where, modifiers);
}


void
TermView::HyperLinkState::WindowActivated(bool active)
{
	if (!active)
		fView->_NextState(fView->fDefaultState);
}


void
TermView::HyperLinkState::VisibleTextBufferChanged()
{
	_UpdateHighlight();
}


rgb_color
TermView::HyperLinkState::ForegroundColor()
{
	return make_color(0, 0, 255);
}


rgb_color
TermView::HyperLinkState::BackgroundColor()
{
	return fView->fTextBackColor;
}


uint32
TermView::HyperLinkState::AdjustTextAttributes(uint32 attributes)
{
	return attributes | UNDERLINE;
}


bool
TermView::HyperLinkState::_GetHyperLinkAt(BPoint where, bool pathPrefixOnly,
	HyperLink& _link, TermPos& _start, TermPos& _end)
{
	TerminalBuffer* textBuffer = fView->fTextBuffer;
	BAutolock textBufferLocker(textBuffer);

	TermPos pos = fView->_ConvertToTerminal(where);

	// try to get a URL first
	BString text;
	if (!textBuffer->FindWord(pos, &fURLCharClassifier, false, _start, _end))
		return false;

	text.Truncate(0);
	textBuffer->GetStringFromRegion(text, _start, _end);
	text.Trim();

	// We're only happy, if it has a protocol part which we know.
	int32 colonIndex = text.FindFirst(':');
	if (colonIndex >= 0) {
		BString protocol(text, colonIndex);
		if (strstr(kKnownURLProtocols, protocol) != NULL) {
			_link = HyperLink(text, HyperLink::TYPE_URL);
			return true;
		}
	}

	// no obvious URL -- try file name
	if (!textBuffer->FindWord(pos, fView->fCharClassifier, false, _start, _end))
		return false;

	// In path-prefix-only mode we determine the end position anew by omitting
	// the '/' in the allowed word chars.
	if (pathPrefixOnly) {
		TermPos componentStart;
		TermPos componentEnd;
		if (textBuffer->FindWord(pos, &fPathComponentCharClassifier, false,
				componentStart, componentEnd)) {
			_end = componentEnd;
		} else {
			// That means pos points to a '/'. We simply use the previous
			// position.
			_end = pos;
			if (_start == _end) {
				// Well, must be just "/". Advance to the next position.
				if (!textBuffer->NextLinePos(_end, false))
					return false;
			}
		}
	}

	text.Truncate(0);
	textBuffer->GetStringFromRegion(text, _start, _end);
	text.Trim();
	if (text.IsEmpty())
		return false;

	// Collect a list of colons in the string and their respective positions in
	// the text buffer. We do this up-front so we can unlock the text buffer
	// while we're doing all the entry existence tests.
	typedef Array<CharPosition> ColonList;
	ColonList colonPositions;
	TermPos searchPos = _start;
	for (int32 index = 0; (index = text.FindFirst(':', index)) >= 0;) {
		TermPos foundStart;
		TermPos foundEnd;
		if (!textBuffer->Find(":", searchPos, true, true, false, foundStart,
				foundEnd)) {
			return false;
		}

		CharPosition colonPosition;
		colonPosition.index = index;
		colonPosition.position = foundStart;
		if (!colonPositions.Add(colonPosition))
			return false;

		index++;
		searchPos = foundEnd;
	}

	textBufferLocker.Unlock();

	// Since we also want to consider ':' a potential path delimiter, in two
	// nested loops we chop off components from the beginning respective the
	// end.
	BString originalText = text;
	TermPos originalStart = _start;
	TermPos originalEnd = _end;

	int32 colonCount = colonPositions.Count();
	for (int32 startColonIndex = -1; startColonIndex < colonCount;
			startColonIndex++) {
		int32 startIndex;
		if (startColonIndex < 0) {
			startIndex = 0;
			_start = originalStart;
		} else {
			startIndex = colonPositions[startColonIndex].index + 1;
			_start = colonPositions[startColonIndex].position;
			if (_start >= pos)
				break;
			_start.x++;
				// Note: This is potentially a non-normalized position (i.e.
				// the end of a soft-wrapped line). While not that nice, it
				// works anyway.
		}

		for (int32 endColonIndex = colonCount; endColonIndex > startColonIndex;
				endColonIndex--) {
			int32 endIndex;
			if (endColonIndex == colonCount) {
				endIndex = originalText.Length();
				_end = originalEnd;
			} else {
				endIndex = colonPositions[endColonIndex].index;
				_end = colonPositions[endColonIndex].position;
				if (_end <= pos)
					break;
			}

			originalText.CopyInto(text, startIndex, endIndex - startIndex);
			if (text.IsEmpty())
				continue;

			// check, whether the file exists
			BString actualPath;
			if (_EntryExists(text, actualPath)) {
				_link = HyperLink(text, actualPath, HyperLink::TYPE_PATH);
				return true;
			}

			// As such this isn't an existing path. We also want to recognize:
			// * "<path>:<line>"
			// * "<path>:<line>:<column>"

			BString path = text;

			for (int32 i = 0; i < 2; i++) {
				int32 colonIndex = path.FindLast(':');
				if (colonIndex <= 0 || colonIndex == path.Length() - 1)
					break;

				char* numberEnd;
				strtol(path.String() + colonIndex + 1, &numberEnd, 0);
				if (*numberEnd != '\0')
					break;

				path.Truncate(colonIndex);
				if (_EntryExists(path, actualPath)) {
					BString address = path == actualPath
						? text
						: BString(actualPath) << (text.String() + colonIndex);
					_link = HyperLink(text, address,
						i == 0
							? HyperLink::TYPE_PATH_WITH_LINE
							: HyperLink::TYPE_PATH_WITH_LINE_AND_COLUMN);
					return true;
				}
			}
		}
	}

	return false;
}


bool
TermView::HyperLinkState::_EntryExists(const BString& path,
	BString& _actualPath) const
{
	if (path.IsEmpty())
		return false;

	if (path[0] == '/' || fCurrentDirectory.IsEmpty()) {
		_actualPath = path;
	} else if (path == "~" || path.StartsWith("~/")) {
		// Replace '~' with the user's home directory. We don't handle "~user"
		// here yet.
		BPath homeDirectory;
		if (find_directory(B_USER_DIRECTORY, &homeDirectory) != B_OK)
			return false;
		_actualPath = homeDirectory.Path();
		_actualPath << path.String() + 1;
	} else {
		_actualPath.Truncate(0);
		_actualPath << fCurrentDirectory << '/' << path;
	}

	struct stat st;
	return lstat(_actualPath, &st) == 0;
}


void
TermView::HyperLinkState::_UpdateHighlight()
{
	BPoint where;
	uint32 buttons;
	fView->GetMouse(&where, &buttons, false);
	_UpdateHighlight(where, fView->fModifiers);
}


void
TermView::HyperLinkState::_UpdateHighlight(BPoint where, int32 modifiers)
{
	TermPos start;
	TermPos end;
	HyperLink link;

	bool pathPrefixOnly = (modifiers & B_SHIFT_KEY) != 0;
	if (_GetHyperLinkAt(where, pathPrefixOnly, link, start, end))
		_ActivateHighlight(start, end);
	else
		_DeactivateHighlight();
}


void
TermView::HyperLinkState::_ActivateHighlight(const TermPos& start,
	const TermPos& end)
{
	if (fHighlightActive) {
		if (fHighlight.Start() == start && fHighlight.End() == end)
			return;

		_DeactivateHighlight();
	}

	fHighlight.SetRange(start, end);
	fView->_AddHighlight(&fHighlight);
	BCursor cursor(B_CURSOR_ID_FOLLOW_LINK);
	fView->SetViewCursor(&cursor);
	fHighlightActive = true;
}


void
TermView::HyperLinkState::_DeactivateHighlight()
{
	if (fHighlightActive) {
		fView->_RemoveHighlight(&fHighlight);
		BCursor cursor(B_CURSOR_ID_SYSTEM_DEFAULT);
		fView->SetViewCursor(&cursor);
		fHighlightActive = false;
	}
}


// #pragma mark - HyperLinkMenuState


class TermView::HyperLinkMenuState::PopUpMenu : public BPopUpMenu {
public:
	PopUpMenu(const BMessenger& messageTarget)
		:
		BPopUpMenu("open hyperlink"),
		fMessageTarget(messageTarget)
	{
		SetAsyncAutoDestruct(true);
	}

	~PopUpMenu()
	{
		fMessageTarget.SendMessage(kMessageMenuClosed);
	}

private:
	BMessenger	fMessageTarget;
};


TermView::HyperLinkMenuState::HyperLinkMenuState(TermView* view)
	:
	State(view),
	fLink()
{
}


void
TermView::HyperLinkMenuState::Prepare(BPoint point, const HyperLink& link)
{
	fLink = link;

	// open context menu
	PopUpMenu* menu = new PopUpMenu(fView);
	BLayoutBuilder::Menu<> menuBuilder(menu);
	switch (link.GetType()) {
		case HyperLink::TYPE_URL:
			menuBuilder
				.AddItem(B_TRANSLATE("Open link"), kMessageOpenLink)
				.AddItem(B_TRANSLATE("Copy link location"), kMessageCopyLink);
			break;

		case HyperLink::TYPE_PATH:
		case HyperLink::TYPE_PATH_WITH_LINE:
		case HyperLink::TYPE_PATH_WITH_LINE_AND_COLUMN:
			menuBuilder.AddItem(B_TRANSLATE("Open path"), kMessageOpenLink);
			menuBuilder.AddItem(B_TRANSLATE("Copy path"), kMessageCopyLink);
			if (fLink.Text() != fLink.Address()) {
				menuBuilder.AddItem(B_TRANSLATE("Copy absolute path"),
					kMessageCopyAbsolutePath);
			}
			break;
	}
	menu->SetTargetForItems(fView);
	menu->Go(fView->ConvertToScreen(point), true, true, true);
}


void
TermView::HyperLinkMenuState::Exited()
{
	fLink = HyperLink();
}


bool
TermView::HyperLinkMenuState::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMessageOpenLink:
			if (fLink.IsValid())
				fLink.Open();
			return true;

		case kMessageCopyLink:
		case kMessageCopyAbsolutePath:
		{
			if (fLink.IsValid()) {
				BString toCopy = message->what == kMessageCopyLink
					? fLink.Text() : fLink.Address();

				if (!be_clipboard->Lock())
					return true;

				be_clipboard->Clear();

				if (BMessage *data = be_clipboard->Data()) {
					data->AddData("text/plain", B_MIME_TYPE, toCopy.String(),
						toCopy.Length());
					be_clipboard->Commit();
				}

				be_clipboard->Unlock();
			}
			return true;
		}

		case kMessageMenuClosed:
			fView->_NextState(fView->fDefaultState);
			return true;
	}

	return false;
}
