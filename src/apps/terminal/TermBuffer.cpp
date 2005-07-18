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
0 ... Wesern (Latin1 / ISO-8859-1, MacRoman)
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

  * Varriables is set CodeConv class.
  * Unicode character width sets CodeConv::UTF8FontWidth member.

JIS X 0201 (half width kana ideograph) character use 1 column,
but it font is full width font on preference panel.

************************************************************************/

#include <SupportDefs.h>
#include <String.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <bsd_mem.h>

#include "TermBuffer.h"
#include "TermConst.h"
#include "CurPos.h"
#include "PrefHandler.h"

extern PrefHandler *gTermPref;


////////////////////////////////////////////////////////////////////////////
//
// TermBuffer class
//	Constructor and Destructor.
//
////////////////////////////////////////////////////////////////////////////
TermBuffer::TermBuffer(int rows, int cols)
{
  mColSize = MAX_COLS;
  mNowColSize = cols;
  mRowSize = rows;

  mRowOffset = 0;

  //buffer_size = ARRAY_SIZE;
  buffer_size  = gTermPref->getInt32 (PREF_HISTORY_SIZE);

  /* Allocate buffer */
  mBase = (term_buffer**)  malloc (sizeof (term_buffer*)  * buffer_size);

  for(int i = 0; i < buffer_size; i++){
    mBase[i] = (term_buffer *) calloc (mColSize + 1, sizeof (term_buffer));
  }
}
////////////////////////////////////////////////////////////////////////////
// TermBuffer Destructer.
//
////////////////////////////////////////////////////////////////////////////
TermBuffer::~TermBuffer()
{
  for(int i = 0; i < buffer_size; i++){
    free (mBase[i]);
  }

  free(mBase);

}
////////////////////////////////////////////////////////////////////////////
// int GetChar (row, col, *buf, attr)
//	Get chracter from TermBuffer.
////////////////////////////////////////////////////////////////////////////
/* This function moved to header file for effeciency */
int
TermBuffer::GetChar (int row, int col, uchar *buf, ushort *attr)
{
  term_buffer *ptr;
  
  ptr = (mBase[row % buffer_size] + col);

  if (ptr->status == A_CHAR)
    memcpy (buf, (char *)(ptr->code), 4);

  *attr = ptr->attr;
  
  return ptr->status;
}

////////////////////////////////////////////////////////////////////////////
// int GetString (row, col, *buf, attr)
//	Get String (length = num) from given position.
////////////////////////////////////////////////////////////////////////////
int
TermBuffer::GetString (int row, int col, int num,
		       uchar *buf, ushort *attr)
{
  int count = 0, all_count = 0;
  term_buffer *ptr;

  ptr = (mBase[row % buffer_size]);
  ptr += col;
  *attr = ptr->attr;

  if (ptr->status == NO_CHAR) {
    /*
     * Buffer is empty and No selected by mouse.
     */
    do {
      if (col >= mNowColSize) return -1;
    
      col++;
      ptr++;
      count--;
      if (ptr->status == A_CHAR)
	return count;

      if (mSelStart.y == row) {
	if (col == mSelStart.x)
	  return count;
      }
      if (mSelEnd.y == row) {
	if (col - 1 == mSelEnd.x)
	  return count;
      }
	
      
    }while (col <= num);
    return count;
  }
  else if (IS_WIDTH(ptr->attr)) {
    memcpy (buf, (char *)ptr->code, 4);
    return 2;
  }
  
  while (col <= num) {
    memcpy (buf, ptr->code, 4);
    all_count++;

    if (*buf == 0) {
      *buf = ' ';
      *(buf + 1) = '\0';
    }
    if (ptr->attr != (ptr + 1)->attr){
      return all_count;
    }

    buf = (uchar *)index ((char *)buf, 0x00);
    ptr++;
    col++;

      if (mSelStart.y == row) {
	if (col == mSelStart.x)
	  return all_count;
      }
      if (mSelEnd.y == row) {
	if (col - 1 == mSelEnd.x)
	  return all_count;
      }
  }
  
  return all_count;
}

////////////////////////////////////////////////////////////////////////////
// void WriteChar (const CurPos &, const uchar *, ushort)
//	Write character at cursor point.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::WriteChar (const CurPos &pos, const uchar *u, ushort attr)
{
  term_buffer *ptr;
  
  const int row = pos.y;
  const int col = pos.x;
  
  ptr = (mBase[ROW(row)] + col);
  memcpy ((char *)ptr->code, u, 4);
  if (IS_WIDTH(attr))
    (ptr + 1)->status = IN_STRING;

  ptr->status = A_CHAR;
  ptr->attr = attr;

  return;

}

