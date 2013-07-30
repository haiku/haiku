/*
 * Copyright 2009-2013 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@gmail.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _WINDOWS_DECORATOR_H_
#define _WINDOWS_DECORATOR_H_


#include "Decorator.h"
#include "DecorManager.h"


struct rgb_color;


class BRect;

class WinDecorAddOn : public DecorAddOn {
public:
								WinDecorAddOn(image_id id, const char* name);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect);
};


class WinDecorator: public Decorator {
public:
								WinDecorator(DesktopSettings& settings, BRect frame);
								~WinDecorator(void);

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

protected:
	virtual	void				DrawButtons(Decorator::Tab* tab,
									const BRect& invalid);

			void				_DoLayout();

	virtual	Decorator::Tab*		_AllocateNewTab();
			WinDecorator::Tab*	_TabAt(int32 index) const;

	virtual	void				_DrawFrame(BRect rect);
	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);

	virtual	void				_DrawMinimize(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);

	virtual	void				_SetTitle(Decorator::Tab* tab, const char* string,
									BRegion* updateRegion = NULL);

			void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion);
			void				_SetLook(Decorator::Tab* tab, DesktopSettings& settings,
									window_look look, BRegion* updateRegion = NULL);
			void				_SetFlags(Decorator::Tab* tab, uint32 flags,
									BRegion* updateRegion = NULL);

			void				_MoveBy(BPoint offset);
			void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

			void				_GetFootprint(BRegion *region);
	virtual	void				_SetFocus(Decorator::Tab* tab);

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
};


#endif	// _WINDOWS_DECORATOR_H_
