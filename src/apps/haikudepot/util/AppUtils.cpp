/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AppUtils.h"

#include <string.h>

#include <Application.h>
#include <Messenger.h>

#include "HaikuDepotConstants.h"

/*! This method can be called to pop up an error in the user interface;
    typically in a background thread.
 */

/* static */ void
AppUtils::NotifySimpleError(const char* title, const char* text)
{
	BMessage message(MSG_ALERT_SIMPLE_ERROR);

	if (title != NULL && strlen(title) != 0)
		message.AddString(KEY_ALERT_TITLE, title);

	if (text != NULL && strlen(text) != 0)
		message.AddString(KEY_ALERT_TEXT, text);

	be_app->PostMessage(&message);
}