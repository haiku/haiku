/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <package/PackageInfo.h>

#include "package.h"


using namespace BPackageKit;


int
command_info(int argc, const char* const* argv)
{
	const char* format = "name: %name%  version: %version%\n";

	while (true) {
		static struct option sLongOptions[] = {
			{ "format", required_argument, 0, 'f' },
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "f:h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'f':
				format = optarg;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One argument should remain -- the package file name.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* fileName = argv[optind++];

	// Read the package info from the package file. If it doesn't look like a
	// package file, assume it is a package info file.
	BPackageInfo info;
	if (BString(fileName).EndsWith(".hpkg")) {
		status_t error = info.ReadFromPackageFile(fileName);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to read package file \"%s\": %s\n",
				fileName, strerror(error));
			return 1;
		}
	} else {
		status_t error = info.ReadFromConfigFile(BEntry(fileName));
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to read package info file \"%s\": "
				"%s\n", fileName, strerror(error));
			return 1;
		}
	}

	// parse format string and produce output
	BString output;
	while (*format != '\0') {
		char c = *format++;
		switch (c) {
			case '%':
			{
				const char* start = format;
				while (*format != '\0' && *format != '%')
					format++;
				if (*format != '%') {
					fprintf(stderr, "Error: Unexpected at end of the format "
						"string. Expected \"%%\".\n");
					return 1;
				}

				if (format == start) {
					output << '%';
				} else {
					BString variable(start, format - start);
					if (variable == "fileName") {
						output << info.CanonicalFileName();
					} else if (variable == "name") {
						output << info.Name();
					} else if (variable == "version") {
						output << info.Version().ToString();
					} else {
						fprintf(stderr, "Error: Unsupported placeholder \"%s\" "
							"in format string.\n", variable.String());
						return 1;
					}
				}

				format++;
				break;
			}

			case '\\':
				c = *format++;
				if (c == '\0') {
					fprintf(stderr, "Error: \"\\\" at the end of the format "
						"string.\n");
					return 1;
				}
				switch (c) {
					case 'n':
						c = '\n';
						break;
					case 't':
						c = '\t';
						break;
				}
				// fall through

			default:
				output << c;
				break;
		}
	}

	fputs(output.String(), stdout);

	return 0;
}
