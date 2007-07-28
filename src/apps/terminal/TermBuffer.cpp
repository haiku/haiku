/*
 * Copyright (c) 2001-2006, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Kian Duffy <myob@users.sourceforge.net>
 */

/************************************************************************
MuTerminal Internal Buffer Format.

a. 4bytes character buffer.

 3bytes character buffer (UTF8).
 1byte status buffer.

b. 2bytes extended buffer.

 2byte attribute buffer.

< attribute buffer>  <------- character buffer -------->
WBURMfbF rrfffbbb / ssssssss 11111111 22222222 33333333
|||||||| || |  |       |       	 |     	 |     	  |
|||||||| || |  |     status  utf8 1st utf8 2nd utf8 3rd
|||||||| || |  |
|||||||| || |  \--> background color
|||||||| || \-----> foreground color
|||||||| |\-------> dumped CR
|||||||| \--------> Reserve
||||||||
||||||||
|||||||\----------> Font information (Not use)
||||||\-----------> background color flag
|||||\------------> foreground color flag
||||\-------------> mouse selected
|||\--------------> reverse attr
||\---------------> underline attr
|\----------------> bold attr
\-----------------> character width

c. 2byte status buffer.

0x00 (A_CHAR):    character available.
0x01 (NO_CHAR):   buffer is empty.
0xFF (IN_STRING): before buffer is full width character (never draw).

**Language environment. (Not impliment)

--- half width character set ---
0 ... Western (Latin1 / ISO-8859-1, MacRoman)
1 ... Central European(Latin2 / ISO-8859-2)
2 ... Turkish (Latin3 / ISO-8859-3)
3 ... Baltic (Latin4 / ISO-8859-4)
4 ... Cyrillic (Cyrillic / ISO-8859-5)
5 ... Greek (Greek / ISO-8859-7)
6 ... Trukish (Latin5 / ISO-8859-9)

--- full width character set ---
7 ... Japanese (EUC, SJIS, ISO-2022-jp / JIS X 0201, 0208, 0212)
8 ... Chinese (Big5, EUC-tw, ISO-2022-cn / GB2312, CNS-11643-1...7)
9 ... Korean (EUC-kr, ISO-2022-kr / KS C 5601)

--- Universal character set ---
10 ... Unicode (UTF8)

  * Variables is set CodeConv class.
  * Unicode character width sets CodeConv::UTF8FontWidth member.

JIS X 0201 (half width kana ideograph) character use 1 column,
but it font is full width font on preference panel.

************************************************************************/

#include "TermBuffer.h"

#include "CurPos.h"
#include "TermConst.h"

#include <SupportDefs.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ROW(x) (((x) + fRowOffset) % fBufferSize)


TermBuffer::TermBuffer(int rows, int cols, int bufferSize)
{
	if (rows < 1)
		rows = 1;
	if (cols < MIN_COLS)
		cols = MIN_COLS;
	else if (cols > MAX_COLS)
		cols = MAX_COLS;

	fColumnSize = MAX_COLS;
	fCurrentColumnSize = cols;
	fRowSize = rows;

	fRowOffset = 0;
	
	fSelStart.Set(-1,-1);
	fSelEnd.Set(-1,-1);

	fBufferSize = bufferSize;
	if (fBufferSize < 1000)
		fBufferSize = 1000;

	// Allocate buffer
	fBuffer = (term_buffer**)malloc(sizeof(term_buffer*) * fBufferSize);

	for (int i = 0; i < fBufferSize; i++) {
		fBuffer[i] = (term_buffer *)calloc(fColumnSize + 1, sizeof(term_buffer));
	}
}


TermBuffer::~TermBuffer()
{
	for (int i = 0; i < fBufferSize; i++) {
		free(fBuffer[i]);
	}

	free(fBuffer);
}


//! Gets a character from TermBuffer
int
TermBuffer::GetChar(int row, int col, uchar *buf, ushort *attr)
{
	if (row < 0 || col < 0)
		return -1;

	term_buffer *ptr = (fBuffer[row % fBufferSize] + col);

	if (ptr->status == A_CHAR)
		memcpy(buf, (char *)(ptr->code), 4);

	*attr = ptr->attr;

	return ptr->status;
}


