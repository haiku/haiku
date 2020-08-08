/*
 * Copyright 2009-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Adrien Destugues, pulkomandy@gmail.com
 *		John Scipione, jscipione@gmail.com
 */
#ifndef MAC_DECORATOR_H
#define MAC_DECORATOR_H


#include "DecorManager.h"
#include "RGBColor.h"
#include "SATDecorator.h"


class MacDecorAddOn : public DecorAddOn {
public:
								MacDecorAddOn(image_id id, const char* name);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect, Desktop* desktop);
};


class MacDecorator: public SATDecorator {
public:
								MacDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~MacDecorator();

			void				Draw(BRect updateRect);
			void				Draw();

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

	virtual	bool				SetRegionHighlight(Region region,
									uint8 highlight, BRegion* dirty,
									int32 tab = -1);

protected:
			void				_DoLayout();

	virtual	void				_DrawFrame(BRect invalid);

	virtual	void				_DrawTab(Decorator::Tab* tab, BRect invalid);
	virtual	void				_DrawButtons(Decorator::Tab* tab,
									const BRect& invalid);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);

	virtual	void				_DrawMinimize(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);

	virtual	void				_SetTitle(Tab* tab, const char* string,
									BRegion* updateRegion = NULL);

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

			Decorator::Tab*		_AllocateNewTab();

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion *region);

	virtual	void				_UpdateFont(DesktopSettings& settings);

private:
			void				_DrawButton(Decorator::Tab* tab, bool direct,
									BRect rect, bool pressed);
			void				_DrawBlendedRect(DrawingEngine* engine,
									BRect r, bool down);

			rgb_color			fButtonHighColor;
			rgb_color			fButtonLowColor;

			rgb_color			fFrameHighColor;
			rgb_color			fFrameMidColor;
			rgb_color			fFrameLowColor;
			rgb_color			fFrameLowerColor;

			rgb_color			fFocusTextColor;
			rgb_color			fNonFocusTextColor;
};


#endif	// MAC_DECORATOR_H
