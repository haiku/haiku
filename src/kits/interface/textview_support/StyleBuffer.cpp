/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */

/**	Style storage used by BTextView */


#include "InlineInput.h"
#include "StyleBuffer.h"

#include <View.h>

#include <stdio.h>


_BStyleRunDescBuffer_::_BStyleRunDescBuffer_()
	: _BTextViewSupportBuffer_<STEStyleRunDesc>(20)
{
}


void
_BStyleRunDescBuffer_::InsertDesc(STEStyleRunDesc* inDesc, int32 index)
{
	InsertItemsAt(1, index, inDesc);
}


void
_BStyleRunDescBuffer_::RemoveDescs(int32 index, int32 count)
{
	RemoveItemsAt(count, index);
}


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
			} else {
				if (offset < fBuffer[index + 1].offset)
					break;
				else
					minIndex = index + 1;
			}
		} else
			maxIndex = index;
	}

	return index;
}


void
_BStyleRunDescBuffer_::BumpOffset(int32 delta, int32 index)
{
	for (int32 i = index; i < fItemCount; i++)
		fBuffer[i].offset += delta;
}


//	#pragma mark -


_BStyleRecordBuffer_::_BStyleRecordBuffer_()
	: _BTextViewSupportBuffer_<STEStyleRecord>()
{
}


int32
_BStyleRecordBuffer_::InsertRecord(const BFont *inFont,
	const rgb_color *inColor)
{
	int32 index = 0;

	// look for style in buffer
	if (MatchRecord(inFont, inColor, &index))
		return index;

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
	const STEStyle style = { *inFont, *inColor };
	const STEStyleRecord newRecord = {
		0,
		fh.ascent,
		fh.descent + fh.leading,
		style
	};
	InsertItemsAt(1, fItemCount, &newRecord);

	return index;
}


void
_BStyleRecordBuffer_::CommitRecord(int32 index)
{
	fBuffer[index].refs++;
}


void
_BStyleRecordBuffer_::RemoveRecord(int32 index)
{
	fBuffer[index].refs--;
}


bool
_BStyleRecordBuffer_::MatchRecord(const BFont *inFont, const rgb_color *inColor,
	int32 *outIndex)
{
	for (int32 i = 0; i < fItemCount; i++) {
		if (*inFont == fBuffer[i].style.font
			&& *inColor == fBuffer[i].style.color) {
			*outIndex = i;
			return true;
		}
	}

	return false;
}


//	#pragma mark -


static void
SetStyleFromMode(uint32 mode, const BFont *fromFont, BFont *toFont,
		const rgb_color *fromColor, rgb_color *toColor)
{
	if (fromFont != NULL && toFont != NULL) {
		if (mode & B_FONT_FAMILY_AND_STYLE)
			toFont->SetFamilyAndStyle(fromFont->FamilyAndStyle());

		if (mode & B_FONT_SIZE)
			toFont->SetSize(fromFont->Size());

		if (mode & B_FONT_SHEAR)
			toFont->SetShear(fromFont->Shear());

		if (mode & B_FONT_FALSE_BOLD_WIDTH)
			toFont->SetFalseBoldWidth(fromFont->FalseBoldWidth());
	}

	if (fromColor != NULL && toColor != NULL
		&& (mode == 0 || mode == B_FONT_ALL))
		*toColor = *fromColor;
}


BTextView::StyleBuffer::StyleBuffer(const BFont *inFont,
	const rgb_color *inColor)
	:
	fValidNullStyle(true)
{
	fNullStyle.font = *inFont;
	fNullStyle.color = *inColor;
}


void
BTextView::StyleBuffer::InvalidateNullStyle()
{
	fValidNullStyle = false;
}


bool
BTextView::StyleBuffer::IsValidNullStyle() const
{
	return fValidNullStyle;
}


void
BTextView::StyleBuffer::SyncNullStyle(int32 offset)
{
	if (fValidNullStyle || fStyleRunDesc.ItemCount() < 1)
		return;

	int32 index = OffsetToRun(offset);
	fNullStyle = fStyleRecord[fStyleRunDesc[index]->index]->style;

	fValidNullStyle = true;
}


void
BTextView::StyleBuffer::SetNullStyle(uint32 inMode, const BFont *inFont,
	const rgb_color *inColor, int32 offset)
{
	if (fValidNullStyle || fStyleRunDesc.ItemCount() < 1) {
		SetStyleFromMode(inMode, inFont, &fNullStyle.font, inColor,
			&fNullStyle.color);
	} else {
		int32 index = OffsetToRun(offset - 1);
		fNullStyle = fStyleRecord[fStyleRunDesc[index]->index]->style;
		SetStyleFromMode(inMode, inFont, &fNullStyle.font, inColor,
			&fNullStyle.color);
	}

	fValidNullStyle = true;
}