//! Get a string (length = num) from given position.
int
TermBuffer::GetString(int row, int col, int num, uchar *buf, 
	ushort *attr)
{
	int count = 0, all_count = 0;
	term_buffer *ptr;

	ptr = (fBuffer[row % fBufferSize]);
	ptr += col;
	*attr = ptr->attr;

	if (ptr->status == NO_CHAR) {
		// Buffer is empty and No selected by mouse.
		do {
			if (col >= fCurrentColumnSize)
				return -1;

			col++;
			ptr++;
			count--;
			if (ptr->status == A_CHAR)
				return count;

			if (fSelStart.y == row) {
				if (col == fSelStart.x)
					return count;
			}

			if (fSelEnd.y == row) {
				if (col - 1 == fSelEnd.x)
					return count;
			}
		} while (col <= num);

		return count;
	} else if (IS_WIDTH(ptr->attr)) {
		memcpy(buf, (char *)ptr->code, 4);
		return 2;
	}

	while (col <= num) {
		memcpy(buf, ptr->code, 4);
		all_count++;

		if (*buf == 0) {
			*buf = ' ';
			*(buf + 1) = '\0';
		}
		if (ptr->attr != (ptr + 1)->attr)
			return all_count;

		buf = (uchar *)index((char *)buf, 0x00);
		ptr++;
		col++;

		if (fSelStart.y == row) {
			if (col == fSelStart.x)
				return all_count;
		}
		if (fSelEnd.y == row) {
			if (col - 1 == fSelEnd.x)
				return all_count;
		}
	}

	return all_count;
}


//! Write a character at the cursor point.
void
TermBuffer::WriteChar(const CurPos &pos, const uchar *u, ushort attr)
{
	const int row = pos.y;
	const int col = pos.x;

	term_buffer *ptr = (fBuffer[ROW(row)] + col);
	memcpy ((char *)ptr->code, u, 4);

	if (IS_WIDTH(attr))
		(ptr + 1)->status = IN_STRING;

	ptr->status = A_CHAR;
	ptr->attr = attr;
}


//! Write CR status to buffer attribute.
void
TermBuffer::WriteCR(const CurPos &pos)
{
	int row = pos.y;
	int col = pos.x;

	term_buffer *ptr = (fBuffer[ROW(row)] + col);
	ptr->attr |= DUMPCR;
}


//! Insert 'num' spaces at cursor point.
void
TermBuffer::InsertSpace(const CurPos &pos, int num)
{
	const int row = pos.y;
	const int col = pos.x;

	for (int i = fCurrentColumnSize - num; i >= col; i--) {
		*(fBuffer[ROW(row)] + i + num) = *(fBuffer[ROW(row)] + i);
	}

	memset(fBuffer[ROW(row)] + col, 0, num * sizeof(term_buffer));
}


//! Delete 'num' characters at cursor point.
void
TermBuffer::DeleteChar(const CurPos &pos, int num)
{
	const int row = pos.y;
	const int col = pos.x;

	term_buffer *ptr = fBuffer[ROW(row)];
	size_t movesize = fCurrentColumnSize - (col + num);

	memmove(ptr + col, ptr + col + num, movesize * sizeof(term_buffer));
	memset(ptr + (fCurrentColumnSize - num), 0, num * sizeof(term_buffer));
}


//! Erase characters below cursor position.
void
TermBuffer::EraseBelow(const CurPos &pos)
{
	const int row = pos.y;
	const int col = pos.x;

	memset(fBuffer[ROW(row)] + col, 0, (fColumnSize - col ) * sizeof(term_buffer));

	for (int i = row; i < fRowSize; i++) {
		_EraseLine(i);
	}
}


//! Scroll the terminal buffer region
void
TermBuffer::ScrollRegion(int top, int bot, int dir, int num)
{
	if (dir == SCRUP) {
		for (int i = 0; i < num; i++) {
			term_buffer *ptr = fBuffer[ROW(top)];

			for (int j = top; j < bot; j++) {
				fBuffer[ROW(j)] = fBuffer[ROW(j+1)];
			}

			fBuffer[ROW(bot)] = ptr;
			_EraseLine(bot);
		}
	} else {
		// scroll up
		for (int i = 0; i < num; i++) {
			term_buffer *ptr = fBuffer[ROW(bot)];

			for (int j = bot; j > top; j--) {
				fBuffer[ROW(j)] = fBuffer[ROW(j-1)];
			}

			fBuffer[ROW(top)] = ptr;
			_EraseLine(top);
		}
	}
}


//! Scroll the terminal buffer.
void
TermBuffer::ScrollLine()
{
	for (int i = fRowSize; i < fRowSize * 2; i++) {
		_EraseLine(i);
	}

	fRowOffset++;
}


