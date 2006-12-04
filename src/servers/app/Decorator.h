/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef _DECORATOR_H_
#define _DECORATOR_H_


#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include "DrawState.h"

class DesktopSettings;
class DrawingEngine;
class ServerFont;
class BRegion;


typedef enum {
	DEC_NONE = 0,
	DEC_ZOOM,
	DEC_CLOSE,
	DEC_MINIMIZE,
	DEC_TAB,
	DEC_DRAG,
	DEC_MOVETOBACK,
	DEC_SLIDETAB,
	
	DEC_RESIZE,
	CLICK_RESIZE_L,
	CLICK_RESIZE_T, 
	CLICK_RESIZE_R,
	CLICK_RESIZE_B,
	CLICK_RESIZE_LT,
	CLICK_RESIZE_RT, 
	CLICK_RESIZE_LB,
	CLICK_RESIZE_RB
} click_type;

class Decorator {
 public:
								Decorator(DesktopSettings& settings, BRect rect,
									window_look look, uint32 flags);
	virtual						~Decorator();

			void				SetDrawingEngine(DrawingEngine *driver);
	inline	DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }
			void				SetFont(ServerFont *font);

	virtual void				SetLook(DesktopSettings& settings,
									window_look look, BRegion* updateRegion = NULL);
	virtual void				SetFlags(uint32 flags, BRegion* updateRegion = NULL);

			void				SetClose(bool pressed);
			void				SetMinimize(bool pressed);
			void				SetZoom(bool pressed);

	virtual	void				SetTitle(const char* string, BRegion* updateRegion = NULL);

			window_look			Look() const;
			uint32				Flags() const;

			const char*			Title() const;

			// we need to know its border(frame). WinBorder's fFrame rect
			// must expand to include Decorator borders. Otherwise we can't
			// draw the border. We also add TabRect because I feel we'll need it
			BRect				BorderRect() const;
			BRect				TabRect() const;
		
			bool				GetClose();
			bool				GetMinimize();
			bool				GetZoom();

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;

			void				SetFocus(bool focussed);
			bool				IsFocus()
									{ return fIsFocused; };

	virtual	void				GetFootprint(BRegion *region);

	virtual	click_type			Clicked(BPoint pt, int32 buttons,
										int32 modifiers);

			void				MoveBy(float x, float y);
	virtual	void				MoveBy(BPoint pt);
			void				ResizeBy(float x, float y, BRegion* dirty);
	virtual	void				ResizeBy(BPoint pt, BRegion* dirty) = 0;

	/*! \return true if tab location updated, false if out of bounds or unsupported */
	virtual	bool				SetTabLocation(float location, BRegion* updateRegion = NULL)
									{ (void)updateRegion; return false; }
	virtual	float				TabLocation() const
									{ return 0.0; }

	virtual	bool				SetSettings(const BMessage& settings,
											BRegion* updateRegion = NULL);
	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				Draw(BRect r);
	virtual	void				Draw();
	virtual	void				DrawClose();
	virtual	void				DrawFrame();
	virtual	void				DrawMinimize();
	virtual	void				DrawTab();
	virtual	void				DrawTitle();
	virtual	void				DrawZoom();

			RGBColor			UIColor(color_which which);

 protected:
			int32				_ClipTitle(float width);

	/*!
		\brief Returns the number of characters in the title
		\return The title character count
	*/
			int32				_TitleWidth() const
									{ return fTitle.CountChars(); }

	virtual	void				_DoLayout();

	virtual	void				_DrawFrame(BRect r);
	virtual	void				_DrawTab(BRect r);

	virtual	void				_DrawClose(BRect r);
	virtual	void				_DrawTitle(BRect r);
	virtual	void				_DrawZoom(BRect r);
	virtual	void				_DrawMinimize(BRect r);

	virtual	void				_SetFocus();

			DrawingEngine*		fDrawingEngine;
			DrawState			fDrawState;

			window_look			fLook;
			uint32				fFlags;

			BRect				fZoomRect;
			BRect				fCloseRect;
			BRect				fMinimizeRect;
			BRect				fTabRect;
			BRect				fFrame;
			BRect				fResizeRect;
			BRect				fBorderRect;

 private:
			bool				fClosePressed;
			bool				fZoomPressed;
			bool				fMinimizePressed;

			bool				fIsFocused;

			BString				fTitle;
};

// add-on stuff
typedef float get_version(void);
typedef Decorator* create_decorator(DesktopSettings& desktopSettings, BRect rect,
	window_look look, uint32 flags);

#endif	/* _DECORATOR_H_ */
