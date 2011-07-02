/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageAttributeValue.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageReader.h>

#include "package.h"
#include "StandardErrorOutput.h"


using namespace BPackageKit::BHPKG;


struct PackageContentDumpHandler : BLowLevelPackageContentHandler {
	PackageContentDumpHandler()
		:
		fLevel(0),
		fErrorOccurred(false),
		fHasChildren(false)
	{
	}

	virtual status_t HandleAttribute(BHPKGAttributeID attributeID,
		const BPackageAttributeValue& value, void* parentToken, void*& _token)
	{
		if (fErrorOccurred)
			return B_OK;

		printf("%*s>%s: ", fLevel * 2, "", AttributeNameForID(attributeID));
		_PrintValue(value);
		printf("\n");

		fHasChildren = false;
		fLevel++;
		return B_OK;
	}

	virtual status_t HandleAttributeDone(BHPKGAttributeID attributeID,
		const BPackageAttributeValue& value, void* parentToken, void* token)
	{
		if (fErrorOccurred)
			return B_OK;

		fLevel--;

		if (fHasChildren)
			printf("%*s<%s\n", fLevel * 2, "", AttributeNameForID(attributeID));

		fHasChildren = true;
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
		fErrorOccurred = true;
	}

private:
	void _PrintValue(const BPackageAttributeValue& value)
	{
		switch (value.type) {
			case B_HPKG_ATTRIBUTE_TYPE_INT:
				printf("%lld (%#llx)", (long long)value.signedInt,
					(long long)value.signedInt);
				break;
			case B_HPKG_ATTRIBUTE_TYPE_UINT:
				printf("%llu (%#llx)", (unsigned long long)value.unsignedInt,
					(unsigned long long)value.unsignedInt);
				break;
			case B_HPKG_ATTRIBUTE_TYPE_STRING:
				printf("\"%s\"", value.string);
				break;
			case B_HPKG_ATTRIBUTE_TYPE_RAW:
				switch (value.encoding) {
					case B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE:
						printf("data: size: %llu, inline",
							(unsigned long long)value.data.size);
						// TODO: Print the data bytes!
						break;
					case B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP:
						printf("data: size: %llu, offset: %llu",
							(unsigned long long)value.data.size,
							(unsigned long long)value.data.offset);
						break;
					default:
						break;
				}
				break;
			default:
				printf("<unknown type %u>\n", value.type);
				break;
		}
	}

private:
	int		fLevel;
	bool	fErrorOccurred;
	bool	fHasChildren;
};


int
command_dump(int argc, const char* const* argv)
{
	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One argument should remain -- the package file name.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];

	// open package
	StandardErrorOutput errorOutput;
	BPackageReader packageReader(&errorOutput);
	status_t error = packageReader.Init(packageFileName);
	if (error != B_OK)
		return 1;

	// list
	PackageContentDumpHandler handler;
	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		return 1;

	return 0;
}
