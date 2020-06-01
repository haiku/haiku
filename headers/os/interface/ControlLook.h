/*
 * Copyright 2009-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONTROL_LOOK_H
#define _CONTROL_LOOK_H


#include <Alignment.h>
#include <Font.h>
#include <Rect.h>
#include <Slider.h>


class BBitmap;
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
		B_GROUP_FRAME,
		B_MENU_FIELD_FRAME,
		B_SCROLL_VIEW_FRAME,
		B_TEXT_CONTROL_FRAME,
	};

	enum background_type {
		B_BUTTON_BACKGROUND,
		B_BUTTON_WITH_POP_UP_BACKGROUND,
		B_MENU_BACKGROUND,
		B_MENU_BAR_BACKGROUND,
		B_MENU_FIELD_BACKGROUND,
		B_MENU_ITEM_BACKGROUND,
		B_HORIZONTAL_SCROLL_BAR_BACKGROUND,
		B_VERTICAL_SCROLL_BAR_BACKGROUND,
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
		B_DOWN_ARROW		= 3,
		B_LEFT_UP_ARROW		= 4,
		B_RIGHT_UP_ARROW	= 5,
		B_RIGHT_DOWN_ARROW	= 6,
		B_LEFT_DOWN_ARROW	= 7
	};

	enum {
		B_FOCUSED				= 1 << 0,
		B_CLICKED				= 1 << 1, // some controls activate on mouse up
		B_ACTIVATED				= 1 << 2,
		B_HOVER					= 1 << 3,
		B_DISABLED				= 1 << 4,
		B_DEFAULT_BUTTON		= 1 << 5,
		B_IGNORE_OUTLINE		= 1 << 6,
		B_PARTIALLY_ACTIVATED	= 1 << 7, // like B_ACTIVATED, but for tri-state
		B_FLAT					= 1 << 8, // flat look (e.g. button background)
		B_INVALID				= 1 << 9, // invalid value, use B_FAILURE_COLOR
		B_IS_CONTROL			= 1 << 10, // use control colors

		B_BLEND_FRAME			= 1 << 16,
	};

	enum {
		B_KNOB_NONE = 0,
		B_KNOB_DOTS,
		B_KNOB_LINES
	};

	virtual BAlignment			DefaultLabelAlignment() const = 0;
	virtual float				DefaultLabelSpacing() const = 0;

	virtual	float				DefaultItemSpacing() const = 0;

	static	float				ComposeSpacing(float spacing);

	virtual uint32				Flags(BControl* control) const = 0;

	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;
	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) = 0;
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) = 0;
	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) = 0;

	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;
	virtual	void				DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, bool popupIndicator,
									uint32 flags = 0) = 0;
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float radius, const rgb_color& base,
									bool popupIndicator, uint32 flags = 0) = 0;
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator, uint32 flags = 0) = 0;
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawMenuBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawMenuItemBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawStatusBar(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition) = 0;

	virtual	void				DrawCheckBox(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0) = 0;

	virtual	void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0) = 0;

	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect1, BRect& rect2,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation) = 0;
	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation) = 0;

	virtual	void				DrawScrollViewFrame(BView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style borderStyle,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawArrowShape(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 direction,
									uint32 flags = 0,
									float tint = B_DARKEN_MAX_TINT) = 0;

	virtual	rgb_color			SliderBarColor(const rgb_color& base) = 0;

	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base,
									rgb_color leftFillColor,
									rgb_color rightFillColor,
									float sliderScale, uint32 flags,
									orientation orientation) = 0;
	virtual	void				DrawSliderBar(BView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, rgb_color fillColor,
									uint32 flags, orientation orientation) = 0;

	virtual	void				DrawSliderThumb(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation) = 0;

	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation) = 0;
	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& fill, uint32 flags,
									orientation orientation) = 0;

	virtual	void				DrawSliderHashMarks(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, int32 count,
									hash_mark_location location,
									uint32 flags, orientation orientation) = 0;

	virtual	void				DrawActiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0) = 0;
	virtual	void				DrawInactiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0) = 0;

	virtual	void				DrawSplitter(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									orientation orientation,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	// various borders

	virtual	void				DrawBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									border_style borderStyle, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawRaisedBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawGroupFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS) = 0;

	virtual	void				DrawTextControlBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS) = 0;

	// aligned labels

	virtual	void				DrawLabel(BView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const rgb_color* textColor = NULL) = 0;
	virtual	void				DrawLabel(BView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL) = 0;
	// TODO: Would be nice to have a (non-virtual) version of this method
	// which takes an array of labels and locations. That would save some
	// setup with the view graphics state.
	virtual	void				DrawLabel(BView* view, const char* label,
									const rgb_color& base, uint32 flags,
									const BPoint& where,
									const rgb_color* textColor = NULL) = 0;

			void				DrawLabel(BView* view, const char* label,
									const BBitmap* icon, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const rgb_color* textColor = NULL);
	virtual	void				DrawLabel(BView* view, const char* label,
									const BBitmap* icon, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL) = 0;

	virtual	void				GetFrameInsets(frame_type frameType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom) = 0;
	virtual	void				GetBackgroundInsets(
									background_type backgroundType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom) = 0;
			void				GetInsets(frame_type frameType,
									background_type backgroundType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom);

	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) = 0;
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) = 0;
	virtual	void				DrawButtonWithPopUpBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL) = 0;

			void				SetBackgroundInfo(
									const BMessage& backgroundInfo);

	virtual	void				DrawTabFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									border_style borderStyle = B_FANCY_BORDER,
									uint32 side = B_TOP_BORDER) = 0;

	virtual	void				DrawScrollBarButton(BView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									int32 direction, orientation orientation,
									bool down = false) = 0;
	virtual	void				DrawScrollBarThumb(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation,
									uint32 knobStyle = B_KNOB_NONE) = 0;
	virtual	void				DrawScrollBarBorder(BView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation) = 0;

private:
	// FBC padding
	virtual	void				_ReservedControlLook5();
	virtual	void				_ReservedControlLook6();
	virtual	void				_ReservedControlLook7();
	virtual	void				_ReservedControlLook8();
	virtual	void				_ReservedControlLook9();
	virtual	void				_ReservedControlLook10();

protected:
			int32				fCachedWorkspace;
			BMessage			fBackgroundInfo;

			uint32				_reserved[20];
};

extern BControlLook* be_control_look;

extern "C" _EXPORT BControlLook *instantiate_control_look(image_id id);


} // namespace BPrivate

using BPrivate::BControlLook;
using BPrivate::be_control_look;


#endif // _CONTROL_LOOK_H
