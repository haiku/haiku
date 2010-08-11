/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef __WIDTHBUFFER_H
#define __WIDTHBUFFER_H


#include <Locker.h>
#include <TextView.h>

#include "TextViewSupportBuffer.h"


class BFont;


namespace BPrivate {


class TextGapBuffer;


struct _width_table_ {
	BFont font;				// corresponding font
	int32 hashCount;		// number of hashed items
	int32 tableCount;		// size of table
	void* widths;			// width table
};


class WidthBuffer : public _BTextViewSupportBuffer_<_width_table_> {
public:
								WidthBuffer();
	virtual						~WidthBuffer();

			float				StringWidth(const char* inText,
									int32 fromOffset, int32 length,
									const BFont* inStyle);
			float				StringWidth(TextGapBuffer& gapBuffer,
									int32 fromOffset, int32 length,
									const BFont* inStyle);

private:
			bool				FindTable(const BFont* font, int32* outIndex);
			int32				InsertTable(const BFont* font);

			bool				GetEscapement(uint32 value, int32 index,
									float* escapement);
			float				HashEscapements(const char* chars,
									int32 numChars, int32 numBytes,
									int32 tableIndex, const BFont* font);

	static	uint32				Hash(uint32);

private:
			BLocker				fLock;
};


extern WidthBuffer* gWidthBuffer;


} // namespace BPrivate


using BPrivate::WidthBuffer;


#if __GNUC__ < 3
//! NetPositive binary compatibility support

class _BWidthBuffer_ : public _BTextViewSupportBuffer_<BPrivate::_width_table_> {
	_BWidthBuffer_();
	virtual ~_BWidthBuffer_();
};

extern
_BWidthBuffer_* gCompatibilityWidthBuffer;

#endif // __GNUC__ < 3


#endif // __WIDTHBUFFER_H
