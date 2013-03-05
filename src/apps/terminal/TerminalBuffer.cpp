/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TerminalBuffer.h"

#include <algorithm>

#include <Message.h>

#include "TermConst.h"


// #pragma mark - public methods


TerminalBuffer::TerminalBuffer()
	:
	BLocker("terminal buffer"),
	fEncoding(M_UTF8),
	fAlternateScreen(NULL),
	fAlternateHistory(NULL),
	fAlternateScreenOffset(0),
	fListenerValid(false)
{
}


TerminalBuffer::~TerminalBuffer()
{
	delete fAlternateScreen;
	delete fAlternateHistory;
}


status_t
TerminalBuffer::Init(int32 width, int32 height, int32 historySize)
{
	if (Sem() < 0)
		return Sem();

	fAlternateScreen = _AllocateLines(width, height);
	if (fAlternateScreen == NULL)
		return B_NO_MEMORY;

	for (int32 i = 0; i < height; i++)
		fAlternateScreen[i]->Clear();

	return BasicTerminalBuffer::Init(width, height, historySize);
}


void
TerminalBuffer::SetListener(BMessenger listener)
{
	fListener = listener;
	fListenerValid = true;
}


void
TerminalBuffer::UnsetListener()
{
	fListenerValid = false;
}


int
TerminalBuffer::Encoding() const
{
	return fEncoding;
}


void
TerminalBuffer::ReportX10MouseEvent(bool reportX10MouseEvent)
{
	if (fListenerValid) {
		BMessage message(MSG_REPORT_MOUSE_EVENT);
		message.AddBool("reportX10MouseEvent", reportX10MouseEvent);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::ReportNormalMouseEvent(bool reportNormalMouseEvent)
{
	if (fListenerValid) {
		BMessage message(MSG_REPORT_MOUSE_EVENT);
		message.AddBool("reportNormalMouseEvent", reportNormalMouseEvent);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::ReportButtonMouseEvent(bool report)
{
	if (fListenerValid) {
		BMessage message(MSG_REPORT_MOUSE_EVENT);
		message.AddBool("reportButtonMouseEvent", report);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::ReportAnyMouseEvent(bool reportAnyMouseEvent)
{
	if (fListenerValid) {
		BMessage message(MSG_REPORT_MOUSE_EVENT);
		message.AddBool("reportAnyMouseEvent", reportAnyMouseEvent);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::SetEncoding(int encoding)
{
	fEncoding = encoding;
}


void
TerminalBuffer::SetTitle(const char* title)
{
	if (fListenerValid) {
		BMessage message(MSG_SET_TERMINAL_TITLE);
		message.AddString("title", title);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::SetColors(uint8* indexes, rgb_color* colors,
		int32 count, bool dynamic)
{
	if (fListenerValid) {
		BMessage message(MSG_SET_TERMINAL_COLORS);
		message.AddInt32("count", count);
		message.AddBool("dynamic", dynamic);
		message.AddData("index", B_UINT8_TYPE,
					indexes, sizeof(uint8), true, count);
		message.AddData("color", B_RGB_COLOR_TYPE,
					colors, sizeof(rgb_color), true, count);

		for (int i = 1; i < count; i++) {
			message.AddData("index", B_UINT8_TYPE, &indexes[i], sizeof(uint8));
			message.AddData("color", B_RGB_COLOR_TYPE, &colors[i],
					sizeof(rgb_color));
		}

		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::ResetColors(uint8* indexes, int32 count, bool dynamic)
{
	if (fListenerValid) {
		BMessage message(MSG_RESET_TERMINAL_COLORS);
		message.AddInt32("count", count);
		message.AddBool("dynamic", dynamic);
		message.AddData("index", B_UINT8_TYPE,
					indexes, sizeof(uint8), true, count);

		for (int i = 1; i < count; i++)
			message.AddData("index", B_UINT8_TYPE, &indexes[i], sizeof(uint8));

		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::SetCursorStyle(int32 style, bool blinking)
{
	if (fListenerValid) {
		BMessage message(MSG_SET_CURSOR_STYLE);
		message.AddInt32("style", style);
		message.AddBool("blinking", blinking);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::SetCursorBlinking(bool blinking)
{
	if (fListenerValid) {
		BMessage message(MSG_SET_CURSOR_STYLE);
		message.AddBool("blinking", blinking);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::SetCursorHidden(bool hidden)
{
	if (fListenerValid) {
		BMessage message(MSG_SET_CURSOR_STYLE);
		message.AddBool("hidden", hidden);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::NotifyQuit(int32 reason)
{
	if (fListenerValid) {
		BMessage message(MSG_QUIT_TERMNAL);
		message.AddInt32("reason", reason);
		fListener.SendMessage(&message);
	}
}


void
TerminalBuffer::NotifyListener()
{
	if (fListenerValid)
		fListener.SendMessage(MSG_TERMINAL_BUFFER_CHANGED);
}


status_t
TerminalBuffer::ResizeTo(int32 width, int32 height)
{
	int32 historyCapacity = 0;
	if (!fAlternateScreenActive)
		historyCapacity = HistoryCapacity();
	else if (fAlternateHistory != NULL)
		historyCapacity = fAlternateHistory->Capacity();

	return ResizeTo(width, height, historyCapacity);
}


status_t
TerminalBuffer::ResizeTo(int32 width, int32 height, int32 historyCapacity)
{
	// switch to the normal screen buffer first
	bool alternateScreenActive = fAlternateScreenActive;
	if (alternateScreenActive)
		_SwitchScreenBuffer();

	int32 oldWidth = fWidth;
	int32 oldHeight = fHeight;

	// Resize the normal screen buffer/history.
	status_t error = BasicTerminalBuffer::ResizeTo(width, height,
		historyCapacity);
	if (error != B_OK) {
		if (alternateScreenActive)
			_SwitchScreenBuffer();
		return error;
	}

	// Switch to the alternate screen buffer and resize it.
	if (fAlternateScreen != NULL) {
		TermPos cursor = fCursor;
		fCursor.SetTo(0, 0);
		fWidth = oldWidth;
		fHeight = oldHeight;

		_SwitchScreenBuffer();

		error = BasicTerminalBuffer::ResizeTo(width, height, 0);

		fWidth = width;
		fHeight = height;
		fCursor = cursor;

		// Switch back.
		if (!alternateScreenActive)
			_SwitchScreenBuffer();

		if (error != B_OK) {
			// This sucks -- we can't do anything about it. Delete the
			// alternate screen buffer.
			_FreeLines(fAlternateScreen, oldHeight);
			fAlternateScreen = NULL;
		}
	}

	return error;
}


void
TerminalBuffer::UseAlternateScreenBuffer(bool clear)
{
	if (fAlternateScreenActive || fAlternateScreen == NULL)
		return;

	_SwitchScreenBuffer();

	if (clear)
		Clear(false);

	_InvalidateAll();
}


void
TerminalBuffer::UseNormalScreenBuffer()
{
	if (!fAlternateScreenActive)
		return;

	_SwitchScreenBuffer();
	_InvalidateAll();
}


void
TerminalBuffer::_SwitchScreenBuffer()
{
	std::swap(fScreen, fAlternateScreen);
	std::swap(fHistory, fAlternateHistory);
	std::swap(fScreenOffset, fAlternateScreenOffset);
	fAlternateScreenActive = !fAlternateScreenActive;
}
