/*
 * Copyright 2005-2009, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include <ChannelSlider.h>

#include <new>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <PropertyInfo.h>
#include <Screen.h>
#include <Window.h>


const static unsigned char
kVerticalKnobData[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0xff,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0xff,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0xcb, 0xcb, 0xcb, 0xcb, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x12,
	0xff, 0xff, 0xff, 0xff, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0xff
};


const static unsigned char
kHorizontalKnobData[] = {
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0xff, 0xff, 0xff, 0x00, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0xff, 0xff,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xcb, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x00, 0x12, 0xff, 0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xcb,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12, 0xff, 0xff, 0x00, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0xcb, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12, 0xff,
	0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0xcb, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x00, 0x12, 0xff, 0xff, 0x00, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12, 0xff, 0xff, 0x00, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x00, 0x12, 0xff,
	0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0c, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0x12, 0x12, 0x12, 0x12,
	0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};


static property_info
sPropertyInfo[] = {
	{ "Orientation",
		{ B_GET_PROPERTY, B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 }, NULL, 0, { B_INT32_TYPE }
	},

	{ 0 }
};


const static float kPadding = 3.0;


BChannelSlider::BChannelSlider(BRect area, const char* name, const char* label,
	BMessage* model, int32 channels, uint32 resizeMode, uint32 flags)
	: BChannelControl(area, name, label, model, channels, resizeMode, flags)
{
	_InitData();
}


BChannelSlider::BChannelSlider(BRect area, const char* name, const char* label,
	BMessage* model, enum orientation orientation, int32 channels,
		uint32 resizeMode, uint32 flags)
	: BChannelControl(area, name, label, model, channels, resizeMode, flags)

{
	_InitData();
	SetOrientation(orientation);
}


BChannelSlider::BChannelSlider(const char* name, const char* label,
	BMessage* model, enum orientation orientation, int32 channels,
		uint32 flags)
	: BChannelControl(name, label, model, channels, flags)

{
	_InitData();
	SetOrientation(orientation);
}


BChannelSlider::BChannelSlider(BMessage* archive)
	: BChannelControl(archive)
{
	_InitData();

	orientation orient;
	if (archive->FindInt32("_orient", (int32*)&orient) == B_OK)
		SetOrientation(orient);
}


BChannelSlider::~BChannelSlider()
{
	delete fBacking;
	delete fLeftKnob;
	delete fMidKnob;
	delete fRightKnob;
	delete[] fInitialValues;
}


BArchivable*
BChannelSlider::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BChannelSlider"))
		return new (std::nothrow) BChannelSlider(archive);

	return NULL;
}


status_t
BChannelSlider::Archive(BMessage* into, bool deep) const
{
	status_t status = BChannelControl::Archive(into, deep);
	if (status == B_OK)
		status = into->AddInt32("_orient", (int32)Orientation());

	return status;
}


void
BChannelSlider::AttachedToWindow()
{
	BView* parent = Parent();
	if (parent != NULL)
		SetViewColor(parent->ViewColor());

	BChannelControl::AttachedToWindow();
}


void
BChannelSlider::AllAttached()
{
	BChannelControl::AllAttached();
}


void
BChannelSlider::DetachedFromWindow()
{
	BChannelControl::DetachedFromWindow();
}


void
BChannelSlider::AllDetached()
{
	BChannelControl::AllDetached();
}


void
BChannelSlider::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SET_PROPERTY: {
		case B_GET_PROPERTY:
			BMessage reply(B_REPLY);
			int32 index = 0;
			BMessage specifier;
			int32 what = 0;
			const char* property = NULL;
			bool handled = false;
			status_t status = message->GetCurrentSpecifier(&index, &specifier,
				&what, &property);
			BPropertyInfo propInfo(sPropertyInfo);
			if (status == B_OK
				&& propInfo.FindMatch(message, index, &specifier, what,
					property) >= 0) {
				handled = true;
				if (message->what == B_SET_PROPERTY) {
					orientation orient;
					if (specifier.FindInt32("data", (int32*)&orient) == B_OK) {
						SetOrientation(orient);
						Invalidate(Bounds());
					}
				} else if (message->what == B_GET_PROPERTY)
					reply.AddInt32("result", (int32)Orientation());
				else
					status = B_BAD_SCRIPT_SYNTAX;
			}

			if (handled) {
				reply.AddInt32("error", status);
				message->SendReply(&reply);
			} else {
				BChannelControl::MessageReceived(message);
			}
		}	break;

		default:
			BChannelControl::MessageReceived(message);
			break;
	}
}


void
BChannelSlider::Draw(BRect updateRect)
{
	_UpdateFontDimens();
	_DrawThumbs();

	BRect bounds(Bounds());
	if (Label()) {
		float labelWidth = StringWidth(Label());
		DrawString(Label(), BPoint((bounds.Width() - labelWidth) / 2.0,
			fBaseLine));
	}

	if (MinLimitLabel()) {
		if (fIsVertical) {
			if (MinLimitLabel()) {
				float x = (bounds.Width() - StringWidth(MinLimitLabel()))
					/ 2.0;
				DrawString(MinLimitLabel(), BPoint(x, bounds.bottom
					- kPadding));
			}
		} else {
			if (MinLimitLabel()) {
				DrawString(MinLimitLabel(), BPoint(kPadding, bounds.bottom
					- kPadding));
			}
		}
	}

	if (MaxLimitLabel()) {
		if (fIsVertical) {
			if (MaxLimitLabel()) {
				float x = (bounds.Width() - StringWidth(MaxLimitLabel()))
					/ 2.0;
				DrawString(MaxLimitLabel(), BPoint(x, 2 * fLineFeed));
			}
		} else {
			if (MaxLimitLabel()) {
				DrawString(MaxLimitLabel(), BPoint(bounds.right - kPadding
					- StringWidth(MaxLimitLabel()), bounds.bottom - kPadding));
			}
		}
	}
}


void
BChannelSlider::MouseDown(BPoint where)
{
	if (!IsEnabled())
		BControl::MouseDown(where);
	else {
		fCurrentChannel = -1;
		fMinPoint = 0;

		// Search the channel on which the mouse was over
		int32 numChannels = CountChannels();
		for (int32 channel = 0; channel < numChannels; channel++) {
			BRect frame = ThumbFrameFor(channel);
			frame.OffsetBy(fClickDelta);

			float range = ThumbRangeFor(channel);
			if (fIsVertical) {
				fMinPoint = frame.top + frame.Height() / 2;
				frame.bottom += range;
			} else {
				// TODO: Fix this, the clickzone isn't perfect
				frame.right += range;
				fMinPoint = frame.Width();
			}

			// Click was on a slider.
			if (frame.Contains(where)) {
				fCurrentChannel = channel;
				SetCurrentChannel(channel);
				break;
			}
		}

		// Click wasn't on a slider. Bail out.
		if (fCurrentChannel == -1)
			return;

		uint32 buttons = 0;
		BMessage* currentMessage = Window()->CurrentMessage();
		if (currentMessage != NULL)
			currentMessage->FindInt32("buttons", (int32*)&buttons);

		fAllChannels = (buttons & B_SECONDARY_MOUSE_BUTTON) == 0;

		if (fInitialValues != NULL && fAllChannels) {
			delete[] fInitialValues;
			fInitialValues = NULL;
		}

		if (fInitialValues == NULL)
			fInitialValues = new (std::nothrow) int32[numChannels];

		if (fInitialValues) {
			if (fAllChannels) {
				for (int32 i = 0; i < numChannels; i++)
					fInitialValues[i] = ValueFor(i);
			} else {
				fInitialValues[fCurrentChannel] = ValueFor(fCurrentChannel);
			}
		}

		if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
			if (!IsTracking()) {
				SetTracking(true);
				_DrawThumbs();
				Flush();
			}

			_MouseMovedCommon(where, B_ORIGIN);
			SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS |
				B_NO_POINTER_HISTORY);
		} else {
			do {
				snooze(30000);
				GetMouse(&where, &buttons);
				_MouseMovedCommon(where, B_ORIGIN);
			} while (buttons != 0);
			_FinishChange();
			fCurrentChannel = -1;
			fAllChannels = false;
		}
	}
}


void
BChannelSlider::MouseUp(BPoint where)
{
	if (IsEnabled() && IsTracking()) {
		_FinishChange();
		SetTracking(false);
		fAllChannels = false;
		fCurrentChannel = -1;
		fMinPoint = 0;
	} else {
		BChannelControl::MouseUp(where);
	}
}


void
BChannelSlider::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	if (IsEnabled() && IsTracking())
		_MouseMovedCommon(where, B_ORIGIN);
	else
		BChannelControl::MouseMoved(where, code, message);
}


void
BChannelSlider::WindowActivated(bool state)
{
	BChannelControl::WindowActivated(state);
}


void
BChannelSlider::KeyDown(const char* bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
}


void
BChannelSlider::KeyUp(const char* bytes, int32 numBytes)
{
	BView::KeyUp(bytes, numBytes);
}


void
BChannelSlider::FrameResized(float newWidth, float newHeight)
{
	BChannelControl::FrameResized(newWidth, newHeight);

	delete fBacking;
	fBacking = NULL;

	Invalidate(Bounds());
}


void
BChannelSlider::SetFont(const BFont* font, uint32 mask)
{
	BChannelControl::SetFont(font, mask);
}


void
BChannelSlider::MakeFocus(bool focusState)
{
	if (focusState && !IsFocus())
		fFocusChannel = -1;
	BChannelControl::MakeFocus(focusState);
}


void
BChannelSlider::GetPreferredSize(float* width, float* height)
{
	_UpdateFontDimens();

	if (fIsVertical) {
		*width = 11.0 * CountChannels();
		*width = max_c(*width, ceilf(StringWidth(Label())));
		*width = max_c(*width, ceilf(StringWidth(MinLimitLabel())));
		*width = max_c(*width, ceilf(StringWidth(MaxLimitLabel())));
		*width += kPadding * 2.0;

		*height = (fLineFeed * 3.0) + (kPadding * 2.0) + 147.0;
	} else {
		*width = max_c(64.0, ceilf(StringWidth(Label())));
		*width = max_c(*width, ceilf(StringWidth(MinLimitLabel()))
			+ ceilf(StringWidth(MaxLimitLabel())) + 10.0);
		*width += kPadding * 2.0;

		*height = 11.0 * CountChannels() + (fLineFeed * 2.0)
			+ (kPadding * 2.0);
	}
}


BHandler*
BChannelSlider::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	BHandler* target = this;
	BPropertyInfo propertyInfo(sPropertyInfo);
	if (propertyInfo.FindMatch(message, index, specifier, form,
		property) != B_OK) {
		target = BChannelControl::ResolveSpecifier(message, index, specifier,
			form, property);
	}
	return target;
}


status_t
BChannelSlider::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t err = data->AddString("suites", "suite/vnd.Be-channel-slider");

	BPropertyInfo propInfo(sPropertyInfo);
	if (err == B_OK)
		err = data->AddFlat("messages", &propInfo);

	if (err == B_OK)
		return BChannelControl::GetSupportedSuites(data);
	return err;
}


void
BChannelSlider::SetEnabled(bool on)
{
	BChannelControl::SetEnabled(on);
}


orientation
BChannelSlider::Orientation() const
{
	return fIsVertical ? B_VERTICAL : B_HORIZONTAL;
}


void
BChannelSlider::SetOrientation(enum orientation orientation)
{
	bool isVertical = orientation == B_VERTICAL;
	if (isVertical != fIsVertical) {
		fIsVertical = isVertical;
		InvalidateLayout();
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
BChannelSlider::DrawChannel(BView* into, int32 channel, BRect area,
	bool pressed)
{
	float hCenter = area.Width() / 2;
	float vCenter = area.Height() / 2;

	BPoint leftTop;
	BPoint bottomRight;
	if (fIsVertical) {
		leftTop.Set(area.left + hCenter, area.top + vCenter);
		bottomRight.Set(leftTop.x, leftTop.y + ThumbRangeFor(channel));
	} else {
		leftTop.Set(area.left, area.top + vCenter);
		bottomRight.Set(area.left + ThumbRangeFor(channel), leftTop.y);
	}

	DrawGroove(into, channel, leftTop, bottomRight);

	BPoint thumbLocation = leftTop;
	if (fIsVertical)
		thumbLocation.y += ThumbDeltaFor(channel);
	else
		thumbLocation.x += ThumbDeltaFor(channel);

	DrawThumb(into, channel, thumbLocation, pressed);
}


void
BChannelSlider::DrawGroove(BView* into, int32 channel, BPoint leftTop,
	BPoint bottomRight)
{
	ASSERT(into != NULL);
	BRect rect(leftTop, bottomRight);

	if (be_control_look != NULL) {
		rect.InsetBy(-2.5, -2.5);
		rect.left = floorf(rect.left);
		rect.top = floorf(rect.top);
		rect.right = floorf(rect.right);
		rect.bottom = floorf(rect.bottom);
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		rgb_color barColor = be_control_look->SliderBarColor(base);
		uint32 flags = 0;
		be_control_look->DrawSliderBar(into, rect, rect, base,
			barColor, flags, Orientation());
		return;
	}

	_DrawGrooveFrame(fBackingView, rect.InsetByCopy(-2.5, -2.5));

	rect.InsetBy(-0.5, -0.5);
	into->FillRect(rect, B_SOLID_HIGH);
}


void
BChannelSlider::DrawThumb(BView* into, int32 channel, BPoint where,
	bool pressed)
{
	ASSERT(into != NULL);

	const BBitmap* thumb = ThumbFor(channel, pressed);
	if (thumb == NULL)
		return;

	BRect bitmapBounds(thumb->Bounds());
	where.x -= bitmapBounds.right / 2.0;
	where.y -= bitmapBounds.bottom / 2.0;


	if (be_control_look != NULL) {
		BRect rect(bitmapBounds.OffsetToCopy(where));
		rect.InsetBy(1, 1);
		rect.left = floorf(rect.left);
		rect.top = floorf(rect.top);
		rect.right = ceilf(rect.right + 0.5);
		rect.bottom = ceilf(rect.bottom + 0.5);
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		uint32 flags = 0;
		be_control_look->DrawSliderThumb(into, rect, rect, base,
			flags, Orientation());
		return;
	}


	into->PushState();

	into->SetDrawingMode(B_OP_OVER);
	into->DrawBitmapAsync(thumb, where);

	if (pressed) {
		into->SetDrawingMode(B_OP_ALPHA);

		rgb_color color = tint_color(into->ViewColor(), B_DARKEN_4_TINT);
		color.alpha = 128;
		into->SetHighColor(color);

		BRect destRect(where, where);
		destRect.right += bitmapBounds.right;
		destRect.bottom += bitmapBounds.bottom;

		into->FillRect(destRect);
	}

	into->PopState();
}


const BBitmap*
BChannelSlider::ThumbFor(int32 channel, bool pressed)
{
	// TODO: Finish me (check allocations... etc)
	if (fLeftKnob == NULL) {
		if (fIsVertical) {
			fLeftKnob = new (std::nothrow) BBitmap(BRect(0, 0, 11, 14),
				B_CMAP8);
			fLeftKnob->SetBits(kVerticalKnobData, sizeof(kVerticalKnobData), 0,
				B_CMAP8);
		} else {
			fLeftKnob = new (std::nothrow) BBitmap(BRect(0, 0, 14, 11),
				B_CMAP8);
			fLeftKnob->SetBits(kHorizontalKnobData,
				sizeof(kHorizontalKnobData), 0, B_CMAP8);
		}
	}

	return fLeftKnob;
}


BRect
BChannelSlider::ThumbFrameFor(int32 channel)
{
	_UpdateFontDimens();

	BRect frame(0.0, 0.0, 0.0, 0.0);
	const BBitmap* thumb = ThumbFor(channel, false);
	if (thumb != NULL) {
		frame = thumb->Bounds();
		if (fIsVertical) {
			frame.OffsetBy(channel * frame.Width(), (frame.Height() / 2.0) -
				(kPadding * 2.0) - 1.0);
		} else {
			frame.OffsetBy(frame.Width() / 2.0, channel * frame.Height()
				+ 1.0);
		}
	}
	return frame;
}


float
BChannelSlider::ThumbDeltaFor(int32 channel)
{
	float delta = 0.0;
	if (channel >= 0 && channel < MaxChannelCount()) {
		float range = ThumbRangeFor(channel);
		int32 limitRange = MaxLimitList()[channel] - MinLimitList()[channel];
		delta = (ValueList()[channel] - MinLimitList()[channel]) * range
			/ limitRange;

		if (fIsVertical)
			delta = range - delta;
	}

	return delta;
}


float
BChannelSlider::ThumbRangeFor(int32 channel)
{
	_UpdateFontDimens();

	float range = 0;
	BRect bounds = Bounds();
	BRect frame = ThumbFrameFor(channel);
	if (fIsVertical) {
		// *height = (fLineFeed * 3.0) + (kPadding * 2.0) + 100.0;
		range = bounds.Height() - frame.Height() - (fLineFeed * 3.0) -
			(kPadding * 2.0);
	} else {
		// *width = some width + kPadding * 2.0;
		range = bounds.Width() - frame.Width() - (kPadding * 2.0);
	}
	return range;
}


// #pragma mark -


void
BChannelSlider::_InitData()
{
	_UpdateFontDimens();

	fLeftKnob = NULL;
	fMidKnob = NULL;
	fRightKnob = NULL;
	fBacking = NULL;
	fBackingView = NULL;
	fIsVertical = Bounds().Width() / Bounds().Height() < 1;
	fClickDelta = B_ORIGIN;

	fCurrentChannel = -1;
	fAllChannels = false;
	fInitialValues = NULL;
	fMinPoint = 0;
	fFocusChannel = -1;

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
BChannelSlider::_FinishChange(bool update)
{
	if (fInitialValues != NULL) {
		bool* inMask = NULL;
		int32 numChannels = CountChannels();
		if (!fAllChannels) {
			inMask = new (std::nothrow) bool[CountChannels()];
			if (inMask) {
				for (int i=0; i<numChannels; i++)
					inMask[i] = false;
				inMask[fCurrentChannel] = true;
			}
		}
		InvokeChannel(update ? ModificationMessage() : NULL, 0, numChannels,
			inMask);
	}

	if (!update) {
		SetTracking(false);
		Invalidate();
	}
}


void
BChannelSlider::_UpdateFontDimens()
{
	font_height height;
	GetFontHeight(&height);
	fBaseLine = height.ascent + height.leading;
	fLineFeed = fBaseLine + height.descent;
}


void
BChannelSlider::_DrawThumbs()
{
	if (fBacking == NULL) {
		// This is the idea: we build a bitmap by taking the coordinates
		// of the first and last thumb frames (top/left and bottom/right)
		BRect first = ThumbFrameFor(0);
		BRect last = ThumbFrameFor(CountChannels() - 1);
		BRect rect(first.LeftTop(), last.RightBottom());

		if (fIsVertical)
			rect.top -= ThumbRangeFor(0);
		else
			rect.right += ThumbRangeFor(0);

		rect.OffsetTo(B_ORIGIN);
		fBacking = new (std::nothrow) BBitmap(rect, B_RGB32, true);
		if (fBacking) {
			fBackingView = new (std::nothrow) BView(rect, "", 0, B_WILL_DRAW);
			if (fBackingView) {
				if (fBacking->Lock()) {
					fBacking->AddChild(fBackingView);
					fBackingView->SetFontSize(10.0);
					fBackingView->SetLowColor(
						ui_color(B_PANEL_BACKGROUND_COLOR));
					fBackingView->SetViewColor(
						ui_color(B_PANEL_BACKGROUND_COLOR));
					fBacking->Unlock();
				}
			} else {
				delete fBacking;
				fBacking = NULL;
			}
		}
	}

	if (fBacking && fBackingView) {
		BPoint drawHere;

		BRect bounds(fBacking->Bounds());
		drawHere.x = (Bounds().Width() - bounds.Width()) / 2.0;
		drawHere.y = (Bounds().Height() - bounds.Height()) - kPadding
			- fLineFeed;

		if (fBacking->Lock()) {
			// Clear the view's background
			fBackingView->FillRect(fBackingView->Bounds(), B_SOLID_LOW);

			BRect channelArea;
			// draw the entire control
			for (int32 channel = 0; channel < CountChannels(); channel++) {
				channelArea = ThumbFrameFor(channel);
				bool pressed = IsTracking()
					&& (channel == fCurrentChannel || fAllChannels);
				DrawChannel(fBackingView, channel, channelArea, pressed);
			}

			// draw some kind of current value tool tip
			if (fCurrentChannel != -1 && fMinPoint != 0) {
				char valueString[32];
				snprintf(valueString, 32, "%" B_PRId32,
					ValueFor(fCurrentChannel));
				float stringWidth = fBackingView->StringWidth(valueString);
				float width = max_c(10.0, stringWidth);
				BRect valueRect(0.0, 0.0, width, 10.0);

				BRect thumbFrame(ThumbFrameFor(fCurrentChannel));
				float thumbDelta(ThumbDeltaFor(fCurrentChannel));

				if (fIsVertical) {
					valueRect.OffsetTo((thumbFrame.Width() - width) / 2.0
						+ fCurrentChannel * thumbFrame.Width(),
						thumbDelta + thumbFrame.Height() + 2.0);
					if (valueRect.bottom > fBackingView->Frame().bottom)
						valueRect.OffsetBy(0.0, -(thumbFrame.Height() + 12.0));
				} else {
					valueRect.OffsetTo((thumbDelta - (width + 2.0)),
						thumbFrame.top);
					if (valueRect.left < fBackingView->Frame().left) {
						valueRect.OffsetBy(thumbFrame.Width() + width + 2.0,
							0.0);
					}
				}

				rgb_color oldColor = fBackingView->HighColor();
				fBackingView->SetHighColor(255, 255, 172);
				fBackingView->FillRect(valueRect);
				fBackingView->SetHighColor(0, 0, 0);
				fBackingView->DrawString(valueString, BPoint(valueRect.left
					+ (valueRect.Width() - stringWidth) / 2.0, valueRect.bottom
					- 1.0));
				fBackingView->StrokeRect(valueRect.InsetByCopy(-0.5, -0.5));
				fBackingView->SetHighColor(oldColor);
			}

			fBackingView->Sync();
			fBacking->Unlock();
		}

		DrawBitmapAsync(fBacking, drawHere);

		// fClickDelta is used in MouseMoved()
		fClickDelta = drawHere;
	}
}


void
BChannelSlider::_DrawGrooveFrame(BView* into, const BRect& area)
{
	if (into) {
		rgb_color oldColor = into->HighColor();

		into->SetHighColor(255, 255, 255);
		into->StrokeRect(area);
		into->SetHighColor(tint_color(into->ViewColor(), B_DARKEN_1_TINT));
		into->StrokeLine(area.LeftTop(), BPoint(area.right, area.top));
		into->StrokeLine(area.LeftTop(), BPoint(area.left, area.bottom - 1));
		into->SetHighColor(tint_color(into->ViewColor(), B_DARKEN_2_TINT));
		into->StrokeLine(BPoint(area.left + 1, area.top + 1),
			BPoint(area.right - 1, area.top + 1));
		into->StrokeLine(BPoint(area.left + 1, area.top + 1),
			BPoint(area.left + 1, area.bottom - 2));

		into->SetHighColor(oldColor);
	}
}


void
BChannelSlider::_MouseMovedCommon(BPoint point, BPoint point2)
{
	float floatValue = 0;
	int32 limitRange = MaxLimitList()[fCurrentChannel] -
			MinLimitList()[fCurrentChannel];
	float range = ThumbRangeFor(fCurrentChannel);
	if (fIsVertical)
		floatValue = range - (point.y - fMinPoint);
	else
		floatValue = range + (point.x - fMinPoint);

	int32 value = (int32)(floatValue / range * limitRange) +
		MinLimitList()[fCurrentChannel];
	if (fAllChannels)
		SetAllValue(value);
	else
		SetValueFor(fCurrentChannel, value);

	if (ModificationMessage()) 
		_FinishChange(true);

	_DrawThumbs();
}


// #pragma mark - FBC padding


void BChannelSlider::_Reserved_BChannelSlider_0(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_1(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_2(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_3(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_4(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_5(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_6(void*, ...) {}
void BChannelSlider::_Reserved_BChannelSlider_7(void*, ...) {}
