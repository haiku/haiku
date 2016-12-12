/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef _SOFTWARE_UPDATER_WINDOW_H
#define _SOFTWARE_UPDATER_WINDOW_H


#include <Button.h>
#include <Roster.h>
#include <StringView.h>
#include <Window.h>

#include "StripeView.h"
#include "UpdateManager.h"


class SoftwareUpdaterWindow : public BWindow {
public:
							SoftwareUpdaterWindow();
							~SoftwareUpdaterWindow();
			bool			QuitRequested();

			void			MessageReceived(BMessage* message);

private:
			void			_Error(const char* error);
			void			_Refresh();

			UpdateManager*	fUpdateManager;
			StripeView*		fStripeView;
			app_info*		fAppInfo;

			BStringView*	fHeaderView;
			BStringView*	fDetailView;
			BButton*		fUpdateButton;
			BButton*		fCancelButton;
};


#endif
