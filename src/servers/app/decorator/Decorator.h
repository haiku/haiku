/*
 * Copyright 2001-2015 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover <looncraz@looncraz.net>
 */
#ifndef DECORATOR_H
#define DECORATOR_H


#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include "DrawState.h"
#include "MultiLocker.h"

class Desktop;
class DesktopSettings;
class DrawingEngine;
class ServerBitmap;
class ServerFont;
class BRegion;


class Decorator {
public:
	struct Tab {
							Tab();
		virtual				~Tab() {}

		BRect				tabRect;

		BRect				zoomRect;
		BRect				closeRect;
		BRect				minimizeRect;

		bool				closePressed : 1;
		bool				zoomPressed : 1;
		bool				minimizePressed : 1;

		window_look			look;
		uint32				flags;
		bool				isFocused : 1;

		BString				title;

		uint32				tabOffset;
		float				tabLocation;
		float				textOffset;

		BString				truncatedTitle;
		int32				truncatedTitleLength;

		bool				buttonFocus : 1;

		bool				isHighlighted : 1;

		float				minTabSize;
		float				maxTabSize;

		ServerBitmap*		closeBitmaps[4];
		ServerBitmap*		minimizeBitmaps[4];
		ServerBitmap*		zoomBitmaps[4];
	};

	enum Region {
		REGION_NONE,

		REGION_TAB,

		REGION_CLOSE_BUTTON,
		REGION_ZOOM_BUTTON,
		REGION_MINIMIZE_BUTTON,

		REGION_LEFT_BORDER,
		REGION_RIGHT_BORDER,
		REGION_TOP_BORDER,
		REGION_BOTTOM_BORDER,

		REGION_LEFT_TOP_CORNER,
		REGION_LEFT_BOTTOM_CORNER,
		REGION_RIGHT_TOP_CORNER,
		REGION_RIGHT_BOTTOM_CORNER,

		REGION_COUNT
	};

	enum {
		HIGHLIGHT_NONE,
		HIGHLIGHT_RESIZE_BORDER,

		HIGHLIGHT_USER_DEFINED
	};

								Decorator(DesktopSettings& settings,
											BRect frame,
											Desktop* desktop);
	virtual						~Decorator();

	virtual	Decorator::Tab*		AddTab(DesktopSettings& settings,
									const char* title, window_look look,
									uint32 flags, int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

	virtual int32				TabAt(const BPoint& where) const;
			Decorator::Tab*		TabAt(int32 index) const
									{ return fTabList.ItemAt(index); }
			int32				CountTabs() const
									{ return fTabList.CountItems(); }
			void				SetTopTab(int32 tab);

			void				SetDrawingEngine(DrawingEngine *driver);
	inline	DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

			void				FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion = NULL);
			void				ColorsChanged(DesktopSettings& settings,
									BRegion* updateRegion = NULL);

	virtual void				UpdateColors(DesktopSettings& settings) = 0;

			void				SetLook(int32 tab, DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
			void				SetFlags(int32 tab, uint32 flags,
									BRegion* updateRegion = NULL);

			window_look			Look(int32 tab) const;
			uint32				Flags(int32 tab) const;

			BRect				BorderRect() const;
			BRect				TitleBarRect() const;
			BRect				TabRect(int32 tab) const;
			BRect				TabRect(Decorator::Tab* tab) const;

			void				SetClose(int32 tab, bool pressed);
			void				SetMinimize(int32 tab, bool pressed);
			void				SetZoom(int32 tab, bool pressed);

			const char*			Title(int32 tab) const;
			const char*			Title(Decorator::Tab* tab) const;
			void				SetTitle(int32 tab, const char* string,
									BRegion* updateRegion = NULL);

			void				SetFocus(int32 tab, bool focussed);
			bool				IsFocus(int32 tab) const;
			bool				IsFocus(Decorator::Tab* tab) const;

	virtual	float				TabLocation(int32 tab) const;
			bool				SetTabLocation(int32 tab, float location,
									bool isShifting,
									BRegion* updateRegion = NULL);
				/*! \return true if tab location updated, false if out of
					bounds or unsupported */

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

			const BRegion&		GetFootprint();
			::Desktop*			GetDesktop();

			void				MoveBy(float x, float y);
			void				MoveBy(BPoint offset);
			void				ResizeBy(float x, float y, BRegion* dirty);
			void				ResizeBy(BPoint offset, BRegion* dirty);

	virtual	bool				SetRegionHighlight(Region region,
									uint8 highlight, BRegion* dirty,
									int32 tab = -1);
	inline	uint8				RegionHighlight(Region region,
									int32 tab = -1) const;

			bool				SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);
	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;
	virtual	void				ExtendDirtyRegion(Region region, BRegion& dirty);