////////////////////////////////////////////////////////////////////////////
// void WriteCR (const CurPos &)
//	Write CR status to buffer attribute.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::WriteCR (const CurPos &pos)
{
  int row = pos.y;
  int col = pos.x;
  
  term_buffer *ptr = (mBase[ROW(row)] + col);
  ptr->attr |= DUMPCR;
  
}


////////////////////////////////////////////////////////////////////////////
// InsertSpace (const CurPos &, int)
//	Insert num space at cursor point.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::InsertSpace (const CurPos &pos, int num)
{
  const int row = pos.y;
  const int col = pos.x;

  for (int i = mNowColSize - num; i >= col; i--) {
    *(mBase[ROW(row)] + i + num) = *(mBase[ROW(row)] + i);
  }

  memset(mBase[ROW(row)] + col, 0, num * sizeof(term_buffer));

  return;

}
////////////////////////////////////////////////////////////////////////////
// void DeleteChar (const CurPos &, int)
// Delete num character at cursor point.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::DeleteChar(const CurPos &pos, int num)
{
  const int row = pos.y;
  const int col = pos.x;
  
  term_buffer *ptr = mBase[ROW(row)];
  size_t movesize = mNowColSize - (col + num);

  memmove (ptr + col, ptr + col + num, movesize * sizeof(term_buffer));
  memset (ptr + (mNowColSize - num), 0, num * sizeof(term_buffer));

  return;
  
}

////////////////////////////////////////////////////////////////////////////
// EraseBelow (const CurPos &)
//	Erase below character from cursor position.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::EraseBelow(const CurPos &pos)
{
  const int row = pos.y;
  const int col = pos.x;

  memset (mBase[ROW(row)] + col, 0, (mColSize - col ) * sizeof (term_buffer));

 for (int i = row; i < mRowSize; i++)
    EraseLine (i);
}

