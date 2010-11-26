/*
 * Copyright 2001-2010, Haiku.
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

			void			SetDrawingEngine(DrawingEngine *driver);
	inline	DrawingEngine*	GetDrawingEngine() const
								{ return fDrawingEngine; }

			void			FontsChanged(DesktopSettings& settings,
								BRegion* updateRegion = NULL);
			void			SetLook(DesktopSettings& settings, window_look look,
								BRegion* updateRegion = NULL);
			void			SetFlags(uint32 flags,
								BRegion* updateRegion = NULL);

			void			SetClose(bool pressed);
			void			SetMinimize(bool pressed);
			void			SetZoom(bool pressed);

			void			SetTitle(const char* string,
								BRegion* updateRegion = NULL);

			window_look		Look() const;
			uint32			Flags() const;

			const char*		Title() const;

			BRect			BorderRect() const;
			BRect			TabRect() const;

			bool			GetClose();
			bool			GetMinimize();
			bool			GetZoom();

	virtual	void			GetSizeLimits(int32* minWidth, int32* minHeight,
								int32* maxWidth, int32* maxHeight) const;

			void			SetFocus(bool focussed);
			bool			IsFocus()
								{ return fIsFocused; };

			const BRegion&	GetFootprint();

	virtual	Region			RegionAt(BPoint where) const;

			void			MoveBy(float x, float y);
			void			MoveBy(BPoint offset);
			void			ResizeBy(float x, float y, BRegion* dirty);
			void			ResizeBy(BPoint offset, BRegion* dirty);

	/*! \return true if tab location updated, false if out of bounds
		or unsupported
	*/
			bool			SetTabLocation(float location,
								BRegion* /*updateRegion*/ = NULL);
	virtual	float			TabLocation() const
								{ return 0.0; }

	virtual	bool			SetRegionHighlight(Region region, uint8 highlight,
								BRegion* dirty);
	inline	uint8			RegionHighlight(Region region) const;

			bool			SetSettings(const BMessage& settings,
								BRegion* updateRegion = NULL);
	virtual	bool			GetSettings(BMessage* settings) const;

	virtual	void			Draw(BRect rect);
	virtual	void			Draw();
	virtual	void			DrawClose();
	virtual	void			DrawFrame();
	virtual	void			DrawMinimize();
	virtual	void			DrawTab();
	virtual	void			DrawTitle();
	virtual	void			DrawZoom();

			rgb_color		UIColor(color_which which);

	virtual	void			ExtendDirtyRegion(Region region, BRegion& dirty);

protected:
			int32			_TitleWidth() const
								{ return fTitle.CountChars(); }

	virtual	void			_DoLayout();

	virtual	void			_DrawFrame(BRect rect);
	virtual	void			_DrawTab(BRect rect);

	virtual	void			_DrawClose(BRect rect);
	virtual	void			_DrawTitle(BRect rect);
	virtual	void			_DrawZoom(BRect rect);
	virtual	void			_DrawMinimize(BRect rect);

	virtual void			_FontsChanged(DesktopSettings& settings,
								BRegion* updateRegion = NULL);
	virtual void			_SetLook(DesktopSettings& settings,
								window_look look, BRegion* updateRegion = NULL);
	virtual void			_SetFlags(uint32 flags,
								BRegion* updateRegion = NULL);

	virtual	void			_SetTitle(const char* string,
								BRegion* updateRegion = NULL) = 0;

	virtual	void			_SetFocus();
	virtual void			_MoveBy(BPoint offset);
	virtual	void			_ResizeBy(BPoint offset, BRegion* dirty) = 0;

	virtual	bool			_SetTabLocation(float location,
								BRegion* /*updateRegion*/ = NULL)
								{ return false; }

	virtual bool			_SetSettings(const BMessage& settings,
								BRegion* updateRegion = NULL);

	virtual	void			_GetFootprint(BRegion *region);
			void			_InvalidateFootprint();

			DrawingEngine*	fDrawingEngine;
			DrawState		fDrawState;

			window_look		fLook;
			uint32			fFlags;

			BRect			fZoomRect;
			BRect			fCloseRect;
			BRect			fMinimizeRect;
			BRect			fTabRect;
			BRect			fFrame;
			BRect			fResizeRect;
			BRect			fBorderRect;

private:
			bool			fClosePressed : 1;
			bool			fZoomPressed : 1;
			bool			fMinimizePressed : 1;

			bool			fIsFocused : 1;

			BString			fTitle;

			BRegion			fFootprint;
			bool			fFootprintValid : 1;

			uint8			fRegionHighlights[REGION_COUNT - 1];
};


uint8
Decorator::RegionHighlight(Region region) const
{
	int32 index = (int32)region - 1;
	return index >= 0 && index < REGION_COUNT - 1
		? fRegionHighlights[index] : 0;
}


#endif	// DECORATOR_H
