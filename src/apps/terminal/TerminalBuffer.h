/*
 * Copyright 2013-2023, Haiku, Inc. All rights reserved.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Simon South, simon@simonsouth.net
 *		Siarzhuk Zharski, zharik@gmx.li
 */
#ifndef TERMINAL_BUFFER_H
#define TERMINAL_BUFFER_H

#include <GraphicsDefs.h>
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
			void				SetColors(uint8* indexes, rgb_color* colors,
									int32 count = 1, bool dynamic = false);
			void				ResetColors(uint8* indexes,
									int32 count = 1, bool dynamic = false);
			void				GetColor(uint8 index);
			void				SetCursorStyle(int32 style);
			int32				CursorStyle() const { return fCursorStyle; };
			void				SetPaletteColor(uint8 index, rgb_color color);
			inline const rgb_color*
								Palette() const { return fColorsPalette; }

			void				NotifyQuit(int32 reason);

	virtual	status_t			ResizeTo(int32 width, int32 height);
	virtual	status_t			ResizeTo(int32 width, int32 height,
									int32 historyCapacity);

			void				UseAlternateScreenBuffer(bool clear);
			void				UseNormalScreenBuffer();

			bool				IsMode(uint32 mode) const;
			void				SetMode(uint32 mode);
			void				ResetMode(uint32 mode);

protected:
	virtual	void				NotifyListener();

private:
			void				_SwitchScreenBuffer();

private:
			int					fEncoding;

			TerminalLine**		fAlternateScreen;
			HistoryBuffer*		fAlternateHistory;
			int32				fAlternateScreenOffset;
			Attributes			fAlternateAttributes;
			rgb_color*			fColorsPalette;

			// listener/dirty region management
			BMessenger			fListener;
			bool				fListenerValid;

			uint32				fMode;
			int					fCursorStyle;
};


#endif	// TERMINAL_BUFFER_H
