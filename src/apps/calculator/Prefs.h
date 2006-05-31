/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#ifndef __PREFS__
#define __PREFS__

bool ReadPrefs(const char *prefsname, void *prefs, size_t sz);
bool WritePrefs(const char *prefsname, void *prefs, size_t sz);

#endif