/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef DEFAULT_DECORATOR_H
#define DEFAULT_DECORATOR_H


#include "Decorator.h"
#include <Region.h>

class Desktop;


class DefaultDecorator: public Decorator {
public:
							DefaultDecorator(DesktopSettings& settings, BRect frame,
								window_look look, uint32 flags);
	virtual					~DefaultDecorator();

	virtual	void			SetTitle(const char* string, BRegion* updateRegion = NULL);
	virtual void			SetLook(DesktopSettings& settings,
								window_look look, BRegion* updateRegion = NULL);
	virtual void			SetFlags(uint32 flags, BRegion* updateRegion = NULL);

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

#endif	/* DEFAULT_DECORATOR_H */
