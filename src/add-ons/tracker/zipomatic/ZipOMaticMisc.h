#ifndef __ZIPOMATIC_MISC__
#define __ZIPOMATIC_MISC__

#include <stdlib.h>
#include <stdio.h>
#include <String.h>

#include <SupportDefs.h>
#include <Volume.h>
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>

#define ZIPOMATIC_APP_SIG		"application/x-vnd.obos.zip-o-matic"
#define ZIPOMATIC_APP_NAME		"ZipOMatic"

#define ZIPPO_WINDOW_QUIT			'winq'

status_t  	find_and_create_directory	(directory_which a_which, BVolume * a_volume = NULL, const char * a_relative_path = NULL, BPath * a_full_path = NULL);

void 		error_message	(const char * a_text, int32 a_status);

#endif // __ZIPOMATIC_MISC__

