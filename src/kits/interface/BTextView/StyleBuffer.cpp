//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		StyleBuffer.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Style storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include "StyleBuffer.h"

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
_BStyleBuffer_::_BStyleBuffer_(const BFont *font, const rgb_color *color)
	:	fRuns(10, 10),
		fRecords(10, 10)
{
	rgb_color black = {0, 0, 0, 255};

	SetNullStyle(0, font ? font : be_plain_font, color ? color : &black, 0);
}
//------------------------------------------------------------------------------
_BStyleBuffer_::~_BStyleBuffer_()
{
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::SetNullStyle(uint32 mode, const BFont *font,
								  const rgb_color *color, int32)
{
	// TODO: use mode.
	if (font)
		fNullStyle.fFont = *font;

	if (color)
		fNullStyle.fColor = *color;
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::GetNullStyle(const BFont **font, const rgb_color **color) const
{
	*font = &fNullStyle.fFont;
	*color = &fNullStyle.fColor;
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::SyncNullStyle(int32)
{
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::InvalidateNullStyle()
{
}
//------------------------------------------------------------------------------
bool _BStyleBuffer_::IsValidNullStyle() const
{
	// TODO: why would a style be invalid?
	return true;
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::SetStyle(uint32 mode, const BFont *font, BFont *,
							  const rgb_color *color, rgb_color *) const
{
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::GetStyle(uint32 mode, BFont *font, rgb_color *color) const
{
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::SetStyleRange(int32 from, int32 to, int32, uint32 mode,
								   const BFont *, const rgb_color *)
{
}
//------------------------------------------------------------------------------
int32 _BStyleBuffer_::GetStyleRange(int32 from, int32 to) const
{
	return 0;
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::ContinuousGetStyle(BFont *, uint32 *, rgb_color *, bool *,
					int32, int32) const
{
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::RemoveStyles(int32 from, int32 to)
{
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::RemoveStyleRange(int32 from, int32 to)
{
}
//------------------------------------------------------------------------------
int32 _BStyleBuffer_::OffsetToRun(int32 offset) const
{
	if (offset == 0)
		return 0;

	for (int i = 0; i < fRuns.fCount; i++)
	{
		if (offset < fRuns.fItems[i].fOffset)
			return i - 1;
	}

	return fRuns.fCount - 2;
}
//------------------------------------------------------------------------------
void _BStyleBuffer_::BumpOffset(int32 run, int32 offset)
{
	for (int i = run; i < fRuns.fCount; i++)
		fRuns.fItems[i].fOffset += offset;
}
//------------------------------------------------------------------------------		
/*void _BStyleBuffer_::Iterate(int32, int32, _BInlineInput_ *, const BFont **,
							 const rgb_color **, float *, uint32 *) const
{
}*/
//------------------------------------------------------------------------------
STEStyleRecord *_BStyleBuffer_::operator[](int32 index)
{
	if (index < fRecords.fCount)
		return fRecords.fItems + index;
	else
		return NULL;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
