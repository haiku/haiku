/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonas Sundström, jonas@kirilla.com
 */
#ifndef ZIPOMATIC_MISC_H
#define ZIPOMATIC_MISC_H


#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Volume.h>


#define ZIPOMATIC_APP_SIG		"application/x-vnd.haiku.zip-o-matic"

#define ZIPPO_WINDOW_QUIT		'winq'

status_t	FindAndCreateDirectory(directory_which which,
				BVolume* volume = NULL, const char* relativePath = NULL,
				BPath* fullPath = NULL);
								
void		ErrorMessage(const char* text, int32 status);

#endif	// ZIPOMATIC_MISC_H

