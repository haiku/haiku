/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_VIEW_H
#define	_VIEW_H


#include <AffineTransform.h>
#include <Alignment.h>
#include <Font.h>
#include <Handler.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Gradient.h>


// mouse button
enum {
	B_PRIMARY_MOUSE_BUTTON				= 0x01,
	B_SECONDARY_MOUSE_BUTTON			= 0x02,
	B_TERTIARY_MOUSE_BUTTON				= 0x04
};

// mouse transit
enum {
	B_ENTERED_VIEW						= 0,
	B_INSIDE_VIEW,
	B_EXITED_VIEW,
	B_OUTSIDE_VIEW
};

// event mask
enum {
	B_POINTER_EVENTS					= 0x00000001,
	B_KEYBOARD_EVENTS					= 0x00000002
};

// event mask options
enum {
	B_LOCK_WINDOW_FOCUS					= 0x00000001,
	B_SUSPEND_VIEW_FOCUS				= 0x00000002,
	B_NO_POINTER_HISTORY				= 0x00000004,
	// NOTE: New in Haiku (unless this flag is
	// specified, both BWindow and BView::GetMouse()
	// will filter out older mouse moved messages)
	B_FULL_POINTER_HISTORY				= 0x00000008
};

enum {
	B_TRACK_WHOLE_RECT,
	B_TRACK_RECT_CORNER
};

// set font mask
enum {
	B_FONT_FAMILY_AND_STYLE				= 0x00000001,
	B_FONT_SIZE							= 0x00000002,
	B_FONT_SHEAR						= 0x00000004,
	B_FONT_ROTATION						= 0x00000008,
	B_FONT_SPACING						= 0x00000010,
	B_FONT_ENCODING						= 0x00000020,
	B_FONT_FACE							= 0x00000040,
	B_FONT_FLAGS						= 0x00000080,
	B_FONT_FALSE_BOLD_WIDTH				= 0x00000100,
	B_FONT_ALL							= 0x000001FF
};

// view flags
const uint32 B_FULL_UPDATE_ON_RESIZE	= 0x80000000UL;	/* 31 */
const uint32 _B_RESERVED1_				= 0x40000000UL;	/* 30 */
const uint32 B_WILL_DRAW				= 0x20000000UL;	/* 29 */
const uint32 B_PULSE_NEEDED				= 0x10000000UL;	/* 28 */
const uint32 B_NAVIGABLE_JUMP			= 0x08000000UL;	/* 27 */
const uint32 B_FRAME_EVENTS				= 0x04000000UL;	/* 26 */
const uint32 B_NAVIGABLE				= 0x02000000UL;	/* 25 */
const uint32 B_SUBPIXEL_PRECISE			= 0x01000000UL;	/* 24 */
const uint32 B_DRAW_ON_CHILDREN			= 0x00800000UL;	/* 23 */
const uint32 B_INPUT_METHOD_AWARE		= 0x00400000UL;	/* 23 */
const uint32 B_SCROLL_VIEW_AWARE		= 0x00200000UL;	/* 22 */
const uint32 B_SUPPORTS_LAYOUT			= 0x00100000UL;	/* 21 */
const uint32 B_INVALIDATE_AFTER_LAYOUT	= 0x00080000UL;	/* 20 */

#define _RESIZE_MASK_ (0xffff)

const uint32 _VIEW_TOP_					= 1UL;
const uint32 _VIEW_LEFT_				= 2UL;
const uint32 _VIEW_BOTTOM_				= 3UL;
const uint32 _VIEW_RIGHT_				= 4UL;
const uint32 _VIEW_CENTER_				= 5UL;

inline uint32 _rule_(uint32 r1, uint32 r2, uint32 r3, uint32 r4)
	{ return ((r1 << 12) | (r2 << 8) | (r3 << 4) | r4); }

#define B_FOLLOW_NONE 0
#define B_FOLLOW_ALL_SIDES	_rule_(_VIEW_TOP_, _VIEW_LEFT_, _VIEW_BOTTOM_, \
								_VIEW_RIGHT_)
#define B_FOLLOW_ALL		B_FOLLOW_ALL_SIDES

