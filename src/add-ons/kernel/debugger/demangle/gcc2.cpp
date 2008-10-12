/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <TypeConstants.h>

#include <debug.h>


enum {
	TYPE_FUNCTION,
	TYPE_METHOD,
};


static void
ignore_qualifiers(const char** _arg)
{
	while (isupper(**_arg))
		(*_arg)++;
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
argument_name_length(const char** _arg)
{
	if (**_arg == 'N')
		return 0;

	ignore_qualifiers(_arg);

	// See if it's a built-in type
	if (isalpha(**_arg))
		return 0;

	return strtoul(*_arg, (char**)_arg, 10);
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

	// See if it's a built-in type
	if (isalpha(**_arg))
		return 1;

	return strtoul(*_arg, (char**)_arg, 10);
}


static const char*
next_argument(const char* arg)
{
	if (arg == NULL)
		return NULL;

	uint32 length = argument_length(&arg);
	arg += length;

	if (!arg[0])
		return NULL;

	return arg;
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
			namespaces = strtoul(mangled + 1, (char**)&mangled, 10);
			if (mangled[0] != '_')
				namespaces = 0;
		} else
			namespaces = mangled[1] - '0';

		mangled++;
	} else if (isdigit(mangled[0]))
		namespaces = 1;

	*_mangled = mangled;
	return namespaces;
}


static const char*
first_argument(const char* mangled)
{
	int32 namespaces = count_namespaces(&mangled);

	while (namespaces-- > 0) {
		if (!isdigit(mangled[0]))
			break;

		mangled += strtoul(mangled, (char**)&mangled, 10);
	}

	return mangled;
}


static const char*
mangled_start(const char* name, size_t* _symbolLength, int32* _symbolType)
{
	if (name == NULL)
		return NULL;

	const char* mangled = strstr(name, "__");
	if (mangled == NULL)
		return NULL;

	if (_symbolLength != NULL)
		*_symbolLength = mangled - name;

	if (mangled[2] == 'F') {
		if (_symbolType != NULL)
			*_symbolType = TYPE_FUNCTION;
		return mangled + 3;
	}

	if (_symbolType != NULL)
		*_symbolType = TYPE_METHOD;
	return mangled + 2;
}



//	#pragma mark -


const char*
demangle_symbol(const char* name, char* buffer, size_t bufferSize,
	bool* _isObjectMethod)
{
	size_t nameLength;
	int32 type;
	const char* mangled = mangled_start(name, &nameLength, &type);
	if (mangled == NULL)
		return NULL;

	if (_isObjectMethod != NULL) {
		// we can only guess with GCC2 mangling
		*_isObjectMethod = type == TYPE_METHOD;
	}

	const char* namespaceStart = mangled;
	int32 namespaces = count_namespaces(&namespaceStart);

	buffer[0] = '\0';

	while (namespaces-- > 0) {
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
get_next_argument(uint32* _cookie, const char* symbol, char* name,
	size_t nameSize, int32* _type, size_t* _argumentLength)
{
	const char* mangled = mangled_start(symbol, NULL, NULL);
	if (mangled == NULL)
		return B_BAD_VALUE;

	const char* arg = first_argument(mangled);

	// (void) is not an argument
	if (arg != NULL && arg[0] == 'v')
		return B_ENTRY_NOT_FOUND;

	uint32 current = *_cookie;
	for (uint32 i = 0; i < current; i++) {
		arg = next_argument(arg);
		if (arg != NULL && arg[0] == 'N') {
			// repeat argument 'count' times
			uint32 index;
			uint32 count = parse_repeats(&arg, index);
			if (current <= i + count) {
				// it's a repeat case
				status_t status = get_next_argument(&index, symbol, name,
					nameSize, _type, _argumentLength);
				if (status == B_OK)
					(*_cookie)++;
				return status;
			}

			i += count - 1;
		}
	}

	if (arg == NULL)
		return B_ENTRY_NOT_FOUND;

	if (arg[0] == 'T') {
		// duplicate argument
		arg++;
		uint32 index = parse_number(&arg, false);
		status_t status = get_next_argument(&index, symbol, name,
			nameSize, _type, _argumentLength);
		if (status == B_OK)
			(*_cookie)++;
		return status;
	}

	(*_cookie)++;

	size_t argumentLength;
	int32 type = argument_type(arg, argumentLength);
	if (_type != NULL)
		*_type = type;
	if (_argumentLength != NULL)
		*_argumentLength = argumentLength;

	uint32 length = argument_name_length(&arg);
	strlcpy(name, arg, min_c(length + 1, nameSize));

	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
#if __GNUC__ != 2
	return B_NOT_SUPPORTED;
#else
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_BAD_VALUE;
#endif
}


static struct debugger_demangle_module_info sModuleInfo = {
	{
		"debugger/demangle/gcc2/v1",
		0,
		std_ops
	},

	demangle_symbol,
	get_next_argument,
};

module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};

