/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef DEFAULT_DECORATOR_H
#define DEFAULT_DECORATOR_H


#include <Region.h>

#include "Decorator.h"


class Desktop;
class ServerBitmap;

class DefaultDecorator: public Decorator {
public:
								DefaultDecorator(DesktopSettings& settings,
									BRect frame, window_look look,
									uint32 flags);
	virtual						~DefaultDecorator();

	virtual float				TabLocation() const
									{ return (float)fTabOffset; }

	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				Draw();

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;

	virtual	Region				RegionAt(BPoint where) const;

	virtual	bool				SetRegionHighlight(Region region,
									uint8 highlight, BRegion* dirty);

	virtual	void				ExtendDirtyRegion(Region region,
									BRegion& dirty);

			float				BorderWidth();
			float				TabHeight();

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

	virtual void				_DrawFrame(BRect r);
	virtual void				_DrawTab(BRect r);

	virtual void				_DrawClose(BRect r);
	virtual void				_DrawTitle(BRect r);
	virtual void				_DrawZoom(BRect r);

	virtual	void				_SetTitle(const char* string,
									BRegion* updateRegion = NULL);

	virtual void				_FontsChanged(DesktopSettings& settings,
									BRegion* updateRegion);
	virtual void				_SetLook(DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
	virtual void				_SetFlags(uint32 flags,
									BRegion* updateRegion = NULL);

	virtual void				_SetFocus();

	virtual	void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty);

	virtual bool				_SetTabLocation(float location,
									BRegion* updateRegion = NULL);

	virtual	bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	void				_GetFootprint(BRegion *region);

			void				_GetButtonSizeAndOffset(const BRect& tabRect,
									float* offset, float* size,
									float* inset) const;

	// DefaultDecorator customization points
	virtual	void				DrawButtons(const BRect& invalid);
	virtual	void				GetComponentColors(Component component,
									uint8 highlight, ComponentColors _colors);

private:
			void				_UpdateFont(DesktopSettings& settings);
 			void				_DrawButtonBitmap(ServerBitmap* bitmap,
 									BRect rect);
			void				_DrawBlendedRect(DrawingEngine *engine,
									BRect rect, bool down,
									const ComponentColors& colors);
			void				_LayoutTabItems(const BRect& tabRect);
			void 				_InvalidateBitmaps();
			ServerBitmap*		_GetBitmapForButton(Component item, bool down,
									int32 width, int32 height);

			void				_GetComponentColors(Component component,
									ComponentColors _colors);

protected:
	static	const rgb_color		kFrameColors[4];
	static	const rgb_color		kFocusFrameColors[2];
	static	const rgb_color		kNonFocusFrameColors[2];

			const rgb_color		kFocusTabColor;
			const rgb_color		kFocusTabColorLight;
			const rgb_color		kFocusTabColorBevel;
			const rgb_color		kFocusTabColorShadow;
			const rgb_color		kFocusTextColor;

			const rgb_color		kNonFocusTabColor;
			const rgb_color		kNonFocusTabColorLight;
			const rgb_color		kNonFocusTabColorBevel;
			const rgb_color		kNonFocusTabColorShadow;
			const rgb_color		kNonFocusTextColor;

			bool				fButtonFocus;
			ServerBitmap*		fCloseBitmaps[4];
			ServerBitmap*		fZoomBitmaps[4];

			// Individual rects for handling window frame
			// rendering the proper way
			BRect				fRightBorder;
			BRect				fLeftBorder;
			BRect				fTopBorder;
			BRect				fBottomBorder;

			int32				fBorderWidth;

			uint32				fTabOffset;
			float				fTabLocation;
			float				fTextOffset;

			float				fMinTabSize;
			float				fMaxTabSize;
			BString				fTruncatedTitle;
			int32				fTruncatedTitleLength;
};


#endif	// DEFAULT_DECORATOR_H
