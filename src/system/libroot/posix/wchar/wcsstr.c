/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <wchar_private.h>


wchar_t*
__wcsstr(const wchar_t* haystack, const wchar_t* needleIn)
{
	if (*needleIn == L'\0')
		return (wchar_t*)haystack;

	for (; *haystack != L'\0'; ++haystack) {
		const wchar_t* needle = needleIn;
		const wchar_t* haystackPointer = haystack;
		while (*needle == *haystackPointer++ && *needle != 0)
			++needle;
		if (*needle == L'\0')
			return (wchar_t*)haystack;
	}

	return NULL;
}


B_DEFINE_WEAK_ALIAS(__wcsstr, wcsstr);
B_DEFINE_WEAK_ALIAS(__wcsstr, wcswcs);
