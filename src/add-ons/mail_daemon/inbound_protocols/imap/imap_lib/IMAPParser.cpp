/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Copyright 2010 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "IMAPParser.h"

#include <stdlib.h>


BString
IMAPParser::ExtractStringAfter(const BString& string, const char* after)
{
	int32 pos = string.FindFirst(after);
	if (pos < 0)
		return "";

	BString extractedString = string;
	extractedString.Remove(0, pos + strlen(after));

	return extractedString.Trim();
}


BString
IMAPParser::ExtractFirstPrimitive(const BString& string)
{
	BString extractedString = string;
	extractedString.Trim();
	int32 end = extractedString.FindFirst(" ");
	if (end < 0)
		return extractedString;
	return extractedString.Truncate(end);
}


BString
IMAPParser::ExtractBetweenBrackets(const BString& string, const char* start,
	const char* end)
{
	BString outString = string;
	int32 startPos = outString.FindFirst(start);
	if (startPos < 0)
		return "";
	outString.Remove(0, startPos + 1);

	int32 searchPos = 0;
	while (true) {
		int32 endPos = outString.FindFirst(end, searchPos);
		if (endPos < 0)
			return "";
		int32 nextStartPos =  outString.FindFirst(start, searchPos);
		if (nextStartPos < 0 || nextStartPos > endPos)
			return outString.Truncate(endPos);
		searchPos = endPos + 1;
	}
	return "";
};


BString
IMAPParser::ExtractNextElement(const BString& string)
{
	if (string[0] == '(')
		return ExtractBetweenBrackets(string, "(", ")");
	else if (string[0] == '[')
		return ExtractBetweenBrackets(string, "[", "]");
	else if (string[0] == '{')
		return ExtractBetweenBrackets(string, "{", "}");
	else
		return ExtractFirstPrimitive(string);

	return "";
}


BString
IMAPParser::ExtractElementAfter(const BString& string, const char* after)
{
	BString rest = ExtractStringAfter(string, after);
	if (rest == "")
		return rest;
	return ExtractNextElement(rest);
}


BString
IMAPParser::RemovePrimitiveFromLeft(BString& string)
{
	BString extracted;
	string.Trim();
	int32 end = string.FindFirst(" ");
	if (end < 0) {
		extracted = string;
		string = "";
	} else {
		string.MoveInto(extracted, 0, end);
		string.Trim();
	}
	return extracted;
}


int
IMAPParser::RemoveIntegerFromLeft(BString& string)
{
	BString extracted = RemovePrimitiveFromLeft(string);
	return atoi(extracted);
}


bool
IMAPParser::RemoveUntagedFromLeft(BString& string, const char* keyword,
	int32& number)
{
	BString result = RemovePrimitiveFromLeft(string);
	if (result != "*")
		return false;
	number = RemoveIntegerFromLeft(string);
	if (number == 0)
		return false;
	result = RemovePrimitiveFromLeft(string);
	if (result != keyword)
		return false;
	return true;
}


bool
IMAPParser::ExtractUntagedFromLeft(const BString& string, const char* keyword,
	int32& number)
{
	//TODO: could be more efficient without copy the string
	BString copy = string;
	return RemoveUntagedFromLeft(copy, keyword, number);
}
