/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#ifndef _TERMBUFFER_H
#define _TERMBUFFER_H

#include <SupportDefs.h>
#include "TermConst.h"
#include "CurPos.h"


#define MIN_COLS 10
#define MAX_COLS 256


struct term_buffer
{
	uchar code[4];
	int16 attr;
	int16 status;
};


class CurPos;
class BString;

class TermBuffer {
public:
	TermBuffer(int row, int col);
	~TermBuffer();

	//
	// Get and Put charactor.
	//
	int	GetChar(int row, int col, uchar *u, ushort *attr);
	int	GetString(int row, int col, int num, uchar *u, ushort *attr);
	void 	GetStringFromRegion (BString &copyStr);  

	void	WriteChar(const CurPos &pos, const uchar *u, ushort attr);
	void	WriteCR(const CurPos &pos);
	void	InsertSpace(const CurPos &pos, int num);

	//
	// Delete Character.
	//
	void	DeleteChar(const CurPos &pos, int num);
	void	EraseBelow(const CurPos &pos);

	//
	// Movement and Scroll buffer.
	//
	void	ScrollRegion(int top, int bot, int dir, int num);
	void	ScrollLine();
		
	//
	// Resize buffer.
	//
	void	ResizeTo(int newRows, int newCols, int offset);

	//
	// Clear contents of TermBuffer.
	//
	void	Clear();
	
	//
	// Selection Methods
	//
	void	Select(const CurPos &start, const CurPos &end);
	void	DeSelect();
	
	const CurPos &GetSelectionStart() { return fSelStart; };
	const CurPos &GetSelectionEnd()   { return fSelEnd; };

	int	CheckSelectedRegion (const CurPos &pos);
	  
	//
	// Other methods
	//
	bool	FindWord (const CurPos &pos, CurPos *start, CurPos *end);

	void	GetCharFromRegion (int x, int y, BString &str);
static	void	AvoidWaste (BString &str);
	 
	void	ToString(BString &str);

	int32	Size() const;
	  
private:
	void	EraseLine(int line);

	term_buffer 	**fBuffer;
	int		fBufferSize;

	int		fColumnSize;
	int		fRowSize;
	int		fCurrentColumnSize;
	int		fRowOffset;

	CurPos		fSelStart;
	CurPos		fSelEnd;	
};

#endif // _TERMBUFFER_H