#define B_FOLLOW_LEFT		_rule_(0, _VIEW_LEFT_, 0, _VIEW_LEFT_)
#define B_FOLLOW_RIGHT		_rule_(0, _VIEW_RIGHT_, 0, _VIEW_RIGHT_)
#define B_FOLLOW_LEFT_RIGHT	_rule_(0, _VIEW_LEFT_, 0, _VIEW_RIGHT_)
#define B_FOLLOW_H_CENTER	_rule_(0, _VIEW_CENTER_, 0, _VIEW_CENTER_)

#define B_FOLLOW_TOP		_rule_(_VIEW_TOP_, 0, _VIEW_TOP_, 0)
#define B_FOLLOW_BOTTOM		_rule_(_VIEW_BOTTOM_, 0, _VIEW_BOTTOM_, 0)
#define B_FOLLOW_TOP_BOTTOM	_rule_(_VIEW_TOP_, 0, _VIEW_BOTTOM_, 0)
#define B_FOLLOW_V_CENTER	_rule_(_VIEW_CENTER_, 0, _VIEW_CENTER_, 0)

#define B_FOLLOW_LEFT_TOP	B_FOLLOW_TOP | B_FOLLOW_LEFT

class BBitmap;
class BCursor;
class BLayout;
class BLayoutContext;
class BLayoutItem;
class BMessage;
class BPicture;
class BPolygon;
class BRegion;
class BScrollBar;
class BScrollView;
class BShape;
class BShelf;
class BString;
class BToolTip;
class BWindow;
struct _array_data_;
struct _array_hdr_;
struct overlay_restrictions;

namespace BPrivate {
	class ViewState;
};


class BView : public BHandler {
public:
								BView(const char* name, uint32 flags,
									BLayout* layout = NULL);
								BView(BRect frame, const char* name,
									uint32 resizingMode, uint32 flags);
	virtual						~BView();

								BView(BMessage* archive);
	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	virtual	status_t			AllUnarchived(const BMessage* archive);
	virtual status_t			AllArchived(BMessage* archive) const;

	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				DetachedFromWindow();
	virtual	void				AllDetached();

	virtual	void				MessageReceived(BMessage* message);

			void				AddChild(BView* child, BView* before = NULL);
			bool				AddChild(BLayoutItem* child);
			bool				RemoveChild(BView* child);
			int32				CountChildren() const;
			BView*				ChildAt(int32 index) const;
			BView*				NextSibling() const;
			BView*				PreviousSibling() const;
			bool				RemoveSelf();

