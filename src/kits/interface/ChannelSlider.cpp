/* 
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Bitmap.h>
#include <ChannelSlider.h>
#include <Debug.h>
#include <PropertyInfo.h>


static property_info
sPropertyInfo[] = {
	{ "Orientation",
	{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	{ B_DIRECT_SPECIFIER, 0 }, "" },
	
	{ "ChannelCount",
	{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	{ B_DIRECT_SPECIFIER, 0 }, "" },
	
	{ "CurrentChannel",
	{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
	{ B_DIRECT_SPECIFIER, 0 }, "" },
	
	{0}
};
	

BChannelSlider::BChannelSlider(BRect area, const char *name, const char *label,
	BMessage *model, int32 channels, uint32 resizeMode, uint32 flags)
	: BChannelControl(area, name, label, model, channels, resizeMode, flags)
{
	InitData();
}


BChannelSlider::BChannelSlider(BRect area, const char *name, const char *label,
	BMessage *model, orientation o, int32 channels, uint32 resizeMode, uint32 flags)
	: BChannelControl(area, name, label, model, channels, resizeMode, flags)

{
	InitData();
	SetOrientation(o);
}


BChannelSlider::BChannelSlider(BMessage *archive)
	: BChannelControl(archive)
{
	// TODO: Implement
}


BChannelSlider::~BChannelSlider()
{
	delete fInitialValues;
}


BArchivable *
BChannelSlider::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BChannelSlider"))
		return new BChannelSlider(archive);
	else
		return NULL;
}


status_t
BChannelSlider::Archive(BMessage *into, bool deep) const
{
	// TODO: Implement
	return B_ERROR;
}


orientation
BChannelSlider::Orientation() const
{
	return Vertical() ? B_VERTICAL : B_HORIZONTAL;
}


void
BChannelSlider::SetOrientation(orientation _orientation)
{
	bool isVertical = _orientation == B_VERTICAL;
	if (isVertical != Vertical()) {
		fVertical = isVertical;
		Invalidate(Bounds());
	}	
}


int32
BChannelSlider::MaxChannelCount() const
{
	return 32;
}


bool
BChannelSlider::SupportsIndividualLimits() const
{
	return false;
}


void
BChannelSlider::AttachedToWindow()
{
	BView *parent = Parent();
	if (parent != NULL)
		SetViewColor(parent->ViewColor());
	
	inherited::AttachedToWindow();
}


void
BChannelSlider::AllAttached()
{
	BControl::AllAttached();
}


void
BChannelSlider::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
}


void
BChannelSlider::AllDetached()
{
	BControl::AllDetached();
}


void
BChannelSlider::MessageReceived(BMessage *message)
{
	inherited::MessageReceived(message);
}


void
BChannelSlider::Draw(BRect updateRect)
{
	UpdateFontDimens();
	DrawThumbs();
	
	BRect bounds(Bounds());
	float labelWidth = StringWidth(Label());
	
	MovePenTo((bounds.Width() - labelWidth) / 2, 10);
	DrawString(Label());
	
	// TODO: Respect label limits !!!	
}


void
BChannelSlider::MouseDown(BPoint where)
{
	// TODO: Implement
}


void
BChannelSlider::MouseUp(BPoint where)
{
	// TODO: Implement
}


void
BChannelSlider::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	if (IsEnabled() && IsTracking())
		MouseMovedCommon(where, B_ORIGIN);
	else
		BControl::MouseMoved(where, code, message);
}


void
BChannelSlider::WindowActivated(bool state)
{
	BControl::WindowActivated(state);
}


void
BChannelSlider::KeyDown(const char *bytes, int32 numBytes)
{
	// TODO: Implement
}


void
BChannelSlider::KeyUp(const char *bytes, int32 numBytes)
{
	BView::KeyUp(bytes, numBytes);
}


void
BChannelSlider::FrameResized(float newWidth, float newHeight)
{
	inherited::FrameResized(newWidth, newHeight);
	Invalidate(Bounds());
}


void
BChannelSlider::SetFont(const BFont *font, uint32 mask)
{
	inherited::SetFont(font, mask);
}


void
BChannelSlider::MakeFocus(bool focusState)
{
	if (focusState && !IsFocus())
		fFocusChannel = -1;
	BControl::MakeFocus(focusState);
}


void
BChannelSlider::SetEnabled(bool on)
{
	BControl::SetEnabled(on);
}


void
BChannelSlider::GetPreferredSize(float *width, float *height)
{
	// TODO: Implement
}


BHandler *
BChannelSlider::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
								int32 form, const char *property)
{
	// TODO: Implement
	return NULL;
}


status_t
BChannelSlider::GetSupportedSuites(BMessage *data)
{
	if (data == NULL)
		return B_BAD_VALUE;
	
	data->AddString("suites", "suite/vnd.Be-channel-slider");

	BPropertyInfo propInfo(sPropertyInfo);
	data->AddFlat("messages", &propInfo, 1);
	
	return inherited::GetSupportedSuites(data);
}


void
BChannelSlider::DrawChannel(BView *into, int32 channel, BRect area, bool pressed)
{
	// TODO: Implement
}



void
BChannelSlider::DrawGroove(BView *into, int32 channel, BPoint topLeft, BPoint bottomRight)
{
	// TODO: Implement
}


void
BChannelSlider::DrawThumb(BView *into, int32 channel, BPoint where, bool pressed)
{
	ASSERT(into != NULL);
		
	const BBitmap *thumb = ThumbFor(channel, pressed);
	if (thumb == NULL)
		return;

	BRect bitmapBounds = thumb->Bounds();
	where.x -= bitmapBounds.right / 2;
	where.y -= bitmapBounds.bottom / 2;
	
	into->DrawBitmapAsync(thumb, where);
		
	if (pressed) {
		into->PushState();
	
		into->SetDrawingMode(B_OP_ALPHA);
		BRect rect(where, where);
		rect.right += bitmapBounds.right;
		rect.bottom += bitmapBounds.bottom;
		into->SetHighColor(tint_color(ViewColor(), B_DARKEN_4_TINT));
		into->FillRect(rect);

		into->PopState();
	}
}


const BBitmap *
BChannelSlider::ThumbFor(int32 channel, bool pressed)
{
	// TODO: Implement
	return NULL;
}


BRect
BChannelSlider::ThumbFrameFor(int32 channel)
{
	UpdateFontDimens();
	
	BRect frame(0, 0, 0, 0);
	const BBitmap *thumb = ThumbFor(channel, false);
	if (thumb != NULL) {
		frame = thumb->Bounds();
		if (Vertical())
			frame.OffsetBy(0, fLineFeed * 2);
		else
			frame.OffsetBy(fLineFeed, fLineFeed);
	}
	
	return frame;	
}


float
BChannelSlider::ThumbDeltaFor(int32 channel)
{
	float delta = 0;
	if (channel >= 0 && channel < MaxChannelCount()) {
		float range = ThumbRangeFor(channel);
		int32 limitRange = MaxLimitList()[channel] - MinLimitList()[channel];
		delta = ValueList()[channel] * range / limitRange;  
			
		if (Vertical())
			delta = range - delta;
	}
	
	return delta;	
}


float
BChannelSlider::ThumbRangeFor(int32 channel)
{
	UpdateFontDimens();
	
	float range = 0;
	
	BRect bounds = Bounds();
	BRect frame = ThumbFrameFor(channel);
	if (Vertical())
		range = bounds.Height() - frame.Height() - fLineFeed * 4;
	else
		range = bounds.Width() - frame.Width() - fLineFeed * 2;
	
	return range; 
}


void
BChannelSlider::InitData()
{
	UpdateFontDimens();
	
	fLeftKnob = NULL;
	fMidKnob = NULL;
	fRightKnob = NULL;
	fBacking = NULL;
	fBackingView = NULL;
	fVertical = Bounds().Width() / Bounds().Height() < 1;
	fClickDelta = B_ORIGIN;
	
	fCurrentChannel = -1;
	fAllChannels = false;
	fInitialValues = NULL;
	fMinpoint = 0;
	fFocusChannel = -1;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// TODO: Set initial values ?
	// actually, looks like initial values are set on MouseDown()/MouseMoved()
}


void
BChannelSlider::FinishChange()
{
	if (fInitialValues != NULL) {
		if (fAllChannels) {
			// TODO: Iterate through the list of channels, and invoke only
			// for changed values
			
			InvokeChannel();
			
		} else {
			if (ValueList()[fCurrentChannel] != fInitialValues[fCurrentChannel]) {
				SetValueFor(fCurrentChannel, ValueList()[fCurrentChannel]);
				Invoke();
			}	
		}
	}
	
	SetTracking(false);
	Redraw();
}


void
BChannelSlider::UpdateFontDimens()
{
	font_height height;
	GetFontHeight(&height);
	fBaseLine = height.ascent + height.leading;
	fLineFeed = fBaseLine + height.descent;
}


void
BChannelSlider::DrawThumbs()
{	
}


void
BChannelSlider::DrawThumbFrame(BView *where, const BRect &area)
{
}


bool
BChannelSlider::Vertical()
{
	return fVertical;
}


void
BChannelSlider::Redraw()
{
	Invalidate(Bounds());
	Flush();
}


void
BChannelSlider::MouseMovedCommon(BPoint , BPoint )
{
	// TODO: Implement
}


void BChannelSlider::_Reserved_BChannelSlider_0(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_1(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_2(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_3(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_4(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_5(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_6(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_7(void *, ...) {}
