/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas.sundstrom@kirilla.com
 */
#ifndef ZIPOMATIC_MISC_H
#define ZIPOMATIC_MISC_H


#include <Directory.h>
#include <FindDirectory.h>

class BPath;
class BVolume;


#define ZIPOMATIC_APP_SIG		"application/x-vnd.haiku.zip-o-matic"
#define ZIPOMATIC_APP_NAME		"ZipOMatic"

#define ZIPPO_WINDOW_QUIT		'winq'

status_t find_and_create_directory(directory_which which, BVolume* volume = NULL,
	const char* relativePath = NULL, BPath* fullPath = NULL);
void error_message(const char* text, int32 status);

#endif	// ZIPOMATIC_MISC_H
