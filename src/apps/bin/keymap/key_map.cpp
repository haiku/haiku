/*
 * Copyright (c) 2004-2005, Haiku
 *
 * This software is part of the Haiku distribution and is covered
 * by the MIT license.
 *
 * Author: Jérôme Duval
 */


#include <stdio.h>
#include <string.h>

#include "Keymap.h"


extern char *__progname;
static const char *sProgramName = __progname;


static void
usage(void)
{
	printf("usage: %s {-o output_file} -[d|l|r|c]\n"
		"  -d  dump key map to standard output\n"
		"  -l  load key map from standard input\n"
		"  -r  restore system default key map\n"
		"  -c  compile source keymap to binary\n"
		"  -h  compile source keymap to header\n"
		"  -o  change output file to output_file (default:keymap.out)\n",
		sProgramName);
}


int
main(int argc, char **argv)
{
	char operation = ' ';
	entry_ref outputRef;
	get_ref_for_path("keymap.out", &outputRef);

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-", 1) == 0) {
			if (strlen(argv[i]) > 1)
				operation = argv[i][1];
			else 
				break;

			if (operation == 'd') {
				Keymap keymap;
				if (keymap.LoadCurrent() != B_OK)
					return 1;
				keymap.Dump();
				return 0;
			} else if (operation == 'r') {
				Keymap keymap;
				keymap.RestoreSystemDefault();
				printf("System default key map restored.\n");
				return 0;
			} else if (operation == 'l') {
				Keymap keymap;
				keymap.LoadSource(stdin);
				keymap.SaveAsCurrent();
				printf("Key map loaded.\n");
				return 0;
			}
		} else {
			if (operation == 'o') {
				get_ref_for_path(argv[i], &outputRef);
			} else if (operation == 'c') {
				entry_ref ref;
				get_ref_for_path(argv[i], &ref);
				Keymap keymap;
				keymap.LoadSourceFromRef(ref);

				keymap.Save(outputRef);
				return 0;
			} else if (operation == 'h') {
		        	entry_ref ref;
		        	get_ref_for_path(argv[i], &ref);
		        	Keymap keymap;
		        	keymap.LoadSourceFromRef(ref);

		        	keymap.SaveAsHeader(outputRef);
		        	return 0;
            		} else 
				break;
		}
	}

	usage();
	return 1;
}
