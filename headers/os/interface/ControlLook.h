/*
 * Copyright 2009-2015, Haiku, Inc. All rights reserved.
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

	virtual BAlignment			DefaultLabelAlignment() const;
	virtual float				DefaultLabelSpacing() const;

	// TODO: Make methods virtual before R1 release
	/*virtual*/	float			DefaultItemSpacing() const;

	static	float				ComposeSpacing(float spacing);

	/*virtual*/ uint32			Flags(BControl* control) const;

	virtual	void				DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	/*virtual*/	void			DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	/*virtual*/	void			DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	/*virtual*/	void			DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	/*virtual*/	void			DrawButtonBackground(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	virtual	void				DrawMenuBarBackground(BView* view, BRect& rect,
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
	/*virtual*/	void			DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	/*virtual*/	void			DrawMenuFieldFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, bool popupIndicator,
									uint32 flags = 0);
	/*virtual*/	void			DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float radius, const rgb_color& base,
									bool popupIndicator, uint32 flags = 0);
	/*virtual*/	void			DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator, uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuItemBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawStatusBar(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition);

	virtual	void				DrawCheckBox(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

	virtual	void				DrawRadioButton(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect1, BRect& rect2,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);

	/*virtual*/	void			DrawScrollViewFrame(BView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style border,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	/*virtual*/	void			DrawArrowShape(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 direction,
									uint32 flags = 0,
									float tint = B_DARKEN_MAX_TINT);

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

	/*virtual*/	void			DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderTriangle(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& fill, uint32 flags,
									orientation orientation);

	virtual	void				DrawSliderHashMarks(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, int32 count,
									hash_mark_location location,
									uint32 flags, orientation orientation);

	virtual	void				DrawActiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawInactiveTab(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	/*virtual*/	void			DrawSplitter(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									orientation orientation,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// various borders

	virtual	void				DrawBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									border_style border, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawRaisedBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawGroupFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawTextControlBorder(BView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// aligned labels

	/*virtual*/	void			DrawLabel(BView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const rgb_color* textColor = NULL);
	virtual	void				DrawLabel(BView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL);
	// TODO: Would be nice to have a (non-virtual) version of this method
	// which takes an array of labels and locations. That would save some
	// setup with the view graphics state.
	/*virtual*/	void			DrawLabel(BView* view, const char* label,
									const rgb_color& base, uint32 flags,
									const BPoint& where,
									const rgb_color* textColor = NULL);

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
									const rgb_color* textColor = NULL);

	virtual	void				GetFrameInsets(frame_type frameType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom);
	virtual	void				GetBackgroundInsets(
									background_type backgroundType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom);
			void				GetInsets(frame_type frameType,
									background_type backgroundType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom);

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

			void				SetBackgroundInfo(
									const BMessage& backgroundInfo);

protected:
			void				_DrawButtonFrame(BView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
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

			void				_DrawButtonBackground(BView* view, BRect& rect,
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
			void				_DrawFlatButtonBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
			void				_DrawNonFlatButtonBackground(BView* view,
									BRect& rect, const BRect& updateRect,
									BRegion& clipping,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

			void				_DrawPopUpMarker(BView* view, const BRect& rect,
									const rgb_color& base, uint32 flags);

			void				_DrawMenuFieldBackgroundOutside(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0);

			void				_DrawMenuFieldBackgroundInside(BView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// Rounded corner methods
			void				_DrawRoundCornerLeftTop(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameLeftTop(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor);

			void				_DrawRoundCornerBackgroundLeftTop(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerRightTop(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeTopColor,
									const rgb_color& edgeRightColor,
									const rgb_color& frameTopColor,
									const rgb_color& frameRightColor,
									const rgb_color& bevelTopColor,
									const rgb_color& bevelRightColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameRightTop(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeTopColor,
									const rgb_color& edgeRightColor,
									const rgb_color& frameTopColor,
									const rgb_color& frameRightColor);

			void				_DrawRoundCornerBackgroundRightTop(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelTopColor,
									const rgb_color& bevelRightColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerLeftBottom(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeLeftColor,
									const rgb_color& edgeBottomColor,
									const rgb_color& frameLeftColor,
									const rgb_color& frameBottomColor,
									const rgb_color& bevelLeftColor,
									const rgb_color& bevelBottomColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameLeftBottom(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeLeftColor,
									const rgb_color& edgeBottomColor,
									const rgb_color& frameLeftColor,
									const rgb_color& frameBottomColor);

			void				_DrawRoundCornerBackgroundLeftBottom(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelLeftColor,
									const rgb_color& bevelBottomColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerRightBottom(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameRightBottom(BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor);

			void				_DrawRoundCornerBackgroundRightBottom(
									BView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

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
									orientation orientation);

	// Border color methods
			rgb_color			_EdgeLightColor(const rgb_color& base,
									float contrast, float brightness,
									uint32 flags);

			rgb_color			_EdgeShadowColor(const rgb_color& base,
									float contrast, float brightness,
									uint32 flags);

			rgb_color			_FrameLightColor(const rgb_color& base,
									uint32 flags);

			rgb_color			_FrameShadowColor(const rgb_color& base,
									uint32 flags);

			rgb_color			_BevelLightColor(const rgb_color& base,
									uint32 flags);

			rgb_color			_BevelShadowColor(const rgb_color& base,
									uint32 flags);

	// Background gradient methods
			void				_FillGradient(BView* view, const BRect& rect,
									const rgb_color& base, float topTint,
									float bottomTint,
									orientation orientation = B_HORIZONTAL);

			void				_FillGlossyGradient(BView* view,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									orientation orientation = B_HORIZONTAL);

			void				_MakeGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float bottomTint,
									orientation orientation = B_HORIZONTAL) const;

			void				_MakeGlossyGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									orientation orientation = B_HORIZONTAL) const;

			void				_MakeButtonGradient(BGradientLinear& gradient,
									BRect& rect, const rgb_color& base,
									uint32 flags, orientation orientation = B_HORIZONTAL) const;

			bool				_RadioButtonAndCheckBoxMarkColor(
									const rgb_color& base, rgb_color& color,
									uint32 flags) const;

private:
			bool				fCachedOutline;
			int32				fCachedWorkspace;
			BMessage			fBackgroundInfo;

			uint32				_reserved[20];
};

extern BControlLook* be_control_look;


} // namespace BPrivate

using BPrivate::BControlLook;
using BPrivate::be_control_look;


#endif // _CONTROL_LOOK_H
