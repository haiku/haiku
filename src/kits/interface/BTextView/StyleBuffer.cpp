//------------------------------------------------------------------------------
//	Copyright (c) 2001-2003, OpenBeOS
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
#include "InlineInput.h"
#include "StyleBuffer.h"

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
_BStyleRunDescBuffer_::_BStyleRunDescBuffer_()
	:	_BTextViewSupportBuffer_<STEStyleRunDesc>(20)
{
}
//------------------------------------------------------------------------------
void
_BStyleRunDescBuffer_::InsertDesc(STEStyleRunDescPtr inDesc, int32 index)
{
	InsertItemsAt(1, index, inDesc);
}
//------------------------------------------------------------------------------
void
_BStyleRunDescBuffer_::RemoveDescs(int32 index, int32 count)
{
	RemoveItemsAt(count, index);
}
//------------------------------------------------------------------------------
int32
_BStyleRunDescBuffer_::OffsetToRun(int32 offset) const
{
	if (fItemCount <= 1)
		return 0;
		
	int32 minIndex = 0;
	int32 maxIndex = fItemCount;
	int32 index = 0;
	
	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= fBuffer[index].offset) {
			if (index >= (fItemCount - 1)) {
				break;
			}
			else {
				if (offset < fBuffer[index + 1].offset)
					break;
				else
					minIndex = index + 1;
			}
		}
		else
			maxIndex = index;
	}
	
	return index;
}
//------------------------------------------------------------------------------
void
_BStyleRunDescBuffer_::BumpOffset(int32 delta, int32 index)
{
	for (int32 i = index; i < fItemCount; i++)
		fBuffer[i].offset += delta;
}
//------------------------------------------------------------------------------
_BStyleRecordBuffer_::_BStyleRecordBuffer_()
	:	_BTextViewSupportBuffer_<STEStyleRecord>()
{
}
//------------------------------------------------------------------------------
int32
_BStyleRecordBuffer_::InsertRecord(const BFont *inFont,
										 const rgb_color *inColor)
{
	int32 index = 0;

	// look for style in buffer
	if (MatchRecord(inFont, inColor, &index))
		return (index);

	// style not found, add it
	font_height fh;
	inFont->GetHeight(&fh);
	
	// check if there's any unused space
	for (index = 0; index < fItemCount; index++) {
		if (fBuffer[index].refs < 1) {
			fBuffer[index].refs = 0;
			fBuffer[index].ascent = fh.ascent;
			fBuffer[index].descent = fh.descent + fh.leading;
			fBuffer[index].style.font = *inFont;
			fBuffer[index].style.color = *inColor;
			return index;
		}	
	}
	
	// no unused space, expand the buffer
	index = fItemCount;
	STEStyleRecord 	newRecord;
	newRecord.refs = 0;
	newRecord.ascent = fh.ascent;
	newRecord.descent = fh.descent + fh.leading;
	newRecord.style.font = *inFont;
	newRecord.style.color = *inColor;
	InsertItemsAt(1, index, &newRecord);

	return index;
}
//------------------------------------------------------------------------------
void
_BStyleRecordBuffer_::CommitRecord(
	int32	index)
{
	fBuffer[index].refs++;
}
//------------------------------------------------------------------------------
void
_BStyleRecordBuffer_::RemoveRecord(
	int32	index)
{
	fBuffer[index].refs--;
}
//------------------------------------------------------------------------------
bool
_BStyleRecordBuffer_::MatchRecord(const BFont *inFont,
									   const rgb_color *inColor, int32 *outIndex)
{
	for (int32 i = 0; i < fItemCount; i++) {
		if ( (inFont->Size() == fBuffer[i].style.font.Size()) &&
			 (inFont->Shear() == fBuffer[i].style.font.Shear()) &&
			 (inFont->Face() == fBuffer[i].style.font.Face()) &&
			 (inColor->red == fBuffer[i].style.color.red) &&
			 (inColor->green == fBuffer[i].style.color.green) &&
			 (inColor->blue == fBuffer[i].style.color.blue) &&
			 (inColor->alpha == fBuffer[i].style.color.alpha) ) {
			*outIndex = i;
			return true;
		}
	}

	return false;
}
//------------------------------------------------------------------------------
_BStyleBuffer_::_BStyleBuffer_(const BFont *inFont, const rgb_color *inColor)
{
	fValidNullStyle = true;
	fNullStyle.font = *inFont;
	fNullStyle.color = *inColor;
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::InvalidateNullStyle()
{
	fValidNullStyle = false;
}
//------------------------------------------------------------------------------
bool
_BStyleBuffer_::IsValidNullStyle() const
{
	return fValidNullStyle;
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::SyncNullStyle(int32 offset)
{
	if ((fValidNullStyle) || (fStyleRunDesc.ItemCount() < 1))
		return;
	
	int32 index = OffsetToRun(offset);
	fNullStyle = fStyleRecord[fStyleRunDesc[index]->index]->style;

	fValidNullStyle = true;
}	
//------------------------------------------------------------------------------
void
_BStyleBuffer_::SetNullStyle(uint32 inMode, const BFont *inFont,
								  const rgb_color *inColor, int32 offset)
{
	if ((fValidNullStyle) || (fStyleRunDesc.ItemCount() < 1))
		SetStyle(inMode, inFont, &fNullStyle.font, inColor, &fNullStyle.color);
	else {
		int32 index = OffsetToRun(offset - 1);
		fNullStyle = fStyleRecord[fStyleRunDesc[index]->index]->style;
		SetStyle(inMode, inFont, &fNullStyle.font, inColor, &fNullStyle.color);	
	}
	
	fValidNullStyle = true;
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::GetNullStyle(const BFont **font,
								  const rgb_color **color) const
{
	if (font)
		*font = &fNullStyle.font;
	if (color)
		*color = &fNullStyle.color;
}
//------------------------------------------------------------------------------
bool
_BStyleBuffer_::IsContinuousStyle(uint32 *ioMode, STEStylePtr outStyle,
									   int32 fromOffset, int32 toOffset)
{	
	if (fStyleRunDesc.ItemCount() < 1) {
		SetStyle(*ioMode, &fNullStyle.font, &outStyle->font,
			&fNullStyle.color, &outStyle->color);
		return true;
	}
		
	bool result = true;
	int32 fromIndex = OffsetToRun(fromOffset);
	int32 toIndex = OffsetToRun(toOffset - 1);
	
	if (fromIndex == toIndex) {		
		int32 styleIndex = fStyleRunDesc[fromIndex]->index;
		STEStylePtr style = &fStyleRecord[styleIndex]->style;
		
		SetStyle(*ioMode, &style->font, &outStyle->font, &style->color,
			&outStyle->color);
		result = true;
	}
	else {
		int32 		styleIndex = fStyleRunDesc[toIndex]->index;
		STEStyle	theStyle = fStyleRecord[styleIndex]->style;
		//STEStylePtr	style = NULL;
		
	/*	for (int32 i = fromIndex; i < toIndex; i++) {
			styleIndex = fStyleRunDesc[i]->index;
			style = &fStyleRecord[styleIndex]->style;
			
			if (*ioMode & doFont) {
				if (strcmp(theStyle.font, style->font) != 0) {
					*ioMode &= ~doFont;
					result = false;
				}	
			}
				
			if (*ioMode & doSize) {
				if (theStyle.size != style->size) {
					*ioMode &= ~doSize;
					result = false;
				}
			}
				
			if (*ioMode & doShear) {
				if (theStyle.shear != style->shear) {
					*ioMode &= ~doShear;
					result = false;
				}
			}
				
			if (*ioMode & doUnderline) {
				if (theStyle.underline != style->underline) {
					*ioMode &= ~doUnderline;
					result = false;
				}
			}
				
			if (*ioMode & doColor) {
				if ( (theStyle.color.red != style->color.red) ||
					 (theStyle.color.green != style->color.green) ||
					 (theStyle.color.blue != style->color.blue) ||
					 (theStyle.color.alpha != style->color.alpha) ) {
					*ioMode &= ~doColor;
					result = false;
				}
			}
			
			if (*ioMode & doExtra) {
				if (theStyle.extra != style->extra) {
					*ioMode &= ~doExtra;
					result = false;
				}
			}
		}*/
		
		SetStyle(*ioMode, &theStyle.font, &outStyle->font, &theStyle.color,
			&outStyle->color);
	}
	
	return result;
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::SetStyleRange(int32 fromOffset, int32 toOffset,
								   int32 textLen, uint32 inMode,
								   const BFont *inFont, const rgb_color *inColor)
{
	if (inFont == NULL)
		inFont = &fNullStyle.font;

	if (inColor == NULL)
		inColor = &fNullStyle.color;

	if (fromOffset == toOffset) {
		SetNullStyle(inMode, inFont, inColor, fromOffset);
		return;
	}
	
	if (fStyleRunDesc.ItemCount() < 1) {
		STEStyleRunDesc newDesc;
		newDesc.offset = fromOffset;
		newDesc.index = fStyleRecord.InsertRecord(inFont, inColor);
		fStyleRunDesc.InsertDesc(&newDesc, 0);
		fStyleRecord.CommitRecord(newDesc.index);
		return;
	}

	int32 styleIndex = 0;
	int32 offset = fromOffset;
	int32 runIndex = OffsetToRun(offset);
	do {
		STEStyleRunDesc	runDesc = *fStyleRunDesc[runIndex];
		int32			runEnd = textLen;
		if (runIndex < (fStyleRunDesc.ItemCount() - 1))
			runEnd = fStyleRunDesc[runIndex + 1]->offset;
		
		STEStyle style = fStyleRecord[runDesc.index]->style;
		SetStyle(inMode, inFont, &style.font, inColor, &style.color);

		styleIndex = fStyleRecord.InsertRecord(inFont, inColor);
		
		if ( (runDesc.offset == offset) && (runIndex > 0) && 
			 (fStyleRunDesc[runIndex - 1]->index == styleIndex) ) {
			RemoveStyles(runIndex);
			runIndex--;	
		}

		if (styleIndex != runDesc.index) {
			if (offset > runDesc.offset) {
				STEStyleRunDesc newDesc;
				newDesc.offset = offset;
				newDesc.index = styleIndex;
				fStyleRunDesc.InsertDesc(&newDesc, runIndex + 1);
				fStyleRecord.CommitRecord(newDesc.index);
				runIndex++;
			}
			else {
				fStyleRunDesc[runIndex]->index = styleIndex;
				fStyleRecord.CommitRecord(styleIndex);
			}
				
			if (toOffset < runEnd) {
				STEStyleRunDesc newDesc;
				newDesc.offset = toOffset;
				newDesc.index = runDesc.index;
				fStyleRunDesc.InsertDesc(&newDesc, runIndex + 1);
				fStyleRecord.CommitRecord(newDesc.index);
			}
		}
		
		runIndex++;
		offset = runEnd;	
	} while (offset < toOffset);
	
	if ( (offset == toOffset) && (runIndex < fStyleRunDesc.ItemCount()) &&
		 (fStyleRunDesc[runIndex]->index == styleIndex) )
		RemoveStyles(runIndex);
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::GetStyle(int32 inOffset, BFont *outFont,
							  rgb_color *outColor) const
{
	if (fStyleRunDesc.ItemCount() < 1)
	{
		if (outFont)
			*outFont = fNullStyle.font;
		if (outColor)
			*outColor = fNullStyle.color;
		return;
	}
	
	int32 runIndex = OffsetToRun(inOffset);
	int32 styleIndex = fStyleRunDesc[runIndex]->index;

	if (outFont)
		*outFont = fStyleRecord[styleIndex]->style.font;
	if (outColor)
		*outColor = fStyleRecord[styleIndex]->style.color;
}
//------------------------------------------------------------------------------
STEStyleRangePtr
_BStyleBuffer_::GetStyleRange(int32 startOffset,
											   int32 endOffset) const
{
	STEStyleRangePtr result = NULL;
	
	int32 startIndex = OffsetToRun(startOffset);
	int32 endIndex = OffsetToRun(endOffset);
	
	int32 numStyles = endIndex - startIndex + 1;
	numStyles = (numStyles < 1) ? 1 : numStyles;
	result = (STEStyleRangePtr)malloc(sizeof(int32) +
									  (sizeof(STEStyleRun) * numStyles));

	result->count = numStyles;
	STEStyleRunPtr run = &result->runs[0];
	for (int32 index = 0; index < numStyles; index++) {
		*run = (*this)[startIndex + index];
		run->offset -= startOffset;
		run->offset = (run->offset < 0) ? 0 : run->offset;
		run++;
	}
	
	return result;
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::RemoveStyleRange(int32 fromOffset, int32 toOffset)
{
	int32 fromIndex = fStyleRunDesc.OffsetToRun(fromOffset);
	int32 toIndex = fStyleRunDesc.OffsetToRun(toOffset) - 1;

	int32 count = toIndex - fromIndex;
	if (count > 0) {
		RemoveStyles(fromIndex + 1, count);
		toIndex = fromIndex;
	}
	
	fStyleRunDesc.BumpOffset(fromOffset - toOffset, fromIndex + 1);

	if ((toIndex == fromIndex) && (toIndex < (fStyleRunDesc.ItemCount() - 1))) {
		STEStyleRunDescPtr runDesc = fStyleRunDesc[toIndex + 1];
		runDesc->offset = fromOffset;
	}
	
	if (fromIndex < (fStyleRunDesc.ItemCount() - 1)) {
		STEStyleRunDescPtr runDesc = fStyleRunDesc[fromIndex];
		if (runDesc->offset == (runDesc + 1)->offset) {
			RemoveStyles(fromIndex);
			fromIndex--;
		}
	}
	
	if ((fromIndex >= 0) && (fromIndex < (fStyleRunDesc.ItemCount() - 1))) {
		STEStyleRunDescPtr runDesc = fStyleRunDesc[fromIndex];
		if (runDesc->index == (runDesc + 1)->index)
			RemoveStyles(fromIndex + 1);
	}
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::RemoveStyles(int32 index, int32 count)
{
	for (int32 i = index; i < (index + count); i++)
		fStyleRecord.RemoveRecord(fStyleRunDesc[i]->index);
		
	fStyleRunDesc.RemoveDescs(index, count);
}
//------------------------------------------------------------------------------
int32
_BStyleBuffer_::Iterate(int32 fromOffset, int32 length, _BInlineInput_ *input,
							 const BFont **outFont, const rgb_color **outColor,
							 float *outAscent, float *outDescent, uint32 *) const
{
	int32 numRuns = fStyleRunDesc.ItemCount();
	if ((length < 1) || (numRuns < 1))
		return (0);

	int32 				result = length;
	int32 				runIndex = fStyleRunDesc.OffsetToRun(fromOffset);
	STEStyleRunDescPtr	run = fStyleRunDesc[runIndex];
	
	if (outFont != NULL)
		*outFont = &fStyleRecord[run->index]->style.font;
	if (outColor != NULL)
		*outColor = &fStyleRecord[run->index]->style.color;
	if (outAscent != NULL)
		*outAscent = fStyleRecord[run->index]->ascent;
	if (outDescent != NULL)
		*outDescent = fStyleRecord[run->index]->descent;
	
	if (runIndex < (numRuns - 1)) {
		int32 nextOffset = (run + 1)->offset - fromOffset;
		result = (result > nextOffset) ? nextOffset : result;
	}
	
	return result;
}
//------------------------------------------------------------------------------
int32
_BStyleBuffer_::OffsetToRun(int32 offset) const
{
	return fStyleRunDesc.OffsetToRun(offset);
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::BumpOffset(int32 delta, int32 index)
{
	fStyleRunDesc.BumpOffset(delta, index);
}
//------------------------------------------------------------------------------
void
_BStyleBuffer_::SetStyle(uint32 mode, const BFont *fromFont,
							  BFont *toFont, const rgb_color *fromColor,
							  rgb_color *toColor)
{	
	if (mode & doFont) 
		toFont->SetFamilyAndStyle(fromFont->FamilyAndStyle());
		
	if (mode & doSize) {
		if (mode & addSize)
			toFont->SetSize(fromFont->Size());
		else
			toFont->SetSize(fromFont->Size());
	}
		
	if (mode & doShear)
		toFont->SetShear(fromFont->Shear());
		
	if (mode & doUnderline)
		toFont->SetFace(fromFont->Face());
		
	if (mode & doColor)
		*toColor = *fromColor;
}
//------------------------------------------------------------------------------
STEStyleRun
_BStyleBuffer_::operator[](int32 index) const
{
	STEStyleRun run;
	
	if (fStyleRunDesc.ItemCount() < 1) {
		run.offset = 0;
		run.style = fNullStyle;
	} else {
		STEStyleRunDescPtr	runDesc = fStyleRunDesc[index];
		STEStyleRecordPtr	record = fStyleRecord[runDesc->index];
		run.offset = runDesc->offset;
		run.style = record->style;
	}
	
	return run;
}
//------------------------------------------------------------------------------

/*void _BStyleBuffer_::ContinuousGetStyle(BFont *, uint32 *, rgb_color *, bool *,
					int32, int32) const
{
}*/

/*
 * $Log $
 *
 * $Id  $
 *
 */
