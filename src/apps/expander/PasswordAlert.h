/*
 * Copyright 2003-2010 Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 */
#ifndef	_PASSWORDALERT_H
#define	_PASSWORDALERT_H


#include <Bitmap.h>
#include <String.h>
#include <TextControl.h>
#include <Window.h>


class PasswordAlert : public BWindow
{
public:
								PasswordAlert(const char* title, 
									const char* text);
	virtual						~PasswordAlert();
			void				Go(BString &password);
	virtual void				MessageReceived(BMessage* msg);
private:
			BBitmap*			InitIcon();
			BTextControl*		fTextControl;
			sem_id				fAlertSem;
};

#endif	// _PASSWORDALERT_H

