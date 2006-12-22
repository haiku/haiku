/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Jack Burton
 */
#ifndef MENU_APP_H
#define MENU_APP_H


#include <Application.h>

class MenuWindow;	


class MenuApp : public BApplication {
	public:
		MenuApp();

		virtual void MessageReceived(BMessage* msg);

	private:
		MenuWindow* fMenuWindow;
};

#endif	// MENU_APP_H