	virtual	void				Draw(BRect updateRect) = 0;
	virtual	void				Draw() = 0;

	virtual	void				DrawTab(int32 tab);
	virtual	void				DrawTitle(int32 tab);

	virtual	void				DrawClose(int32 tab);
	virtual	void				DrawMinimize(int32 tab);
	virtual	void				DrawZoom(int32 tab);

			rgb_color			UIColor(color_which which);

			float				BorderWidth();
			float				TabHeight();

protected:
	virtual	Decorator::Tab*		_AllocateNewTab();

	virtual	void				_DoLayout() = 0;
		//! method for calculating layout for the decorator

	virtual	void				_DrawFrame(BRect rect) = 0;
	virtual	void				_DrawTabs(BRect rect);

	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect) = 0;
	virtual	void				_DrawTitle(Decorator::Tab* tab,
									BRect rect) = 0;

	virtual	void				_DrawButtons(Decorator::Tab* tab,
									const BRect& invalid) = 0;
	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect) = 0;
	virtual	void				_DrawMinimize(Decorator::Tab* tab, bool direct,
									BRect rect) = 0;
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect) = 0;

	virtual	void				_SetTitle(Decorator::Tab* tab,
									const char* string,
									BRegion* updateRegion = NULL) = 0;
			int32				_TitleWidth(Decorator::Tab* tab) const
									{ return tab->title.CountChars(); }

	virtual	void				_SetFocus(Decorator::Tab* tab);
	virtual	bool				_SetTabLocation(Decorator::Tab* tab,
									float location, bool isShifting,
									BRegion* updateRegion = NULL);

	virtual	Decorator::Tab*		_TabAt(int32 index) const;

	virtual void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion = NULL);
	virtual	void				_UpdateFont(DesktopSettings& settings) = 0;

	virtual void				_SetLook(Decorator::Tab* tab,
									DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
	virtual void				_SetFlags(Decorator::Tab* tab, uint32 flags,
									BRegion* updateRegion = NULL);

	virtual void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty) = 0;

	virtual bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL) = 0;
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL) = 0;
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL) = 0;

	virtual	void				_GetFootprint(BRegion *region);
			void				_InvalidateFootprint();

			void 				_InvalidateBitmaps();

protected:
	mutable		MultiLocker	fLocker;

			DrawingEngine*		fDrawingEngine;
			DrawState			fDrawState;

			// Individual rects for handling window frame
			// rendering the proper way
			BRect				fTitleBarRect;
			BRect				fFrame;
			BRect				fResizeRect;
			BRect				fBorderRect;

			BRect				fLeftBorder;
			BRect				fTopBorder;
			BRect				fBottomBorder;
			BRect				fRightBorder;

			int32				fBorderWidth;

			Decorator::Tab*		fTopTab;
			BObjectList<Decorator::Tab>	fTabList;

private:
			Desktop*			fDesktop;
			BRegion				fFootprint;
			bool				fFootprintValid : 1;

			uint8				fRegionHighlights[REGION_COUNT - 1];
};


uint8
Decorator::RegionHighlight(Region region, int32 tab) const
{
	int32 index = (int32)region - 1;
	return index >= 0 && index < REGION_COUNT - 1
		? fRegionHighlights[index] : 0;
}


#endif	// DECORATOR_H
