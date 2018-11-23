/*
 * Copyright 2010, Jérôme Duval.
 * Copyright 2004-2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */


#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <File.h>
#include <Mime.h>
#include <TypeConstants.h>

#include "addAttr.h"


#define ERR(msg, args...)	fprintf(stderr, "%s: " msg, kProgramName, args)
#define ERR_0(msg)			fprintf(stderr, "%s: " msg, kProgramName)


static struct option const kLongOptions[] = {
	{"help", no_argument, 0, 'h'},
	{NULL}
};


extern const char *__progname;
static const char *kProgramName = __progname;


// supported types (if you add any, make sure that writeAttr() handles
// them properly)

const struct {
	type_code	type;
	const char	*name;
} kSupportedTypes[] = {
	{B_STRING_TYPE, "string"},
	{B_MIME_STRING_TYPE, "mime"},

	{B_INT32_TYPE, "int32"},
	{B_INT32_TYPE, "int"},
	{B_UINT32_TYPE, "uint32"},
	{B_UINT32_TYPE, "uint"},

	{B_INT64_TYPE, "int64"},
	{B_INT64_TYPE, "llong"},
	{B_UINT64_TYPE, "uint64"},
	{B_UINT64_TYPE, "ullong"},

	{B_FLOAT_TYPE, "float"},
	{B_DOUBLE_TYPE, "double"},

	{B_BOOL_TYPE, "bool"},

	{B_TIME_TYPE, "time"},

	{B_VECTOR_ICON_TYPE, "icon"},
	{B_RAW_TYPE, "raw"},
};
const uint32 kNumSupportedTypes = sizeof(kSupportedTypes)
	/ sizeof(kSupportedTypes[0]);


/*!	For the given string that the user specifies as attribute type
	in the command line, this function tries to figure out the
	corresponding Be API value.

	On success, "result" will contain that value
	On failure, B_BAD_VALUE is returned and "result" is not modified
*/
static status_t
typeForString(const char* string, type_code* _result)
{
	for (uint32 i = 0; i < kNumSupportedTypes; i++) {
		if (!strcmp(string, kSupportedTypes[i].name)) {
			*_result = kSupportedTypes[i].type;
			return B_OK;
		}
	}

	// type didn't show up - in this case, we try to parse
	// the string as number and use it directly as type code

	if (sscanf(string, "%" B_SCNi32, _result) == 1)
		return B_OK;

	// if that didn't work, try the string as a char-type-code
	// enclosed in single quotes
	uchar type[4];
	if (sscanf(string, "'%c%c%c%c'", &type[0], &type[1], &type[2], &type[3]) == 4) {
		*_result = (type[0] << 24) | (type[1] << 16) | (type[2] << 8) | type[3];
		return B_OK;
	}

	return B_BAD_VALUE;
}


void
usage(int returnValue)
{
	fprintf(stderr, "usage: %s [-t type|-c code] [ -P ] attr value file1 [file2...]\n"
		"   or: %s [-f value-from-file] [-t type|-c code] [ -P ] attr file1 [file2...]\n\n"
		"\t-P : Don't resolve links\n"
		"\tThe '-t' and '-c' options are alternatives; use one or the other.\n"
		"\ttype is one of:\n"
		"\t\tstring, mime, int, int32, uint32, llong, int64, uint64,\n"
		"\t\tfloat, double, bool, icon, time, raw\n"
		"\t\tor a numeric value (ie. 0x1234, 42, ...),\n"
		"\t\tor an escape-quoted type code, eg. \\'MICN\\'\n"
		"\tThe default is \"string\"\n"
		"\tcode is a four-char type ID (eg. MICN)\n", kProgramName, kProgramName);

	exit(returnValue);
}


void
invalidAttrType(const char* attrTypeName)
{
	fprintf(stderr, "%s: attribute type \"%s\" is not valid\n", kProgramName,
		attrTypeName);
	fprintf(stderr, "\tTry one of: string, mime, int, llong, float, double,\n"
		"\t\tbool, icon, time, raw, or a numeric value (ie. 0x1234, 42, ...),\n"
		"\t\tor a quoted type code, eg.: \\'MICN\\'\n"
		"\t\tOr enter the actual type code with the '-c' option\n");

	exit(1);
}


void
invalidTypeCode(const char* attrTypeName)
{
	fprintf(stderr, "%s: attribute type code \"%s\" is not valid\n", kProgramName,
		attrTypeName);
	fprintf(stderr, "\tIt must be exactly four characters\n");

	exit(1);
}


void
invalidBoolValue(const char* value)
{
	fprintf(stderr, "%s: attribute value \"%s\" is not valid\n", kProgramName,
		value);
	fprintf(stderr, "\tBool accepts: 0, f, false, disabled, off,\n"
		"\t\t1, t, true, enabled, on\n");

	exit(1);
}


int
main(int argc, char* argv[])
{
	type_code attrType = B_STRING_TYPE;
	char* attrValue = NULL;
	size_t valueFileLength = 0;
	bool resolveLinks = true;

	int c;
	while ((c = getopt_long(argc, argv, "hf:t:c:P", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'f':
			{
				// retrieve attribute value from file
				BFile file;
				off_t size;
				status_t status = file.SetTo(optarg, B_READ_ONLY);
				if (status < B_OK) {
					ERR("can't read attribute value from file %s: %s\n",
						optarg, strerror(status));
					return 1;
				}

				status = file.GetSize(&size);
				if (status == B_OK) {
					if (size == 0) {
						ERR_0("attribute value is empty: 0 bytes\n");
						return 1;
					}
					if (size > 4 * 1024 * 1024) {
						ERR("attribute value is too large: %" B_PRIdOFF
							" bytes\n", size);
						return 1;
					}
					attrValue = (char*)malloc(size);
					if (attrValue != NULL)
						status = file.Read(attrValue, size);
					else
						status = B_NO_MEMORY;
				}

				if (status < B_OK) {
					ERR("can't read attribute value: %s\n", strerror(status));
					return 1;
				}

				valueFileLength = (size_t)size;
				break;
			}
			case 't':
				// Get the attribute type
				if (typeForString(optarg, &attrType) != B_OK)
					invalidAttrType(optarg);
				break;
			case 'c':
				if (strlen(optarg) == 4) {
					// Get the type code directly
					char code[] = "'    '";
					strncpy(code + 1, optarg, 4);
					if (typeForString(code, &attrType) == B_OK)
						break;
				}
				invalidTypeCode(optarg);
			case 'P':
				resolveLinks = false;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind < 1)
		usage(1);
	const char* attrName = argv[optind++];

	if (argc - optind < 1)
		usage(1);
	if (!valueFileLength)
		attrValue = argv[optind++];

	if (argc - optind < 1)
		usage(1);

	// Now that we gathered all the information proceed
	// to add the attribute to the file(s)

	int result = 0;

	for (; optind < argc; optind++) {
		status_t status = addAttr(argv[optind], attrType, attrName, attrValue,
			valueFileLength, resolveLinks);

		// special case for bool types
		if (status == B_BAD_VALUE && attrType == B_BOOL_TYPE)
			invalidBoolValue(attrValue);

		if (status != B_OK) {
			ERR("can't add attribute to file %s: %s\n", argv[optind],
				strerror(status));

			// proceed files, but return an error at the end
			result = 1;
		}
	}

	if (valueFileLength)
		free(attrValue);

	return result;
}

