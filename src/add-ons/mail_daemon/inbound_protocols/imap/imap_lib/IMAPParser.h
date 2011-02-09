/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_PARSER_H
#define IMAP_PARSER_H


#include <String.h>


namespace IMAPParser {

BString		ExtractStringAfter(const BString& string, const char* after);
BString		ExtractFirstPrimitive(const BString& string);
BString		ExtractBetweenBrackets(const BString& string, const char* start,
				const char* end);
BString		ExtractNextElement(const BString& string);
BString		ExtractElementAfter(const BString& string, const char* after);

BString		RemovePrimitiveFromLeft(BString& string);
int			RemoveIntegerFromLeft(BString& string);

// remove the part like "* 234 FETCH " where FETCH is the keyword
bool		RemoveUntagedFromLeft(BString& string, const char* keyword,
				int32& number);
bool		ExtractUntagedFromLeft(const BString& string, const char* keyword,
				int32& number);

}


#endif // IMAP_PARSER_H
