/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT license.
 */

#include "Utilities.h"

#include <ctype.h>
#include <stdlib.h>

#if __GNUC__ < 4
#define va_copy		__va_copy
#endif

#include <Catalog.h>
#include <Locale.h>
#include <Message.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utilities"


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


void
parse_named_url(const BString& namedURL, BString& name, BString& url)
{
	int32 urlStart = namedURL.FindFirst('<');
	int32 urlEnd = namedURL.FindLast('>');
	if (urlStart < 0 || urlEnd < 0 || urlStart + 1 >= urlEnd) {
		name = namedURL;
		url = namedURL;
		return;
	}

	url.SetTo(namedURL.String() + urlStart + 1, urlEnd - urlStart - 1);

	if (urlStart > 0)
		name = trim_string(namedURL, urlStart);
	else
		name = url;
}


// #pragma mark - StringVector


StringVector::StringVector()
	:
	fStrings(NULL),
	fCount(0)
{
}


StringVector::StringVector(const char* string,...)
	:
	fStrings(NULL),
	fCount(0)
{
	if (string == NULL)
		return;

	va_list list;
	va_start(list, string);
	SetTo(string, list);
	va_end(list);
}


StringVector::StringVector(const BMessage& strings, const char* fieldName)
	:
	fStrings(NULL),
	fCount(0)
{
	SetTo(strings, fieldName);
}


StringVector::StringVector(const StringVector& other)
	:
	fStrings(NULL),
	fCount(0)
{
	if (other.fCount == 0)
		return;

	fStrings = new BString[other.fCount];
	fCount = other.fCount;

	for (int32 i = 0; i < fCount; i++)
		fStrings[i] = other.fStrings[i];
}


StringVector::~StringVector()
{
	Unset();
}


void
StringVector::SetTo(const char* string,...)
{
	va_list list;
	va_start(list, string);
	SetTo(string, list);
	va_end(list);
}


void
StringVector::SetTo(const char* string, va_list _list)
{
	// free old strings
	Unset();

	if (string == NULL)
		return;

	// count strings
	va_list list;
	va_copy(list, _list);
	fCount = 1;
	while (va_arg(list, const char*) != NULL)
		fCount++;
	va_end(list);

	// create array and copy them
	fStrings = new BString[fCount];
	fStrings[0] = string;

	va_copy(list, _list);
	for (int32 i = 1; i < fCount; i++)
		fStrings[i] = va_arg(list, const char*);
	va_end(list);

}


void
StringVector::SetTo(const BMessage& strings, const char* fieldName,
	const char* prefix)
{
	Unset();

	type_code type;
	int32 count;
	if (strings.GetInfo(fieldName, &type, &count) != B_OK
		|| type != B_STRING_TYPE) {
		return;
	}

	fStrings = new BString[count];

	for (int32 i = 0; i < count; i++) {
		if (strings.FindString(fieldName, i, &fStrings[i]) != B_OK)
			return;

		if (prefix != NULL)
			fStrings[i].Prepend(prefix);

		fCount++;
	}
}


void
StringVector::Unset()
{
	delete[] fStrings;
	fStrings = NULL;
	fCount = 0;
}


const char*
StringVector::StringAt(int32 index) const
{
	return (index >= 0 && index < fCount ? fStrings[index].String() : NULL);
}


// #pragma mark - PackageCredit


PackageCredit::PackageCredit(const char* packageName)
	:
	fPackageName(packageName)
{
}


PackageCredit::PackageCredit(const BMessage& packageDescription)
{
	const char* package;
	const char* copyright;
	const char* url;

	// package and copyright are mandatory
	if (packageDescription.FindString("Package", &package) != B_OK
		|| packageDescription.FindString("Copyright", &copyright) != B_OK) {
		return;
	}

	// URL is optional
	if (packageDescription.FindString("URL", &url) != B_OK)
		url = NULL;

	fPackageName = package;
	fCopyrights.SetTo(packageDescription, "Copyright", COPYRIGHT_STRING);
	fLicenses.SetTo(packageDescription, "License");
	fSources.SetTo(packageDescription, "SourceURL");
	fURL = url;
}


