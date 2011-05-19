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

#include "DecorManager.h"
#include "RGBColor.h"


class Desktop;


class BeDecorAddOn : public DecorAddOn {
public:
								BeDecorAddOn(image_id id, const char* name);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);
};


class BeDecorator: public Decorator {
public:
								BeDecorator(DesktopSettings& settings,
									BRect frame, window_look look,
									uint32 flags);
	virtual						~BeDecorator();

	virtual float				TabLocation() const
									{ return (float)fTabOffset; }

	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;

	virtual Region				RegionAt(BPoint where) const;

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

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual bool				_SetTabLocation(float location,
									BRegion* updateRegion = NULL);

	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion* region);

private:
			void				_UpdateFont(DesktopSettings& settings);
			void				_DrawBlendedRect(BRect r, bool down);
			void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float*size) const;
			void				_LayoutTabItems(const BRect& tabRect);

private:
			RGBColor			fButtonHighColor;
			RGBColor			fButtonLowColor;
			RGBColor			fTextColor;
			RGBColor			fTabColor;

			RGBColor*			fFrameColors;

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

			bool				fWasDoubleClick;
};


#endif	// DEFAULT_DECORATOR_H
