/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMINAL_BUFFER_H
#define TERMINAL_BUFFER_H

#include <Locker.h>
#include <Messenger.h>

#include "BasicTerminalBuffer.h"


class TerminalBuffer : public BasicTerminalBuffer, public BLocker {
public:
								TerminalBuffer();
	virtual						~TerminalBuffer();

			status_t			Init(int32 width, int32 height,
									int32 historySize);

			void				SetListener(BMessenger listener);

			int					Encoding() const;
			void				SetEncoding(int encoding);

			void				SetTitle(const char* title);
			void				NotifyQuit(int32 reason);

protected:
	virtual	void				NotifyListener();

private:
			int					fEncoding;

			// listener/dirty region management
			BMessenger			fListener;
};


#endif	// TERMINAL_BUFFER_H
