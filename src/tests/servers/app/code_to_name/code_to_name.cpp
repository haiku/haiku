/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Entry.h>
#include <Path.h>
#include <Query.h>
#include <Volume.h>

#include <fs_info.h>

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


extern const char* __progname;


void
skip_white_space(char*& line)
{
	while (isspace(line[0]))
		line++;
}


void
get_name(char* line)
{
	while (isalnum(line[0]) || line[0] == '_') {
		line++;
	}

	line[0] = '\0';
}


void
print_code(BPath& path, int32 code)
{
	FILE* file = fopen(path.Path(), "r");
	if (file == NULL)
		return;

	int32 lineNumber = 0;
	char buffer[4096];

	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		char* line = buffer;
		skip_white_space(line);

		if (strncmp(line, "AS_", 3))
			continue;

		if (++lineNumber != code)
			continue;

		get_name(line);
		printf("code %ld: %s\n", lineNumber, line);
		fclose(file);
		return;
	}

	printf("unknown code %ld!\n", code);
	fclose(file);
}


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <message-code>\n", __progname);
		return -1;
	}

	int32 number = atol(argv[1]);

	BQuery query;
	query.SetPredicate("name=ServerProtocol.h");

	// search on current volume only
	dev_t device = dev_for_path(".");
	BVolume volume(device);

	query.SetVolume(&volume);
	query.Fetch();

	status_t status;
	BEntry entry;
	while ((status = query.GetNextEntry(&entry)) == B_OK) {
		BPath path(&entry);
		puts(path.Path());
		if (strstr(path.Path(), "headers/private/app/ServerProtocol.h") != NULL) {
			print_code(path, number);
			break;
		}
	}

	if (status != B_OK) {
		fprintf(stderr, "%s: could not find ServerProtocol.h", __progname);
		return -1;
	}
	return 0;
}
