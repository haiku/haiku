/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "JwtTokenHelper.h"

#include "DataIOUtils.h"
#include "Json.h"
#include "JsonMessageWriter.h"

#include <ctype.h>


/*static*/ bool
JwtTokenHelper::IsValid(const BString& value)
{
	int countDots = 0;

	for (int i = 0; i < value.Length(); i++) {
		char ch = value[i];

		if ('.' == ch)
			countDots++;
		else {
			if (!_IsBase64(ch))
				return false;
		}
	}

	return 2 == countDots;
}


/*! A JWT token is split into three parts separated by a '.' character. The
    middle section is a base-64 encoded string and within the string is JSON
    structured data. The JSON data contains key-value pairs which carry data
    about the token. This method will take a JWT token, will find the middle
    section and will parse the claims into the supplied 'message' parameter.
*/

/*static*/ status_t
JwtTokenHelper::ParseClaims(const BString& token, BMessage& message)
{
	int firstDot = token.FindFirst('.');

	if (firstDot == B_ERROR)
		return B_BAD_VALUE;

	// find the end of the first section by looking for the next dot.

	int secondDot = token.FindFirst('.', firstDot + 1);

	if (secondDot == B_ERROR)
		return B_BAD_VALUE;

	BMemoryIO memoryIo(&(token.String())[firstDot + 1], (secondDot - firstDot) - 1);
	Base64DecodingDataIO base64DecodingIo(&memoryIo, '-', '_');
	BJsonMessageWriter writer(message);
	BJson::Parse(&base64DecodingIo, &writer);

	return writer.ErrorStatus();
}


/*! Note this is base64 "url" standard that disallows "/" and "+" and instead
    uses "-" and "_".
*/

/*static*/ bool
JwtTokenHelper::_IsBase64(char ch)
{
	return isalnum(ch)
		|| '=' == ch
		|| '-' == ch
		|| '_' == ch;
}
