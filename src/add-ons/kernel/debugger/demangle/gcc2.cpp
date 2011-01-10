/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <TypeConstants.h>

#ifdef _KERNEL_MODE
#	include <debug.h>
#endif

#include "demangle.h"


//#define TRACE_GCC2_DEMANGLER
#ifdef TRACE_GCC2_DEMANGLER
#	define TRACE(x...) kprintf(x)
#else
#	define TRACE(x...) ;
#endif


enum {
	TYPE_FUNCTION,
	TYPE_METHOD,
};


static void
ignore_qualifiers(const char** _arg)
{
	while (isupper(**_arg)) {
		if (**_arg == 'Q') {
			// argument has namespaces
			break;
		}
		if (**_arg == 'F') {
			// skip function declaration
			while (**_arg && **_arg != '_')
				(*_arg)++;
		}

		(*_arg)++;
	}
}


static uint32
argument_type(const char* arg, size_t& length)
{
	length = sizeof(int);

	switch (char type = arg[0]) {
		case 'P':	// pointer
		case 'R':	// reference
			length = sizeof(void*);
			ignore_qualifiers(&arg);
			if (arg[0] == 'c')
				return B_STRING_TYPE;
			if (arg[0] == 't') {
				// TODO: templates are not yet supported
				return 0;
			}

			return type == 'P' ? B_POINTER_TYPE : B_REF_TYPE;
		case 'x':
			length = sizeof(long long);
			return B_INT64_TYPE;
		case 'l':
			if (sizeof(long) == 4)
				return B_INT32_TYPE;
			return B_INT64_TYPE;
		case 'i':
			return B_INT32_TYPE;
		case 's':
			return B_INT16_TYPE;
		case 'c':
			return B_INT8_TYPE;
		case 'b':
			return B_BOOL_TYPE;
		case 'U':
			switch (arg[1]) {
				case 'x':
					length = sizeof(long long);
					return B_UINT64_TYPE;
				case 'l':
					if (sizeof(long) == 4)
						return B_UINT32_TYPE;
					return B_UINT64_TYPE;
				case 'i':
					return B_UINT32_TYPE;
				case 's':
					return B_UINT16_TYPE;
				case 'c':
					return B_UINT8_TYPE;
				default:
					return B_UINT32_TYPE;
			}
			break;

		case 'f':
			return B_FLOAT_TYPE;
		case 'd':
			length = sizeof(double);
			return B_DOUBLE_TYPE;
		case 'r':
			// TODO: is "long double" supported under Haiku at all?
			return 0;

		case 't':
			// TODO: templates are not yet supported
			return 0;

		default:
			return B_ANY_TYPE;
	}
}


static uint32
parse_number(const char** _arg, bool numberLeft)
{
	const char* arg = *_arg;

	while (isdigit(arg[0]))
		arg++;

	uint32 value;

	if (arg[0] == '_' && (!numberLeft || isdigit(arg[1]))) {
		// long value
		value = strtoul(*_arg, (char**)_arg, 10);
		if (**_arg == '_')
			(*_arg)++;
	} else {
		value = **_arg - '0';
		(*_arg)++;
	}

	return value;
}


static uint32
parse_repeats(const char** _arg, uint32& index)
{
	if (**_arg != 'N')
		return 0;

	(*_arg)++;

	uint32 count = parse_number(_arg, true);
	index = parse_number(_arg, false);

	return count;
}


static void
skip_numbers(const char** _arg, int32 count)
{
	// skip leading character
	(*_arg)++;

	while (count-- > 0) {
		parse_number(_arg, count != 0);
	}
}


static uint32
count_namespaces(const char** _mangled)
{
	const char* mangled = *_mangled;

	int32 namespaces = 0;
	if (mangled[0] == 'Q') {
		// more than one namespace
		if (mangled[1] == '_') {
			// more than 9 namespaces
			namespaces = strtoul(mangled + 2, (char**)&mangled, 10);
			if (mangled[0] != '_')
				namespaces = 0;

			mangled++;
		} else {
			namespaces = mangled[1] - '0';
			mangled += 2;
		}
	} else if (isdigit(mangled[0]))
		namespaces = 1;

	*_mangled = mangled;
	return namespaces;
}


static void
skip_namespaces(const char** _mangled)
{
	const char* mangled = *_mangled;
	int32 namespaces = count_namespaces(&mangled);

	while (namespaces-- > 0) {
		if (!isdigit(mangled[0]))
			break;

		mangled += strtoul(mangled, (char**)&mangled, 10);
	}

	*_mangled = mangled;
}


static bool
has_named_argument(const char** _arg)
{
	ignore_qualifiers(_arg);

	// See if it's a built-in type
	return **_arg == 'Q' || isdigit(**_arg);
}


static uint32
argument_length(const char** _arg)
{
	if (**_arg == 'N') {
		// skip repeats
		skip_numbers(_arg, 2);
		return 0;
	} else if (**_arg == 'T') {
		// skip reference
		skip_numbers(_arg, 1);
		return 0;
	}

	ignore_qualifiers(_arg);

	if (!**_arg)
		return 0;

	// See if it's a built-in type
	if (**_arg != 'Q' && !isdigit(**_arg))
		return 1;

	const char* mangled = *_arg;
	skip_namespaces(&mangled);

	return mangled - *_arg;
}


static const char*
next_argument(const char* arg)
{
	if (arg == NULL || !arg[0])
		return NULL;

	uint32 length = argument_length(&arg);
	arg += length;

	if (!arg[0])
		return NULL;

	return arg;
}


static const char*
first_argument(const char* mangled)
{
	skip_namespaces(&mangled);

	return mangled;
}


