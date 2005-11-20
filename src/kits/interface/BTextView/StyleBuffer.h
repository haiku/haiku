/*
 * Copyright 2001-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */


#include <Font.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>

#include "TextViewSupportBuffer.h"


typedef struct STEStyle {
	BFont			font;		// font
	rgb_color		color;		// pen color
} STEStyle;


typedef struct STEStyleRun {
	long			offset;		// byte offset of first character of run
	STEStyle		style;		// style info
} STEStyleRun;


typedef struct STEStyleRange {
	long			count;		// number of style runs
	STEStyleRun		runs[1];	// array of count number of runs
} STEStyleRange;


typedef struct STEStyleRecord {
	long			refs;		// reference count for this style
	float			ascent;		// ascent for this style
	float			descent;	// descent for this style
	STEStyle		style;		// style info
} STEStyleRecord;


typedef struct STEStyleRunDesc {
	long			offset;		// byte offset of first character of run
	long			index;		// index of corresponding style record
} STEStyleRunDesc;


// _BStyleRunDescBuffer_ class -------------------------------------------------
class _BStyleRunDescBuffer_ : public _BTextViewSupportBuffer_<STEStyleRunDesc> {
	public:
		_BStyleRunDescBuffer_();

		void	InsertDesc(STEStyleRunDesc* inDesc, int32 index);
		void	RemoveDescs(int32 index, int32 count = 1);

		long	OffsetToRun(int32 offset) const;
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


class _BInlineInput_;

// _BStyleBuffer_ class --------------------------------------------------------
class _BStyleBuffer_ {
	public:
						_BStyleBuffer_(const BFont *inFont,
							const rgb_color *inColor);

		void			InvalidateNullStyle();
		bool			IsValidNullStyle() const;

		void			SyncNullStyle(int32 offset);
		void			SetNullStyle(uint32 inMode, const BFont *inFont,
							  const rgb_color *inColor, int32 offset = 0);
		void			GetNullStyle(const BFont **font,
							const rgb_color **color) const;	

		void			SetStyleRange(int32 fromOffset, int32 toOffset,
							int32 textLen, uint32 inMode, 
							const BFont *inFont, const rgb_color *inColor);
		void			GetStyle(int32 inOffset, BFont *outFont,
							rgb_color *outColor) const;
		void			ContinuousGetStyle(BFont *, uint32 *, rgb_color *,
							bool *, int32, int32) const;
		STEStyleRange*	GetStyleRange(int32 startOffset, int32 endOffset) const;
		
		void			RemoveStyleRange(int32 fromOffset, int32 toOffset);
		void			RemoveStyles(int32 index, int32 count = 1);
		
		int32			Iterate(int32 fromOffset, int32 length, _BInlineInput_ *,
							const BFont **outFont = NULL,
							const rgb_color **outColor = NULL,
							float *outAscent = NULL,
							float *outDescen = NULL, uint32 * = NULL) const;	
		
		int32			OffsetToRun(int32 offset) const;
		void			BumpOffset(int32 delta, int32 index);
		
		void			SetStyle(uint32 mode, const BFont *fromFont,
						  BFont *toFont, const rgb_color *fromColor,
						  rgb_color *toColor);

		STEStyleRun		operator[](int32 index) const;
		int32			NumRuns() const;

		const _BStyleRunDescBuffer_& RunBuffer() const;
		const _BStyleRecordBuffer_& RecordBuffer() const;

	protected:
		_BStyleRunDescBuffer_ fStyleRunDesc;
		_BStyleRecordBuffer_ fStyleRecord;
		bool			fValidNullStyle;
		STEStyle		fNullStyle;
};


inline int32
_BStyleBuffer_::NumRuns() const
{
	return fStyleRunDesc.ItemCount();
}


inline const _BStyleRunDescBuffer_&
_BStyleBuffer_::RunBuffer() const
{
	return fStyleRunDesc;
}


inline const _BStyleRecordBuffer_&
_BStyleBuffer_::RecordBuffer() const
{
	return fStyleRecord;
}
