/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */

#include "Utilities.h"

#include <ctype.h>
#include <stdarg.h>

#include <Message.h>


BString
trim_string(const char* string, size_t len)
{
	BString trimmed;
	size_t i = 0;
	bool addSpace = false;

	while (i < len) {
		// skip space block
		while (i < len && isspace(string[i]))
			i++;

		// write non-spaced (if any)
		if (i < len && !isspace(string[i])) {
			// pad with a single space, for all but the first block
			if (addSpace)
				trimmed << ' ';
			else
				addSpace = true;

			// append chars
			while (i < len && !isspace(string[i]))
				trimmed << string[i++];
		}
	}

	return trimmed;
}


// #pragma mark - Licenses


Licenses::Licenses(const char* license,...)
	:
	fLicenses(NULL),
	fCount(0)
{
	if (license == NULL)
		return;

	va_list list;

	// count licenses
	va_start(list, license);
	fCount = 1;
	while (va_arg(list, const char*) != NULL)
		fCount++;
	va_end(list);

	// create array and copy them
	fLicenses = new BString[fCount];
	fLicenses[0] = license;

	va_start(list, license);
	for (int32 i = 1; i < fCount; i++)
		fLicenses[i] = va_arg(list, const char*);
	va_end(list);
}


Licenses::Licenses(const BMessage& licenses)
	:
	fLicenses(NULL),
	fCount(0)
{
	type_code type;
	int32 count;
	if (licenses.GetInfo("License", &type, &count) != B_OK
		|| type != B_STRING_TYPE) {
		return;
	}

	fLicenses = new BString[count];

	for (int32 i = 0; i < count; i++) {
		if (licenses.FindString("License", i, &fLicenses[i]) != B_OK)
			return;
		fCount++;
	}
}


Licenses::~Licenses()
{
	delete[] fLicenses;
}


const char*
Licenses::LicenseAt(int32 index) const
{
	return (index >= 0 && index < fCount ? fLicenses[index].String() : NULL);
}
