/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */
#ifndef __TEXTGAPBUFFER_H
#define __TEXTGAPBUFFER_H


#include <SupportDefs.h>
#include <TextView.h>


class BFile;


namespace BPrivate {


class TextGapBuffer {
public:
								TextGapBuffer();
								~TextGapBuffer();

			void				InsertText(const char* inText, int32 inNumItems,
									int32 inAtIndex);
			void				InsertText(BFile* file, int32 fileOffset,
									int32 amount, int32 atIndex);
			void				RemoveRange(int32 start, int32 end);

			bool				FindChar(char inChar, int32 fromIndex,
									int32* ioDelta);

			const char*			Text();
			const char*			RealText();
			int32				Length() const;

			const char*			GetString(int32 fromOffset, int32* numBytes);
			void				GetString(int32 offset, int32 length,
									char* buffer);

			char				RealCharAt(int32 offset) const;

			bool				PasswordMode() const;
			void				SetPasswordMode(bool);

private:
			void				_MoveGapTo(int32 toIndex);
			void				_EnlargeGapTo(int32 inCount);
			void				_ShrinkGapTo(int32 inCount);

			int32				fItemCount;			// logical count
			char*				fBuffer;			// allocated memory
			int32				fBufferCount;		// physical count
			int32				fGapIndex;			// gap position
			int32				fGapCount;			// gap count
			char*				fScratchBuffer;		// for GetString
			int32				fScratchSize;		// scratch size
			bool				fPasswordMode;
};


inline int32
TextGapBuffer::Length() const
{
	return fItemCount;
}


inline char
TextGapBuffer::RealCharAt(int32 index) const
{
	if (index < 0 || index >= fItemCount) {
		if (index != fItemCount)
			debugger("RealCharAt: invalid index supplied");
		return 0;
	}

	return index < fGapIndex ? fBuffer[index] : fBuffer[index + fGapCount];
}


} // namespace BPrivate


#endif //__TEXTGAPBUFFER_H
