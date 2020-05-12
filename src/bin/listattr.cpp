/*
 * Copyright 2004-2020, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Ryan Fleet.
 *
 * Distributed under the terms of the MIT license.
 */


#include <String.h>
#include <TypeConstants.h>
#include <Mime.h>

#include <fs_attr.h>

#include <ctype.h>
#include <string.h>
#include <stdio.h>


/*!	Dumps the contents of the attribute in the form of raw data. This view
	is used for the type B_RAW_TYPE, for custom types and for any type that
	is not directly supported by the utility "addattr".
*/
static void
dump_raw_data(const char *buffer, size_t size)
{
	const uint32 kChunkSize = 16;
	uint32 dumpPosition = 0;

	while (dumpPosition < size) {
		// Position for this line
		printf("\t%04" B_PRIx32 ": ", dumpPosition);

		// Print the bytes in form of hexadecimal numbers
		for (uint32 i = 0; i < kChunkSize; i++) {
			if (dumpPosition + i < size) {
				printf("%02x ", (uint8)buffer[dumpPosition + i]);
			} else
				printf("   ");
		}

		// Print the bytes in form of printable characters
		// (whenever possible)
		printf(" ");
		for (uint32 i = 0; i < kChunkSize; i++) {
			if (dumpPosition < size) {
				char c = buffer[dumpPosition];
				putchar(isgraph(c) ? c : '.');
			} else
				putchar(' ');

			dumpPosition++;
		}
		printf("\n");
	}
}


static void
show_attr_contents(BNode& node, const char* attribute, const attr_info& info)
{
	// limit size of the attribute, only the first kLimit byte will make it on
	// screen
	int kLimit = 256;
	bool cut = false;
	off_t size = info.size;
	if (size > kLimit) {
		size = kLimit;
		cut = true;
	}

	char buffer[kLimit];
	ssize_t bytesRead = node.ReadAttr(attribute, info.type, 0, buffer, size);
	if (bytesRead != size) {
		fprintf(stderr, "Could only read %" B_PRIdOFF " bytes from attribute!\n",
			size);
		return;
	}
	buffer[min_c(bytesRead, kLimit - 1)] = '\0';

	switch (info.type) {
		case B_INT8_TYPE:
			printf("%" B_PRId8 "\n", *((int8 *)buffer));
			break;
		case B_UINT8_TYPE:
			printf("%" B_PRIu8 "\n", *((uint8 *)buffer));
			break;
		case B_INT16_TYPE:
			printf("%" B_PRId16 "\n", *((int16 *)buffer));
			break;
		case B_UINT16_TYPE:
			printf("%" B_PRIu16 "\n", *((uint16 *)buffer));
			break;
		case B_INT32_TYPE:
			printf("%" B_PRId32 "\n", *((int32 *)buffer));
			break;
		case B_UINT32_TYPE:
			printf("%" B_PRIu32 "\n", *((uint32 *)buffer));
			break;
		case B_INT64_TYPE:
			printf("%" B_PRId64 "\n", *((int64 *)buffer));
			break;
		case B_UINT64_TYPE:
			printf("%" B_PRIu64 "\n", *((uint64 *)buffer));
			break;
		case B_FLOAT_TYPE:
			printf("%f\n", *((float *)buffer));
			break;
		case B_DOUBLE_TYPE:
			printf("%f\n", *((double *)buffer));
			break;
		case B_BOOL_TYPE:
			printf("%d\n", *((unsigned char *)buffer));
			break;
		case B_TIME_TYPE:
		{
			char stringBuffer[256];
			struct tm timeInfo;
			localtime_r((time_t *)buffer, &timeInfo);
			strftime(stringBuffer, sizeof(stringBuffer), "%c", &timeInfo);
			printf("%s\n", stringBuffer);
			break;
		}
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
		case 'MSIG':
		case 'MSDC':
		case 'MPTH':
			printf("%s\n", buffer);
			break;

		case B_MESSAGE_TYPE:
		{
			BMessage message;
			if (!cut && message.Unflatten(buffer) == B_OK) {
				putchar('\n');
				message.PrintToStream();
				putchar('\n');
				break;
			}
			// supposed to fall through
		}

		default:
			// The rest of the attributes types are displayed as raw data
			putchar('\n');
			dump_raw_data(buffer, size);
			putchar('\n');
			break;
	}
}


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

			if (missed < 2) {
				sprintf(buffer, "'%c%c%c%c'", value[0], value[1], value[2],
					value[3]);
			} else
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

	bool printContents = false;

	if (argc > 2 && (!strcmp(argv[1], "--long") || !strcmp(argv[1], "-l"))) {
		printContents = true;
		argc--;
		argv++;
	}

	if (argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
		printf("usage: %s [-l|--long] 'filename' ['filename' ...]\n"
			"  -l, --long  Shows the attribute contents as well.\n", program);
		return argc == 2 ? 0 : 1;
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

		printf("File: %s\n", argv[i]);

		const int kTypeWidth = 12;
		const int kSizeWidth = 10;
		const int kNameWidth = 36;
		const int kContentsWidth = 21;
		printf("%*s %*s  %-*s%s\n", kTypeWidth, "Type", kSizeWidth, "Size",
			kNameWidth, "Name", printContents ? "Contents" : "");

		BString separator;
		separator.SetTo('-', kTypeWidth + kSizeWidth + kNameWidth
			+ (printContents ? kContentsWidth : 0));
		puts(separator.String());

		char name[B_ATTR_NAME_LENGTH];
		while (node.GetNextAttrName(name) == B_OK) {
			attr_info attrInfo;

			status = node.GetAttrInfo(name, &attrInfo);
			if (status >= B_OK) {
				printf("%*s", kTypeWidth, get_type(attrInfo.type));
				printf("% *" B_PRId64 "  ", kSizeWidth, attrInfo.size);
				printf("\"%s\"", name);

				if (printContents) {
					// padding
					int length = kNameWidth - 2 - strlen(name);
					if (length > 0)
						printf("%*s", length, "");

					show_attr_contents(node, name, attrInfo);
				} else
					putchar('\n');

				total += attrInfo.size;
			} else {
				fprintf(stderr, "%s: stat failed for \"%s\": %s\n",
					program, name, strerror(status));
			}
		}
	}

	printf("\n%" B_PRId64 " bytes total in attributes.\n", total);
	return 0;
}
