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

// System Includes -------------------------------------------------------------
#include <Control.h>
#include <PropertyInfo.h>
#include <Window.h>
#include <Errors.h>

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
BControl::BControl(BRect frame, const char *name, const char *label, BMessage *message,
					uint32 resizingMode, uint32 flags)
	:	BView(frame, name, resizingMode, flags),
		BInvoker(message, NULL),
		fValue(B_CONTROL_OFF),
		fEnabled(true),
		fFocusChanging(false),
		fTracking(false),
		fWantsNav(true)
{
	fLabel = strdup(label);
}
//------------------------------------------------------------------------------
BControl::~BControl ()
{
	if ( fLabel )
		delete fLabel;
}
//------------------------------------------------------------------------------
BControl::BControl(BMessage *archive) : BView(archive)
{
	const char *label;

	if (archive->FindInt32("_val", &fValue) != B_OK)
		fValue = B_CONTROL_OFF;

	if (archive->FindString("_label", &label) != B_OK)
		fLabel = NULL;
	else
		SetLabel(label);

	if ( archive->FindBool("_disable", &fEnabled) != B_OK)
		fEnabled = true;
	else
		fEnabled = !fEnabled;

	fFocusChanging = false;
	fTracking = false;
	fWantsNav = true;

	BMessage message;

	if (archive->FindMessage("_msg", &message) == B_OK)
		SetMessage(new BMessage(message));
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
	// TODO: redraw if focus
	BView::WindowActivated(active);
}
//------------------------------------------------------------------------------
void BControl::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if ( Target() == NULL)
		BInvoker::SetTarget(BMessenger(Window(), NULL));

	BView::AttachedToWindow();
}
//------------------------------------------------------------------------------
void BControl::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case B_CONTROL_INVOKED:
			Invoke();
			break;

		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BPropertyInfo propInfo(prop_list);
			BMessage specifier;
			const char *property;

			if (message->GetCurrentSpecifier(NULL, &specifier) != B_OK ||
				specifier.FindString("property", &property) != B_OK)
				return;

			switch (propInfo.FindMatch(message, 0, &specifier, specifier.what, property))
			{
				case B_ERROR:
				{
					BView::MessageReceived(message);
					break;
				}
				case 0:
				{
					BMessage reply;
					reply.AddBool("result", fEnabled);
					reply.AddBool("error", B_OK);
					message->SendReply(&reply);
					break;
				}
				case 1:
				{
					bool enabled;
					message->FindBool("data", &enabled);
					SetEnabled(enabled);
					break;
				}
				case 2:
				{
					BMessage reply;
					reply.AddString("result", fLabel);
					reply.AddBool("error", B_OK);
					message->SendReply(&reply);
					break;
				}
				case 3:
				{
					const char *label;
					message->FindString("data", &label);
					SetLabel(label);
					break;
				}
				case 4:
				{
					BMessage reply;
					reply.AddInt32("result", fValue);
					reply.AddBool("error", B_OK);
					message->SendReply(&reply);
					break;
				}
				case 5:
				{
					int32 value;
					message->FindInt32("data", &value);
					SetValue(value);
					break;
				}
			}

			break;
		}
		default:
		{
			BView::MessageReceived(message);
			break;
		}
	}
}
//------------------------------------------------------------------------------
void BControl::MakeFocus(bool focused)
{
	BView::MakeFocus(focused);

	fFocusChanging = true;
	Draw(Bounds());
	Flush();
	fFocusChanging = false;
}
//------------------------------------------------------------------------------
void BControl::KeyDown(const char *bytes, int32 numBytes)
{
	BMessage* message = Window()->CurrentMessage();

	if (numBytes == 1)
	{
		switch (bytes[0])
		{
			case B_UP_ARROW:
				message->ReplaceInt64("when", (int64)system_time());
				message->ReplaceInt32("key", 38);
				message->ReplaceInt32("raw_char", B_TAB);
				message->ReplaceInt32("modifiers", B_SCROLL_LOCK | B_SHIFT_KEY);
				message->ReplaceInt8("byte", B_TAB);
				message->ReplaceString("bytes", "");
				Looper()->PostMessage(message);
				break;

			case B_DOWN_ARROW:
				message->ReplaceInt64("when", (int64)system_time());
				message->ReplaceInt32("key", 38);
				message->ReplaceInt32("raw_char", B_TAB);
				message->ReplaceInt8("byte", B_TAB);
				message->ReplaceString("bytes", "");
				Looper()->PostMessage(message);
				break;

			case B_ENTER:
			case B_SPACE:
				if (Value())
					SetValue(B_CONTROL_OFF);
				else
					SetValue(B_CONTROL_ON);
				
				BInvoker::Invoke();
				break;

			default:
				BView::KeyDown(bytes, numBytes);
		}
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
	if (fLabel)
		delete fLabel;

	fLabel = strdup(string);

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

	Invalidate();
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

	Invalidate();
}
//------------------------------------------------------------------------------
bool BControl::IsEnabled() const
{
	return fEnabled;
}
//------------------------------------------------------------------------------
void BControl::GetPreferredSize(float *width, float *height)
{
	*width = 1.0f;
	*height = 1.0f;
}
//------------------------------------------------------------------------------
void BControl::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}
//------------------------------------------------------------------------------
status_t BControl::Invoke(BMessage *message)
{
	if (message)
	{
		BMessage copy(*message);
		copy.AddInt64("when", (int64)system_time());
		copy.AddPointer("source", this);
		return BInvoker::Invoke(&copy);
	}
	else if ( Message () )
	{
		BMessage copy (*Message());
		copy.AddInt64 ("when", (int64)system_time());
		copy.AddPointer ("source", this);
		return BInvoker::Invoke(&copy);
	}

	return B_BAD_VALUE;
}
//------------------------------------------------------------------------------
BHandler *BControl::ResolveSpecifier(BMessage *message, int32 index,
									 BMessage *specifier, int32 what,
									 const char *property)
{
	BPropertyInfo propInfo(prop_list);
	BHandler *target = NULL;

	switch (propInfo.FindMatch(message, 0, specifier, what, property))
	{
		case B_ERROR:
			break;

		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			target = this;
			break;
	}

	if (!target)
		target = BView::ResolveSpecifier(message, index, specifier, what,
		property);

	return target;
}
//------------------------------------------------------------------------------
status_t BControl::GetSupportedSuites(BMessage *message)
{
	status_t err;

	if (message == NULL)
		return B_BAD_VALUE;

	err = message->AddString("suites", "suite/vnd.Be-control");

	if (err != B_OK)
		return err;
	
	BPropertyInfo prop_info(prop_list);
	err = message->AddFlat("messages", &prop_info);

	if (err != B_OK)
		return err;
	
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
	return B_ERROR;
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

}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
