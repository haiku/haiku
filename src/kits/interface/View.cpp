//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		View.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class which draws stuff 
//
//------------------------------------------------------------------------------
#include <View.h>
#include <Message.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <Picture.h>
#include <Polygon.h>
#include <Region.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <Shape.h>
#include <Shelf.h>
#include <String.h>
#include <Window.h>
#include <SupportDefs.h>

struct
{
	int8 dummy;
} _view_attr_ ;

struct
{
	int8 dummy;
} _array_data_;

struct
{
	int8 dummy;
} _array_hdr_;

BView::BView(BRect frame,const char *name,uint32 resizeMask,uint32 flags) :
server_token(-1),
f_type(0),
origin_h(0),
origin_v(0),
owner(NULL),
parent(NULL),
next_sibling(NULL),
prev_sibling(NULL),
first_child(NULL),
fShowLevel(0),
top_level_view(false),
fNoISInteraction(false),
cpicture(NULL),
comm(NULL),
fVerScroller(NULL),
fHorScroller(NULL),
f_is_printing(false),
attached(false),
_unused_bool1(false),
_unused_bool2(false),
fPermanentState(NULL),
fState(NULL),
fCachedBounds(frame.OffsetToCopy(0,0)),
fShelf(NULL),
pr_state(NULL),
fEventMask(0xFFFFFFFFL),
fEventOptions(0xFFFFFFFFL)
{
}

BView::~BView()
{
}

BView::BView(BMessage *data) :
server_token(-1),
f_type(0),
origin_h(0),
origin_v(0),
owner(NULL),
parent(NULL),
next_sibling(NULL),
prev_sibling(NULL),
first_child(NULL),
fShowLevel(0),
top_level_view(false),
fNoISInteraction(false),
cpicture(NULL),
comm(NULL),
fVerScroller(NULL),
fHorScroller(NULL),
f_is_printing(false),
attached(false),
_unused_bool1(false),
_unused_bool2(false),
fPermanentState(NULL),
fState(NULL),
fCachedBounds(0,0,0,0),
fShelf(NULL),
pr_state(NULL),
fEventMask(0xFFFFFFFFL),
fEventOptions(0xFFFFFFFFL)
{
}

BArchivable *BView::Instantiate(BMessage *data)
{
}

status_t BView::Archive(BMessage *data, bool deep) const
{
}

void BView::AttachedToWindow()
{
}

void BView::AllAttached()
{
}

void BView::DetachedFromWindow()
{
}

void BView::AllDetached()
{
}

void BView::MessageReceived(BMessage *msg)
{
}

void BView::AddChild(BView *child, BView *before)
{
}

bool BView::RemoveChild(BView *child)
{
}

int32 BView::CountChildren() const
{
	int32 count=0;
	if(BView *child=first_child)
	{
		while (child)
		{
			count++;
			child=child->next_sibling;
		}
	}
}

BView *BView::ChildAt(int32 index) const
{
	int32 count=0;
	if(BView *child=first_child)
	{
		while (child)
		{
			count++;
			child=child->next_sibling;
			if(count==index)
				return child;
		}
	}
	return NULL;
}

BView *BView::NextSibling() const
{
	return next_sibling;
}

BView *BView::PreviousSibling() const
{
	return prev_sibling;
}

bool BView::RemoveSelf()
{
	if(!parent)
		return false;

	parent->RemoveChild(this);
	return true;
}

BWindow *BView::Window() const
{
	return owner;
}

void BView::Draw(BRect updateRect)
{
}

void BView::MouseDown(BPoint where)
{
}

void BView::MouseUp(BPoint where)
{
}

void BView::MouseMoved(BPoint where, uint32 code,const BMessage *a_message)
{
}

void BView::WindowActivated(bool state)
{
}

void BView::KeyDown(const char *bytes, int32 numBytes)
{
}

void BView::KeyUp(const char *bytes, int32 numBytes)
{
}

void BView::Pulse()
{
}

void BView::FrameMoved(BPoint new_position)
{
}

void BView::FrameResized(float new_width,float new_height)
{
}

void BView::TargetedByScrollView(BScrollView *scroll_view)
{
}

void BView::BeginRectTracking(BRect startRect,uint32 style)
{
}

void BView::EndRectTracking()
{
}

void BView::GetMouse(BPoint* location,uint32 *buttons,bool checkMessageQueue)
{
}

