/* 
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <ChannelSlider.h>
#include <Message.h>


BChannelSlider::BChannelSlider(BRect area, const char *name, const char *label,
	BMessage *model, int32 channels, uint32 resizeMode, uint32 flags)
	: BChannelControl(area, name, label, model, channels, resizeMode, flags)
{
}


BChannelSlider::BChannelSlider(BRect area, const char *name, const char *label,
	BMessage *model, orientation o, int32 channels, uint32 resizeMode, uint32 flags)
	: BChannelControl(area, name, label, model, channels, resizeMode, flags)
{
}


BChannelSlider::BChannelSlider(BMessage *archive)
	: BChannelControl(archive)
{
}


BChannelSlider::~BChannelSlider()
{
}


BArchivable *
BChannelSlider::Instantiate(BMessage *archive)
{
	return NULL;
}


status_t
BChannelSlider::Archive(BMessage *into, bool deep) const
{
	return B_ERROR;
}


orientation
BChannelSlider::Orientation() const
{
	return B_VERTICAL;
}


void
BChannelSlider::SetOrientation(orientation o)
{
}


int32
BChannelSlider::MaxChannelCount() const
{
	return -1;
}


bool
BChannelSlider::SupportsIndividualLimits() const
{
	return false;
}


void
BChannelSlider::AttachedToWindow()
{
}


void
BChannelSlider::AllAttached()
{
}


void
BChannelSlider::DetachedFromWindow()
{
}


void
BChannelSlider::AllDetached()
{
}


void
BChannelSlider::MessageReceived(BMessage *msg)
{
}


void
BChannelSlider::Draw(BRect area)
{
}


void
BChannelSlider::MouseDown(BPoint where)
{
}


void
BChannelSlider::MouseUp(BPoint pt)
{
}


void
BChannelSlider::MouseMoved(BPoint pt, uint32 code, const BMessage *message)
{
}


void
BChannelSlider::WindowActivated(bool state)
{
}


void
BChannelSlider::KeyDown(const char *bytes, int32 numBytes)
{
}


void
BChannelSlider::KeyUp(const char *bytes, int32 numBytes)
{
}


void
BChannelSlider::FrameResized(float width, float height)
{
}


void
BChannelSlider::SetFont(const BFont *font, uint32 mask)
{
}


void
BChannelSlider::MakeFocus(bool focusState)
{
}


void
BChannelSlider::SetEnabled(bool on)
{
}


void
BChannelSlider::GetPreferredSize(float *width, float *height)
{
}


BHandler *
BChannelSlider::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier, int32 form, const char *property)
{
	return NULL;
}


status_t
BChannelSlider::GetSupportedSuites(BMessage *data)
{
	return B_ERROR;
}


void
BChannelSlider::DrawChannel(BView *into, int32 channel, BRect area, bool pressed)
{
}


void
BChannelSlider::DrawGroove(BView *into, int32 channel, BPoint tl, BPoint br)
{
}


void
BChannelSlider::DrawThumb(BView *into, int32 channel, BPoint where, bool pressed)
{
}


const BBitmap *
BChannelSlider::ThumbFor(int32 channel, bool pressed)
{
	return NULL;
}


BRect 
BChannelSlider::ThumbFrameFor(int32 channel)
{
	return BRect();
}


float 
BChannelSlider::ThumbDeltaFor(int32 channel)
{
	return 0.0f;
}


float 
BChannelSlider::ThumbRangeFor(int32 channel)
{
	return 0.0f;
}


void 
BChannelSlider::InitData()
{
}


void 
BChannelSlider::FinishChange()
{
}


void 
BChannelSlider::UpdateFontDimens()
{
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
	return false;
}


void 
BChannelSlider::Redraw()
{
}


void 
BChannelSlider::MouseMovedCommon(BPoint, BPoint)
{
}


void BChannelSlider::_Reserved_BChannelSlider_0(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_1(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_2(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_3(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_4(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_5(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_6(void *, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_7(void *, ...) {}

