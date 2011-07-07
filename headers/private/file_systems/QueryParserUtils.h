/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef _FILE_SYSTEMS_QUERY_PARSER_UTILS_H
#define _FILE_SYSTEMS_QUERY_PARSER_UTILS_H


#include <sys/cdefs.h>

#include <SupportDefs.h>


namespace QueryParser {


enum match {
	NO_MATCH = 0,
	MATCH_OK = 1,

	MATCH_BAD_PATTERN = -2,
	MATCH_INVALID_CHARACTER
};

// return values from isValidPattern()
enum {
	PATTERN_INVALID_ESCAPE = -3,
	PATTERN_INVALID_RANGE,
	PATTERN_INVALID_SET
};


__BEGIN_DECLS


void		skipWhitespace(char** expr, int32 skip = 0);
void		skipWhitespaceReverse(char** expr, char* stop);
int			compareKeys(uint32 type, const void* key1, size_t length1,
				const void* key2, size_t length2);
uint32		utf8ToUnicode(char** string);
int32		getFirstPatternSymbol(char* string);
status_t	isValidPattern(char* pattern);
status_t	matchString(char* pattern, char* string);


__END_DECLS


static inline bool
isPattern(char* string)
{
	return getFirstPatternSymbol(string) >= 0 ? true : false;
}


}	// namespace QueryParser


#endif	// _FILE_SYSTEMS_QUERY_PARSER_UTILS_H
