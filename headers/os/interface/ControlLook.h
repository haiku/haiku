/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONTROL_LOOK_H
#define _CONTROL_LOOK_H

#include <Alignment.h>
#include <Font.h>
#include <Rect.h>
#include <Slider.h>


class BControl;
class BGradientLinear;
class BView;


// WARNING! This is experimental API and may change! Be prepared to
// recompile your software in a next version of haiku.


namespace BPrivate {


class BControlLook {
public:
								BControlLook();
	virtual						~BControlLook();

	// TODO: Probably more convenient to pull these enums into global
	// namespace and rename them to e.g. "B_CONTROL_LOOK_FOCUSED" instead of
	// "BControlLook::B_FOCUSED".

	enum frame_type {
		B_BUTTON_FRAME,
		B_MENU_FRAME,
		B_LISTVIEW_FRAME,
		B_INPUT_FRAME
	};

	enum background_type {
		B_BUTTON_BACKGROUND,
		B_MENU_BACKGROUND,
		B_LISTVIEW_BACKGROUND,
		B_INPUT_BACKGROUND
	};

	enum {
		B_LEFT_BORDER		= 1 << 0,
		B_RIGHT_BORDER		= 1 << 1,
		B_TOP_BORDER		= 1 << 2,
		B_BOTTOM_BORDER		= 1 << 3,

		B_ALL_BORDERS		= B_LEFT_BORDER | B_RIGHT_BORDER
								| B_TOP_BORDER | B_BOTTOM_BORDER
	};

	enum {
		B_LEFT_ARROW		= 0,
		B_RIGHT_ARROW		= 1,
		B_UP_ARROW			= 2,
		B_DOWN_ARROW		= 3
	};

	enum {
		B_FOCUSED			= 1 << 0,
		B_CLICKED			= 1 << 1, // some controls activate on mouse up
		B_ACTIVATED			= 1 << 2,
		B_HOVER				= 1 << 3,
		B_DISABLED			= 1 << 4,
		B_DEFAULT_BUTTON	= 1 << 5,

		B_BLEND_FRAME		= 1 << 16
	};

	virtual BAlignment			DefaultLabelAlignment() const;
	virtual float				DefaultLabelSpacing() const;

	/* TODO: virtual*/
			float				DefaultItemSpacing() const;

	static	float				ComposeSpacing(float spacing);

			uint32				Flags(BControl* control) const;

	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									enum orientation orientation
										= B_HORIZONTAL);

	virtual void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, bool popupIndicator,
									uint32 flags = 0);

	virtual void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawMenuBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawMenuItemBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawStatusBar(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition);

	virtual void				DrawCheckBox(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

	virtual void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

	virtual void				DrawScrollBarBackground(BView* view,
									BRect& rect1, BRect& rect2,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									enum orientation orientation);

	virtual void				DrawScrollBarBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									enum orientation orientation);

	// TODO: Make all these virtual before R1 release
			void				DrawScrollViewFrame(BView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style border,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

			void				DrawArrowShape(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 direction,
									uint32 flags = 0,
									float tint = B_DARKEN_MAX_TINT);

	virtual	rgb_color			SliderBarColor(const rgb_color& base);

	virtual void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base,
									rgb_color leftFillColor,
									rgb_color rightFillColor,
									float sliderScale, uint32 flags,
									enum orientation orientation);

	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, rgb_color fillColor,
									uint32 flags, enum orientation orientation);

	virtual	void				DrawSliderThumb(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									enum orientation orientation);

			void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									enum orientation orientation);

	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& fill, uint32 flags,
									enum orientation orientation);

	virtual	void				DrawSliderHashMarks(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, int32 count,
									hash_mark_location location,
									uint32 flags, enum orientation orientation);

	virtual void				DrawActiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawInactiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	/*virtual*/	void			DrawSplitter(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									enum orientation orientation,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// various borders

	virtual void				DrawBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									border_style border, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawRaisedBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawGroupFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS);

	virtual void				DrawTextControlBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// aligned labels

			void				DrawLabel(BView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags);

	virtual void				DrawLabel(BView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment);

	// TODO: This should also be virtual
	// TODO: Would be nice to have a (non-virtual) version of this method
	// which takes an array of labels and locations. That would save some
	// setup with the view graphics state.
			void				DrawLabel(BView* view, const char* label,
									const rgb_color& base, uint32 flags,
									const BPoint& where);

protected:
			void				_DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									float contrast, float brightness = 1.0,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

			void				_DrawOuterResessedFrame(BView* view,
									BRect& rect, const rgb_color& base,
									float contrast = 1.0f,
									float brightness = 1.0f,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

			void				_DrawFrame(BView* view, BRect& rect,
									const rgb_color& left,
									const rgb_color& top,
									const rgb_color& right,
									const rgb_color& bottom,
									uint32 borders = B_ALL_BORDERS);

			void				_DrawFrame(BView* view, BRect& rect,
									const rgb_color& left,
									const rgb_color& top,
									const rgb_color& right,
									const rgb_color& bottom,
									const rgb_color& rightTop,
									const rgb_color& leftBottom,
									uint32 borders = B_ALL_BORDERS);

			void				_FillGradient(BView* view, const BRect& rect,
									const rgb_color& base, float topTint,
									float bottomTint,
									enum orientation orientation
										= B_HORIZONTAL);

			void				_FillGlossyGradient(BView* view,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									enum orientation orientation
										= B_HORIZONTAL);

			void				_MakeGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float bottomTint,
									enum orientation orientation
										= B_HORIZONTAL) const;

			void				_MakeGlossyGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									enum orientation orientation
										= B_HORIZONTAL) const;

			bool				_RadioButtonAndCheckBoxMarkColor(
									const rgb_color& base, rgb_color& color,
									uint32 flags) const;

			void				_DrawRoundBarCorner(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& edgeLightColor,
									const rgb_color& edgeShadowColor,
									const rgb_color& frameLightColor,
									const rgb_color& frameShadowColor,
									const rgb_color& fillLightColor,
									const rgb_color& fillShadowColor,
									float leftInset, float topInset,
									float rightInset, float bottomInset,
									enum orientation orientation);

			void				_DrawRoundCornerLeftTop(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& edgeColor,
									const rgb_color& frameColor,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);
			void				_DrawRoundCornerRightTop(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& edgeTopColor,
									const rgb_color& edgeRightColor,
									const rgb_color& frameTopColor,
									const rgb_color& frameRightColor,
									const rgb_color& bevelTopColor,
									const rgb_color& bevelRightColor,
									const BGradientLinear& fillGradient);
};

extern BControlLook* be_control_look;


} // namespace BPrivate

using BPrivate::BControlLook;
using BPrivate::be_control_look;

#endif // _CONTROL_LOOK_H
