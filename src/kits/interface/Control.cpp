/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

/*!	BControl is the base class for user-event handling objects. */


#include <stdlib.h>
#include <string.h>

#include <Control.h>
#include <PropertyInfo.h>
#include <Window.h>

#include <binary_compatibility/Interface.h>


static property_info sPropertyList[] = {
	{
		"Enabled",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_BOOL_TYPE }
	},
	{
		"Label",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_STRING_TYPE }
	},
	{
		"Value",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},
	{}
};


BControl::BControl(BRect frame, const char *name, const char *label,
	BMessage *message, uint32 resizingMode, uint32 flags)
	: BView(frame, name, resizingMode, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}


BControl::BControl(const char *name, const char *label, BMessage *message,
	uint32 flags)
	: BView(name, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}


BControl::~BControl()
{
	free(fLabel);
	SetMessage(NULL);
}


BControl::BControl(BMessage *archive)
	: BView(archive)
{
	InitData(archive);

	BMessage message;
	if (archive->FindMessage("_msg", &message) == B_OK)
		SetMessage(new BMessage(message));

	const char *label;
	if (archive->FindString("_label", &label) == B_OK)
		SetLabel(label);

	int32 value;
	if (archive->FindInt32("_val", &value) == B_OK)
		SetValue(value);

	bool toggle;
	if (archive->FindBool("_disable", &toggle) == B_OK)
		SetEnabled(!toggle);

	if (archive->FindBool("be:wants_nav", &toggle) == B_OK)
		fWantsNav = toggle;
}


BArchivable *
BControl::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BControl"))
		return new BControl(archive);

	return NULL;
}


status_t
BControl::Archive(BMessage *archive, bool deep) const
{
	status_t status = BView::Archive(archive, deep);

	if (status == B_OK && Message())
		status = archive->AddMessage("_msg", Message());

	if (status == B_OK && fLabel)
		status = archive->AddString("_label", fLabel);

	if (status == B_OK && fValue != B_CONTROL_OFF)
		status = archive->AddInt32("_val", fValue);

	if (status == B_OK && !fEnabled)
		status = archive->AddBool("_disable", true);

	return status;
}


void
BControl::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
BControl::AttachedToWindow()
{
	rgb_color color;

	BView* parent = Parent();
	if (parent != NULL) {
		// inherit the color from parent
		color = parent->ViewColor();
		if (color == B_TRANSPARENT_COLOR)
			color = ui_color(B_PANEL_BACKGROUND_COLOR);
	} else
		color = ui_color(B_PANEL_BACKGROUND_COLOR);

	SetViewColor(color);
	SetLowColor(color);

	if (!Messenger().IsValid())
		SetTarget(Window());

	BView::AttachedToWindow();
}


void
BControl::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}


void
BControl::AllAttached()
{
	BView::AllAttached();
}


void
BControl::AllDetached()
{
	BView::AllDetached();
}


void
BControl::MessageReceived(BMessage *message)
{
	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		BMessage reply(B_REPLY);
		bool handled = false;

		BMessage specifier;
		int32 index;
		int32 form;
		const char *property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
			if (strcmp(property, "Label") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddString("result", fLabel);
					handled = true;
				} else {
					// B_SET_PROPERTY
					const char *label;
					if (message->FindString("data", &label) == B_OK) {
						SetLabel(label);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (strcmp(property, "Value") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddInt32("result", fValue);
					handled = true;
				} else {
					// B_SET_PROPERTY
					int32 value;
					if (message->FindInt32("data", &value) == B_OK) {
						SetValue(value);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (strcmp(property, "Enabled") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddBool("result", fEnabled);
					handled = true;
				} else {
					// B_SET_PROPERTY
					bool enabled;
					if (message->FindBool("data", &enabled) == B_OK) {
						SetEnabled(enabled);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
		}

		if (handled) {
			message->SendReply(&reply);
			return;
		}
	}

	BView::MessageReceived(message);
}


void
BControl::MakeFocus(bool focused)
{
	if (focused == IsFocus())
		return;

	BView::MakeFocus(focused);

 	if (Window()) {
		fFocusChanging = true;
		Invalidate(Bounds());
		Flush();
		fFocusChanging = false;
	}
}


void
BControl::KeyDown(const char *bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!fEnabled)
			return;

		SetValue(Value() ? B_CONTROL_OFF : B_CONTROL_ON);
		Invoke();
	} else
		BView::KeyDown(bytes, numBytes);
}


void
BControl::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}


void
BControl::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}


