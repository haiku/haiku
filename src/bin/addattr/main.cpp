/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */


#include "addAttr.h"

#include <File.h>
#include <Mime.h>
#include <TypeConstants.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// supported types (if you add any, make sure that writeAttr() handles them properly)

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

	{B_RAW_TYPE, "raw"},
};
const uint32 kNumSupportedTypes = sizeof(kSupportedTypes) / sizeof(kSupportedTypes[0]);

char *gProgramName;


/**	For the given string that the user specifies as attribute type
 *	in the command line, this function tries to figure out the
 *	corresponding Be API value.
 *
 *	On success, "result" will contain that value
 *	On failure, B_BAD_VALUE is returned and "result" is not modified
 */

static status_t
typeForString(const char *string, type_code *_result)
{
	for (uint32 i = 0; i < kNumSupportedTypes; i++) {
		if (!strcmp(string, kSupportedTypes[i].name)) {
			*_result = kSupportedTypes[i].type;
			return B_OK;
		}
	}

	// type didn't show up - in this case, we parse the string
	// as number and use it directly as type code

	if (sscanf(string, "%lu", _result) == 1)
		return B_OK;

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
	fprintf(stderr, "usage: %s [-t type] attr value file1 [file2...]\n"
		"   or: %s [-f value-from-file] [-t type] attr file1 [file2...]\n\n"
		"\tType is one of:\n"
		"\t\tstring, mime, int, llong, float, double, bool, raw\n"
		"\t\tor a numeric value (ie. 0x1234, 42, 'ABCD', ...)\n"
		"\tThe default is \"string\"\n", gProgramName, gProgramName);

	exit(returnValue);
}


void
assertArgument(int i, int argc)
{
	if (i >= argc)
		usage(1);
}


void
invalidAttrType(const char *attrTypeName)
{
	fprintf(stderr, "%s: attribute type \"%s\" is not valid\n", gProgramName, attrTypeName);
	fprintf(stderr, "\tTry one of: string, mime, int, llong, float, double,\n");
	fprintf(stderr, "\t\tbool, raw, or a numeric value (ie. 0x1234, 42, 'ABCD', ...)\n");

	exit(1);
}


void
invalidBoolValue(const char *value)
{
	fprintf(stderr, "%s: attribute value \"%s\" is not valid\n", gProgramName, value);
	fprintf(stderr, "\tBool accepts: 0, f, false, disabled, off,\n");
	fprintf(stderr, "\t\t1, t, true, enabled, on\n");

	exit(1);
}


int
main(int argc, char *argv[])
{
	gProgramName = strrchr(argv[0], '/');
	if (gProgramName == NULL)
		gProgramName = argv[0];
	else
		gProgramName++;

	assertArgument(1, argc);
	if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
		usage(0);

	type_code attrType = B_STRING_TYPE;

	char *attrValue = NULL;
	size_t valueFileLength = 0;
	int32 i = 1;

	if (!strcmp(argv[i], "-f")) {
		// retrieve attribute value from file
		BFile file;
		off_t size;
		assertArgument(i, argc);
		status_t status = file.SetTo(argv[i + 1], B_READ_ONLY);
		if (status < B_OK) {
			fprintf(stderr, "%s: can't read attribute value from file %s: %s\n",
				gProgramName, argv[i], strerror(status));

			return 1;
		}

 		status = file.GetSize(&size);
 		if (status == B_OK) {
 			if (size > 4 * 1024 * 1024) {
				fprintf(stderr, "%s: attribute value is too large: %Ld bytes\n",
					gProgramName, size);

				return 1;
 			}
 			attrValue = (char *)malloc(size);
 			if (attrValue != NULL)
 				status = file.Read(attrValue, size);
 			else
 				status = B_NO_MEMORY;
 		}

		if (status < B_OK) {
			fprintf(stderr, "%s: can't read attribute value: %s\n",
				gProgramName, strerror(status));

			return 1;
		}

		valueFileLength = (size_t)size;
		i += 2;
	}

	assertArgument(i, argc);
	if (!strcmp(argv[i], "-t")) {
		// Get the attribute type
		assertArgument(i, argc);
		if (typeForString(argv[i + 1], &attrType) != B_OK)
			invalidAttrType(argv[i + 1]);

		i += 2;
	}

	assertArgument(i, argc);
	const char *attrName = argv[i++];

	assertArgument(i, argc);
	if (!valueFileLength)
		attrValue = argv[i++];

	// no files specified?
	assertArgument(i, argc);

	// Now that we gathered all the information proceed
	// to add the attribute to the file(s)

	int result = 0;

	for (; i < argc; i++) {
		status_t status = addAttr(argv[i], attrType, attrName, attrValue,
			valueFileLength);

		// special case for bool types
		if (status == B_BAD_VALUE && attrType == B_BOOL_TYPE)
			invalidBoolValue(attrValue);

		if (status != B_OK) {
			fprintf(stderr, "%s: can't add attribute to file %s: %s\n",
				gProgramName, argv[i], strerror(status));

			// proceed files, but return an error at the end
			result = 1;
		}
	}

	if (valueFileLength)
		free(attrValue);

	return result;
}

