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
//	File Name:		StyleBuffer.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Style storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Font.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>

#include "TextViewSupportBuffer.h"


typedef struct STEStyle {
	BFont			font;		// font
	rgb_color		color;		// pen color
} STEStyle, *STEStylePtr;

typedef struct STEStyleRun {
	long			offset;		// byte offset of first character of run
	STEStyle		style;		// style info
} STEStyleRun, *STEStyleRunPtr;

typedef struct STEStyleRange {
	long			count;		// number of style runs
	STEStyleRun		runs[1];	// array of count number of runs
} STEStyleRange, *STEStyleRangePtr;

typedef struct STEStyleRecord {
	long			refs;		// reference count for this style
	float			ascent;		// ascent for this style
	float			descent;	// descent for this style
	STEStyle		style;		// style info
} STEStyleRecord, *STEStyleRecordPtr;

typedef struct STEStyleRunDesc {
	long			offset;		// byte offset of first character of run
	long			index;		// index of corresponding style record
} STEStyleRunDesc, *STEStyleRunDescPtr;

// Globals ---------------------------------------------------------------------

// _BStyleRunDescBuffer_ class -------------------------------------------------
class _BStyleRunDescBuffer_ : public _BTextViewSupportBuffer_<STEStyleRunDesc> {

public:
				_BStyleRunDescBuffer_();
						
		void	InsertDesc(STEStyleRunDescPtr inDesc, int32 index);
		void	RemoveDescs(int32 index, int32 count = 1);

		long	OffsetToRun(int32 offset) const;
		void	BumpOffset(int32 delta, int32 index);
	
		const STEStyleRunDescPtr	operator[](int32 index) const;
};
//------------------------------------------------------------------------------
inline const
STEStyleRunDescPtr _BStyleRunDescBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}
//------------------------------------------------------------------------------
	
// _BStyleRecordBuffer_ class --------------------------------------------------
class _BStyleRecordBuffer_ : public _BTextViewSupportBuffer_<STEStyleRecord> {

public:
				_BStyleRecordBuffer_();
						
		int32	InsertRecord(const BFont *inFont, const rgb_color *inColor);
		void	CommitRecord(int32 index);
		void	RemoveRecord(int32 index); 
	
		bool	MatchRecord(const BFont *inFont, const rgb_color *inColor,
					int32 *outIndex);
	
		const STEStyleRecordPtr	operator[](int32 index) const;
};
//------------------------------------------------------------------------------
inline const
STEStyleRecordPtr _BStyleRecordBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}
//------------------------------------------------------------------------------

class _BInlineInput_;
// _BStyleBuffer_ class --------------------------------------------------------
class _BStyleBuffer_ {

public:
							_BStyleBuffer_(const BFont *inFont,
								const rgb_color *inColor);
	
		void				InvalidateNullStyle();
		bool				IsValidNullStyle() const;
		
		void				SyncNullStyle(int32 offset);
		void				SetNullStyle(uint32 inMode, const BFont *inFont,
								  const rgb_color *inColor, int32 offset = 0);
		void				GetNullStyle(const BFont **font,
								const rgb_color **color) const;	
		
		void				SetStyleRange(int32 fromOffset, int32 toOffset,
								int32 textLen, uint32 inMode, 
								const BFont *inFont, const rgb_color *inColor);
		void				GetStyle(int32 inOffset, BFont *outFont,
								rgb_color *outColor) const;
		void				ContinuousGetStyle(BFont *, uint32 *, rgb_color *,
								bool *, int32, int32) const;
		STEStyleRangePtr	GetStyleRange(int32 startOffset, int32 endOffset) const;
		
		void				RemoveStyleRange(int32 fromOffset, int32 toOffset);
		void				RemoveStyles(int32 index, int32 count = 1);
		
		int32				Iterate(int32 fromOffset, int32 length, _BInlineInput_ *,
								const BFont **outFont = NULL,
								const rgb_color **outColor = NULL,
								float *outAscent = NULL,
								float *outDescen = NULL, uint32 * = NULL) const;	
		
		int32				OffsetToRun(int32 offset) const;
		void				BumpOffset(int32 delta, int32 index);
		
		void				SetStyle(uint32 mode, const BFont *fromFont,
							  BFont *toFont, const rgb_color *fromColor,
							  rgb_color *toColor);
								
		STEStyleRun			operator[](int32 index) const;
									
		int32							NumRuns() const;
		const _BStyleRunDescBuffer_&	RunBuffer() const;
		const _BStyleRecordBuffer_&		RecordBuffer() const;

protected:
		_BStyleRunDescBuffer_	fStyleRunDesc;
		_BStyleRecordBuffer_	fStyleRecord;
		bool					fValidNullStyle;
		STEStyle				fNullStyle;
};
//------------------------------------------------------------------------------
inline int32
_BStyleBuffer_::NumRuns() const
{
	return fStyleRunDesc.ItemCount();
}
//------------------------------------------------------------------------------
inline const
_BStyleRunDescBuffer_ &_BStyleBuffer_::RunBuffer() const
{
	return fStyleRunDesc;
}
//------------------------------------------------------------------------------	
inline const
_BStyleRecordBuffer_ &_BStyleBuffer_::RecordBuffer() const
{
	return fStyleRecord;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
