/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TerminalBuffer.h"

#include <algorithm>

#include <Message.h>

#include "Coding.h"
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
TerminalBuffer::SetEncoding(int encoding)
{
	fEncoding = encoding;
}


void
TerminalBuffer::SetTitle(const char* title)
{
	if (fListenerValid) {
		BMessage message(MSG_SET_TERMNAL_TITLE);
		message.AddString("title", title);
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

	TermPos cursor = fCursor;

	// Switch to the alternate screen buffer and resize it.
	if (fAlternateScreen != NULL) {
		_SwitchScreenBuffer();

		fWidth = oldWidth;
		fHeight = oldHeight;
		fCursor.SetTo(0, 0);

		error = BasicTerminalBuffer::ResizeTo(width, height, 0);
		if (error != B_OK) {
			// This sucks -- we can't do anything about it. Delete the
			// alternate screen buffer.
			_FreeLines(fAlternateScreen, oldHeight);
			fAlternateScreen = NULL;
		}

		// Switch back.
		if (!alternateScreenActive)
			_SwitchScreenBuffer();

		fWidth = width;
		fHeight = height;
		fCursor = cursor;
	}

	return error;
}


void
TerminalBuffer::UseAlternateScreenBuffer()
{
	if (fAlternateScreenActive || fAlternateScreen == NULL)
		return;

	_SwitchScreenBuffer();
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
