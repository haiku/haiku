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
			void				UnsetListener();

			int					Encoding() const;
			void				SetEncoding(int encoding);

			void				SetTitle(const char* title);
			void				NotifyQuit(int32 reason);

	virtual	status_t			ResizeTo(int32 width, int32 height);
	virtual	status_t			ResizeTo(int32 width, int32 height,
									int32 historyCapacity);

			void				UseAlternateScreenBuffer(bool clear);
			void				UseNormalScreenBuffer();

			void				ReportX10MouseEvent(bool report);
			void				ReportNormalMouseEvent(bool report);
			void				ReportButtonMouseEvent(bool report);
			void				ReportAnyMouseEvent(bool report);

protected:
	virtual	void				NotifyListener();

private:
			void				_SwitchScreenBuffer();

private:
			int					fEncoding;

			TerminalLine**		fAlternateScreen;
			HistoryBuffer*		fAlternateHistory;
			int32				fAlternateScreenOffset;

			// listener/dirty region management
			BMessenger			fListener;
			bool				fListenerValid;
};


#endif	// TERMINAL_BUFFER_H
