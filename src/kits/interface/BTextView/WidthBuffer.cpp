/*
 * Copyright (c) 2003-2004 Haiku, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 *
 * File:			WidthBuffer.cpp
 * Author:			Stefano Ceccherini (burton666@libero.it)
 * Description:		Caches string widths in a hash table, to avoid a trip to
 *					the app server.
 */
#include <Debug.h>
#include <Font.h>

#include "moreUTF8.h"
#include "TextGapBuffer.h"
#include "WidthBuffer.h"

#include <cstdio>

const uint32 kTableCount = 128;

struct hashed_escapement
{
	uint32 code;
	float escapement;
	hashed_escapement() {
		code = (uint32)-1;
		escapement = 0;
	}
};


/*! \brief Convert a UTF8 char to a code, which will be used
		to uniquely identify the charachter in the hash table.
	\param text A pointer to the charachter to examine.
	\param charLen the length of the charachter to examine.
	\return The code for the given charachter,
*/
static inline uint32
CharToCode(const char *text, const int32 charLen)
{
	uint32 value = 0;
	int32 shiftVal = 24;
	for (int32 c = 0; c < charLen; c++) {
		uchar ch = text[c];
		value |= (ch << shiftVal);
		shiftVal -= 8;				
	}
	return value;
}


/*! \brief Initializes the object.
*/
_BWidthBuffer_::_BWidthBuffer_()
	:
	_BTextViewSupportBuffer_<_width_table_>(1, 0)
{
}


/*! \brief Frees the allocated resources.
*/
_BWidthBuffer_::~_BWidthBuffer_()
{
	for (int32 x = 0; x < fItemCount; x++)
		delete[] (hashed_escapement*)fBuffer[x].widths;
}


/*! \brief Returns how much room is required to draw a string in the font.
	\param inText The string to be examined.
	\param fromOffset The offset in the string where to begin the examination.
	\param lenght The amount of bytes to be examined.
	\param inStyle The font.
	\return The space (in pixels) required to draw the given string.
*/
float
_BWidthBuffer_::StringWidth(const char *inText, int32 fromOffset, int32 length,
		const BFont *inStyle)
{
	if (inText == NULL)
		return 0;
	
	int32 index = 0;
	if (!FindTable(inStyle, &index))
		index = InsertTable(inStyle);
	
	char *text = NULL;	
	int32 numChars = 0;
	int32 textLen = 0;
	
	float fontSize = inStyle->Size();
	float stringWidth = 0;
	if (length > 0) {		
		for (int32 charLen = 0, currentOffset = fromOffset;
								currentOffset < fromOffset + length;
								currentOffset += charLen) {
			
			charLen = UTF8NextCharLen(inText + currentOffset);
			// End of string, bail out
			if (charLen == 0)
				break;
			
			// Some magic, to uniquely identify this charachter	
			uint32 value = CharToCode(inText + currentOffset, charLen);
			
			float escapement;
			if (GetEscapement(value, index, &escapement)) {
				// Well, we've got a match for this charachter
				stringWidth += escapement;
			} else {
				// Store this charachter into an array, which we'll
				// pass to HashEscapements() later
				int32 offset = textLen;
				textLen += charLen;
				numChars++;
				text = (char *)realloc(text, textLen);
				for (int32 x = 0; x < charLen; x++)
					text[offset + x] = inText[currentOffset + x];
			}			
		}		
	}
	
	if (text != NULL) {
		// We've found some charachters which aren't yet in the hash table.
		// Get their width via HashEscapements()
		stringWidth += HashEscapements(text, numChars, textLen, index, inStyle);	
		free(text);
	}

	return stringWidth * fontSize;
}


/*! \brief Returns how much room is required to draw a string in the font.
	\param inBuffer The _BTextGapBuffer_ to be examined.
	\param fromOffset The offset in the _BTextGapBuffer_ where to begin the examination.
	\param lenght The amount of bytes to be examined.
	\param inStyle The font.
	\return The space (in pixels) required to draw the given string.
*/
float
_BWidthBuffer_::StringWidth(_BTextGapBuffer_ &inBuffer, int32 fromOffset, int32 length,
		const BFont *inStyle)
{
	return StringWidth(inBuffer.Text(), fromOffset, length, inStyle);
}


