/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ValidationUtils.h"

#include <ctype.h>

#include <UnicodeChar.h>


#define MIN_LENGTH_NICKNAME			4
#define MAX_LENGTH_NICKNAME			16

#define MIN_LENGTH_PASSWORD_CLEAR	8
#define MIN_UPPER_PASSWORD_CLEAR	2
#define MIN_DIGITS_PASSWORD_CLEAR	2

/*! 1 if the character would be suitable for use in an email address mailbox
    or domain part.
*/

static int
hd_is_email_domain_or_mailbox_part(int c)
{
	if (0 == isspace(c) && c != 0x40)
		return 1;
	return 0;
}


/*! Returns true if the entire string is lower case alpha numeric.
 */

static int
hd_is_lower_alnum(int c)
{
	if ((c >= 0x30 && c <= 0x39) || (c >= 0x60 && c <= 0x7a))
		return 1;
	return 0;
}


static bool
hd_str_all_matches_fn(const BString& string, int (*hd_match_c)(int c))
{
	const char* c = string.String();
	for (int32 i = 0; i < string.CountChars(); i++) {
		if (0 == hd_match_c(c[i]))
			return false;
	}

	return true;
}


static int32
hd_str_count_upper_case(const BString& string)
{
	int32 upperCaseLetters = 0;
	const char* c = string.String();
	for (int32 i = 0; i < string.CountChars(); i++) {
		uint32 unicodeChar = BUnicodeChar::FromUTF8(&c);
		if (BUnicodeChar::IsUpper(unicodeChar))
			upperCaseLetters++;
	}
	return upperCaseLetters;
}


static int32
hd_str_count_digit(const BString& string)
{
	int32 digits = 0;
	const char* c = string.String();
	for (int32 i = 0; i < string.CountChars(); i++) {
		uint32 unicodeChar = BUnicodeChar::FromUTF8(&c);
		if (BUnicodeChar::IsDigit(unicodeChar))
			digits++;
	}
	return digits;
}


/*static*/ bool
ValidationUtils::IsValidNickname(const BString& value)
{
	return hd_str_all_matches_fn(value, &hd_is_lower_alnum)
		&& value.CountChars() >= MIN_LENGTH_NICKNAME
		&& value.CountChars() <= MAX_LENGTH_NICKNAME;
}


/*! Email addresses are quite difficult to validate 100% correctly so go fairly
    light on the enforcement here; it should be a string with an '@' symbol,
    something either side of the '@' and there should be no whitespace.
*/

/*static*/ bool
ValidationUtils::IsValidEmail(const BString& value)
{
	const char* c = value.String();
	size_t len = strlen(c);
	bool foundAt = false;

	for (size_t i = 0; i < len; i++) {
		if (c[i] == 0x40 && !foundAt) {
			if (i == 0 || i == len - 1)
				return false;
			foundAt = true;
		}
		else {
			if (0 == hd_is_email_domain_or_mailbox_part(c[i]))
				return false;
		}
	}

	return foundAt;
}


/*static*/ bool
ValidationUtils::IsValidPasswordClear(const BString& value)
{
	return value.Length() >= MIN_LENGTH_PASSWORD_CLEAR
		&& hd_str_count_digit(value) >= MIN_DIGITS_PASSWORD_CLEAR
		&& hd_str_count_upper_case(value) >= MIN_UPPER_PASSWORD_CLEAR;
}

