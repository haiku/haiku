/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Demangler.h"

#include "demangle.h"


/*static*/ BString
Demangler::Demangle(const BString& mangledName)
{
	char buffer[1024];
	const char* demangled;

	if (mangledName.Compare("_Z", 2) == 0) {
		demangled = demangle_name_gcc3(mangledName.String(), buffer,
			sizeof(buffer));
		if (demangled != NULL)
			return demangled;
	}

	// fallback is gcc2
	demangled = demangle_symbol_gcc2(mangledName.String(), buffer, 
		sizeof(buffer), NULL);
	if (demangled != NULL)
		return demangled;

	return mangledName;
}
