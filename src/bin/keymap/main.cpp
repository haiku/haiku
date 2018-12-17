/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <FileIO.h>

#include "Keymap.h"


extern char *__progname;
static const char *sProgramName = __progname;


static void
usage()
{
	printf("usage: %s {-o <output-file>} [-[l|r] | -[b|h|c|d <input-file>]]\n"
		"  -o, --output       Change output file to output-file (default: "
			"keymap.out|h).\n"
		"  -d, --dump         Decompile key map to standard output (can be "
			"redirected\n"
		"                     via -o).\n"
		"  -l, --load         Load key map. If no input-file is specified, it "
			"will be\n"
		"                     read from standard input.\n"
		"  -s, --load-source  Load source key map from standard input when no\n"
		"                     input-file is specified.\n"
		"  -r, --restore      Restore system default key map.\n"
		"  -c, --compile      Compile source keymap to binary.\n"
		"  -h, --header       Translate source keymap to C++ header.\n",
		sProgramName);
}


static const char*
keymap_error(status_t status)
{
	if (status == KEYMAP_ERROR_UNKNOWN_VERSION)
		return "Unknown keymap version";

	return strerror(status);
}


static void
load_keymap(Keymap& keymap, const char* name, bool source)
{
	status_t status;
	if (source) {
		if (name != NULL)
			status = keymap.LoadSource(name);
		else
			status = keymap.LoadSource(stdin);
	} else {
		if (name != NULL)
			status = keymap.SetTo(name);
		else {
			BFileIO fileIO(stdin);
			status = keymap.SetTo(fileIO);
		}
	}

	if (status != B_OK) {
		fprintf(stderr, "%s: error when loading the keymap: %s\n", sProgramName,
			keymap_error(status));
		exit(1);
	}
}


int
main(int argc, char** argv)
{
	const char* output = NULL;
	const char* input = NULL;
	enum {
		kUnspecified,
		kLoadBinary,
		kLoadText,
		kSaveText,
		kRestore,
		kCompile,
		kSaveHeader,
	} mode = kUnspecified;

	static struct option const kLongOptions[] = {
		{"output", required_argument, 0, 'o'},
		{"dump", optional_argument, 0, 'd'},
		{"load", optional_argument, 0, 'l'},
		{"load-source", optional_argument, 0, 's'},
		{"restore", no_argument, 0, 'r'},
		{"compile", optional_argument, 0, 'c'},
		{"header", optional_argument, 0, 'h'},
		{"help", no_argument, 0, 'H'},
		{NULL}
	};

	int c;
	while ((c = getopt_long(argc, argv, "o:dblsrchH", kLongOptions,
			NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'o':
				output = optarg;
				break;
			case 'd':
				mode = kSaveText;
				input = optarg;
				break;
			case 'l':
			case 'b':
				mode = kLoadBinary;
				input = optarg;
				break;
			case 's':
				mode = kLoadText;
				input = optarg;
				break;
			case 'r':
				mode = kRestore;
				break;
			case 'c':
				mode = kCompile;
				input = optarg;
				break;
			case 'h':
				mode = kSaveHeader;
				input = optarg;
				break;

			case 'H':
			default:
				mode = kUnspecified;
				break;
		}
	}

	if (argc > optind && input == NULL)
		input = argv[optind];

	BApplication* app = new BApplication("application/x-vnd.Haiku-keymap-cli");
	Keymap keymap;

	switch (mode) {
		case kUnspecified:
			usage();
			break;

		case kLoadBinary:
		case kLoadText:
		{
			load_keymap(keymap, input, mode == kLoadText);

			status_t status = keymap.SaveAsCurrent();
			if (status != B_OK) {
				fprintf(stderr, "%s: error when saving as current: %s",
					sProgramName, strerror(status));
				return 1;
			}

			printf("Key map loaded.\n");
			break;
		}

		case kSaveText:
		{
			if (input == NULL) {
				status_t status = keymap.SetToCurrent();
				if (status != B_OK) {
					fprintf(stderr, "%s: error while getting keymap: %s!\n",
						sProgramName, keymap_error(status));
					return 1;
				}
			} else
				load_keymap(keymap, input, false);

			if (output != NULL)
				keymap.SaveAsSource(output);
			else
				keymap.SaveAsSource(stdout);
			break;
		}

		case kRestore:
			keymap.RestoreSystemDefault();
			break;

		case kCompile:
		{
			load_keymap(keymap, input, true);

			if (output == NULL)
				output = "keymap.out";

			status_t status = keymap.Save(output);
			if (status != B_OK) {
				fprintf(stderr, "%s: error saving \"%s\": %s\n", sProgramName,
					output, strerror(status));
				return 1;
			}
			break;
		}

		case kSaveHeader:
		{
			load_keymap(keymap, input, true);

			if (output == NULL)
				output = "keymap.h";

			status_t status = keymap.SaveAsCppHeader(output, input);
			if (status != B_OK) {
				fprintf(stderr, "%s: error saving \"%s\": %s\n", sProgramName,
					output, strerror(status));
				return 1;
			}
			break;
		}
	}

	return 0;
}
