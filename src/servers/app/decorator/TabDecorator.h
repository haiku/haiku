/*
 * Copyright 2001-2015 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef TAB_DECORATOR_H
#define TAB_DECORATOR_H


#include <Region.h>

#include "Decorator.h"


class Desktop;


class TabDecorator: public Decorator {
public:
								TabDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~TabDecorator();

protected:
			enum {
				COLOR_TAB_FRAME_LIGHT	= 0,
				COLOR_TAB_FRAME_DARK	= 1,
				COLOR_TAB				= 2,
				COLOR_TAB_LIGHT			= 3,
				COLOR_TAB_BEVEL			= 4,
				COLOR_TAB_SHADOW		= 5,
				COLOR_TAB_TEXT			= 6
			};

			enum {
				COLOR_BUTTON			= 0,
				COLOR_BUTTON_LIGHT		= 1
			};

			enum Component {
				COMPONENT_TAB,

				COMPONENT_CLOSE_BUTTON,
				COMPONENT_ZOOM_BUTTON,

				COMPONENT_LEFT_BORDER,
				COMPONENT_RIGHT_BORDER,
				COMPONENT_TOP_BORDER,
				COMPONENT_BOTTOM_BORDER,

				COMPONENT_RESIZE_CORNER
			};

			typedef rgb_color ComponentColors[7];

public:
	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

	virtual	bool				SetRegionHighlight(Region region,
									uint8 highlight, BRegion* dirty,
									int32 tab = -1);

	virtual void				UpdateColors(DesktopSettings& settings);

protected:
	virtual	void				_DoLayout();
	virtual	void				_DoTabLayout();
			void				_DistributeTabSize(float delta);

	virtual	void				_DrawFrame(BRect rect) = 0;
	virtual	void				_DrawTab(Decorator::Tab* tab, BRect r) = 0;

	virtual	void				_DrawButtons(Decorator::Tab* tab,
									const BRect& invalid);
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect r) = 0;
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect r) = 0;
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect r) = 0;

	virtual	void				_SetTitle(Decorator::Tab* tab,
									const char* string,
									BRegion* updateRegion = NULL);

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual	void				_SetFocus(Decorator::Tab* tab);
	virtual	bool				_SetTabLocation(Decorator::Tab* tab,
									float location, bool isShifting,
									BRegion* updateRegion = NULL);

	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion *region);

	virtual	void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;

	virtual	void				_UpdateFont(DesktopSettings& settings);

private:
			void				_LayoutTabItems(Decorator::Tab* tab,
									const BRect& tabRect);

protected:
	inline	float				_DefaultTextOffset() const;
	inline	float				_SingleTabOffsetAndSize(float& tabSize);

			void				_CalculateTabsRegion();

protected:
			BRegion				fTabsRegion;
			BRect				fOldMovingTab;

			rgb_color			fFocusFrameColor;

			rgb_color			fFocusTabColor;
			rgb_color			fFocusTabColorLight;
			rgb_color			fFocusTabColorBevel;
			rgb_color			fFocusTabColorShadow;
			rgb_color			fFocusTextColor;

			rgb_color			fNonFocusFrameColor;

			rgb_color			fNonFocusTabColor;
			rgb_color			fNonFocusTabColorLight;
			rgb_color			fNonFocusTabColorBevel;
			rgb_color			fNonFocusTabColorShadow;
			rgb_color			fNonFocusTextColor;
};


#endif	// TAB_DECORATOR_H