PackageCredit::PackageCredit(const PackageCredit& other)
	:
	fPackageName(other.fPackageName),
	fCopyrights(other.fCopyrights),
	fLicenses(other.fLicenses),
	fSources(other.fSources),
	fURL(other.fURL)
{
}


PackageCredit::~PackageCredit()
{
}


bool
PackageCredit::IsValid() const
{
	// should have at least a package name and a copyright
	return fPackageName.Length() > 0 && !fCopyrights.IsEmpty();
}


bool
PackageCredit::IsBetterThan(const PackageCredit& other) const
{
	// We prefer credits with licenses.
	if (CountLicenses() > 0 && other.CountLicenses() == 0)
		return true;

	// Scan the copyrights for year numbers and let the greater one win.
	return _MaxCopyrightYear() > other._MaxCopyrightYear();
}


PackageCredit&
PackageCredit::SetCopyrights(const char* copyright,...)
{
	va_list list;
	va_start(list, copyright);
	fCopyrights.SetTo(copyright, list);
	va_end(list);

	return *this;
}


PackageCredit&
PackageCredit::SetCopyright(const char* copyright)
{
	return SetCopyrights(copyright, NULL);
}


PackageCredit&
PackageCredit::SetLicenses(const char* license,...)
{
	va_list list;
	va_start(list, license);
	fLicenses.SetTo(license, list);
	va_end(list);

	return *this;
}


PackageCredit&
PackageCredit::SetLicense(const char* license)
{
	return SetLicenses(license, NULL);
}


PackageCredit&
PackageCredit::SetSources(const char* source,...)
{
	va_list list;
	va_start(list, source);
	fSources.SetTo(source, list);
	va_end(list);

	return *this;
}


PackageCredit&
PackageCredit::SetSource(const char* source)
{
	return SetSources(source, NULL);
}


PackageCredit&
PackageCredit::SetURL(const char* url)
{
	fURL = url;

	return *this;
}


const char*
PackageCredit::PackageName() const
{
	return fPackageName.String();
}


const StringVector&
PackageCredit::Copyrights() const
{
	return fCopyrights;
}


int32
PackageCredit::CountCopyrights() const
{
	return fCopyrights.CountStrings();
}


const char*
PackageCredit::CopyrightAt(int32 index) const
{
	return fCopyrights.StringAt(index);
}


const StringVector&
PackageCredit::Licenses() const
{
	return fLicenses;
}


int32
PackageCredit::CountLicenses() const
{
	return fLicenses.CountStrings();
}


const char*
PackageCredit::LicenseAt(int32 index) const
{
	return fLicenses.StringAt(index);
}


const StringVector&
PackageCredit::Sources() const
{
	return fSources;
}


int32
PackageCredit::CountSources() const
{
	return fSources.CountStrings();
}


const char*
PackageCredit::SourceAt(int32 index) const
{
	return fSources.StringAt(index);
}


const char*
PackageCredit::URL() const
{
	return fURL.Length() > 0 ? fURL.String() : NULL;
}


/*static*/ bool
PackageCredit::NameLessInsensitive(const PackageCredit* a,
	const PackageCredit* b)
{
	return a->fPackageName.ICompare(b->fPackageName) < 0;
}


int
PackageCredit::_MaxCopyrightYear() const
{
	int maxYear = 0;

	for (int32 i = 0; const char* string = CopyrightAt(i); i++) {
		// iterate through the numbers
		int32 start = 0;
		while (true) {
			// find the next number start
			while (string[start] != '\0' && !isdigit(string[start]))
				start++;

			if (string[start] == '\0')
				break;

			// find the end
			int32 end = start + 1;
			while (string[end] != '\0' && isdigit(string[end]))
				end++;

			if (end - start == 4) {
				int year = atoi(string + start);
				if (year > 1900 && year < 2200 && year > maxYear)
					maxYear = year;
			}

			start = end;
		}
	}

	return maxYear;
}
