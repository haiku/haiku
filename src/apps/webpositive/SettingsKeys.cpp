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
const char* kDefaultSearchPageURL = "https://duckduckgo.com/?q=%s";

const char* kSettingsKeyUseProxy = "use http proxy";
const char* kSettingsKeyProxyAddress = "http proxy address";
const char* kSettingsKeyProxyPort = "http proxy port";
const char* kSettingsKeyUseProxyAuth = "use http proxy authentication";
const char* kSettingsKeyProxyUsername = "http proxy username";
const char* kSettingsKeyProxyPassword = "http proxy password";

const char* kSettingsShowBookmarkBar = "show bookmarks bar";

const struct SearchEngine kSearchEngines[] = {
	{ "Baidu",      "https://www.baidu.com/search?wd=%s",             "a " },
	{ "Bing",       "https://bing.com/search?q=%s",                   "b " },
	{ "DuckDuckGo", "https://duckduckgo.com/?q=%s",                   "d " },
	{ "Ecosia",     "https://www.ecosia.org/search?q=%s",             "e " },
	{ "Google",     "https://google.com/search?q=%s",                 "g " },
	{ "Qwant",      "https://www.qwant.com/?q=%s",                    "q " },
	{ "Wikipedia",  "https://en.wikipedia.org/w/index.php?search=%s", "w " },
	{ "Yandex",     "https://yandex.com/search/?text=%s",             "y " },
	{ "Yahoo",      "https://search.yahoo.com/search?p=%s",           "z " },
	{ NULL, NULL, NULL }
};
