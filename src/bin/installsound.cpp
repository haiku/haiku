// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//
//
//  File:        installsound.cpp
//  Author:      Jérôme Duval
//  Description: manages sound events
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <stdlib.h>
#include <Application.h>
#include <Entry.h>
#include <MediaFiles.h>
#include <Path.h>
#include <String.h>

void
usage(void)
{
	printf("installsound eventname filename\n");
	printf("        installs a new named sound event in the Sounds preferences panel.\n");
	printf("installsound --list\n");
	printf("        lists all sound events.\n");
	printf("installsound --test eventname\n");
	printf("        prints the file for the given event name, or nothing and returns error if none.\n");
	printf("installsound --clear eventname\n");
	printf("        clears a named event in the Sounds preferences panel.\n");
	printf("installsound --remove eventname\n");
	printf("        removes a named event from the Sounds preferences panel.\n");
	exit(1);
}

int main(int argc, const char **argv)
{
	// Make sure we have the minimum number of arguments.
	if (argc < 2) usage();
	
	BApplication app("application/x-vnd.be.installsound");
	
	BMediaFiles mfiles;
	
	if(strcmp(argv[1], "--list")==0) {
		mfiles.RewindRefs(BMediaFiles::B_SOUNDS);
	
		BString name;
		entry_ref ref;
		while(mfiles.GetNextRef(&name,&ref) == B_OK) {
			printf("%s:\t%s\n", name.String(),BPath(&ref).Path());
		}
	} else {
		if (argc != 3) usage();
		
		if(strcmp(argv[1], "--test")==0) {
			entry_ref ref;
			if(mfiles.GetRefFor(BMediaFiles::B_SOUNDS, argv[2], &ref) == B_OK)
				printf("%s\n", BPath(&ref).Path());
			else {
				printf("%s: No such sound event\n", argv[2]);
				exit(1);
			}
		} else if(strcmp(argv[1], "--clear")==0) {
			entry_ref ref;
			if(mfiles.GetRefFor(BMediaFiles::B_SOUNDS, argv[2], &ref) == B_OK)
				mfiles.RemoveRefFor(BMediaFiles::B_SOUNDS, argv[2], ref);
			else {
				printf("clear %s: No such sound event\n", argv[2]);
				exit(1);
			}
		} else if(strcmp(argv[1], "--remove")==0) {
			if(mfiles.RemoveItem(BMediaFiles::B_SOUNDS, argv[2]) != B_OK) {
				printf("remove %s: No such sound event\n", argv[2]);
				exit(1);
			}
		} else {
			entry_ref ref;
			if(get_ref_for_path(argv[2], &ref)!=B_OK || !BEntry(&ref, true).Exists()) {
				printf("error: %s not found\n", argv[2]);
				exit(1);
			}
			
			mfiles.SetRefFor(BMediaFiles::B_SOUNDS, argv[1], ref);
		}	
	}
	
	exit(0);	
}
