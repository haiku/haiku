/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef DEFAULT_DECORATOR_H
#define DEFAULT_DECORATOR_H


#include <Region.h>

#include "Decorator.h"


class Desktop;
class ServerBitmap;

class DefaultDecorator: public Decorator {
public:
								DefaultDecorator(DesktopSettings& settings,
									BRect frame, window_look look,
									uint32 flags);
	virtual						~DefaultDecorator();

	virtual float				TabLocation() const
									{ return (float)fTabOffset; }

	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;

	virtual	click_type			Clicked(BPoint pt, int32 buttons,
									int32 modifiers);

protected:
	virtual void				_DoLayout();

	virtual void				_DrawFrame(BRect r);
	virtual void				_DrawTab(BRect r);

	virtual void				_DrawClose(BRect r);
	virtual void				_DrawTitle(BRect r);
	virtual void				_DrawZoom(BRect r);

	virtual	void				_SetTitle(const char* string,
									BRegion* updateRegion = NULL);

	virtual void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion);
	virtual void				_SetLook(DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
	virtual void				_SetFlags(uint32 flags,
									BRegion* updateRegion = NULL);

	virtual void				_SetFocus();
	virtual void				_SetColors();

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual bool				_SetTabLocation(float location,
									BRegion* updateRegion = NULL);

	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion *region);


private:
			void				_UpdateFont(DesktopSettings& settings);
 			void				_DrawButtonBitmap(ServerBitmap* bitmap,
 									BRect rect);
			void				_DrawBlendedRect(DrawingEngine *engine,
									BRect rect, bool down, bool focus);
			void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;
			void				_LayoutTabItems(const BRect& tabRect);
			void 				_InvalidateBitmaps();
	static	ServerBitmap*		_GetBitmapForButton(int32 item, bool down,
									bool focus, int32 width, int32 height,
									DefaultDecorator* object);

protected:
			rgb_color			fButtonHighColor;
			rgb_color			fButtonLowColor;
			rgb_color			fTabColor;
			rgb_color			fFocusTabColor;
			rgb_color			fNonFocusTabColor;
			rgb_color			fTextColor;
			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;

			rgb_color			fTabColorLight;
			rgb_color			fTabColorBevel;
			rgb_color			fTabColorShadow;

			rgb_color			fFrameColors[6];
			rgb_color			fFocusFrameColors[2];
			rgb_color			fNonFocusFrameColors[2];

			bool				fButtonFocus;
			ServerBitmap*		fCloseBitmaps[4];
			ServerBitmap*		fZoomBitmaps[4];

			// Individual rects for handling window frame
			// rendering the proper way
			BRect				fRightBorder;
			BRect				fLeftBorder;
			BRect				fTopBorder;
			BRect				fBottomBorder;

			int32				fBorderWidth;

			uint32				fTabOffset;
			float				fTabLocation;
			float				fTextOffset;

			float				fMinTabSize;
			float				fMaxTabSize;
			BString				fTruncatedTitle;
			int32				fTruncatedTitleLength;

private:
			bigtime_t			fLastClicked;
			bool				fWasDoubleClick;
};

#endif	// DEFAULT_DECORATOR_H
