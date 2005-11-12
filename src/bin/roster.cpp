/*
 * Copyright 2002-2005 Haiku Inc.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Mathew Hounsell
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include <Roster.h>
#include <Path.h>
#include <Entry.h>
#include <List.h>
#include <String.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>


static struct option const kLongOptions[] = {
	{"name", no_argument, 0, 'n'},
	{"no-trunc", no_argument, 0, 't'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

extern const char *__progname;
static const char *kProgramName = __progname;

static const int32 kNameFieldWidth = 34;

// view modes
static const int32 kStandardMode	= 0x0;
static const int32 kNameOnlyMode	= 0x1;
static const int32 kNoTruncateMode	= 0x2;


void
truncate_string(BString &name, int32 length)
{
	if (name.Length() <= length)
		return;
	if (length < 6)
		length = 6;
	
	int32 beginLength = length / 3 - 1;
	int32 endLength = length - 3 - beginLength;

	BString begin, end;
	name.CopyInto(begin, 0, beginLength); 
	name.CopyInto(end, name.Length() - endLength, endLength);

	name = begin;
	name.Append("...").Append(end);
}


void
output_team(team_id id, int32 mode)
{
	// Get info on team
	app_info info;
	if (be_roster->GetRunningAppInfo(id, &info) != B_OK)
		return;

	// Allocate a entry and get it's path.
	// - works as they are independant (?)
	BEntry entry(&info.ref);
	BPath path(&entry);

	BString name;
	if (mode & kNameOnlyMode)
		name = info.ref.name;
	else
		name = path.Path();

	if ((mode & kNoTruncateMode) == 0)
		truncate_string(name, kNameFieldWidth);

	printf("%6ld %*s %5ld %5lx (%s)\n", 
		id, (int)kNameFieldWidth, name.String(), 
		info.port, info.flags, info.signature);
}


void
usage(int exitCode)
{
	fprintf(stderr, "usage: %s [-nt]\n"
		"  -n, --name\t\tInstead of the full path, only the name of the teams are written\n"
		"  -t, --no-trunc\tDon't truncate the path name\n"
		"  -h, --help\t\tDisplay this help and exit\n",
		kProgramName);

	exit(exitCode);
}


int
main(int argc, char **argv)
{
	int32 mode = kStandardMode;

	// Don't have an BApplication as it is not needed for what we do

	int c;
	while ((c = getopt_long(argc, argv, "nth", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 'n':
				mode |= kNameOnlyMode;
				break;
			case 't':
				mode |= kNoTruncateMode;
				break;
			case 0:
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	// print title line

	printf("  team %*s  port flags signature\n", 
		(int)kNameFieldWidth, mode & kNameOnlyMode ? "name" : "path");

	printf("------ ");
	for (int32 i = 0; i < kNameFieldWidth; i++) 
		putchar('-');
	puts(" ----- ----- ---------");

	// Retrieve the running list.
	BList applicationList;
	be_roster->GetAppList(&applicationList);

	// Iterate through the list.
	for (int32 i = 0; i < applicationList.CountItems(); i++)
		output_team((team_id)applicationList.ItemAt(i), mode);

	return 0;
}
