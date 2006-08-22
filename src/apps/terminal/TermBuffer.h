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
#ifndef TERMBUFFER_H_INCLUDED
#define TERMBUFFER_H_INCLUDED

#include <SupportDefs.h>
#include "TermConst.h"
#include "CurPos.h"

// Scroll direction
const int array_up = 0;
const int array_down = 1;

#define ARRAY_SIZE 512

#define MIN_COLS 10
#define MAX_COLS 256

#define ROW(x) (((x) + mRowOffset) % buffer_size)

typedef struct term_buffer
{
  uchar code[4];
  int16 attr;
  int16 status;
} term_buffer;

class CurPos;
class BString;

class TermBuffer
{
public:
  TermBuffer(int row, int col);
  ~TermBuffer();

  //
  // Get and Put charactor.
  //
  int	GetChar (int row, int col, uchar *u, ushort *attr);

  int	GetString (int row, int col, int num, uchar *u, ushort *attr);
  void  GetStringFromRegion (BString &copyStr);  

  void	WriteChar (const CurPos &pos, const uchar *u, ushort attr);
  void	WriteCR (const CurPos &pos);
  void	InsertSpace (const CurPos &pos, int num);

  //
  // Delete Charactor.
  //
  void	DeleteChar (const CurPos &pos, int num);
  void	EraseBelow (const CurPos &pos);

  //
  // Movement and Scroll buffer.
  //
  void	ScrollRegion (int top, int bot, int dir, int num);
  void	ScrollLine (void);
	
  //
  // Resize buffer.
  //
  void	ResizeTo(int newRows, int newCols, int offset);

  //
  // Clear contents of TermBuffer.
  //
  void	ClearAll (void);

  //
  // Selection Methods
  //
  void	Select (const CurPos &start, const CurPos &end);
  void	DeSelect (void);
  
  const CurPos &GetSelectionStart (void) { return mSelStart; };
  const CurPos &GetSelectionEnd (void)   { return mSelEnd; };

  int	CheckSelectedRegion (const CurPos &pos);
  
  //
  // Other methods
  //
  bool	FindWord (const CurPos &pos, CurPos *start, CurPos *end);

  void	GetCharFromRegion (int x, int y, BString &str);
  void	AvoidWaste (BString &str);
  
  void  ToString (BString &str);

  int32 GetArraySize (void);
  
  /*
   * PRIVATE MEMBER FUNCTIONS.
   */
private:

  //
  // Erase 1 column.
  //
  void	EraseLine (int row);

  /*
   * DATA MEMBER.
   */
  term_buffer 	**mBase;
  
  int		mColSize;
  int		mRowSize;
  int		mNowColSize;
  int		mRowOffset;

  CurPos	mSelStart;
  CurPos	mSelEnd;

  int		buffer_size;
	
};

#endif //TERMARAY_H_INCLUDED
