/*
 * Copyright 2001-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef DECORATOR_H
#define DECORATOR_H


#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include "DrawState.h"

class DesktopSettings;
class DrawingEngine;
class ServerFont;
class BRegion;


class Decorator {
public:
			class Tab {
			public:
								Tab();
				virtual			~Tab() {}

				BRect			zoomRect;
				BRect			closeRect;
				BRect			minimizeRect;
				BRect			tabRect;

				bool			closePressed : 1;
				bool			zoomPressed : 1;
				bool			minimizePressed : 1;

				bool			isFocused : 1;

				BString			title;
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

public:
							Decorator(DesktopSettings& settings, BRect rect,
								window_look look, uint32 flags);
	virtual					~Decorator();

	virtual	Decorator::Tab*	AddTab(const char* title, int32 index = -1,
								BRegion* updateRegion = NULL);
	virtual	bool			RemoveTab(int32 index,
								BRegion* updateRegion = NULL);
	virtual	bool			MoveTab(int32 from, int32 to, bool isMoving,
								BRegion* updateRegion = NULL);
	virtual int32			TabAt(const BPoint& where) const;
			Decorator::Tab*	TabAt(int32 index)
								{ return fTabList.ItemAt(index); }
			int32			CountTabs() const
								{ return fTabList.CountItems(); }
			void			SetTopTap(int32 tab);

			void			SetDrawingEngine(DrawingEngine *driver);
	inline	DrawingEngine*	GetDrawingEngine() const
								{ return fDrawingEngine; }

			void			FontsChanged(DesktopSettings& settings,
								BRegion* updateRegion = NULL);
			void			SetLook(DesktopSettings& settings, window_look look,
								BRegion* updateRegion = NULL);
			void			SetFlags(uint32 flags,
								BRegion* updateRegion = NULL);

			window_look		Look() const;
			uint32			Flags() const;

			BRect			BorderRect() const;
			BRect			TitleBarRect() const;
			BRect			TabRect(int32 tab) const;
			BRect			TabRect(Decorator::Tab* tab) const;

			void			SetClose(int32 tab, bool pressed);
			void			SetMinimize(int32 tab, bool pressed);
			void			SetZoom(int32 tab, bool pressed);

			const char*		Title(int32 tab) const;
			const char*		Title(Decorator::Tab* tab) const;
			void			SetTitle(int32 tab, const char* string,
								BRegion* updateRegion = NULL);

			void			SetFocus(int32 tab, bool focussed);
			bool			IsFocus(int32 tab) const;
			bool			IsFocus(Decorator::Tab* tab) const;

			/*! \return true if tab location updated, false if out of bounds
			or unsupported */
			bool			SetTabLocation(int32 tab, float location,
								bool isShifting, BRegion* updateRegion = NULL);
	virtual	float			TabLocation(int32 tab) const
								{ return 0.0; }

	virtual	Region			RegionAt(BPoint where, int32& tab) const;

	virtual	void			GetSizeLimits(int32* minWidth, int32* minHeight,
								int32* maxWidth, int32* maxHeight) const;

			const BRegion&	GetFootprint();

			void			MoveBy(float x, float y);
			void			MoveBy(BPoint offset);
			void			ResizeBy(float x, float y, BRegion* dirty);
			void			ResizeBy(BPoint offset, BRegion* dirty);

	virtual	bool			SetRegionHighlight(Region region, uint8 highlight,
								BRegion* dirty, int32 tab = -1);
	inline	uint8			RegionHighlight(Region region,
								int32 tab = -1) const;

			bool			SetSettings(const BMessage& settings,
								BRegion* updateRegion = NULL);
	virtual	bool			GetSettings(BMessage* settings) const;

	virtual	void			Draw(BRect rect);
	virtual	void			Draw();
	virtual	void			DrawClose(int32 tab);
	virtual	void			DrawMinimize(int32 tab);
	virtual	void			DrawTab(int32 tab);
	virtual	void			DrawTitle(int32 tab);
	virtual	void			DrawZoom(int32 tab);
	virtual	void			DrawFrame();

			rgb_color		UIColor(color_which which);

	virtual	void			ExtendDirtyRegion(Region region, BRegion& dirty);

protected:
	virtual	void			_DoLayout();

	virtual	void			_DrawFrame(BRect rect);

	virtual	void			_DrawTabs(BRect rect);
	virtual	void			_DrawTab(Decorator::Tab* tab, BRect rect);
	virtual	void			_DrawClose(Decorator::Tab* tab, BRect rect);
	virtual	void			_DrawTitle(Decorator::Tab* tab, BRect rect);
	virtual	void			_DrawZoom(Decorator::Tab* tab, BRect rect);
	virtual	void			_DrawMinimize(Decorator::Tab* tab, BRect rect);

	virtual	Decorator::Tab*	_AllocateNewTab() = 0;

	virtual	void			_SetTitle(Decorator::Tab* tab, const char* string,
								BRegion* updateRegion = NULL) = 0;
			int32			_TitleWidth(Decorator::Tab* tab) const
								{ return tab->title.CountChars(); }

	virtual	bool			_SetTabLocation(Decorator::Tab* tab, float location,
								bool isShifting, BRegion* updateRegion = NULL);
	virtual	void			_SetFocus(Decorator::Tab* tab);

	virtual void			_FontsChanged(DesktopSettings& settings,
								BRegion* updateRegion = NULL);
	virtual void			_SetLook(DesktopSettings& settings,
								window_look look, BRegion* updateRegion = NULL);
	virtual void			_SetFlags(uint32 flags,
								BRegion* updateRegion = NULL);

	virtual void			_MoveBy(BPoint offset);
	virtual	void			_ResizeBy(BPoint offset, BRegion* dirty) = 0;

	virtual bool			_SetSettings(const BMessage& settings,
								BRegion* updateRegion = NULL);

	virtual bool			_AddTab(int32 index = -1,
								BRegion* updateRegion = NULL) = 0;
	virtual	bool			_RemoveTab(int32 index,
								BRegion* updateRegion = NULL) = 0;
	virtual	bool			_MoveTab(int32 from, int32 to, bool isMoving,
								BRegion* updateRegion = NULL) = 0;

	virtual	void			_GetFootprint(BRegion *region);
			void			_InvalidateFootprint();

			DrawingEngine*	fDrawingEngine;
			DrawState		fDrawState;

			window_look		fLook;
			uint32			fFlags;

			BRect			fTitleBarRect;
			BRect			fFrame;
			BRect			fResizeRect;
			BRect			fBorderRect;

			Decorator::Tab*	fTopTab;
			BObjectList<Decorator::Tab>	fTabList;
private:
			BRegion			fFootprint;
			bool			fFootprintValid : 1;

			uint8			fRegionHighlights[REGION_COUNT - 1];
};


uint8
Decorator::RegionHighlight(Region region, int32 tab) const
{
	int32 index = (int32)region - 1;
	return index >= 0 && index < REGION_COUNT - 1
		? fRegionHighlights[index] : 0;
}


#endif	// DECORATOR_H