/*! \brief Searches for the table for the given font.
	\param inStyle The font to search for.
	\param outIndex a pointer to an int32, where the function will store
	the index of the table, if found, or -1, if not.
	\return \c true if the function founds the table,
		\c false if not.
*/
bool
_BWidthBuffer_::FindTable(const BFont *inStyle, int32 *outIndex)
{
	if (inStyle == NULL)
		return false;

	float fontSize = inStyle->Size();
	int32 fontCode = inStyle->FamilyAndStyle();
	int32 tableIndex = -1;

	for (int32 i = 0; i < fItemCount; i++) {
	
#if USE_DANO_WIDTHBUFFER
		if (*inStyle == fBuffer[i].font) {
#else
		if (fontSize == fBuffer[i].fontSize 
				&& fontCode == fBuffer[i].fontCode) {
#endif
			tableIndex = i;
			break;
		}
	}	
	if (outIndex != NULL)
		*outIndex = tableIndex;
	
	return tableIndex != -1;
}


/*!	\brief Creates and insert an empty table for the given font.
	\param font The font to create the table for.
	\return The index of the newly created table.
*/
int32
_BWidthBuffer_::InsertTable(const BFont *font)
{
	_width_table_ table;
	hashed_escapement *deltas = new hashed_escapement[kTableCount];

#if USE_DANO_WIDTHBUFFER
	table.font = *font;
#else
	table.fontSize = font->Size();
	table.fontCode = font->FamilyAndStyle();
#endif

	table.hashCount = 0;
	table.tableCount = kTableCount;
	table.widths = deltas;
	
	int32 position = fItemCount;
	InsertItemsAt(1, position, &table);
	
	return position;
}

/*! \brief Gets the escapement for the given charachter.
	\param value An integer which uniquely identifies a charachter.
	\param index The index of the table to search.
	\param escapement A pointer to a float, where the function will
	store the escapement.
	\return \c true if the function could find the escapement
		for the given charachter, \c false if not.
*/
bool 
_BWidthBuffer_::GetEscapement(uint32 value, int32 index, float *escapement)
{
	_width_table_ *table = &fBuffer[index];	
	hashed_escapement *widths = static_cast<hashed_escapement *>(table->widths);
	uint32 hashed = Hash(value) & (table->tableCount - 1);
	
	DEBUG_ONLY(uint32 iterations = 1;)
	uint32 found;
	while ((found = widths[hashed].code) != (uint32)-1) {
		if (found == value)
			break;	
		
		if (++hashed >= (uint32)table->tableCount)
			hashed = 0;
		DEBUG_ONLY(iterations++;)
	}
	
	if (found == (uint32)-1)
		return false;
	
	PRINT(("Value found with %d iterations\n", iterations));
		
	if (escapement != NULL)
		*escapement = widths[hashed].escapement;
	
	return true;
}


uint32
_BWidthBuffer_::Hash(uint32 val)
{
	uint32 shifted = val >> 24;
	uint32 result = (val >> 15) + (shifted * 3);	
			
	result ^= (val >> 6) - (shifted * 22);
	result ^= (val << 3);

	return result;
}


/*! \brief Gets the escapements for the given string, and put them into
	the hash table.
	\param inText The string to be examined.
	\param numChars The amount of charachters contained in the string.
	\param textLen the amount of bytes contained in the string.
	\param tableIndex the index of the table where the escapements
		should be put.
	\param inStyle the font.
	\return The width of the supplied string (which should be multiplied by
		the size of the font).
*/
float
_BWidthBuffer_::HashEscapements(const char *inText, int32 numChars, int32 textLen,
		int32 tableIndex, const BFont *inStyle)
{
	float *escapements = new float[numChars];
	inStyle->GetEscapements(inText, numChars, escapements);
	
	_width_table_ *table = &fBuffer[tableIndex];	
	hashed_escapement *widths = static_cast<hashed_escapement *>(table->widths);
	
	int32 offset = 0;
	int32 charCount = 0;
	
	// Insert the escapements into the hash table
	do {
		int32 charLen = UTF8NextCharLen(inText + offset);
		if (charLen == 0)
			break;
			
		uint32 value = CharToCode(inText + offset, charLen);
				
		uint32 hashed = Hash(value) & (table->tableCount - 1);
		uint32 found = widths[hashed].code;
		
		// Check if the value is already in the table
		if (found != value) {
			while ((found = widths[hashed].code) != (uint32)-1) {
				if (found == value)
					break;
				if (++hashed >= (uint32)table->tableCount)
					hashed = 0;
			}
			
			if (found == (uint32)-1) {
				// The value is not in the table. Add it.
				widths[hashed].code = value;
				widths[hashed].escapement = escapements[charCount];
				table->hashCount++;
			
				// We always keep some free space in the hash table
				// TODO: Not sure how much space, currently we double
				// the current size when hashCount is at least 2/3 of
				// the total size.
				if (table->tableCount * 2 / 3 <= table->hashCount) {
					table->hashCount = 0;
					int32 newSize = table->tableCount * 2;
					
					// Create and initialize a new hash table
					hashed_escapement *newWidths = new hashed_escapement[newSize];

					// Rehash the values, and put them into the new table
					for (int32 oldPos = 0; oldPos < table->tableCount; oldPos++) {
						if (widths[oldPos].code != (uint32) -1) {
							uint32 newPos = Hash(widths[oldPos].code) & (newSize - 1);
							while (newWidths[newPos].code != (uint32)-1) {
								if (++newPos >= (uint32)newSize)
									newPos = 0;
							}					
							newWidths[newPos].code = widths[oldPos].code;
							newWidths[newPos].escapement = widths[oldPos].escapement;
							table->hashCount++;
						}
					}
					table->tableCount = newSize;
					
					// Delete the old table, and put the new pointer into the _width_table_
					delete[] widths;
					widths = newWidths;
				}
			}
		}
		charCount++;
		offset += charLen;
	} while (offset < textLen);
	
	// Calculate the width of the string
	float width = 0;
	for (int32 x = 0; x < numChars; x++)
		width += escapements[x];

	delete[] escapements;

	return width;
}
