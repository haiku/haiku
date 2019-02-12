/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 * Coptright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		John Scipione, jscipione@gmail.com
 */


#include "SettingsKeys.h"


const char* kSettingsKeyDownloadPath = "download path";
const char* kSettingsKeyShowTabsIfSinglePageOpen
	= "show tabs if single page open";
const char* kSettingsKeyAutoHideInterfaceInFullscreenMode
	= "auto hide interface in full screen mode";
const char* kSettingsKeyAutoHidePointer = "auto hide pointer";
const char* kSettingsKeyShowHomeButton = "show home button";

const char* kSettingsKeyNewWindowPolicy = "new window policy";
const char* kSettingsKeyNewTabPolicy = "new tab policy";
const char* kSettingsKeyStartUpPolicy = "start up policy";
const char* kSettingsKeyStartPageURL = "start page url";
const char* kSettingsKeySearchPageURL = "search page url";

const char* kDefaultDownloadPath = "/boot/home/Desktop/";
const char* kDefaultStartPageURL
	= "file:///boot/home/config/settings/WebPositive/LoaderPages/Welcome";
const char* kDefaultSearchPageURL = "http://www.google.com/search?q=%s";

const char* kSettingsKeyUseProxy = "use http proxy";
const char* kSettingsKeyProxyAddress = "http proxy address";
const char* kSettingsKeyProxyPort = "http proxy port";
const char* kSettingsKeyUseProxyAuth = "use http proxy authentication";
const char* kSettingsKeyProxyUsername = "http proxy username";
const char* kSettingsKeyProxyPassword = "http proxy password";

const char* kSettingsShowBookmarkBar = "show bookmarks bar";