void
BControl::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}


void
BControl::SetLabel(const char *label)
{
	if (label != NULL && !label[0])
		label = NULL;

	// Has the label been changed?
	if ((fLabel && label && !strcmp(fLabel, label))
		|| ((fLabel == NULL || !fLabel[0]) && label == NULL))
		return;

	free(fLabel);
	fLabel = label ? strdup(label) : NULL;

	InvalidateLayout();
	Invalidate();
}


const char *
BControl::Label() const
{
	return fLabel;
}


void
BControl::SetValue(int32 value)
{
	if (value == fValue)
		return;

	fValue = value;
	Invalidate();
}


void
BControl::SetValueNoUpdate(int32 value)
{
	fValue = value;
}


int32
BControl::Value() const
{
	return fValue;
}


void
BControl::SetEnabled(bool enabled)
{
	if (fEnabled == enabled)
		return;

	fEnabled = enabled;

	if (fEnabled && fWantsNav)
		SetFlags(Flags() | B_NAVIGABLE);
	else if (!fEnabled && (Flags() & B_NAVIGABLE)) {
		fWantsNav = true;
		SetFlags(Flags() & ~B_NAVIGABLE);
	} else
		fWantsNav = false;

	if (Window()) {
		Invalidate(Bounds());
		Flush();
	}
}


bool
BControl::IsEnabled() const
{
	return fEnabled;
}


void
BControl::GetPreferredSize(float *_width, float *_height)
{
	BView::GetPreferredSize(_width, _height);
}


void
BControl::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}


status_t
BControl::Invoke(BMessage *message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	if (!message && !notify)
		message = Message();

	BMessage clone(kind);

	if (!message) {
		if (!IsWatched())
			return B_BAD_VALUE;
	} else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value", fValue);
	clone.AddMessenger("be:sender", BMessenger(this));

	// ToDo: is this correct? If message == NULL (even if IsWatched()), we always return B_BAD_VALUE
	status_t err;
	if (message)
		err = BInvoker::Invoke(&clone);
	else
		err = B_BAD_VALUE;

	// TODO: asynchronous messaging
	SendNotices(kind, &clone);

	return err;
}


BHandler *
BControl::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 what, const char *property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return BView::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
BControl::GetSupportedSuites(BMessage *message)
{
	message->AddString("suites", "suite/vnd.Be-control");

	BPropertyInfo propInfo(sPropertyList);
	message->AddFlat("messages", &propInfo);

	return BView::GetSupportedSuites(message);
}


status_t
BControl::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= BControl::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= BControl::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= BControl::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= BControl::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= BControl::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			BControl::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			perform_data_set_layout* data = (perform_data_set_layout*)_data;
			BControl::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			BControl::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			BControl::DoLayout();
			return B_OK;
		}
	}

	return BView::Perform(code, _data);
}


bool
BControl::IsFocusChanging() const
{
	return fFocusChanging;
}


bool
BControl::IsTracking() const
{
	return fTracking;
}


void
BControl::SetTracking(bool state)
{
	fTracking = state;
}


void BControl::_ReservedControl1() {}
void BControl::_ReservedControl2() {}
void BControl::_ReservedControl3() {}
void BControl::_ReservedControl4() {}


BControl &
BControl::operator=(const BControl &)
{
	return *this;
}


void
BControl::InitData(BMessage *data)
{
	fLabel = NULL;
	SetLabel(B_EMPTY_STRING);
	fValue = B_CONTROL_OFF;
	fEnabled = true;
	fFocusChanging = false;
	fTracking = false;
	fWantsNav = Flags() & B_NAVIGABLE;

	if (data && data->HasString("_fname"))
		SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
}

