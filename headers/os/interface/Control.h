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
//	File Name:		Control.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BControl is the base class for user-event handling objects.
//------------------------------------------------------------------------------

#ifndef _CONTROL_H
#define _CONTROL_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Invoker.h>
#include <Message.h>	/* For convenience */
#include <View.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
enum {
	B_CONTROL_OFF = 0,
	B_CONTROL_ON = 1
};

// Globals ---------------------------------------------------------------------


class BWindow;

// BControl class --------------------------------------------------------------
class BControl : public BView, public BInvoker {

public:
					BControl(BRect frame,
						const char *name,
						const char *label,
						BMessage *message,
						uint32 resizingMode,
						uint32 flags);
virtual				~BControl();

					BControl(BMessage *archive);
static BArchivable	*Instantiate(BMessage *archive);
virtual status_t	Archive(BMessage *archive, bool deep = true) const;

virtual void		WindowActivated(bool active);
virtual void		AttachedToWindow();
virtual void		MessageReceived(BMessage *message);
virtual void		MakeFocus(bool focused = true);
virtual void		KeyDown(const char *bytes, int32 numBytes);
virtual	void		MouseDown(BPoint point);
virtual	void		MouseUp(BPoint point);
virtual	void		MouseMoved(BPoint point, uint32 transit, const BMessage *message);
virtual	void		DetachedFromWindow();

virtual void		SetLabel(const char *string);
		const char	*Label() const;

virtual void		SetValue(int32 value);
		int32		Value() const;

virtual void		SetEnabled(bool enabled);
		bool		IsEnabled() const;

virtual	void		GetPreferredSize(float *width, float *height);
virtual void		ResizeToPreferred();

virtual status_t	Invoke(BMessage *message = NULL);
virtual BHandler	*ResolveSpecifier(BMessage *message,
									int32 index,
									BMessage *specifier,
									int32 what,
									const char *property);
virtual status_t	GetSupportedSuites(BMessage *message);

virtual void		AllAttached();
virtual void		AllDetached();

virtual status_t	Perform(perform_code d, void *arg);

protected:

		bool		IsFocusChanging() const;
		bool		IsTracking() const;
		void		SetTracking(bool state);

		void		SetValueNoUpdate(int32 value);

private:

virtual	void		_ReservedControl1();
virtual	void		_ReservedControl2();
virtual	void		_ReservedControl3();
virtual	void		_ReservedControl4();

		BControl	&operator=(const BControl &);

		void		InitData(BMessage *data = NULL);

		char		*fLabel;
		int32		fValue;
		bool		fEnabled;
		bool		fFocusChanging;
		bool		fTracking;
		bool		fWantsNav;
		uint32		_reserved[4];
};
//------------------------------------------------------------------------------

#endif // _CONTROL_H

/*
 * $Log $
 *
 * $Id  $
 *
 */