static const char*
mangled_start(const char* name, size_t* _symbolLength, int32* _symbolType)
{
	if (name == NULL)
		return NULL;

	// search '__' starting from the end, don't accept them at the start
	size_t pos = strlen(name) - 1;
	const char* mangled = NULL;

	while (pos > 1) {
		if (name[pos] == '_') {
			if (name[pos - 1] == '_') {
				mangled = name + pos + 1;
				break;
			} else
				pos--;
		}
		pos--;
	}

	if (mangled == NULL)
		return NULL;

	if (mangled[0] == 'H') {
		// TODO: we don't support templates yet
		return NULL;
	}

	if (_symbolLength != NULL)
		*_symbolLength = pos - 1;

	if (mangled[0] == 'F') {
		if (_symbolType != NULL)
			*_symbolType = TYPE_FUNCTION;
		return mangled + 1;
	}

	if (_symbolType != NULL)
		*_symbolType = TYPE_METHOD;
	return mangled;
}


static status_t
get_next_argument_internal(uint32* _cookie, const char* symbol, char* name,
	size_t nameSize, int32* _type, size_t* _argumentLength, bool repeating)
{
	const char* mangled = mangled_start(symbol, NULL, NULL);
	if (mangled == NULL)
		return B_BAD_VALUE;

	const char* arg = first_argument(mangled);

	// (void) is not an argument
	if (arg[0] == 'v')
		return B_ENTRY_NOT_FOUND;

	uint32 current = *_cookie;
	if (current > 32)
		return B_TOO_MANY_ARGS;

	for (uint32 i = 0; i < current; i++) {
		arg = next_argument(arg);
		if (arg != NULL && arg[0] == 'N') {
			// repeat argument 'count' times
			uint32 index;
			uint32 count = parse_repeats(&arg, index);
			if (current <= i + count) {
				if (repeating)
					return B_LINK_LIMIT;

				// it's a repeat case
				status_t status = get_next_argument_internal(&index, symbol,
					name, nameSize, _type, _argumentLength, true);
				if (status == B_OK)
					(*_cookie)++;
				return status;
			}

			i += count - 1;
		}
	}

	if (arg == NULL)
		return B_ENTRY_NOT_FOUND;

	TRACE("\n\targ %ld: %s\n", current, arg);

	if (arg[0] == 'T') {
		// duplicate argument
		if (repeating)
			return B_LINK_LIMIT;

		arg++;
		uint32 index = parse_number(&arg, false);
		status_t status = get_next_argument_internal(&index, symbol, name,
			nameSize, _type, _argumentLength, true);
		if (status == B_OK)
			(*_cookie)++;
		return status;
	}

	(*_cookie)++;

	size_t argumentLength;
	int32 type = argument_type(arg, argumentLength);
	if (type == 0)
		return B_NOT_SUPPORTED;

	if (_type != NULL)
		*_type = type;
	if (_argumentLength != NULL)
		*_argumentLength = argumentLength;

	name[0] = '\0';
	if (!has_named_argument(&arg))
		return B_OK;

	const char* namespaceStart = arg;
	int32 namespaces = count_namespaces(&namespaceStart);

	while (namespaces-- > 0) {
		if (namespaceStart[0] == 't') {
			// It's a template class after all
			return B_BAD_TYPE;
		}
		if (!isdigit(namespaceStart[0]))
			break;

		uint32 length = strtoul(namespaceStart, (char**)&namespaceStart, 10);
		uint32 max = strlen(name) + length + 1;
		strlcat(name, namespaceStart, min_c(max, nameSize));
		if (namespaces > 0)
			strlcat(name, "::", nameSize);
		namespaceStart += length;
	}

	return B_OK;
}


//	#pragma mark -


const char*
demangle_symbol_gcc2(const char* name, char* buffer, size_t bufferSize,
	bool* _isObjectMethod)
{
	size_t nameLength;
	int32 type;
	const char* mangled = mangled_start(name, &nameLength, &type);
	if (mangled == NULL)
		return NULL;

	if (mangled[0] == 'C') {
		// ignore const method
		type = TYPE_METHOD;
		mangled++;
	}

	if (_isObjectMethod != NULL) {
		// we can only guess with GCC2 mangling
		*_isObjectMethod = type == TYPE_METHOD;
	}

	const char* namespaceStart = mangled;
	int32 namespaces = count_namespaces(&namespaceStart);

	buffer[0] = '\0';

	while (namespaces-- > 0) {
		if (namespaceStart[0] == 't') {
			// It's a template class after all
			return NULL;
		}
		if (!isdigit(namespaceStart[0]))
			break;

		uint32 length = strtoul(namespaceStart, (char**)&namespaceStart, 10);
		uint32 max = strlen(buffer) + length + 1;
		strlcat(buffer, namespaceStart, min_c(max, bufferSize));
		strlcat(buffer, "::", bufferSize);
		namespaceStart += length;
	}

	size_t max = strlen(buffer) + nameLength + 1;
	strlcat(buffer, name, min_c(max, bufferSize));
	return buffer;
}


status_t
get_next_argument_gcc2(uint32* _cookie, const char* symbol, char* name,
	size_t nameSize, int32* _type, size_t* _argumentLength)
{
	status_t error = get_next_argument_internal(_cookie, symbol, name, nameSize,
		_type, _argumentLength, false);
	if (error != B_OK)
		return error;

	// append the missing '*'/'&' for pointer/ref types
	if (name[0] != '\0' && (*_type == B_POINTER_TYPE || *_type == B_REF_TYPE))
		strlcat(name, *_type == B_POINTER_TYPE ? "*" : "&", nameSize);

	return B_OK;
}