			BWindow*			Window() const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);
	virtual	void				WindowActivated(bool active);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				KeyUp(const char* bytes, int32 numBytes);
	virtual	void				Pulse();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				TargetedByScrollView(BScrollView* scrollView);
			void				BeginRectTracking(BRect startRect,
									uint32 style = B_TRACK_WHOLE_RECT);
			void				EndRectTracking();

			void				GetMouse(BPoint* location, uint32* buttons,
									bool checkMessageQueue = true);

			void				DragMessage(BMessage* message, BRect dragRect,
									BHandler* replyTo = NULL);
			void				DragMessage(BMessage* message, BBitmap* bitmap,
									BPoint offset, BHandler* replyTo = NULL);
			void				DragMessage(BMessage* message, BBitmap* bitmap,
									drawing_mode dragMode, BPoint offset,
									BHandler* replyTo = NULL);

			BView*				FindView(const char* name) const;
			BView*				Parent() const;
			BRect				Bounds() const;
			BRect				Frame() const;
			void				ConvertToScreen(BPoint* point) const;
			BPoint				ConvertToScreen(BPoint point) const;
			void				ConvertFromScreen(BPoint* point) const;
			BPoint				ConvertFromScreen(BPoint point) const;
			void				ConvertToScreen(BRect* rect) const;
			BRect				ConvertToScreen(BRect rect) const;
			void				ConvertFromScreen(BRect* rect) const;
			BRect				ConvertFromScreen(BRect rect) const;
			void				ConvertToParent(BPoint* point) const;
			BPoint				ConvertToParent(BPoint point) const;
			void				ConvertFromParent(BPoint* point) const;
			BPoint				ConvertFromParent(BPoint point) const;
			void				ConvertToParent(BRect* rect) const;
			BRect				ConvertToParent(BRect rect) const;
			void				ConvertFromParent(BRect* rect) const;
			BRect				ConvertFromParent(BRect rect) const;
			BPoint				LeftTop() const;

			void				GetClippingRegion(BRegion* region) const;
	virtual	void				ConstrainClippingRegion(BRegion* region);
			void				ClipToPicture(BPicture* picture,
									BPoint where = B_ORIGIN, bool sync = true);
			void				ClipToInversePicture(BPicture* picture,
									BPoint where = B_ORIGIN, bool sync = true);

			void				ClipToRect(BRect rect);
			void				ClipToInverseRect(BRect rect);
			void				ClipToShape(BShape* shape);
			void				ClipToInverseShape(BShape* shape);

	virtual	void				SetDrawingMode(drawing_mode mode);
			drawing_mode		DrawingMode() const;

			void				SetBlendingMode(source_alpha srcAlpha,
									alpha_function alphaFunc);
			void				GetBlendingMode(source_alpha* srcAlpha,
									alpha_function* alphaFunc) const;

	virtual	void				SetPenSize(float size);
			float				PenSize() const;

			void				SetViewCursor(const BCursor* cursor,
									bool sync = true);

			bool				HasDefaultColors() const;
			bool				HasSystemColors() const;
			void				AdoptParentColors();
			void				AdoptSystemColors();
			void				AdoptViewColors(BView* view);

	virtual	void				SetViewColor(rgb_color color);
			void				SetViewColor(uchar red, uchar green, uchar blue,
									uchar alpha = 255);
			rgb_color			ViewColor() const;

			void				SetViewUIColor(color_which which,
									float tint = B_NO_TINT);
			color_which			ViewUIColor(float* tint = NULL) const;

			void				SetViewBitmap(const BBitmap* bitmap,
									BRect srcRect, BRect dstRect,
									uint32 followFlags = B_FOLLOW_LEFT_TOP,
									uint32 options = B_TILE_BITMAP);
			void				SetViewBitmap(const BBitmap* bitmap,
									uint32 followFlags = B_FOLLOW_LEFT_TOP,
									uint32 options = B_TILE_BITMAP);
			void				ClearViewBitmap();

			status_t			SetViewOverlay(const BBitmap* overlay,
									BRect srcRect, BRect dstRect,
									rgb_color* colorKey,
									uint32 followFlags = B_FOLLOW_LEFT_TOP,
									uint32 options = 0);
			status_t			SetViewOverlay(const BBitmap* overlay,
									rgb_color* colorKey,
									uint32 followFlags = B_FOLLOW_LEFT_TOP,
									uint32 options = 0);
			void				ClearViewOverlay();

	virtual	void				SetHighColor(rgb_color color);
			void				SetHighColor(uchar red, uchar green, uchar blue,
									uchar alpha = 255);
			rgb_color			HighColor() const;

			void				SetHighUIColor(color_which which,
									float tint = B_NO_TINT);
			color_which			HighUIColor(float* tint = NULL) const;

	virtual	void				SetLowColor(rgb_color color);
			void				SetLowColor(uchar red, uchar green, uchar blue,
									uchar alpha = 255);
			rgb_color			LowColor() const;

			void				SetLowUIColor(color_which which,
									float tint = B_NO_TINT);
			color_which			LowUIColor(float* tint = NULL) const;

			void				SetLineMode(cap_mode lineCap,
									join_mode lineJoin,
									float miterLimit = B_DEFAULT_MITER_LIMIT);
			join_mode			LineJoinMode() const;
			cap_mode			LineCapMode() const;
			float				LineMiterLimit() const;

			void				SetFillRule(int32 rule);
			int32				FillRule() const;

			void				SetOrigin(BPoint where);
			void				SetOrigin(float x, float y);
			BPoint				Origin() const;

								// Works in addition to Origin and Scale.
								// May be used in parallel or as a much
								// more powerful alternative.
			void				SetTransform(BAffineTransform transform);
			BAffineTransform	Transform() const;
			void				TranslateBy(double x, double y);
			void				ScaleBy(double x, double y);
			void				RotateBy(double angleRadians);

			void				PushState();
			void				PopState();

			void				MovePenTo(BPoint pt);
			void				MovePenTo(float x, float y);
			void				MovePenBy(float x, float y);
			BPoint				PenLocation() const;
			void				StrokeLine(BPoint toPoint,
									::pattern pattern = B_SOLID_HIGH);
			void				StrokeLine(BPoint start, BPoint end,
									::pattern pattern = B_SOLID_HIGH);
			void				BeginLineArray(int32 count);
			void				AddLine(BPoint start, BPoint end,
									rgb_color color);
			void				EndLineArray();

			void				StrokePolygon(const BPolygon* polygon,
									bool closed = true,
									::pattern pattern = B_SOLID_HIGH);
			void				StrokePolygon(const BPoint* pointArray,
									int32 numPoints, bool closed = true,
									::pattern pattern = B_SOLID_HIGH);
			void				StrokePolygon(const BPoint* pointArray,
									int32 numPoints, BRect bounds,
									bool closed = true,
									::pattern pattern = B_SOLID_HIGH);
			void				FillPolygon(const BPolygon* polygon,
									::pattern pattern = B_SOLID_HIGH);
			void				FillPolygon(const BPoint* pointArray,
									int32 numPoints,
									::pattern pattern = B_SOLID_HIGH);
			void				FillPolygon(const BPoint* pointArray,
									int32 numPoints, BRect bounds,
									::pattern pattern = B_SOLID_HIGH);
			void				FillPolygon(const BPolygon* polygon,
									const BGradient& gradient);
			void				FillPolygon(const BPoint* pointArray,
									int32 numPoints, const BGradient& gradient);
			void				FillPolygon(const BPoint* pointArray,
									int32 numPoints, BRect bounds,
									const BGradient& gradient);

			void				StrokeTriangle(BPoint point1, BPoint point2,
									BPoint point3, BRect bounds,
									::pattern pattern = B_SOLID_HIGH);
			void				StrokeTriangle(BPoint point1, BPoint point2,
									BPoint point3,
									::pattern pattern = B_SOLID_HIGH);
			void				FillTriangle(BPoint point1, BPoint point2,
									BPoint point3,
									::pattern pattern = B_SOLID_HIGH);
			void				FillTriangle(BPoint point1, BPoint point2,
									BPoint point3, BRect bounds,
									::pattern pattern = B_SOLID_HIGH);
			void				FillTriangle(BPoint point1, BPoint point2,
									BPoint point3, const BGradient& gradient);
			void				FillTriangle(BPoint point1, BPoint point2,
									BPoint point3, BRect bounds,
									const BGradient& gradient);

			void				StrokeRect(BRect rect,
									::pattern pattern = B_SOLID_HIGH);
			void				FillRect(BRect rect,
									::pattern pattern = B_SOLID_HIGH);
			void				FillRect(BRect rect, const BGradient& gradient);
			void				FillRegion(BRegion* rectegion,
									::pattern pattern = B_SOLID_HIGH);
			void				FillRegion(BRegion* rectegion,
									const BGradient& gradient);
			void				InvertRect(BRect rect);

			void				StrokeRoundRect(BRect rect, float xRadius,
									float yRadius,
									::pattern pattern = B_SOLID_HIGH);
			void				FillRoundRect(BRect rect, float xRadius,
									float yRadius,
									::pattern pattern = B_SOLID_HIGH);
			void				FillRoundRect(BRect rect, float xRadius,
									float yRadius, const BGradient& gradient);

			void				StrokeEllipse(BPoint center, float xRadius,
									float yRadius,
									::pattern pattern = B_SOLID_HIGH);
			void				StrokeEllipse(BRect rect,
									::pattern pattern = B_SOLID_HIGH);
			void				FillEllipse(BPoint center, float xRadius,
									float yRadius,
									::pattern pattern = B_SOLID_HIGH);
			void				FillEllipse(BRect rect,
									::pattern pattern = B_SOLID_HIGH);
			void				FillEllipse(BPoint center, float xRadius,
									float yRadius, const BGradient& gradient);
			void				FillEllipse(BRect rect,
									const BGradient& gradient);

			void				StrokeArc(BPoint center, float xRadius,
									float yRadius, float startAngle,
									float arcAngle,
									::pattern pattern = B_SOLID_HIGH);
			void				StrokeArc(BRect rect, float startAngle,
									float arcAngle,
									::pattern pattern = B_SOLID_HIGH);
			void				FillArc(BPoint center, float xRadius,
									float yRadius, float startAngle,
									float arcAngle,
									::pattern pattern = B_SOLID_HIGH);
			void				FillArc(BRect rect, float startAngle,
									float arcAngle,
									::pattern pattern = B_SOLID_HIGH);
			void				FillArc(BPoint center, float xRadius,
									float yRadius, float startAngle,
									float arcAngle, const BGradient& gradient);
			void				FillArc(BRect rect, float startAngle,
									float arcAngle, const BGradient& gradient);

			void				StrokeBezier(BPoint* controlPoints,
									::pattern pattern = B_SOLID_HIGH);
			void				FillBezier(BPoint* controlPoints,
									::pattern pattern = B_SOLID_HIGH);
			void				FillBezier(BPoint* controlPoints,
									const BGradient& gradient);

			void				StrokeShape(BShape* shape,
									::pattern pattern = B_SOLID_HIGH);
			void				FillShape(BShape* shape,
									::pattern pattern = B_SOLID_HIGH);
			void				FillShape(BShape* shape,
									const BGradient& gradient);

			void				CopyBits(BRect src, BRect dst);

			void				DrawBitmapAsync(const BBitmap* aBitmap,
									BRect bitmapRect, BRect viewRect,
									uint32 options);
			void				DrawBitmapAsync(const BBitmap* aBitmap,
									BRect bitmapRect, BRect viewRect);
			void				DrawBitmapAsync(const BBitmap* aBitmap,
									BRect viewRect);
			void				DrawBitmapAsync(const BBitmap* aBitmap,
									BPoint where);
			void				DrawBitmapAsync(const BBitmap* aBitmap);

			void				DrawBitmap(const BBitmap* aBitmap,
									BRect bitmapRect, BRect viewRect,
									uint32 options);
			void				DrawBitmap(const BBitmap* aBitmap,
									BRect bitmapRect, BRect viewRect);
			void				DrawBitmap(const BBitmap* aBitmap,
									BRect viewRect);
			void				DrawBitmap(const BBitmap* aBitmap,
									BPoint where);
			void				DrawBitmap(const BBitmap* aBitmap);

			void				DrawChar(char aChar);
			void				DrawChar(char aChar, BPoint location);
			void				DrawString(const char* string,
									escapement_delta* delta = NULL);
			void				DrawString(const char* string,
									BPoint location,
									escapement_delta* delta = NULL);
			void				DrawString(const char* string, int32 length,
									escapement_delta* delta = NULL);
			void				DrawString(const char* string, int32 length,
									BPoint location,
									escapement_delta* delta = 0L);
			void				DrawString(const char* string,
									const BPoint* locations,
									int32 locationCount);
			void				DrawString(const char* string, int32 length,
									const BPoint* locations,
									int32 locationCount);

	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);

			void				GetFont(BFont* font) const;
			void				TruncateString(BString* in_out, uint32 mode,
									float width) const;
			float				StringWidth(const char* string) const;
			float				StringWidth(const char* string,
									int32 length) const;
			void				GetStringWidths(char* stringArray[],
									int32 lengthArray[], int32 numStrings,
									float widthArray[]) const;
			void				SetFontSize(float size);
			void				ForceFontAliasing(bool enable);
			void				GetFontHeight(font_height* height) const;

			void				Invalidate(BRect invalRect);
			void				Invalidate(const BRegion* invalRegion);
			void				Invalidate();
			void				DelayedInvalidate(bigtime_t delay);
			void				DelayedInvalidate(bigtime_t delay,
									BRect invalRect);

			void				SetDiskMode(char* filename, long offset);

			void				BeginPicture(BPicture* a_picture);
			void				AppendToPicture(BPicture* a_picture);
			BPicture*			EndPicture();

			void				DrawPicture(const BPicture* a_picture);
			void				DrawPicture(const BPicture* a_picture,
									BPoint where);
			void				DrawPicture(const char* filename, long offset,
									BPoint where);
			void				DrawPictureAsync(const BPicture* a_picture);
			void				DrawPictureAsync(const BPicture* a_picture,
									BPoint where);
			void				DrawPictureAsync(const char* filename,
									long offset, BPoint where);

			void				BeginLayer(uint8 opacity);
			void				EndLayer();

			status_t			SetEventMask(uint32 mask, uint32 options = 0);
			uint32				EventMask();
			status_t			SetMouseEventMask(uint32 mask,
									uint32 options = 0);

	virtual	void				SetFlags(uint32 flags);
			uint32				Flags() const;
	virtual	void				SetResizingMode(uint32 mode);
			uint32				ResizingMode() const;
			void				MoveBy(float dh, float dv);
			void				MoveTo(BPoint where);
			void				MoveTo(float x, float y);
			void				ResizeBy(float dh, float dv);
			void				ResizeTo(float width, float height);
			void				ResizeTo(BSize size);
			void				ScrollBy(float dh, float dv);
			void				ScrollTo(float x, float y);
	virtual	void				ScrollTo(BPoint where);
	virtual	void				MakeFocus(bool focus = true);
			bool				IsFocus() const;

	virtual	void				Show();
	virtual	void				Hide();
			bool				IsHidden() const;
			bool				IsHidden(const BView* looking_from) const;

			void				Flush() const;
			void				Sync() const;

	virtual	void				GetPreferredSize(float* _width, float* _height);
	virtual	void				ResizeToPreferred();

			BScrollBar*			ScrollBar(orientation direction) const;

	virtual	BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 form,
									const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

			bool				IsPrinting() const;
			void				SetScale(float scale) const;
			float				Scale() const;
									// new for Haiku

	virtual	status_t			Perform(perform_code code, void* data);

	virtual	void				DrawAfterChildren(BRect updateRect);

	// layout related

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			LayoutAlignment();

			void				SetExplicitMinSize(BSize size);
			void				SetExplicitMaxSize(BSize size);
			void				SetExplicitPreferredSize(BSize size);
			void				SetExplicitSize(BSize size);
			void				SetExplicitAlignment(BAlignment alignment);

			BSize				ExplicitMinSize() const;
			BSize				ExplicitMaxSize() const;
			BSize				ExplicitPreferredSize() const;
			BAlignment			ExplicitAlignment() const;

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

			void				InvalidateLayout(bool descendants = false);
	virtual	void				SetLayout(BLayout* layout);
			BLayout*			GetLayout() const;

			void				EnableLayoutInvalidation();
			void				DisableLayoutInvalidation();
			bool				IsLayoutInvalidationDisabled();
			bool				IsLayoutValid() const;
			void				ResetLayoutInvalidation();

			BLayoutContext*		LayoutContext() const;

			void				Layout(bool force);
			void				Relayout();

	class Private;

