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
"usage: %s {-o output_file} -[d|l|r|c] \n"
"	-d  # dump key map to standard output\n"
"	-l  # load key map from standard input\n"
"	-r  # restore system default key map\n"
"	-c  # compile source keymap to binary\n"
"	-h  # compile source keymap to header\n"
"	-o  # change output file to output_file (default:keymap.out)\n"
		, prog);
}


int main(int argc, char **argv)
{
	char operation = ' ';
	entry_ref output_ref;
	get_ref_for_path("keymap.out", &output_ref);
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
			} else if (operation == 'r') {
				Keymap keymap;
				keymap.RestoreSystemDefault();
				return 0;
			} else if (operation == 'l') {
				Keymap keymap;
				keymap.LoadSource(stdin);
				keymap.SaveAsCurrent();
				return 0;
			}
			
		} else {
			if (operation == 'o') {
				get_ref_for_path(argv[i], &output_ref);
			} else if (operation == 'c') {
				entry_ref ref;
				get_ref_for_path(argv[i], &ref);
				Keymap keymap;
				keymap.LoadSourceFromRef(ref);
				
				keymap.Save(output_ref);
				return 0;
			} else if (operation == 'h') {
                                entry_ref ref;
                                get_ref_for_path(argv[i], &ref);
                                Keymap keymap;
                                keymap.LoadSourceFromRef(ref);

                                keymap.SaveAsHeader(output_ref);
                                return 0;
                        } else 
				break;
		}
	}
	usage("/bin/keymap");
	return 1;
}