//! Resize the terminal buffer.
void
TermBuffer::ResizeTo(int newRows, int newCols, int offset)
{
	int i;

	// make sure the new size is within the allowed limits
	if (newRows < 1)
		newRows = 1;

	if (newCols < MIN_COLS)
		newCols = MIN_COLS;
	else if (newCols > MAX_COLS)
		newCols = MAX_COLS;

	if (newRows <= fRowSize) {
		for (i = newRows; i <= fRowSize; i++)
			_EraseLine(i);
	} else {
		for (i = fRowSize; i <= newRows * 2; i++)
			_EraseLine(i);
	}

	fCurrentColumnSize = newCols;
	fRowOffset += offset;
	fRowSize = newRows;
}


//! Get the buffer's size.
int32
TermBuffer::Size() const
{
	return fBufferSize;
}


void
TermBuffer::_EraseLine(int row)
{
	memset(fBuffer[ROW(row)], 0, fColumnSize * sizeof(term_buffer));
}


//! Clear the contents of the TermBuffer.
void
TermBuffer::Clear()
{
	for (int i = 0; i < fBufferSize; i++) {
		memset(fBuffer[i], 0, fColumnSize * sizeof(term_buffer));
	}

	fRowOffset = 0;
	DeSelect();
}


//! Mark text in the given range as selected
void
TermBuffer::Select(const CurPos &start, const CurPos &end)
{
	if (end < start) {
		fSelStart = end;
		fSelEnd = start;
	} else {
		fSelStart = start;
		fSelEnd = end;
	}
}


//! Mark text in the given range as not selected
void
TermBuffer::DeSelect()
{
	fSelStart.Set(-1, -1);
	fSelEnd.Set(-1, -1);
}



bool
TermBuffer::FindWord(const CurPos &pos, CurPos *start, CurPos *end)
{
	uchar buf[5];
	ushort attr;
	int x, y, start_x;

	y = pos.y;

	// Search start point
	for (x = pos.x - 1; x >= 0; x--) {
		if (GetChar(y, x, buf, &attr) == NO_CHAR || *buf == ' ') {
			++x;
			break;
		}
	}
	start_x = x;

	// Search end point
	for (x = pos.x; x < fColumnSize; x++) {
		if (GetChar(y, x, buf, &attr) == NO_CHAR || *buf == ' ') {
			--x;
			break;
		}
	}

	if (start_x > x)
		return false;

	if (start != NULL)
		start->Set(start_x, y);

	if (end != NULL)
		end->Set(x, y);

	return true;
}


//! Get one character from the selected region.
void
TermBuffer::GetCharFromRegion(int x, int y, BString &str)
{
	uchar buf[5];
	ushort attr;
	int status;

	status = GetChar(y, x, buf, &attr);

	switch (status) {
		case NO_CHAR:
			if (IS_CR (attr))
				str += '\n';
			else
				str += ' ';
			break;

		case IN_STRING:
			break;

		default:
			str += (const char *)&buf;
			break;
	}
}


//! Delete useless char at end of line and convert LF code.
/* static */
inline void
TermBuffer::AvoidWaste(BString &str)
{
	// TODO: Remove the goto

	int32 len, point;

start:
	len = str.Length() - 1;
	point = str.FindLast (' ');

	if (len > 0 && len == point) {
		str.RemoveLast (" ");
		goto start;
	}
	//  str += '\n';
}


//! Get a string from the given region.
void
TermBuffer::GetStringFromRegion(BString &str, const CurPos &start, const CurPos &end)
{
	int y;

	if (start.y == end.y) {
		y = start.y;
		for (int x = start.x ; x <= end.x; x++) {
			GetCharFromRegion(x, y, str);
		}
	} else {
		y = start.y;
		for (int x = start.x ; x < fCurrentColumnSize; x++) {
			GetCharFromRegion(x, y, str);
		}
		AvoidWaste(str);

		for (y = start.y + 1 ; y < end.y; y++) {
			for (int x = 0 ; x < fCurrentColumnSize; x++) {
				GetCharFromRegion(x, y, str);
			}
			AvoidWaste(str);
		}

		y = end.y;
		for (int x = 0 ; x <= end.x; x++) {
			GetCharFromRegion(x, y, str);
		}
	}
}

//Returns the complete internal buffer as a BString
void
TermBuffer::ToString(BString &str)
{
	for (int y = 0; y < fRowSize; y++) {
		for (int x = 0; x < fCurrentColumnSize; x++) {
			GetCharFromRegion(x, y, str);
		}
	}
	AvoidWaste(str);
}