void
BTextView::StyleBuffer::GetNullStyle(const BFont **font,
	const rgb_color **color) const
{
	if (font != NULL)
		*font = &fNullStyle.font;
	if (color != NULL)
		*color = &fNullStyle.color;
}


STEStyleRange *
BTextView::StyleBuffer::AllocateStyleRange(const int32 numStyles) const
{
	STEStyleRange* range = (STEStyleRange *)malloc(sizeof(int32)
		+ sizeof(STEStyleRun) * numStyles);
	if (range != NULL)
		range->count = numStyles;
	return range;
}


void
BTextView::StyleBuffer::SetStyleRange(int32 fromOffset, int32 toOffset,
	int32 textLen, uint32 inMode, const BFont *inFont,
	const rgb_color *inColor)
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

	int32 offset = fromOffset;
	int32 runIndex = OffsetToRun(offset);
	int32 styleIndex = 0;
	do {
		const STEStyleRunDesc runDesc = *fStyleRunDesc[runIndex];
		int32 runEnd = textLen;
		if (runIndex < fStyleRunDesc.ItemCount() - 1)
			runEnd = fStyleRunDesc[runIndex + 1]->offset;

		STEStyle style = fStyleRecord[runDesc.index]->style;
		SetStyleFromMode(inMode, inFont, &style.font, inColor, &style.color);

		styleIndex = fStyleRecord.InsertRecord(&style.font, &style.color);

		if (runDesc.offset == offset && runIndex > 0
			&& fStyleRunDesc[runIndex - 1]->index == styleIndex) {
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
			} else {
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

	if (offset == toOffset && runIndex < fStyleRunDesc.ItemCount()
		&& fStyleRunDesc[runIndex]->index == styleIndex)
		RemoveStyles(runIndex);
}


void
BTextView::StyleBuffer::GetStyle(int32 inOffset, BFont *outFont,
	rgb_color *outColor) const
{
	if (fStyleRunDesc.ItemCount() < 1) {
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


STEStyleRange*
BTextView::StyleBuffer::GetStyleRange(int32 startOffset, int32 endOffset) const
{
	int32 startIndex = OffsetToRun(startOffset);
	int32 endIndex = OffsetToRun(endOffset);

	int32 numStyles = endIndex - startIndex + 1;
	if (numStyles < 1)
		numStyles = 1;

	STEStyleRange* result = AllocateStyleRange(numStyles);
	if (!result)
		return NULL;

	STEStyleRun* run = &result->runs[0];
	for (int32 index = 0; index < numStyles; index++) {
		*run = (*this)[startIndex + index];
		run->offset -= startOffset;
		if (run->offset < 0)
			run->offset = 0;
		run++;
	}

	return result;
}


void
BTextView::StyleBuffer::RemoveStyleRange(int32 fromOffset, int32 toOffset)
{
	int32 fromIndex = fStyleRunDesc.OffsetToRun(fromOffset);
	int32 toIndex = fStyleRunDesc.OffsetToRun(toOffset) - 1;

	int32 count = toIndex - fromIndex;
	if (count > 0) {
		RemoveStyles(fromIndex + 1, count);
		toIndex = fromIndex;
	}

	fStyleRunDesc.BumpOffset(fromOffset - toOffset, fromIndex + 1);

	if (toIndex == fromIndex && toIndex < fStyleRunDesc.ItemCount() - 1) {
		STEStyleRunDesc* runDesc = fStyleRunDesc[toIndex + 1];
		runDesc->offset = fromOffset;
	}

	if (fromIndex < fStyleRunDesc.ItemCount() - 1) {
		STEStyleRunDesc* runDesc = fStyleRunDesc[fromIndex];
		if (runDesc->offset == (runDesc + 1)->offset) {
			RemoveStyles(fromIndex);
			fromIndex--;
		}
	}

	if (fromIndex >= 0 && fromIndex < fStyleRunDesc.ItemCount() - 1) {
		STEStyleRunDesc* runDesc = fStyleRunDesc[fromIndex];
		if (runDesc->index == (runDesc + 1)->index)
			RemoveStyles(fromIndex + 1);
	}
}


void
BTextView::StyleBuffer::RemoveStyles(int32 index, int32 count)
{
	for (int32 i = index; i < (index + count); i++)
		fStyleRecord.RemoveRecord(fStyleRunDesc[i]->index);

	fStyleRunDesc.RemoveDescs(index, count);
}


int32
BTextView::StyleBuffer::Iterate(int32 fromOffset, int32 length,
	InlineInput *input,
	const BFont **outFont, const rgb_color **outColor,
	float *outAscent, float *outDescent, uint32 *) const
{
	// TODO: Handle the InlineInput style here in some way
	int32 numRuns = fStyleRunDesc.ItemCount();
	if (length < 1 || numRuns < 1)
		return 0;

	int32 result = length;
	int32 runIndex = fStyleRunDesc.OffsetToRun(fromOffset);
	STEStyleRunDesc* run = fStyleRunDesc[runIndex];

	if (outFont != NULL)
		*outFont = &fStyleRecord[run->index]->style.font;
	if (outColor != NULL)
		*outColor = &fStyleRecord[run->index]->style.color;
	if (outAscent != NULL)
		*outAscent = fStyleRecord[run->index]->ascent;
	if (outDescent != NULL)
		*outDescent = fStyleRecord[run->index]->descent;

	if (runIndex < numRuns - 1) {
		int32 nextOffset = (run + 1)->offset - fromOffset;
		result = min_c(result, nextOffset);
	}

	return result;
}


int32
BTextView::StyleBuffer::OffsetToRun(int32 offset) const
{
	return fStyleRunDesc.OffsetToRun(offset);
}


void
BTextView::StyleBuffer::BumpOffset(int32 delta, int32 index)
{
	fStyleRunDesc.BumpOffset(delta, index);
}


STEStyleRun
BTextView::StyleBuffer::operator[](int32 index) const
{
	STEStyleRun run;

	if (fStyleRunDesc.ItemCount() < 1) {
		run.offset = 0;
		run.style = fNullStyle;
	} else {
		STEStyleRunDesc* runDesc = fStyleRunDesc[index];
		run.offset = runDesc->offset;
		run.style = fStyleRecord[runDesc->index]->style;
	}

	return run;
}


// TODO: Horrible name, but can't think of a better one
// ? CompareStyles ?
// ? FilterStyles ?
static void
FixupMode(const STEStyle &firstStyle, const STEStyle &otherStyle, uint32 &mode,
	bool &sameColor)
{
	if (mode & B_FONT_FAMILY_AND_STYLE) {
		if (firstStyle.font != otherStyle.font)
			mode &= ~B_FONT_FAMILY_AND_STYLE;
	}
	if (mode & B_FONT_SIZE) {
		if (firstStyle.font.Size() != otherStyle.font.Size())
			mode &= ~B_FONT_SIZE;
	}
	if (mode & B_FONT_SHEAR) {
		if (firstStyle.font.Shear() != otherStyle.font.Shear())
			mode &= ~B_FONT_SHEAR;
	}
	if (mode & B_FONT_FALSE_BOLD_WIDTH) {
		if (firstStyle.font.FalseBoldWidth()
			!= otherStyle.font.FalseBoldWidth()) {
			mode &= ~B_FONT_FALSE_BOLD_WIDTH;
		}
	}
	if (firstStyle.color != otherStyle.color)
		sameColor = false;

	// TODO: Finish this: handle B_FONT_FACE, B_FONT_FLAGS, etc.
	// if needed
}


void
BTextView::StyleBuffer::ContinuousGetStyle(BFont *outFont, uint32 *ioMode,
	rgb_color *outColor, bool *sameColor, int32 fromOffset,
	int32 toOffset) const
{
	uint32 mode = B_FONT_ALL;

	if (fStyleRunDesc.ItemCount() < 1) {
		if (ioMode)
			*ioMode = mode;
		if (outFont)
			*outFont = fNullStyle.font;
		if (outColor)
			*outColor = fNullStyle.color;
		if (sameColor)
			*sameColor = true;
		return;
	}

	int32 fromIndex = OffsetToRun(fromOffset);
	int32 toIndex = OffsetToRun(toOffset - 1);

	if (fromIndex == toIndex) {
		int32 styleIndex = fStyleRunDesc[fromIndex]->index;
		const STEStyle* style = &fStyleRecord[styleIndex]->style;

		if (ioMode)
			*ioMode = mode;
		if (outFont)
			*outFont = style->font;
		if (outColor)
			*outColor = style->color;
		if (sameColor)
			*sameColor = true;
	} else {
		bool oneColor = true;
		int32 styleIndex = fStyleRunDesc[toIndex]->index;
		STEStyle theStyle = fStyleRecord[styleIndex]->style;

		for (int32 i = fromIndex; i < toIndex; i++) {
			styleIndex = fStyleRunDesc[i]->index;
			FixupMode(fStyleRecord[styleIndex]->style, theStyle, mode,
				oneColor);
		}

		if (ioMode)
			*ioMode = mode;
		if (outFont)
			*outFont = theStyle.font;
		if (outColor)
			*outColor = theStyle.color;
		if (sameColor)
			*sameColor = oneColor;
	}
}