////////////////////////////////////////////////////////////////////////////
// ScrollRegion (int, int, int, int)
//	Scroll terminal buffer region.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::ScrollRegion(int top, int bot, int dir, int num)
{

  if(dir == SCRUP){
    for(int i = 0; i < num; i++){

      term_buffer *ptr = mBase[ROW(top)];

      for(int j = top; j < bot; j++){
	    mBase[ROW(j)] = mBase[ROW(j+1)];
      }
      mBase[ROW(bot)] = ptr;
      
      EraseLine(bot);
    }
  }else{
    // scroll up
    for(int i = 0; i < num; i++){
      term_buffer *ptr = mBase[ROW(bot)];
      
      for(int j = bot; j > top; j--){
	    mBase[ROW(j)] = mBase[ROW(j-1)];
      }
      mBase[ROW(top)] = ptr;
      
      EraseLine(top);
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// ScrollLine (void)
//	Scroll terminal buffer.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::ScrollLine(void)
{
  for (int i = mRowSize; i < mRowSize * 2; i++) {
    EraseLine (i);
  }
  
  mRowOffset++;

  return;

}
////////////////////////////////////////////////////////////////////////////
// ResizeTo (int, int, int)
//	Resize terminal buffer.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::ResizeTo(int newRows, int newCols, int offset)
{

  int i;
  
#if 0  
  if (newCols > mColSize) {
    for (i = 0; i < buffer_size; i++) {
      mBase[i] = (ulong *) realloc (mBase[i], sizeof(ulong)  * newCols);
      base = mBase[i] + mColSize;
      for (j= 0; j < newCols - mColSize; j++) {
	*base++ = 0;
      }
    }
    mColSize = newCols;
  }
#endif
  if ( newRows <= mRowSize) {
    for (i = newRows; i <= mRowSize; i++) {
      EraseLine (i);
    }
  } else {
    for (i = mRowSize; i <= newRows * 2; i++) {
      EraseLine (i);
    }
  }
  
  mNowColSize = newCols;
  mRowOffset += offset;
  mRowSize = newRows;

  return;

}

////////////////////////////////////////////////////////////////////////////
// GetArraySize (void)
// 	Get buffer size.
////////////////////////////////////////////////////////////////////////////
int32
TermBuffer::GetArraySize (void)
{
  return buffer_size;
}

/*
 * PRIVATE MEMBER FUNCTIONS.
 */

////////////////////////////////////////////////////////////////////////////
// EraseLine (int)
//	Erase one column.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::EraseLine (int row)
{
  memset (mBase[ROW(row)], 0, mColSize * sizeof (term_buffer));
  
  return;
}
////////////////////////////////////////////////////////////////////////////
// ClearAll()
// Clear contents of TermBuffer.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::ClearAll (void)
{
  for(int i = 0; i < buffer_size; i++){
    memset (mBase[i], 0, mColSize * sizeof (term_buffer));
  }
  mRowOffset = 0;
  this->DeSelect ();
}
////////////////////////////////////////////////////////////////////////////
// Select()
// Mark Text Selected to given range.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::Select(const CurPos &start, const CurPos &end)
{

  if (end < start) {
    mSelStart = end;
    mSelEnd = start;
  }
  else {
    mSelStart = start;
    mSelEnd = end;
  }
  return;

}
////////////////////////////////////////////////////////////////////////////
// DeSelect()
// Mark Text UnSelected to given range.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::DeSelect (void)
{

  mSelStart.Set (-1, -1);
  mSelEnd.Set (-1, -1);
  return;
  
}
////////////////////////////////////////////////////////////////////////////
// CheckSelectedRegion (CurPos &)
//	Check cursor position which be including selected area.
////////////////////////////////////////////////////////////////////////////
int
TermBuffer::CheckSelectedRegion (const CurPos &pos)
{

  if (mSelStart.y == pos.y || mSelEnd.y == pos.y) {
    return true;
  }
  return false;

}
////////////////////////////////////////////////////////////////////////////
// FindWord (CurPos &, CurPos *, CurPos *) 
//  Find word from TermBuffer
////////////////////////////////////////////////////////////////////////////
bool
TermBuffer::FindWord(const CurPos &pos, CurPos *start, CurPos *end)
{
  uchar buf[5];
  ushort attr;
  int x, y, start_x;

  y = pos.y;
  
  // Search start point
  for(x = pos.x - 1; x >= 0; x--){
    if((this->GetChar (y, x, buf, &attr) == NO_CHAR) || (*buf == ' ')){
      ++x;
      break;
     }
  }
  start_x = x;
  
  // Search end point
  for(x = pos.x; x < mColSize; x++){
    if((this->GetChar (y, x, buf, &attr) == NO_CHAR) || (*buf == ' ')){
      --x;
      break;
    }
  }

  if (start_x > x) return false;
  
  mSelStart.Set(start_x, y);
  if (start != NULL){
    start->Set(start_x, y);
  }
  
  mSelEnd.Set(x, y);
  if (end != NULL){
    end->Set(x, y);
  }

  return true;
  
}
////////////////////////////////////////////////////////////////////////////
// GetCharFromRegion (int, int, BString)
//	Get one character from selected region.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::GetCharFromRegion (int x, int y, BString &str)
{
  uchar buf[5];
  ushort attr;
  int status;
  
  status = GetChar(y, x, buf, &attr);
  
  switch(status){
  case NO_CHAR:
    if (IS_CR (attr)) {
      str += '\n';
    } else {
      str += ' ';
    }
    
    break;
    
  case IN_STRING:
     break;

  default:
    str += (const char *)&buf;
    break;
  }
}

////////////////////////////////////////////////////////////////////////////
// AvoidWaste (BString str)
// 	Delete useless char at end of line, and convert LF code.
////////////////////////////////////////////////////////////////////////////
inline void
TermBuffer::AvoidWaste (BString &str)
{
  int32 len, point;

 start:
  len = str.Length() - 1;
  point = str.FindLast (' ');

  if (len > 0 && len == point) {
    str.RemoveLast (" ");
    goto start;
  }

  //  str += '\n';
  return;
  
}

////////////////////////////////////////////////////////////////////////////
// GetStringFromRegion (BString str)
// 	Get string from selected region.
////////////////////////////////////////////////////////////////////////////
void
TermBuffer::GetStringFromRegion(BString &str)
{
  int y;

  if(mSelStart.y == mSelEnd.y){
    y = mSelStart.y;
    for(int x = mSelStart.x ; x <= mSelEnd.x; x++){
      GetCharFromRegion (x, y, str);
    }
  }else{
    y = mSelStart.y;
    for(int x = mSelStart.x ; x < mNowColSize; x++){
      GetCharFromRegion (x, y, str);
    }
    AvoidWaste (str);

    for(y = mSelStart.y + 1 ; y < mSelEnd.y; y++){
      for(int x = 0 ; x < mNowColSize; x++){
        GetCharFromRegion (x, y, str);
      }
      AvoidWaste (str);
    }
    y = mSelEnd.y;
    for(int x = 0 ; x <= mSelEnd.x; x++){
      GetCharFromRegion (x, y, str);
    }
  }    
}

