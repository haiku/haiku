/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "Demangler.h"

#include <TypeConstants.h>

#include "demangle.h"


/*static*/ BString
Demangler::Demangle(const BString& mangledName)
{
	char buffer[1024];

	if (mangledName.Compare("_Z", 2) == 0) {
		// probably a gcc3+ mangled symbol
		const char* demangled = demangle_name_gcc3(mangledName.String(), buffer,
			sizeof(buffer));
		if (demangled != NULL)
			return demangled;
	}

	// try gcc 2 demangling
	const char* demangled = demangle_symbol_gcc2(mangledName.String(), buffer,
		sizeof(buffer), NULL);

	if (demangled == NULL) {
		// name not mangled
		return mangledName;
	}

	BString demangledName(demangled);
	demangledName << "(";

	size_t length;
	int32 type;
	int32 i = 0;
	uint32 cookie = 0;
	while (get_next_argument_gcc2(&cookie, mangledName.String(), buffer,
			sizeof(buffer), &type, &length) == B_OK) {
		if (i++ > 0)
			demangledName << ", ";

		if (buffer[0]) {
			demangledName << buffer;
			continue;
		}

		// unnamed argument: fallback to known type
		switch (type) {
			case B_ANY_TYPE:
				break;
			case B_INT64_TYPE:
				demangledName << "int64";
				break;
			case B_INT32_TYPE:
				demangledName << "int32";
				break;
			case B_INT16_TYPE:
				demangledName << "int16";
				break;
			case B_INT8_TYPE:
				demangledName << "int8";
				break;
			case B_UINT64_TYPE:
				demangledName << "uint64";
				break;
			case B_UINT32_TYPE:
				demangledName << "uint32";
				break;
			case B_UINT16_TYPE:
				demangledName << "uint16";
				break;
			case B_UINT8_TYPE:
				demangledName << "uint8";
				break;
			case B_BOOL_TYPE:
				demangledName << "bool";
				break;
			case B_CHAR_TYPE:
				demangledName << "char";
				break;
			case B_FLOAT_TYPE:
				demangledName << "float";
				break;
			case B_DOUBLE_TYPE:
				demangledName << "double";
				break;
			case B_POINTER_TYPE:
				// TODO: use length as hint on pointer type
				demangledName << "void*";
				break;
			case B_REF_TYPE:
			case B_NODE_REF_TYPE:
				// TODO: use length as hint on reference type
				demangledName << "&";
				break;
			case B_STRING_TYPE:
				demangledName << "char*";
				break;
			default:
				demangledName << "?";
				break;
		}
	}

	demangledName << ")";
	return demangledName;
}
