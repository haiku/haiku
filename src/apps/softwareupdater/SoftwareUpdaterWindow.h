/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _SOFTWARE_UPDATER_WINDOW_H
#define _SOFTWARE_UPDATER_WINDOW_H


#include <Roster.h>
#include <StatusBar.h>
#include <Window.h>

#include "StripeView.h"


class SoftwareUpdaterWindow : public BWindow {
public:
							SoftwareUpdaterWindow();
							~SoftwareUpdaterWindow();
			bool			QuitRequested();

private:
			StripeView*		fStripeView;
			app_info*		fAppInfo;
};


#endif
