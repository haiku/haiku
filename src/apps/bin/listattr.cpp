/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Ryan Fleet.
 *
 * Distributed under the terms of the MIT license.
 */


#include <TypeConstants.h>
#include <Mime.h>
#include <Node.h>

#include <fs_attr.h>

#include <string.h>
#include <stdio.h>


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

		case 'MICN':
			return "Mini Icon";
		case 'ICON':
			return "Icon";

		default:
			sprintf(buffer, "'%c%c%c%c'",
				uint8(type >> 24), uint8(type >> 16), uint8(type >> 8), uint8(type));
			return buffer;
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
		printf("usage: %s 'filename' ['filename' ...]\n", program);
		return 1;
	}

	off_t total = 0;

	for (int i = 1; i < argc; ++i) {
		BNode node(argv[i]);

		status_t status = node.InitCheck();
		if (status < B_OK) {
			fprintf(stderr, "%s: initialization failed for \"%s\": %s\n",
				program, argv[i], strerror(status));
			return 0;
		}

		printf("file %s\n", argv[i]);
		printf("  Type         Size                 Name\n");
		printf("-----------  ---------  -------------------------------\n");

		char name[B_ATTR_NAME_LENGTH];
		while (node.GetNextAttrName(name) == B_OK) {
			attr_info attrInfo;

			status = node.GetAttrInfo(name, &attrInfo);
			if (status >= B_OK) {
				printf("%11s ", get_type(attrInfo.type));
				printf("% 10Li  ", attrInfo.size);
				printf("\"%s\"\n", name);
				total += attrInfo.size;
			} else {
				fprintf(stderr, "%s: stat failed for \"%s\": %s\n",
					program, name, strerror(status));
			}
		}
	}

	printf("\n%Ld bytes total in attributes.\n", total);
	return 0;
}
