/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */


#include "addAttr.h"

#include <TypeConstants.h>
#include <Mime.h>

#include <stdio.h>
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

	return B_BAD_VALUE;
}


void
usage(void)
{
	fprintf(stderr, "usage: %s [-t type] attr value file1 [file2...]\n", gProgramName);
	fprintf(stderr, "\tType is one of:\n");
	fprintf(stderr, "\t\tstring, mime, int, llong, float, double, bool,\n");
	fprintf(stderr, "\t\tor a numeric value (ie. 0x1234, 42, ...)\n");
	fprintf(stderr, "\tThe default is `string\'\n");

	exit(1);
}


void
invalidAttrType(const char *attrTypeName)
{
	fprintf(stderr, "%s: attribute type \"%s\" is not valid\n", gProgramName, attrTypeName);
	fprintf(stderr, "\tTry one of: string, mime, int, llong, float, double,\n");
	fprintf(stderr, "\t\tbool, or a numeric value (ie. 0x1234, 42, ...)\n");

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

	if (argc < 3 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))
		usage();

	type_code attrType = B_STRING_TYPE;

	int32 i = 1;

	if (!strcmp(argv[1], "-t")) {
		// Get the attribute type
		if (typeForString(argv[2], &attrType) != B_OK)
			invalidAttrType(argv[2]);

		i += 2;
	}

	const char *attrName = argv[i++];
	const char *attrValue = argv[i++];

	// no files specified
	if (argv[i] == NULL)
		usage();

	// Now that we gathered all the information proceed
	// to add the attribute to the file(s)

	int result = 0;

	for (; i < argc; i++) {
		status_t status = addAttr(argv[i], attrType, attrName, attrValue);

		// special case for bool types
		if (status == B_BAD_VALUE && attrType == B_BOOL_TYPE)
			invalidBoolValue(attrValue);

		if (status != B_OK) {
			fprintf(stderr, "%s: can\'t add attribute to file %s: %s\n",
				gProgramName, argv[i], strerror(status));

			// proceed files, but return an error at the end
			result = 1;
		}
	}

	return result;
}

