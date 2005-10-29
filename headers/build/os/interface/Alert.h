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
//	File Name:		Alert.h
//	Author:			Erik Jaesler (erik@cgsoftware.com)
//	Description:	BAlert displays a modal alert window.
//------------------------------------------------------------------------------
 
#ifndef	_ALERT_H
#define	_ALERT_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Window.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BBitmap;
class BButton;
class BInvoker;
class BTextView;

// enum for flavors of alert ---------------------------------------------------
enum alert_type {
	B_EMPTY_ALERT = 0,
	B_INFO_ALERT,
	B_IDEA_ALERT,
	B_WARNING_ALERT,
	B_STOP_ALERT
};


enum button_spacing {
	B_EVEN_SPACING = 0,
	B_OFFSET_SPACING
};

// BAlert class ----------------------------------------------------------------
class BAlert : public BWindow
{
public:

					BAlert(	const char *title,
							const char *text,
							const char *button1,
							const char *button2 = NULL,
							const char *button3 = NULL,
							button_width width = B_WIDTH_AS_USUAL,
							alert_type type = B_INFO_ALERT);
					BAlert(	const char *title,
							const char *text,
							const char *button1,
							const char *button2,
							const char *button3,
							button_width width,
							button_spacing spacing,
							alert_type type = B_INFO_ALERT);
virtual				~BAlert();
	
// Archiving
					BAlert(BMessage *data);
static	BArchivable	*Instantiate(BMessage *data);
virtual	status_t	Archive(BMessage *data, bool deep = true) const;
	
// BAlert guts
		void		SetShortcut(int32 button_index, char key);
		char		Shortcut(int32 button_index) const;

		int32		Go();
		status_t	Go(BInvoker *invoker);

virtual	void		MessageReceived(BMessage *an_event);
virtual	void		FrameResized(float new_width, float new_height);
		BButton		*ButtonAt(int32 index) const;
		BTextView	*TextView() const;

virtual BHandler	*ResolveSpecifier(BMessage *msg,
									int32 index,
									BMessage *specifier,
									int32 form,
									const char *property);
virtual	status_t	GetSupportedSuites(BMessage *data);

virtual void		DispatchMessage(BMessage *msg, BHandler *handler);
virtual	void		Quit();
virtual	bool		QuitRequested();

static	BPoint		AlertPosition(float width, float height);

// Private or reserved ---------------------------------------------------------
virtual status_t	Perform(perform_code d, void *arg);

private:
friend class _BAlertFilter_;

virtual	void		_ReservedAlert1();
virtual	void		_ReservedAlert2();
virtual	void		_ReservedAlert3();

		void		InitObject(const char *text,
							const char *button1,
							const char *button2 = NULL,
							const char *button3 = NULL,
							button_width width = B_WIDTH_AS_USUAL,
							button_spacing spacing = B_EVEN_SPACING,
							alert_type type = B_INFO_ALERT);
		BBitmap		*InitIcon();

		sem_id			fAlertSem;
		int32			fAlertVal;
		BButton			*fButtons[3];
		BTextView		*fTextView;
		char			fKeys[3];
		alert_type		fMsgType;
		button_width	fButtonWidth;
		BInvoker		*fInvoker;
		uint32			_reserved[4];
};
//------------------------------------------------------------------------------

#endif	// _ALERT_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

