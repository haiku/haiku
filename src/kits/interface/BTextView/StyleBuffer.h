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
//	File Name:		StyleBuffer.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Style storage used by BTextView
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <SupportDefs.h>
#include <InterfaceDefs.h>
#include <Font.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "TextViewSupportBuffer.h"

// Local Defines ---------------------------------------------------------------
struct STEStyleRunDesc {
	int32	fOffset;
	int32	fStyle;
};

struct STEStyleRecord {
	BFont		fFont;
	rgb_color	fColor;
};

// Globals ---------------------------------------------------------------------

// _BStyleBuffer_ class --------------------------------------------------------
class _BStyleBuffer_ {

public:
						_BStyleBuffer_(const BFont *, const rgb_color *);
virtual					~_BStyleBuffer_();

		void			SetNullStyle(uint32, const BFont *, const rgb_color *, int32);
		void			GetNullStyle(const BFont **, const rgb_color **) const;
		void			SyncNullStyle(int32);
		void			InvalidateNullStyle();
		bool			IsValidNullStyle() const;

		void			SetStyle(uint32, const BFont *, BFont *, const rgb_color *,
							rgb_color *) const;
		void			GetStyle(uint32, BFont *, rgb_color *) const;
				
		void			SetStyleRange(int32, int32, int32,  uint32 mode, const BFont *,
							const rgb_color *);
		int32			GetStyleRange(int32, int32) const;

		void			ContinuousGetStyle(BFont *, uint32 *, rgb_color *, bool *,
							int32, int32) const;

		void			RemoveStyles(int32 from, int32 to);
		void			RemoveStyleRange(int32 from, int32 to);

		int32			OffsetToRun(int32 offset) const;

		void			BumpOffset(int32 run, int32 offset);		
//		void			Iterate(int32, int32, _BInlineInput_ *, const BFont **,
//							const rgb_color **, float *, uint32 *) const;

		STEStyleRecord	*operator[](int32);

private:
		_BTextViewSupportBuffer_<STEStyleRunDesc>	fRuns;
		_BTextViewSupportBuffer_<STEStyleRecord>	fRecords;
		STEStyleRecord								fNullStyle;
};
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
