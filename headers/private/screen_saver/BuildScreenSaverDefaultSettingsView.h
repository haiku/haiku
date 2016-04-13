/*
 * Copyright 2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef _BUILD_SCREEN_SAVER_DEFAULT_SETTINGS_VIEW_H
#define _BUILD_SCREEN_SAVER_DEFAULT_SETTINGS_VIEW_H


class BView;

namespace BPrivate {

void
BuildScreenSaverDefaultSettingsView(BView* view, const char* moduleName,
	const char* info);

}	// namespace BPrivate


#endif	// _BUILD_SCREEN_SAVER_DEFAULT_SETTINGS_VIEW_H

