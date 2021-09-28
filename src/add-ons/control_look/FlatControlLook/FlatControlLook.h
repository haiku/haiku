/*
 * Copyright 2009-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol, revol@free.fr
 *		Nahuel Tello, ntello@unarix.com.ar
 */
#ifndef FLAT_CONTROL_LOOK_H
#define FLAT_CONTROL_LOOK_H


#include <ControlLook.h>

#include "HaikuControlLook.h"


class BBitmap;
class BControl;
class BGradientLinear;
class BView;

namespace BPrivate {

using BPrivate::HaikuControlLook;

class FlatControlLook : public HaikuControlLook {
public:
								FlatControlLook();
	virtual						~FlatControlLook();


	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuItemBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base,
									const rgb_color& background, uint32 flags, uint32 borders);
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect, float radius, const rgb_color& base,
									const rgb_color& background, uint32 flags, uint32 borders);
	virtual void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect, float leftTopRadius,
									float rightTopRadius, float leftBottomRadius,
									float rightBottomRadius, const rgb_color& base,
									const rgb_color& background, uint32 flags, uint32 borders);


	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	void						_DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	void						_DrawNonFlatButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									BRegion& clipping,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator,
									uint32 flags,
									uint32 borders,
									orientation orientation);

	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	virtual	void				DrawMenuFieldBackground(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base,
									bool popupIndicator, uint32 flags);
	virtual	void				DrawMenuFieldBackground(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base,
									uint32 flags, uint32 borders);
	virtual	void				DrawMenuFieldBackground(BView* view, BRect& rect,
									const BRect& updateRect, float radius, const rgb_color& base,
									bool popupIndicator, uint32 flags);
	virtual	void				DrawMenuFieldBackground(BView* view, BRect& rect,
									const BRect& updateRect, float leftTopRadius,
									float rightTopRadius, float leftBottomRadius,
									float rightBottomRadius, const rgb_color& base,
									bool popupIndicator, uint32 flags);
	virtual	void				DrawSplitter(BView* view, BRect& rect, const BRect& updateRect,
									const rgb_color& base, orientation orientation, uint32 flags,
									uint32 borders);
	void 						_DrawMenuFieldBackgroundInside(BView* view, BRect& rect,
									const BRect& updateRect, float leftTopRadius,
									float rightTopRadius, float leftBottomRadius,
									float rightBottomRadius, const rgb_color& base,
									uint32 flags, uint32 borders);
	void						_DrawMenuFieldBackgroundOutside(BView* view, BRect& rect,
									const BRect& updateRect, float leftTopRadius,
									float rightTopRadius, float leftBottomRadius,
									float rightBottomRadius, const rgb_color& base,
									bool popupIndicator, uint32 flags);
	virtual void				DrawRaisedBorder(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base, uint32 flags,
									uint32 borders);
	virtual void				_DrawOuterResessedFrame(BView* view, BRect& rect,
									const rgb_color& base, float contrast, float brightness,
									uint32 flags, uint32 borders);
	virtual void				DrawScrollBarThumb(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base, uint32 flags,
									orientation orientation, uint32 knobStyle);
	virtual void				DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
									const rgb_color& base, border_style borderStyle, uint32 flags,
									uint32 borders);
	rgb_color					_EdgeLightColor(const rgb_color& base, float contrast,
									float brightness, uint32 flags);
	rgb_color					_EdgeShadowColor(const rgb_color& base, float contrast,
									float brightness, uint32 flags);
};

}// bprivate

#endif	// FLAT_CONTROL_LOOK_H
