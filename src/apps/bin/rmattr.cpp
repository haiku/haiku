// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        rmattr.cpp
//  Author:      Jérôme Duval
//  Description: remove an attribute from a file
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <kernel/fs_attr.h>

int main(int32 argc, const char **argv)
{
	// Make sure we have the minimum number of arguments.
	if (argc < 3) {
		printf("usage: %s attr filename1 [filename2...]\n", argv[0]);
    	printf("        attr is the name of an attribute of the file\n");
		exit(0);	
	}
	
	for(int32 i=2; i<argc; i++) {
		int fd = open(argv[i], O_WRONLY);
		if (fd < 0) {
			fprintf( stderr, "%s: can\'t open file %s to remove attribute\n", 
				argv[0], argv[i]);
			exit(0);
		}
				
		if(fs_remove_attr(fd, argv[1])!=B_OK) {
			fprintf( stderr, "%s: error removing attribute %s from %s : No such attribute\n", 
				argv[0], argv[1], argv[i] );
		}
	}

	exit(0);	
}
