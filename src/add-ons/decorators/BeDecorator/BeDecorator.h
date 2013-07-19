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
class ServerBitmap;

class BeDecorAddOn : public DecorAddOn {
public:
								BeDecorAddOn(image_id id, const char* name);

protected:
	virtual Decorator*			_AllocateDecorator(DesktopSettings& settings,
									BRect rect);
};

class BeDecorator: public Decorator {
public:
	class Tab : public Decorator::Tab {
	public:
								Tab();

			uint32				tabOffset;
			float				tabLocation;
			float				textOffset;

			BString				truncatedTitle;
			int32				truncatedTitleLength;

			bool				buttonFocus : 1;

			bool				isHighlighted : 1;
			ServerBitmap*		closeBitmaps[4];
			ServerBitmap*		zoomBitmaps[4];

			float				minTabSize;
			float				maxTabSize;
	};

								BeDecorator(DesktopSettings& settings, BRect frame);
	virtual						~BeDecorator();

	virtual float				TabLocation(int32 tab) const;

	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;

	virtual Region				RegionAt(BPoint where, int32& tab) const;

protected:
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

			typedef rgb_color ComponentColors[7];

protected:
	virtual void				_DoLayout();
	virtual void				_DoTabLayout();
			void				_DistributeTabSize(float delta);

	virtual	Decorator::Tab*		_AllocateNewTab();
			BeDecorator::Tab*	_TabAt(int32 index) const;

	virtual	void				_DrawFrame(BRect rect);
	virtual	void				_DrawTab(Decorator::Tab* tab, BRect rect);
	virtual	void				_DrawTitle(Decorator::Tab* tab, BRect rect);

	virtual	void				_DrawClose(Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawZoom(Decorator::Tab* tab, bool direct,
									BRect rect);

	virtual	void				_SetTitle(Decorator::Tab* tab, const char* string,
									BRegion* updateRegion = NULL);

	virtual void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion);
	virtual void				_SetLook(Decorator::Tab* tab,
									DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
	virtual void				_SetFlags(Decorator::Tab* tab, uint32 flags,
									BRegion* updateRegion = NULL);

	virtual void				_SetFocus(Decorator::Tab* tab);

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual	bool				_AddTab(DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

	virtual bool				_SetTabLocation(Decorator::Tab* tab, float location,
									bool isShifting, BRegion* updateRegion = NULL);

	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion* region);

			void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;

	virtual	void				DrawButtons(Decorator::Tab* tab,
									const BRect& invalid);

private:
			void				_UpdateFont(DesktopSettings& settings);
			void				_DrawBlendedRect(BRect r, bool down);
			void				_LayoutTabItems(Decorator::Tab* _tab, const BRect& tabRect);
			void 				_InvalidateBitmaps();

	inline	float				_DefaultTextOffset() const;
	inline	float				_SingleTabOffsetAndSize(float& tabSize);

			void				_CalculateTabsRegion();

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

			BRegion				fTabsRegion;
			BRect				fOldMovingTab;

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