void BView::DragMessage(BMessage *aMessage,BRect dragRect,BHandler *reply_to)
{
}

void BView::DragMessage(BMessage *aMessage,BBitmap *anImage,BPoint offset,BHandler *reply_to)
{
}

void BView::DragMessage(BMessage *aMessage,BBitmap *anImage,drawing_mode dragMode,BPoint offset,BHandler *reply_to)
{
}

BView *BView::FindView(const char *name) const
{
}

BView *BView::Parent() const
{
	return parent;
}

BRect BView::Bounds() const
{
	BRect bounds=fCachedBounds;
	bounds.OffsetTo(0,0);
	return bounds;
}

BRect BView::Frame() const
{
	return fCachedBounds;
}

void BView::ConvertToScreen(BPoint* pt) const
{
}

BPoint BView::ConvertToScreen(BPoint pt) const
{
}

void BView::ConvertFromScreen(BPoint* pt) const
{
}

BPoint BView::ConvertFromScreen(BPoint pt) const
{
}

void BView::ConvertToScreen(BRect *r) const
{
}

BRect BView::ConvertToScreen(BRect r) const
{
}

void BView::ConvertFromScreen(BRect *r) const
{
}

BRect BView::ConvertFromScreen(BRect r) const
{
}

void BView::ConvertToParent(BPoint *pt) const
{
}

BPoint BView::ConvertToParent(BPoint pt) const
{
}

void BView::ConvertFromParent(BPoint *pt) const
{
}

BPoint BView::ConvertFromParent(BPoint pt) const
{
}

void BView::ConvertToParent(BRect *r) const
{
}

BRect BView::ConvertToParent(BRect r) const
{
}

void BView::ConvertFromParent(BRect *r) const
{
}

BRect BView::ConvertFromParent(BRect r) const
{
}

BPoint BView::LeftTop() const
{
}

void BView::GetClippingRegion(BRegion *region) const
{
}

void BView::ConstrainClippingRegion(BRegion *region)
{
}

void BView::ClipToPicture(BPicture *picture,BPoint where,bool sync)
{
}

void BView::ClipToInversePicture(BPicture *picture,BPoint where,bool sync)
{
}

void BView::SetDrawingMode(drawing_mode mode)
{
}

drawing_mode BView::DrawingMode() const
{
}

void BView::SetBlendingMode(source_alpha srcAlpha, alpha_function alphaFunc)
{
}

void BView::GetBlendingMode(source_alpha *srcAlpha, alpha_function *alphaFunc) const
{
}

void BView::SetPenSize(float size)
{
}

float BView::PenSize() const
{
}

void BView::SetViewCursor(const BCursor *cursor, bool sync)
{
}

void BView::SetViewColor(rgb_color c)
{
}

rgb_color BView::ViewColor() const
{
}

void BView::SetViewBitmap(const BBitmap *bitmap,BRect srcRect, BRect dstRect,uint32 followFlags,uint32 options)
{
}

void BView::SetViewBitmap(const BBitmap *bitmap,uint32 followFlags,uint32 options)
{
}

void BView::ClearViewBitmap()
{
}

status_t BView::SetViewOverlay(const BBitmap *overlay,BRect srcRect, BRect dstRect,rgb_color *colorKey,uint32 followFlags,uint32 options)
{
}

status_t BView::SetViewOverlay(const BBitmap *overlay, rgb_color *colorKey,uint32 followFlags,uint32 options)
{
}

void BView::ClearViewOverlay()
{
}

void BView::SetHighColor(rgb_color a_color)
{
}

rgb_color BView::HighColor() const
{
}

void BView::SetLowColor(rgb_color a_color)
{
}

rgb_color BView::LowColor() const
{
}

void BView::SetLineMode(cap_mode lineCap,join_mode lineJoin,float miterLimit)
{
}

join_mode BView::LineJoinMode() const
{
}

cap_mode BView::LineCapMode() const
{
}

float BView::LineMiterLimit() const
{
}

void BView::SetOrigin(BPoint pt)
{
}

void BView::SetOrigin(float x, float y)
{
}

BPoint BView::Origin() const
{
}

void BView::PushState()
{
}

void BView::PopState()
{
}

void BView::MovePenTo(BPoint pt)
{
}

void BView::MovePenTo(float x, float y)
{
}

void BView::MovePenBy(float x, float y)
{
}

BPoint BView::PenLocation() const
{
}

