/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <locale.h>

#include <LocaleBackend.h>
#include <PosixCtype.h>


namespace BPrivate {
namespace Libroot {


// A NULL databridge means that the object represents the POSIX locale.
static const unsigned short*
GetClassInfoTable(locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;

	if (locale->databridge == NULL) {
		return &gPosixClassInfo[128];
	}
	return *locale->databridge->ctypeDataBridge.addrOfClassInfoTable;
}


static const int*
GetToUpperTable(locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;

	if (locale->databridge == NULL) {
		return &gPosixToUpperMap[128];
	}
	return *locale->databridge->ctypeDataBridge.addrOfToUpperTable;
}


static const int*
GetToLowerTable(locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;

	if (locale->databridge == NULL) {
		return &gPosixToLowerMap[128];
	}
	return *locale->databridge->ctypeDataBridge.addrOfToLowerTable;
}


extern "C" {


int
isalnum_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & (_ISupper | _ISlower | _ISdigit);

	return 0;
}


int
isalpha_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & (_ISupper | _ISlower);

	return 0;
}


int
isblank_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISblank;

	return 0;
}


int
iscntrl_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _IScntrl;

	return 0;
}


int
isdigit_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISdigit;

	return 0;
}


int
isgraph_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISgraph;

	return 0;
}


int
islower_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISlower;

	return 0;
}


int
isprint_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISprint;

	return 0;
}


int
ispunct_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISpunct;

	return 0;
}


int
isspace_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISspace;

	return 0;
}


int
isupper_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISupper;

	return 0;
}


int
isxdigit_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetClassInfoTable(l)[c] & _ISxdigit;

	return 0;
}


int
tolower_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetToLowerTable(l)[c];

	return c;
}


int
toupper_l(int c, locale_t l)
{
	if (c >= -128 && c < 256)
		return GetToUpperTable(l)[c];

	return c;
}


}


}	// namespace Libroot
}	// namespace BPrivate
