/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TERMINAL_BUFFER_H
#define TERMINAL_BUFFER_H

#include <SupportDefs.h>


class TerminalBuffer {
public:
								TerminalBuffer();
	virtual						~TerminalBuffer();

	virtual	int					Encoding() const = 0;

	// Output Character
	virtual	void				Insert(uchar* string, ushort attr) = 0;
	virtual	void				InsertCR() = 0;
	virtual	void				InsertLF() = 0;
	virtual	void				InsertNewLine(int num) = 0;
	virtual	void				SetInsertMode(int flag) = 0;
	virtual	void				InsertSpace(int num) = 0;
	
	// Delete Character
	virtual	void				EraseBelow() = 0;
	virtual	void				DeleteChar(int num) = 0;
	virtual	void				DeleteColumns() = 0;
	virtual	void				DeleteLine(int num) = 0;

	// Get and Set Cursor position
	virtual	void				SetCurPos(int x, int y) = 0;
	virtual	void				SetCurX(int x) = 0;
	virtual	void				SetCurY(int y) = 0;
	virtual	int					GetCurX() = 0;
	virtual	void				SaveCursor() = 0;
	virtual	void				RestoreCursor() = 0;

	// Move Cursor
	virtual	void				MoveCurRight(int num) = 0;
	virtual	void				MoveCurLeft(int num) = 0;
	virtual	void				MoveCurUp(int num) = 0;
	virtual	void				MoveCurDown(int num) = 0;

	// Cursor setting
	virtual	void				SetCurDraw(bool flag) = 0;

	// Scroll region
	virtual	void				ScrollRegion(int top, int bot, int dir,
									int num) = 0;
	virtual	void				SetScrollRegion(int top, int bot) = 0;
	virtual	void				ScrollAtCursor() = 0;

	// Other
	virtual	void				DeviceStatusReport(int) = 0;
	virtual	void				UpdateLine() = 0;

	virtual void				SetTitle(const char* title) = 0;
	virtual void				NotifyQuit(int32 reason) = 0;
};

#endif	// TERMINAL_BUFFER_H