void BView::StrokeLine(BPoint toPt,pattern p)
{
}

void BView::StrokeLine(BPoint pt0,BPoint pt1,pattern p)
{
}

void BView::BeginLineArray(int32 count)
{
}

void BView::AddLine(BPoint pt0, BPoint pt1, rgb_color col)
{
}

void BView::EndLineArray()
{
}

void BView::StrokePolygon(const BPolygon *aPolygon,bool  closed,pattern p)
{
}

void BView::StrokePolygon(const BPoint *ptArray,int32 numPts, bool  closed,pattern p)
{
}

void BView::StrokePolygon(const BPoint *ptArray,int32 numPts,BRect bounds,bool  closed,pattern p)
{
}

void BView::FillPolygon(const BPolygon *aPolygon,pattern p)
{
}

void BView::FillPolygon(const BPoint *ptArray,int32 numPts,pattern p)
{
}

void BView::FillPolygon(const BPoint *ptArray,int32 numPts,BRect bounds,pattern p)
{
}

void BView::StrokeTriangle(BPoint pt1,BPoint pt2,BPoint pt3,BRect bounds,pattern p)
{
}

void BView::StrokeTriangle(BPoint pt1,BPoint pt2,BPoint pt3,pattern p)
{
}

void BView::FillTriangle(BPoint pt1,BPoint pt2,BPoint pt3,pattern p)
{
}

void BView::FillTriangle(BPoint pt1,BPoint pt2,BPoint pt3,BRect bounds,pattern p)
{
}

void BView::StrokeRect(BRect r, pattern p)
{
}

void BView::FillRect(BRect r, pattern p)
{
}

void BView::FillRegion(BRegion *a_region, pattern p)
{
}

void BView::InvertRect(BRect r)
{
}

void BView::StrokeRoundRect(BRect r,float xRadius,float yRadius,pattern p)
{
}

void BView::FillRoundRect(BRect r,float xRadius,float yRadius,pattern p)
{
}

void BView::StrokeEllipse(BPoint center,float xRadius,float yRadius,pattern p)
{
}

void BView::StrokeEllipse(BRect r, pattern p)
{
}

void BView::FillEllipse(BPoint center,float xRadius,float yRadius,pattern p)
{
}

void BView::FillEllipse(BRect r, pattern p)
{
}

void BView::StrokeArc(BPoint center,float xRadius,float yRadius,float start_angle,float arc_angle,pattern p)
{
}

void BView::StrokeArc(BRect r,float start_angle,float arc_angle,pattern p)
{
}

void BView::FillArc(BPoint center,float xRadius,float yRadius,float start_angle,float arc_angle,pattern p)
{
}

void BView::FillArc(BRect r,float start_angle,float arc_angle,pattern p)
{
}

void BView::StrokeBezier(BPoint *controlPoints,pattern p)
{
}

void BView::FillBezier(BPoint *controlPoints,pattern p)
{
}

void BView::StrokeShape(BShape *shape,pattern p)
{
}

void BView::FillShape(BShape *shape,pattern p)
{
}

void BView::CopyBits(BRect src, BRect dst)
{
}

void BView::DrawBitmapAsync(const BBitmap *aBitmap,BRect srcRect,BRect dstRect)
{
}

void BView::DrawBitmapAsync(const BBitmap *aBitmap)
{
}

void BView::DrawBitmapAsync(const BBitmap *aBitmap, BPoint where)
{
}

void BView::DrawBitmapAsync(const BBitmap *aBitmap, BRect dstRect)
{
}

void BView::DrawBitmap(const BBitmap *aBitmap,BRect srcRect,BRect dstRect)
{
}

void BView::DrawBitmap(const BBitmap *aBitmap)
{
}

void BView::DrawBitmap(const BBitmap *aBitmap, BPoint where)
{
}

void BView::DrawBitmap(const BBitmap *aBitmap, BRect dstRect)
{
}

void BView::DrawChar(char aChar)
{
}

void BView::DrawChar(char aChar, BPoint location)
{
}

void BView::DrawString(const char *aString,escapement_delta *delta)
{
}

void BView::DrawString(const char *aString, BPoint location,escapement_delta *delta)
{
}

void BView::DrawString(const char *aString, int32 length,escapement_delta *delta)
{
}

void BView::DrawString(const char *aString,int32 length,BPoint location,escapement_delta *delta)
{
}