protected:
	virtual	void				LayoutInvalidated(bool descendants = false);
	virtual	void				DoLayout();

public:
	// tool tip support

			void				SetToolTip(const char* text);
			void				SetToolTip(BToolTip* tip);
			BToolTip*			ToolTip() const;

			void				ShowToolTip(BToolTip* tip = NULL);
			void				HideToolTip();

protected:
	virtual	bool				GetToolTipAt(BPoint point, BToolTip** _tip);

	virtual	void				LayoutChanged();

			status_t			ScrollWithMouseWheelDelta(BScrollBar*, float);

private:
			void				_Layout(bool force, BLayoutContext* context);
			void				_LayoutLeft(BLayout* deleted);
			void				_InvalidateParentLayout();

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedView13();
	virtual	void				_ReservedView14();
	virtual	void				_ReservedView15();
	virtual	void				_ReservedView16();

								BView(const BView&);
			BView&				operator=(const BView&);

private:
	struct LayoutData;

	friend class Private;
	friend class BBitmap;
	friend class BLayout;
	friend class BPrintJob;
	friend class BScrollBar;
	friend class BShelf;
	friend class BTabView;
	friend class BWindow;

			void				_InitData(BRect frame, const char* name,
									uint32 resizingMode, uint32 flags);
			status_t			_SetViewBitmap(const BBitmap* bitmap,
									BRect srcRect, BRect dstRect,
									uint32 followFlags, uint32 options);
			void				_ClipToPicture(BPicture* picture, BPoint where,
									bool invert, bool sync);

			void				_ClipToRect(BRect rect, bool inverse);
			void				_ClipToShape(BShape* shape, bool inverse);

			bool				_CheckOwnerLockAndSwitchCurrent() const;
			bool				_CheckOwnerLock() const;
			void				_CheckLockAndSwitchCurrent() const;
			void				_CheckLock() const;
			void				_SwitchServerCurrentView() const;

			void				_SetOwner(BWindow* newOwner);
			void				_RemoveCommArray();

			BShelf*				_Shelf() const;
			void				_SetShelf(BShelf* shelf);

			void				_MoveTo(int32 x, int32 y);
			void				_ResizeBy(int32 deltaWidth, int32 deltaHeight);
			void				_ParentResizedBy(int32 deltaWidth,
									int32 deltaHeight);

			void				_ConvertToScreen(BPoint* pt,
									bool checkLock) const;
			void				_ConvertFromScreen(BPoint* pt,
									bool checkLock) const;

			void				_ConvertToParent(BPoint* pt,
									bool checkLock) const;
			void				_ConvertFromParent(BPoint* pt,
									bool checkLock) const;

			void				_Activate(bool state);
			void				_Attach();
			void				_ColorsUpdated(BMessage* message);
			void				_Detach();
			void				_Draw(BRect screenUpdateRect);
			void				_DrawAfterChildren(BRect screenUpdateRect);
			void				_FontsUpdated(BMessage*);
			void				_Pulse();

			void				_UpdateStateForRemove();
			void				_UpdatePattern(::pattern pattern);

			void				_FlushIfNotInTransaction();

			bool				_CreateSelf();
			bool				_AddChildToList(BView* child,
									BView* before = NULL);
			bool				_RemoveChildFromList(BView* child);

			bool				_AddChild(BView *child, BView *before);
			bool				_RemoveSelf();
			void				_RemoveLayoutItemsFromLayout(bool deleteItems);

	// Debugging methods
			void				_PrintToStream();
			void				_PrintTree();

			int32				_unused_int1;

			uint32				fFlags;
			BPoint				fParentOffset;
			BWindow*			fOwner;
			BView*				fParent;
			BView*				fNextSibling;
			BView*				fPreviousSibling;
			BView*				fFirstChild;

			int16				fShowLevel;
			bool				fTopLevelView;
			bool				fNoISInteraction;
			BPicture*			fCurrentPicture;
			_array_data_*		fCommArray;

			BScrollBar*			fVerScroller;
			BScrollBar*			fHorScroller;
			bool				fIsPrinting;
			bool				fAttached;
			bool				_unused_bool1;
			bool				_unused_bool2;
			::BPrivate::ViewState* fState;
			BRect				fBounds;
			BShelf*				fShelf;
			uint32				fEventMask;
			uint32				fEventOptions;
			uint32				fMouseEventOptions;

			LayoutData*			fLayoutData;
			BToolTip*			fToolTip;

			uint32				_reserved[6];
};


// #pragma mark - inline definitions


inline void
BView::ScrollTo(float x, float y)
{
	ScrollTo(BPoint(x, y));
}


inline void
BView::SetViewColor(uchar red, uchar green, uchar blue, uchar alpha)
{
	rgb_color color;
	color.red = red;
	color.green = green;
	color.blue = blue;
	color.alpha = alpha;
	SetViewColor(color);
}


inline void
BView::SetHighColor(uchar red, uchar green, uchar blue, uchar alpha)
{
	rgb_color color;
	color.red = red;
	color.green = green;
	color.blue = blue;
	color.alpha = alpha;
	SetHighColor(color);
}


inline void
BView::SetLowColor(uchar red, uchar green, uchar blue, uchar alpha)
{
	rgb_color color;
	color.red = red;
	color.green = green;
	color.blue = blue;
	color.alpha = alpha;
	SetLowColor(color);
}


#endif // _VIEW_H
