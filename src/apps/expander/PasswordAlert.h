/*
 * Copyright 2003-2026 Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */

#ifndef _PASSWORD_ALERT_H
#define _PASSWORD_ALERT_H


#include <Bitmap.h>
#include <Rect.h>
#include <String.h>
#include <Window.h>

class BBitmap;
class BTextControl;


class PasswordAlert : public BWindow {
public:
						PasswordAlert(const char* title,
							const char* text);

	status_t			Go(BString& password);
	void				MessageReceived(BMessage* message);

private:
	static	BRect		_IconSize();

private:
	BBitmap				fIcon;
	BTextControl*		fTextControl;
	sem_id				fSemaphore;
	status_t			fStatus;
};


#endif	// _PASSWORD_ALERT_H
