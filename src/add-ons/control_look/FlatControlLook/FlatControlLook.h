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


	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

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

	virtual	void				DrawMenuBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawScrollBarBorder(BView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarButton(BView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, const rgb_color& text,
									uint32 flags, int32 direction, orientation orientation,
									bool down = false);
	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect1, BRect& rect2,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarThumb(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation,
									uint32 knobStyle = B_KNOB_NONE);

	virtual	void				DrawScrollViewFrame(BView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style borderStyle,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	rgb_color			SliderBarColor(const rgb_color& base);

	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base,
									rgb_color leftFillColor,
									rgb_color rightFillColor,
									float sliderScale, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, rgb_color fillColor,
									uint32 flags, orientation orientation);

	virtual	void				DrawSliderThumb(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);

	virtual	void				DrawActiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0);

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

	virtual	void				DrawGroupFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawTextControlBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

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
	virtual void				DrawRaisedBorder(BView* view, BRect& rect,
									const BRect& updateRect, const rgb_color& base, uint32 flags,
									uint32 borders);
	virtual void				DrawBorder(BView* view, BRect& rect, const BRect& updateRect,
									const rgb_color& base, border_style borderStyle, uint32 flags,
									uint32 borders);

protected:
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

	void						_DrawOuterResessedFrame(BView* view, BRect& rect, const rgb_color& base, float contrast, 
									float brightness, uint32 flags = 0, uint32 borders = B_ALL_BORDERS);

	void						_DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect, float leftTopRadius,
									float rightTopRadius, float leftBottomRadius, float rightBottomRadius,
									const rgb_color& base, const rgb_color& background, float contrast,
									float brightness = 1.0, uint32 flags = 0, uint32 borders = B_ALL_BORDERS);

	void						_DrawPopUpMarker(BView* view, const BRect& rect,
									const rgb_color& base, uint32 flags);

	rgb_color					_EdgeLightColor(const rgb_color& base, float contrast,
									float brightness, uint32 flags);
	rgb_color					_EdgeShadowColor(const rgb_color& base, float contrast,
									float brightness, uint32 flags);

	rgb_color					_BevelLightColor(const rgb_color& base, uint32 flags);

	rgb_color					_BevelShadowColor(const rgb_color& base, uint32 flags);

	void						_MakeGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float bottomTint,
									orientation orientation = B_HORIZONTAL) const;

	void						_MakeGlossyGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									orientation orientation = B_HORIZONTAL) const;

	void						_MakeButtonGradient(BGradientLinear& gradient,
									BRect& rect, const rgb_color& base,
									uint32 flags, orientation orientation = B_HORIZONTAL) const;
};

}// bprivate

#endif	// FLAT_CONTROL_LOOK_H
