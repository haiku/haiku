/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include <Font.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>
#include <TextView.h>

#include "TextViewSupportBuffer.h"


struct STEStyle {
	BFont		font;		// font
	rgb_color	color;		// pen color
};


struct STEStyleRun {
	long		offset;		// byte offset of first character of run
	STEStyle	style;		// style info
};


struct STEStyleRange {
	long		count;		// number of style runs
	STEStyleRun	runs[1];	// array of count number of runs
};


struct STEStyleRecord {
	long		refs;		// reference count for this style
	float		ascent;		// ascent for this style
	float		descent;	// descent for this style
	STEStyle	style;		// style info
};


struct STEStyleRunDesc {
	long		offset;		// byte offset of first character of run
	long		index;		// index of corresponding style record
};


// _BStyleRunDescBuffer_ class -------------------------------------------------
class _BStyleRunDescBuffer_ : public _BTextViewSupportBuffer_<STEStyleRunDesc> {
	public:
		_BStyleRunDescBuffer_();

		void	InsertDesc(STEStyleRunDesc* inDesc, int32 index);
		void	RemoveDescs(int32 index, int32 count = 1);

		int32	OffsetToRun(int32 offset) const;
		void	BumpOffset(int32 delta, int32 index);

		STEStyleRunDesc* operator[](int32 index) const;
};


inline STEStyleRunDesc*
_BStyleRunDescBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}


// _BStyleRecordBuffer_ class --------------------------------------------------
class _BStyleRecordBuffer_ : public _BTextViewSupportBuffer_<STEStyleRecord> {
	public:
		_BStyleRecordBuffer_();

		int32	InsertRecord(const BFont *inFont, const rgb_color *inColor);
		void	CommitRecord(int32 index);
		void	RemoveRecord(int32 index);

		bool	MatchRecord(const BFont *inFont, const rgb_color *inColor,
					int32 *outIndex);

		STEStyleRecord*	operator[](int32 index) const;
};


inline STEStyleRecord*
_BStyleRecordBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}


// StyleBuffer class --------------------------------------------------------
class BTextView::StyleBuffer {
	public:
						StyleBuffer(const BFont *inFont,
							const rgb_color *inColor);

		void			InvalidateNullStyle();
		bool			IsValidNullStyle() const;

		void			SyncNullStyle(int32 offset);
		void			SetNullStyle(uint32 inMode, const BFont *inFont,
							const rgb_color *inColor, int32 offset = 0);
		void			GetNullStyle(const BFont **font,
							const rgb_color **color) const;

		void			GetStyle(int32 inOffset, BFont *outFont,
							rgb_color *outColor) const;
		void			ContinuousGetStyle(BFont *, uint32 *, rgb_color *,
							bool *, int32, int32) const;

		STEStyleRange*	AllocateStyleRange(const int32 numStyles) const;
		void			SetStyleRange(int32 fromOffset, int32 toOffset,
							int32 textLen, uint32 inMode,
							const BFont *inFont, const rgb_color *inColor);
		STEStyleRange*	GetStyleRange(int32 startOffset,
							int32 endOffset) const;

		void			RemoveStyleRange(int32 fromOffset, int32 toOffset);
		void			RemoveStyles(int32 index, int32 count = 1);

		int32			Iterate(int32 fromOffset, int32 length,
							InlineInput* input,
							const BFont **outFont = NULL,
							const rgb_color **outColor = NULL,
							float *outAscent = NULL,
							float *outDescen = NULL, uint32 * = NULL) const;

		int32			OffsetToRun(int32 offset) const;
		void			BumpOffset(int32 delta, int32 index);

		STEStyleRun		operator[](int32 index) const;
		int32			NumRuns() const;

		const _BStyleRunDescBuffer_& RunBuffer() const;
		const _BStyleRecordBuffer_& RecordBuffer() const;

	private:
		_BStyleRunDescBuffer_	fStyleRunDesc;
		_BStyleRecordBuffer_	fStyleRecord;
		bool			fValidNullStyle;
		STEStyle		fNullStyle;
};


inline int32
BTextView::StyleBuffer::NumRuns() const
{
	return fStyleRunDesc.ItemCount();
}


inline const _BStyleRunDescBuffer_&
BTextView::StyleBuffer::RunBuffer() const
{
	return fStyleRunDesc;
}


inline const _BStyleRecordBuffer_&
BTextView::StyleBuffer::RecordBuffer() const
{
	return fStyleRecord;
}