void BView::SetFont(const BFont *font, uint32 mask)
{
}

void BView::GetFont(BFont *font) const
{
}

void BView::TruncateString(BString* in_out,uint32 mode,float width) const
{
}

float BView::StringWidth(const char *string) const
{
}

float BView::StringWidth(const char *string, int32 length) const
{
}

void BView::GetStringWidths(char *stringArray[],int32 lengthArray[],int32 numStrings,float widthArray[]) const
{
}

void BView::SetFontSize(float size)
{
}

void BView::ForceFontAliasing(bool enable)
{
}

void BView::GetFontHeight(font_height *height) const
{
}

void BView::Invalidate(BRect invalRect)
{
}

void BView::Invalidate(const BRegion *invalRegion)
{
}

void BView::Invalidate()
{
}

void BView::SetDiskMode(char *filename, long offset)
{
}

void BView::BeginPicture(BPicture *a_picture)
{
}

void BView::AppendToPicture(BPicture *a_picture)
{
}

BPicture *BView::EndPicture()
{
}

void BView::DrawPicture(const BPicture *a_picture)
{
}

void BView::DrawPicture(const BPicture *a_picture, BPoint where)
{
}

void BView::DrawPicture(const char *filename, long offset, BPoint where)
{
}

void BView::DrawPictureAsync(const BPicture *a_picture)
{
}

void BView::DrawPictureAsync(const BPicture *a_picture, BPoint where)
{
}

void BView::DrawPictureAsync(const char *filename, long offset, BPoint where)
{
}

status_t BView::SetEventMask(uint32 mask, uint32 options)
{
}

uint32 BView::EventMask()
{
}

status_t BView::SetMouseEventMask(uint32 mask, uint32 options)
{
}

void BView::SetFlags(uint32 flags)
{
}

uint32 BView::Flags() const
{
}

void BView::SetResizingMode(uint32 mode)
{
}

uint32 BView::ResizingMode() const
{
}

void BView::MoveBy(float dh, float dv)
{
}

void BView::MoveTo(BPoint where)
{
}

void BView::MoveTo(float x, float y)
{
}

void BView::ResizeBy(float dh, float dv)
{
}

void BView::ResizeTo(float width, float height)
{
}

void BView::ScrollBy(float dh, float dv)
{
}

void BView::ScrollTo(BPoint where)
{
}

void BView::MakeFocus(bool focusState)
{
}

bool BView::IsFocus() const
{
}

void BView::Show()
{
}

void BView::Hide()
{
}

bool BView::IsHidden() const
{
	return (fShowLevel==0)?false:true;
}

bool BView::IsHidden(const BView* looking_from) const
{
}

void BView::Flush() const
{
}

void BView::Sync() const
{
}

void BView::GetPreferredSize(float *width, float *height)
{
}

void BView::ResizeToPreferred()
{
}

BScrollBar *BView::ScrollBar(orientation posture) const
{
}

BHandler *BView::ResolveSpecifier(BMessage *msg,int32 index,BMessage *specifier,int32 form,const char *property)
{
}

status_t BView::GetSupportedSuites(BMessage *data)
{
}

bool BView::IsPrinting() const
{
}

void BView::SetScale(float scale) const
{
}

status_t Perform(perform_code d, void *arg)
{
}

void BView::DrawAfterChildren(BRect r)
{
}

void BView::_ReservedView2()
{	// Empty placeholder virtual
}

void BView::_ReservedView3()
{	// Empty placeholder virtual
}

void BView::_ReservedView4()
{	// Empty placeholder virtual
}

void BView::_ReservedView5()
{	// Empty placeholder virtual
}

void BView::_ReservedView6()
{	// Empty placeholder virtual
}

void BView::_ReservedView7()
{	// Empty placeholder virtual
}

void BView::_ReservedView8()
{	// Empty placeholder virtual
}

void BView::_ReservedView9()
{	// Empty placeholder virtual
}

void BView::_ReservedView10()
{	// Empty placeholder virtual
}

void BView::_ReservedView11()
{	// Empty placeholder virtual
}

void BView::_ReservedView12()
{	// Empty placeholder virtual
}

void BView::_ReservedView13()
{	// Empty placeholder virtual
}

void BView::_ReservedView14()
{	// Empty placeholder virtual
}

void BView::_ReservedView15()
{	// Empty placeholder virtual
}

void BView::_ReservedView16()
{	// Empty placeholder virtual
}

