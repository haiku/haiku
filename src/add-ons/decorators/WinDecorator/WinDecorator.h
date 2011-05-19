/*
 * Copyright 2009-2010, Haiku.
 * Distributed under the terms of the MIT License.
 */
#ifndef _WINDOWS_DECORATOR_H_
#define _WINDOWS_DECORATOR_H_


#include "DecorManager.h"
#include "RGBColor.h"


class WinDecorAddOn : public DecorAddOn {
public:
								WinDecorAddOn(image_id id, const char* name);


protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, window_look look, uint32 flags);
};


class WinDecorator: public Decorator {
public:
								WinDecorator(DesktopSettings& settings,
									BRect frame, window_look wlook,
									uint32 wflags);
								~WinDecorator(void);

			void				Draw(BRect r);
			void				Draw();

	virtual	Region				RegionAt(BPoint where) const;

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

			void				_MoveBy(BPoint pt);
			void				_ResizeBy(BPoint pt, BRegion* dirty);

			void				_GetFootprint(BRegion *region);
			void				_SetFocus(void);

private:
			void				_UpdateFont(DesktopSettings& settings);
			void				_DrawBeveledRect(BRect r, bool down);

private:
			uint32 taboffset;

			rgb_color tab_highcol;
			rgb_color tab_lowcol;
			rgb_color frame_highcol;
			rgb_color frame_midcol;
			rgb_color frame_lowcol;
			rgb_color frame_lowercol;
			rgb_color textcol;
			rgb_color			fFocusTabColor;
			rgb_color			fNonFocusTabColor;
			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
			uint64 solidhigh, solidlow;

			BString				fTruncatedTitle;
			int32				fTruncatedTitleLength;

			bool slidetab;
			int textoffset;
};


#endif	// _WINDOWS_DECORATOR_H_
