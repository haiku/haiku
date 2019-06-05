/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <File.h>
#include <Mime.h>
#include <Resources.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <string.h>


static const char *
get_type(type_code type)
{
	static char buffer[32];

	switch (type) {
		case B_MIME_STRING_TYPE:
			return "MIME String";
		case B_RAW_TYPE:
			return "Raw Data";

		case B_STRING_TYPE:
			return "Text";
		case B_INT64_TYPE:
			return "Int-64";
		case B_UINT64_TYPE:
			return "Uint-64";
		case B_INT32_TYPE:
			return "Int-32";
		case B_UINT32_TYPE:
			return "Uint-32";
		case B_INT16_TYPE:
			return "Int-16";
		case B_UINT16_TYPE:
			return "Uint-16";
		case B_INT8_TYPE:
			return "Int-8";
		case B_UINT8_TYPE:
			return "Uint-8";
		case B_BOOL_TYPE:
			return "Boolean";
		case B_FLOAT_TYPE:
			return "Float";
		case B_DOUBLE_TYPE:
			return "Double";

		case B_MINI_ICON_TYPE:
			return "Mini Icon";
		case B_LARGE_ICON_TYPE:
			return "Icon";

		default:
		{
			int32 missed = 0, shift = 24;
			uint8 value[4];
			for (int32 i = 0; i < 4; i++, shift -= 8) {
				value[i] = uint8(type >> shift);
				if (value[i] < ' ' || value[i] > 127) {
					value[i] = '.';
					missed++;
				}
			}

			if (missed < 2)
				sprintf(buffer, "'%c%c%c%c'", value[0], value[1], value[2], value[3]);
			else
				sprintf(buffer, "0x%08" B_PRIx32, type);
			return buffer;
		}
	}
}


int
main(int argc, char *argv[])
{
	const char *program = strrchr(argv[0], '/');
	if (program == NULL)
		program = argv[0];
	else
		program++;

	if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		printf("usage: %s <filename> [<filename> ...]\n", program);
		return 1;
	}

	off_t total = 0;

	for (int i = 1; i < argc; ++i) {
		BFile file(argv[i], B_READ_ONLY);

		status_t status = file.InitCheck();
		if (status < B_OK) {
			fprintf(stderr, "%s: opening file failed for \"%s\": %s\n",
				program, argv[i], strerror(status));
			return 1;
		}

		BResources resources;
		status = resources.SetTo(&file);
		if (status != B_OK) {
			fprintf(stderr, "%s: opening resources failed for \"%s\": %s\n",
				program, argv[i], strerror(status));
			return 1;
		}

		printf("File: %s\n", argv[i]);
		printf("  Type         ID     Size                 Name\n");
		printf("-----------  -----  --------  -------------------------------\n");

		int32 index = 0;
		const char* name;
		type_code type;
		size_t size;
		int32 id;
		while (resources.GetResourceInfo(index++, &type, &id, &name, &size)) {
			printf("%11s %6" B_PRId32 " %9ld  \"%s\"\n",
				get_type(type), id, size, name);

			total += size;
		}
	}

	printf("\n%" B_PRIdOFF " bytes total in resources.\n", total);
	return 0;
}
