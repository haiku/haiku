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
//	File Name:		Control.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BControl is the base class for user-event handling objects.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <Control.h>
#include <PropertyInfo.h>
#include <Window.h>
#include <Errors.h>
#include <Debug.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
static property_info prop_list[] =
{
	{
		"Enabled",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns whether or not the BControl is currently enabled.", 0,
		{ B_BOOL_TYPE, 0 }
	},
	{
		"Enabled",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Enables or disables the BControl.", 0,
		{ B_BOOL_TYPE, 0 }
	},
	{
		"Label",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the BControl's label.", 0,
		{ B_STRING_TYPE, 0 }
	},
	{
		"Label",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Sets the label of the BControl.", 0,
		{ B_STRING_TYPE, 0 }
	},
	{
		"Value",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the BControl's value.", 0,
		{ B_INT32_TYPE, 0 }
	},
	{
		"Value",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Sets the value of the BControl.", 0,
		{ B_INT32_TYPE, 0 },
	},
	{ 0 }
};

//------------------------------------------------------------------------------
BControl::BControl(BRect frame, const char *name, const char *label,
				   BMessage *message, uint32 resizingMode, uint32 flags)
	:	BView(frame, name, resizingMode, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}
//------------------------------------------------------------------------------
BControl::~BControl()
{
	if (fLabel)
		free(fLabel);

	SetMessage(NULL);
}
//------------------------------------------------------------------------------
BControl::BControl(BMessage *archive)
	:	BView(archive)
{
	InitData(archive);

	BMessage message;

	if (archive->FindMessage("_msg", &message) == B_OK)
		SetMessage(new BMessage(message));

	const char *label;

	if (archive->FindString("_label", &label) != B_OK)
		SetLabel(label);

	int32 value;

	if (archive->FindInt32("_val", &value) != B_OK)
		SetValue(value);

	bool toggle;

	if (archive->FindBool("_disable", &toggle) != B_OK)
		SetEnabled(!toggle);

	if (archive->FindBool("be:wants_nav", &toggle) != B_OK)
		fWantsNav = toggle;
}
//------------------------------------------------------------------------------
BArchivable *BControl::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BControl"))
		return new BControl(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BControl::Archive(BMessage *archive, bool deep) const
{
	status_t err = BView::Archive(archive, deep);

	if (err != B_OK)
		return err;

	if (Message())
		err = archive->AddMessage("_msg", Message ());

	if (err != B_OK)
		return err;

	if (fLabel)
		err = archive->AddString("_label", fLabel);

	if (err != B_OK )
		return err;

	if (fValue != B_CONTROL_OFF)
		err = archive->AddInt32("_val", fValue);

	if (err != B_OK)
		return err;
	
	if (!fEnabled)
		err = archive->AddBool("_disable", true);

	return err;
}
//------------------------------------------------------------------------------
void BControl::WindowActivated(bool active)
{
	BView::WindowActivated(active);

	if (IsFocus())
		Invalidate(Bounds());
}
//------------------------------------------------------------------------------
void BControl::AttachedToWindow()
{
	if (Parent())
	{
		rgb_color color = Parent()->ViewColor();

		SetViewColor(color);
		SetLowColor(color);
	}

	if (!Messenger().IsValid())
		BInvoker::SetTarget(BMessenger(Window(), NULL));

	BView::AttachedToWindow();
}
//------------------------------------------------------------------------------
void BControl::MessageReceived(BMessage *message)
{
	bool handled = false;
	BMessage reply(B_REPLY);

	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY)
	{
		BPropertyInfo propInfo(prop_list);
		BMessage specifier;
		int32 index;
		int32 form;
		const char *property;

		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK)
		{
			if (strcmp(property, "Label") == 0)
			{
				if (message->what == B_GET_PROPERTY)
				{
					reply.AddString("result", fLabel);
					handled = true;
				}
				else
				{
					const char *label;
					
					if (message->FindString("data", &label) == B_OK)
					{
						SetLabel(label);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
			else if (strcmp(property, "Value") == 0)
			{
				if (message->what == B_GET_PROPERTY)
				{
					reply.AddInt32("result", fValue);
					handled = true;
				}
				else
				{
					int32 value;
					
					if (message->FindInt32("data", &value) == B_OK)
					{
						SetValue(value);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
			else if (strcmp(property, "Enabled") == 0)
			{
				if (message->what == B_GET_PROPERTY)
				{
					reply.AddBool("result", fEnabled);
					handled = true;
				}
				else
				{
					bool enabled;
					
					if (message->FindBool("data", &enabled) == B_OK)
					{
						SetEnabled(enabled);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			}
		}
	}

	if (handled)
		message->SendReply(&reply);
	else
		BView::MessageReceived(message);
}
//------------------------------------------------------------------------------
void BControl::MakeFocus(bool focused)
{
	if (focused == IsFocus())
		return;

	BView::MakeFocus(focused);

 	if(Window())
	{
		fFocusChanging = true;
		Invalidate(Bounds());
		Flush();
		fFocusChanging = false;
	}
}
//------------------------------------------------------------------------------
void BControl::KeyDown(const char *bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE)
	{
		if (!fEnabled)
			return;

		if (Value())
			SetValue(B_CONTROL_OFF);
		else
			SetValue(B_CONTROL_ON);
		
		Invoke();
	}
	else
		BView::KeyDown(bytes, numBytes);
}
//------------------------------------------------------------------------------
void BControl::MouseDown(BPoint point)
{
	BView::MouseDown(point);
}
//------------------------------------------------------------------------------
void BControl::MouseUp(BPoint point)
{
	BView::MouseUp(point);
}
//------------------------------------------------------------------------------
void BControl::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
	BView::MouseMoved(point, transit, message);
}
//------------------------------------------------------------------------------
void BControl::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}
//------------------------------------------------------------------------------
void BControl::SetLabel(const char *string)
{
	if (fLabel && string && strcmp(fLabel, string) == 0)
		return;

	if (fLabel)
		free(fLabel);

	if (string)
		fLabel = strdup(string);
	else
		fLabel = strdup(B_EMPTY_STRING);

	Invalidate();
}
//------------------------------------------------------------------------------
const char *BControl::Label() const
{
	return fLabel;
}
//------------------------------------------------------------------------------
void BControl::SetValue(int32 value)
{
	if (fValue == value)
		return;

	fValue = value;

 	if (Window())
	{
		Invalidate(Bounds());
		Flush();
	}
}
//------------------------------------------------------------------------------
int32 BControl::Value() const
{
	return fValue;
}
//------------------------------------------------------------------------------
void BControl::SetEnabled(bool enabled)
{
	if (fEnabled == enabled)
		return;

	fEnabled = enabled;

	if (fEnabled)
		BView::SetFlags(Flags() | B_NAVIGABLE);
	else
		BView::SetFlags(Flags() & ~B_NAVIGABLE);

	if (Window())
	{
		Invalidate(Bounds());
		Flush();
	}
}
//------------------------------------------------------------------------------
bool BControl::IsEnabled() const
{
	return fEnabled;
}
//------------------------------------------------------------------------------
void BControl::GetPreferredSize(float *width, float *height)
{
	BView::GetPreferredSize(width, height);
}
//------------------------------------------------------------------------------
void BControl::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}
//------------------------------------------------------------------------------
status_t BControl::Invoke(BMessage *message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	BMessage clone(kind);
	status_t err = B_BAD_VALUE;

	if (!message && !notify)
		message = Message();
		
	if (!message)
	{
		if (!IsWatched())
			return err;
	}
	else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value", fValue);
	clone.AddMessenger("be:sender", BMessenger(this));

	if (message)
		err = BInvoker::Invoke(&clone);

	// TODO: asynchronous messaging
	SendNotices(kind, &clone);

	return err;
}
//------------------------------------------------------------------------------
BHandler *BControl::ResolveSpecifier(BMessage *message, int32 index,
									 BMessage *specifier, int32 what,
									 const char *property)
{
	BPropertyInfo propInfo(prop_list);

	if (propInfo.FindMatch(message, 0, specifier, what, property) < B_OK)
		return BView::ResolveSpecifier(message, index, specifier, what,
			property);
	else
		return this;
}
//------------------------------------------------------------------------------
status_t BControl::GetSupportedSuites(BMessage *message)
{
	message->AddString("suites", "suite/vnd.Be-control");
	
	BPropertyInfo prop_info(prop_list);
	message->AddFlat("messages", &prop_info);
	
	return BView::GetSupportedSuites(message);
}
//------------------------------------------------------------------------------
void BControl::AllAttached()
{
	BView::AllAttached();
}
//------------------------------------------------------------------------------
void BControl::AllDetached()
{
	BView::AllDetached();
}
//------------------------------------------------------------------------------
status_t BControl::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}
//------------------------------------------------------------------------------
bool BControl::IsFocusChanging() const
{
	return fFocusChanging;
}
//------------------------------------------------------------------------------
bool BControl::IsTracking() const
{
	return fTracking;
}
//------------------------------------------------------------------------------
void BControl::SetTracking(bool state)
{
	fTracking = state;
}
//------------------------------------------------------------------------------
void BControl::SetValueNoUpdate(int32 value)
{
	fValue = value;
}

//------------------------------------------------------------------------------
void BControl::_ReservedControl1() {}
void BControl::_ReservedControl2() {}
void BControl::_ReservedControl3() {}
void BControl::_ReservedControl4() {}
//------------------------------------------------------------------------------
BControl &BControl::operator=(const BControl &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BControl::InitData(BMessage *data)
{
	fLabel = NULL;
	SetLabel(B_EMPTY_STRING);
	fValue = B_CONTROL_OFF;
	fEnabled = true;
	fFocusChanging = false;
	fTracking = false;
	fWantsNav = true;

	if (data && data->HasString("_fname"))
		SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
}
//------------------------------------------------------------------------------
