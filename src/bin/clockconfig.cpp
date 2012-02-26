/*
 * Copyright 2004, Jérôme Duval, jerome.duval@free.fr.
 * Copyright 2010, 2012, Oliver Tappe <zooey@hirschkaefer.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <OS.h>
#include <Path.h>
#include <String.h>
#include <TimeZone.h>

#include <syscalls.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static char* program;


void
setRealTimeClockIsGMT(BPath path)
{
	path.Append("RTC_time_settings");
	BFile file;
	status_t status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK) {
		fprintf(stderr, "%s: can't open RTC settings file\n", program);
		return;
	}

	char buffer[10];
	ssize_t bytesRead = file.Read(buffer, sizeof(buffer));
	if (bytesRead < 0) {
		fprintf(stderr, "%s: unable to read RTC settings file\n", program);
		return;
	}
	bool isGMT = strncmp(buffer, "local", 5) != 0;

	_kern_set_real_time_clock_is_gmt(isGMT);
	printf("RTC stores %s time.\n", isGMT ? "GMT" : "local" );
}


void
setTimeZoneOffset(BPath path)
{
	path.Append("Time settings");
	BFile file;
	status_t status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK) {
		fprintf(stderr, "%s: can't open Time settings file\n", program);
		return;
	}

	BMessage settings;
	status = settings.Unflatten(&file);
	if (status != B_OK) {
		fprintf(stderr, "%s: unable to parse Time settings file\n", program);
		return;
	}
	BString timeZoneID;
	if (settings.FindString("timezone", &timeZoneID) != B_OK) {
		fprintf(stderr, "%s: no timezone found\n", program);
		return;
	}
	int32 timeZoneOffset = BTimeZone(timeZoneID.String()).OffsetFromGMT();

	_kern_set_timezone(timeZoneOffset, timeZoneID.String(),
		timeZoneID.Length());
	printf("timezone is %s, offset is %ld seconds from GMT.\n",
		timeZoneID.String(), timeZoneOffset);
}


int
main(int argc, char **argv)
{
	program = argv[0];

	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK) {
		fprintf(stderr, "%s: can't find settings directory\n", program);
		return EXIT_FAILURE;
	}

	setRealTimeClockIsGMT(path);
	setTimeZoneOffset(path);

	return 0;
}
