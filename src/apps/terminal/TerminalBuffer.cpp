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
	fEncoding(M_UTF8)
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
	BMessage message(MSG_SET_TERMNAL_TITLE);
	message.AddString("title", title);
	fListener.SendMessage(&message);
}


void
TerminalBuffer::NotifyQuit(int32 reason)
{
	BMessage message(MSG_QUIT_TERMNAL);
	message.AddInt32("reason", reason);
	fListener.SendMessage(&message);
}


void
TerminalBuffer::NotifyListener()
{
	fListener.SendMessage(MSG_TERMINAL_BUFFER_CHANGED);
}
