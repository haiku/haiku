/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * This file may be used under the terms of the MIT License.
 */


#include <file_systems/QueryParserUtils.h>

#include <string.h>

#include <algorithm>

#include <TypeConstants.h>


namespace QueryParser {


template<typename Key>
static inline int
compare_integral(const Key& a, const Key& b)
{
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	return 0;
}


// #pragma mark -


void
skipWhitespace(char** expr, int32 skip)
{
	char* string = (*expr) + skip;
	while (*string == ' ' || *string == '\t') string++;
	*expr = string;
}


void
skipWhitespaceReverse(char** expr, char* stop)
{
	char* string = *expr;
	while (string > stop && (*string == ' ' || *string == '\t'))
		string--;
	*expr = string;
}


int
compareKeys(uint32 type, const void* key1, size_t length1, const void* key2,
	size_t length2)
{
	switch (type) {
		case B_INT32_TYPE:
			return compare_integral(*(int32*)key1, *(int32*)key2);
		case B_UINT32_TYPE:
			return compare_integral(*(uint32*)key1, *(uint32*)key2);
		case B_INT64_TYPE:
			return compare_integral(*(int64*)key1, *(int64*)key2);
		case B_UINT64_TYPE:
			return compare_integral(*(uint64*)key1, *(uint64*)key2);
		case B_FLOAT_TYPE:
			return compare_integral(*(float*)key1, *(float*)key2);
		case B_DOUBLE_TYPE:
			return compare_integral(*(double*)key1, *(double*)key2);
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
		{
			int result = strncmp((const char*)key1, (const char*)key2,
				std::min(length1, length2));
			if (result == 0) {
				result = compare_integral(strnlen((const char*)key1, length1),
					strnlen((const char*)key2, length2));
			}
			return result;
		}
	}
	return -1;
}


//	#pragma mark -


uint32
utf8ToUnicode(char** string)
{
	uint8* bytes = (uint8*)*string;
	int32 length;
	uint8 mask = 0x1f;

	switch (bytes[0] & 0xf0) {
		case 0xc0:
		case 0xd0:
			length = 2;
			break;
		case 0xe0:
			length = 3;
			break;
		case 0xf0:
			mask = 0x0f;
			length = 4;
			break;
		default:
			// valid 1-byte character
			// and invalid characters
			(*string)++;
			return bytes[0];
	}
	uint32 c = bytes[0] & mask;
	int32 i = 1;
	for (; i < length && (bytes[i] & 0x80) > 0; i++)
		c = (c << 6) | (bytes[i] & 0x3f);

	if (i < length) {
		// invalid character
		(*string)++;
		return (uint32)bytes[0];
	}
	*string += length;
	return c;
}


int32
getFirstPatternSymbol(char* string)
{
	char c;

	for (int32 index = 0; (c = *string++); index++) {
		if (c == '*' || c == '?' || c == '[')
			return index;
	}
	return -1;
}


status_t
isValidPattern(char* pattern)
{
	while (*pattern) {
		switch (*pattern++) {
			case '\\':
				// the escape character must not be at the end of the pattern
				if (!*pattern++)
					return PATTERN_INVALID_ESCAPE;
				break;

			case '[':
				if (pattern[0] == ']' || !pattern[0])
					return PATTERN_INVALID_SET;

				while (*pattern != ']') {
					if (*pattern == '\\' && !*++pattern)
						return PATTERN_INVALID_ESCAPE;

					if (!*pattern)
						return PATTERN_INVALID_SET;

					if (pattern[0] == '-' && pattern[1] == '-')
						return PATTERN_INVALID_RANGE;

					pattern++;
				}
				break;
		}
	}
	return B_OK;
}


/*!	Matches the string against the given wildcard pattern.
	Returns either MATCH_OK, or NO_MATCH when everything went fine, or
	values < 0 (see enum at the top of Query.cpp) if an error occurs.
*/
status_t
matchString(char* pattern, char* string)
{
	while (*pattern) {
		// end of string == valid end of pattern?
		if (!string[0]) {
			while (pattern[0] == '*')
				pattern++;
			return !pattern[0] ? MATCH_OK : NO_MATCH;
		}

		switch (*pattern++) {
			case '?':
			{
				// match exactly one UTF-8 character; we are
				// not interested in the result
				utf8ToUnicode(&string);
				break;
			}

			case '*':
			{
				// compact pattern
				while (true) {
					if (pattern[0] == '?') {
						if (!*++string)
							return NO_MATCH;
					} else if (pattern[0] != '*')
						break;

					pattern++;
				}

				// if the pattern is done, we have matched the string
				if (!pattern[0])
					return MATCH_OK;

				while(true) {
					// we have removed all occurences of '*' and '?'
					if (pattern[0] == string[0]
						|| pattern[0] == '['
						|| pattern[0] == '\\') {
						status_t status = matchString(pattern, string);
						if (status < B_OK || status == MATCH_OK)
							return status;
					}

					// we could be nice here and just jump to the next
					// UTF-8 character - but we wouldn't gain that much
					// and it'd be slower (since we're checking for
					// equality before entering the recursion)
					if (!*++string)
						return NO_MATCH;
				}
				break;
			}

			case '[':
			{
				bool invert = false;
				if (pattern[0] == '^' || pattern[0] == '!') {
					invert = true;
					pattern++;
				}

				if (!pattern[0] || pattern[0] == ']')
					return MATCH_BAD_PATTERN;

				uint32 c = utf8ToUnicode(&string);
				bool matched = false;

				while (pattern[0] != ']') {
					if (!pattern[0])
						return MATCH_BAD_PATTERN;

					if (pattern[0] == '\\')
						pattern++;

					uint32 first = utf8ToUnicode(&pattern);

					// Does this character match, or is this a range?
					if (first == c) {
						matched = true;
						break;
					} else if (pattern[0] == '-' && pattern[1] != ']'
							&& pattern[1]) {
						pattern++;

						if (pattern[0] == '\\') {
							pattern++;
							if (!pattern[0])
								return MATCH_BAD_PATTERN;
						}
						uint32 last = utf8ToUnicode(&pattern);

						if (c >= first && c <= last) {
							matched = true;
							break;
						}
					}
				}

				if (invert)
					matched = !matched;

				if (matched) {
					while (pattern[0] != ']') {
						if (!pattern[0])
							return MATCH_BAD_PATTERN;
						pattern++;
					}
					pattern++;
					break;
				}
				return NO_MATCH;
			}

            case '\\':
				if (!pattern[0])
					return MATCH_BAD_PATTERN;
				// supposed to fall through
			default:
				if (pattern[-1] != string[0])
					return NO_MATCH;
				string++;
				break;
		}
	}

	if (string[0])
		return NO_MATCH;

	return MATCH_OK;
}


}	// namespace QueryParser
