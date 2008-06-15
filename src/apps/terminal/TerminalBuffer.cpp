/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "TerminalBuffer.h"

#include <Message.h>

#include "Coding.h"
#include "TermConst.h"


// #pragma mark - public methods


TerminalBuffer::TerminalBuffer()
	:
	BLocker("terminal buffer"),
	fEncoding(M_UTF8),
	fListenerValid(false)
{
}


TerminalBuffer::~TerminalBuffer()
{
}


status_t
TerminalBuffer::Init(int32 width, int32 height, int32 historySize)
{
	if (Sem() < 0)
		return Sem();

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
