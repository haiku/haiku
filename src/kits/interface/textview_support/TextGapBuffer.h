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


class BFile;
// _BTextGapBuffer_ class ------------------------------------------------------
class _BTextGapBuffer_ {

public:
					_BTextGapBuffer_();
virtual				~_BTextGapBuffer_();

		void		InsertText(const char *inText, int32 inNumItems, int32 inAtIndex);
		void		InsertText(BFile *file, int32 fileOffset, int32 amount, int32 atIndex);
		void		RemoveRange(int32 start, int32 end);
		
		void		MoveGapTo(int32 toIndex);
		void		SizeGapTo(int32 inCount);

		const char *GetString(int32 fromOffset, int32 numChars);
		bool		FindChar(char inChar, int32 fromIndex, int32 *ioDelta);

		const char *Text();
		int32		Length() const;
		char		operator[](int32 index) const;

		
//		char 	*RealText();
		void	GetString(int32 offset, int32 length, char *buffer);
//		void	GetString(int32, int32 *);
		
		char	RealCharAt(int32 offset) const;
				
		bool	PasswordMode() const;
		void	SetPasswordMode(bool);

//		void	Resize(int32 size);

protected:
		int32	fExtraCount;		// when realloc()-ing
		int32	fItemCount;			// logical count
		char	*fBuffer;			// allocated memory
		int32	fBufferCount;		// physical count
		int32	fGapIndex;			// gap position
		int32	fGapCount;			// gap count
		char	*fScratchBuffer;	// for GetString
		int32	fScratchSize;		// scratch size
		bool	fPasswordMode;
};


inline int32 
_BTextGapBuffer_::Length() const
{
	return fItemCount;
}


inline char 
_BTextGapBuffer_::operator[](long index) const
{
	return (index < fGapIndex) ? fBuffer[index] : fBuffer[index + fGapCount];
}

#endif //__TEXTGAPBUFFER_H
