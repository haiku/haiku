/*
 * Copyright (c) 2004-2006, Haiku, Inc.
 *
 * This software is part of the Haiku distribution and is covered
 * by the MIT license.
 *
 * Author: Jérôme Duval
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Keymap.h"


extern char *__progname;
static const char *sProgramName = __progname;


static void
usage(void)
{
	printf("usage: %s {-o output_file} -[d|l|r|c|b|h input_file]\n"
		"  -o  change output file to output_file (default:keymap.out)\n"
		"  -d  dump key map to standard output\n"
		"  -l  load key map from standard input\n"
		"  -b  load binary key map from file\n"
		"  -r  restore system default key map\n"
		"  -c  compile source keymap to binary\n"
		"  -h  compile source keymap to header\n",
		sProgramName);
}


static const char *
keymap_error(status_t status)
{
	if (status == KEYMAP_ERROR_UNKNOWN_VERSION)
		return "Unknown keymap version";

	return strerror(status);
}


static void
load_keymap_source(Keymap& keymap, const char* name)
{
	entry_ref ref;
	get_ref_for_path(name, &ref);

	status_t status = keymap.LoadSourceFromRef(ref);
	if (status != B_OK) {
		fprintf(stderr, "%s: error when loading the keymap: %s\n",
			sProgramName, keymap_error(status));
		exit(1);
	}
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
				if (keymap.LoadCurrent() != B_OK) {
					fprintf(stderr, "%s: error while getting current keymap!\n", sProgramName);
					return 1;
				}
				keymap.Dump();
				return 0;
			} else if (operation == 'r') {
				Keymap keymap;
				keymap.RestoreSystemDefault();
				printf("System default key map restored.\n");
				return 0;
			} else if (operation == 'l') {
				Keymap keymap;
				status_t status = keymap.LoadSource(stdin);
				if (status != B_OK) {
					fprintf(stderr, "%s: error when loading the keymap: %s\n",
						sProgramName, keymap_error(status));
					return 1;
				}
				keymap.SaveAsCurrent();
				printf("Key map loaded.\n");
				return 0;
			} 
		} else {
			// TODO: the actual action should be issued *AFTER* the command line
			//	has been parsed, not mixed in.
			if (operation == 'o') {
				get_ref_for_path(argv[i], &outputRef);
			} else if (operation == 'c') {
				Keymap keymap;
				load_keymap_source(keymap, argv[i]);

				status_t status = keymap.Save(outputRef);
				if (status < B_OK) {
					fprintf(stderr, "%s: error saving \"%s\": %s\n", sProgramName,
						argv[i], strerror(status));
					return 1;
				}
				return 0;
			} else if (operation == 'h') {
		        Keymap keymap;
				load_keymap_source(keymap, argv[i]);
				keymap.SaveAsHeader(outputRef);
				return 0;
			} else if (operation == 'b') {
				entry_ref ref;
				get_ref_for_path(argv[i], &ref);
				Keymap keymap;
				status_t status = keymap.Load(ref);
				if (status != B_OK) {
					fprintf(stderr, "%s: error when loading the keymap: %s\n",
						sProgramName, keymap_error(status));
					return 1;
				}
				keymap.SaveAsCurrent();
				printf("Key map loaded.\n");
				return 0;
			} else 
				break;
		}
	}

	usage();
	return 1;
}
