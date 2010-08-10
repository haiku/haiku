/*
 * Copyright 2009-2010, Haiku.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAC_DECORATOR_H_
#define _MAC_DECORATOR_H_


#include "DecorManager.h"
#include "RGBColor.h"


class MacDecorAddOn : public DecorAddOn {
public:
								MacDecorAddOn(image_id id, const char* name);

			float				Version();

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);
};


class MacDecorator: public Decorator {
public:
								MacDecorator(DesktopSettings& settings,
									BRect frame, window_look look,
									uint32 flags);
	virtual						~MacDecorator();

			void				Draw(BRect updateRect);
			void				Draw();

	click_type					MouseAction(const BMessage* message,
									BPoint point, int32 buttons,
									int32 modifiers);

protected:
			void				_DoLayout();

			void				_DrawFrame(BRect r);
			void				_DrawTab(BRect r);

			void				_DrawClose(BRect r);
			void				_DrawTitle(BRect r);
			void				_DrawZoom(BRect r);
			void				_DrawMinimize(BRect r);

			void				_SetTitle(const char* string,
	 								BRegion* updateRegion = NULL);

			void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion);
			void				_SetLook(DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
			void				_SetFlags(uint32 flags,
									BRegion* updateRegion = NULL);
	
			void				_SetColors();

			void				_MoveBy(BPoint offset);
			void				_ResizeBy(BPoint offset, BRegion* dirty);

			void				_GetFootprint(BRegion *region);

private:
			void				_UpdateFont(DesktopSettings& settings);
			void				_DrawBlendedRect(DrawingEngine* engine,
									BRect r, bool down);

			rgb_color			fButtonHighColor;
			rgb_color			fButtonLowColor;
			rgb_color			frame_highcol;
			rgb_color			frame_midcol;
			rgb_color			frame_lowcol;
			rgb_color			frame_lowercol;
			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;

			uint64 solidhigh, solidlow;

			BString				fTruncatedTitle;
			int32				fTruncatedTitleLength;

			int32 				fBorderWidth;

			bool slidetab;
			int textoffset;
			float titlepixelwidth;
};

#endif
