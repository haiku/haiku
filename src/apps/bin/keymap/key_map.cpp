// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered
//  by the Haiku license.
//
//
//  File:        key_map.cpp
//  Author:      Jérôme Duval
//  Description: keymap bin
//  Created :    July 30, 2004
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <stdio.h>
#include <string.h>
#include "Keymap.h"


static void usage(char *prog)
{
	printf(
"usage: %s -d  # dump key map to standard output\n"
"       %s -l  # load key map from standard input\n"
"       %s -r  # restore system default key map\n", prog, prog, prog);
}


int main(int argc, char **argv)
{
	char operation = ' ';
	int i;
	for (i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-", 1) == 0) {
			if (strlen(argv[i]) > 1)
				operation = argv[i][1];
			else 
				break;
			if (operation == 'd') {
				Keymap keymap;
				keymap.LoadCurrent();
				keymap.Dump();
				return 0;
			}
		} else {
			if (operation == 'o') {
				//return do_compile(argv[i]);
				return 0;
			} else 
				break;
		}
	}
	usage("/bin/keymap");
	return 1;
}
