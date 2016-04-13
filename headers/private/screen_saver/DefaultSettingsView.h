/*
 * Copyright 2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */
#ifndef _DEFAULT_SETTINGS_VIEW_H
#define _DEFAULT_SETTINGS_VIEW_H


class BView;

namespace BPrivate {

void
BuildDefaultSettingsView(BView* view, const char* moduleName, const char* info);

}	// namespace BPrivate


#endif	// _DEFAULT_SETTINGS_VIEW_H

