//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
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
//	File Name:		DefaultDecorator.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Fallback decorator for the app_server
//  
//------------------------------------------------------------------------------
#ifndef _DEFAULT_DECORATOR_H_
#define _DEFAULT_DECORATOR_H_

#include "Decorator.h"
#include <Region.h>

class DefaultDecorator: public Decorator {
public:
							DefaultDecorator(BRect frame,
											 int32 look,
											 int32 feel,
											 int32 flags);
	virtual					~DefaultDecorator();
	
	virtual	void			MoveBy(float x, float y);
	virtual	void			MoveBy(BPoint pt);
	virtual	void			ResizeBy(float x, float y);
	virtual	void			ResizeBy(BPoint pt);

	virtual	void			Draw(BRect r);
	virtual	void			Draw();

	virtual	void			GetSizeLimits(float* minWidth, float* minHeight,
										  float* maxWidth, float* maxHeight) const;

	virtual	void			GetFootprint(BRegion *region);

	virtual	click_type		Clicked(BPoint pt, int32 buttons,
									int32 modifiers);

protected:
	virtual void			_DoLayout();

	virtual void			_DrawFrame(BRect r);
	virtual void			_DrawTab(BRect r);

	virtual void			_DrawClose(BRect r);
	virtual void			_DrawTitle(BRect r);
	virtual void			_DrawZoom(BRect r);

	virtual void			_SetFocus();
	virtual void			_SetColors();

private:
			void			_DrawBlendedRect(BRect r, bool down);
			void			_GetButtonSizeAndOffset(const BRect& tabRect,
													float* offset, float*size) const;
			void			_LayoutTabItems(const BRect& tabRect);

			RGBColor		fButtonHighColor;
			RGBColor		fButtonLowColor;
			RGBColor		fTextColor;
			RGBColor		fTabColor;

			RGBColor*		fFrameColors;
	
			// Individual rects for handling window frame
			// rendering the proper way
			BRect			fRightBorder;
			BRect			fLeftBorder;
			BRect			fTopBorder;
			BRect			fBottomBorder;

			int32			fBorderWidth;

			uint32			fTabOffset;
			float			fTextOffset;

			float			fMinTabWidth;
			float			fMaxTabWidth;
			BString			fTruncatedTitle;
			int32			fTruncatedTitleLength;
};

#endif
