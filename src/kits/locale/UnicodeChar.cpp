/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Siarzhuk Zharski, zharik@gmx.li
 *
 */


#include <UnicodeChar.h>

#include <unicode/uchar.h>
#include <unicode/utf8.h>


BUnicodeChar::BUnicodeChar()
{
}


// Returns the general category value for the code point.
int8
BUnicodeChar::Type(uint32 c)
{
	return u_charType(c);
}


// Determines whether the specified code point is a letter character.
// True for general categories "L" (letters).
bool
BUnicodeChar::IsAlpha(uint32 c)
{
	return u_isalpha(c);
}


// Determines whether the specified code point is an alphanumeric character
// (letter or digit).
// True for characters with general categories
// "L" (letters) and "Nd" (decimal digit numbers).
bool
BUnicodeChar::IsAlNum(uint32 c)
{
	return u_isalnum(c);
}


// Check if a code point has the Lowercase Unicode property (UCHAR_LOWERCASE).
bool
BUnicodeChar::IsLower(uint32 c)
{
	return u_isULowercase(c);
}


// Check if a code point has the Uppercase Unicode property (UCHAR_UPPERCASE).
bool
BUnicodeChar::IsUpper(uint32 c)
{
	return u_isUUppercase(c);
}


// Determines whether the specified code point is a titlecase letter.
// True for general category "Lt" (titlecase letter).
bool
BUnicodeChar::IsTitle(uint32 c)
{
	return u_istitle(c);
}


// Determines whether the specified code point is a digit character.
// True for characters with general category "Nd" (decimal digit numbers).
// Beginning with Unicode 4, this is the same as
// testing for the Numeric_Type of Decimal.
bool
BUnicodeChar::IsDigit(uint32 c)
{
	return u_isdigit(c);
}


// Determines whether the specified code point is a hexadecimal digit.
// This is equivalent to u_digit(c, 16)>=0.
// True for characters with general category "Nd" (decimal digit numbers)
// as well as Latin letters a-f and A-F in both ASCII and Fullwidth ASCII.
// (That is, for letters with code points
// 0041..0046, 0061..0066, FF21..FF26, FF41..FF46.)
bool
BUnicodeChar::IsHexDigit(uint32 c)
{
	return u_isxdigit(c);
}


// Determines whether the specified code point is "defined",
// which usually means that it is assigned a character.
// True for general categories other than "Cn" (other, not assigned),
// i.e., true for all code points mentioned in UnicodeData.txt.
bool
BUnicodeChar::IsDefined(uint32 c)
{
	return u_isdefined(c);
}


// Determines whether the specified code point is a base character.
// True for general categories "L" (letters), "N" (numbers),
// "Mc" (spacing combining marks), and "Me" (enclosing marks).
bool
BUnicodeChar::IsBase(uint32 c)
{
	return u_isbase(c);
}


// Determines whether the specified code point is a control character
// (as defined by this function).
// A control character is one of the following:
// - ISO 8-bit control character (U+0000..U+001f and U+007f..U+009f)
// - U_CONTROL_CHAR (Cc)
// - U_FORMAT_CHAR (Cf)
// - U_LINE_SEPARATOR (Zl)
// - U_PARAGRAPH_SEPARATOR (Zp)
bool
BUnicodeChar::IsControl(uint32 c)
{
	return u_iscntrl(c);
}


// Determines whether the specified code point is a punctuation character.
// True for characters with general categories "P" (punctuation).
bool
BUnicodeChar::IsPunctuation(uint32 c)
{
	return u_ispunct(c);
}


// Determine if the specified code point is a space character according to Java.
// True for characters with general categories "Z" (separators),
// which does not include control codes (e.g., TAB or Line Feed).
bool
BUnicodeChar::IsSpace(uint32 c)
{
	return u_isJavaSpaceChar(c);
}


// Determines if the specified code point is a whitespace character
// A character is considered to be a whitespace character if and only
// if it satisfies one of the following criteria:
// - It is a Unicode Separator character (categories "Z" = "Zs" or "Zl" or "Zp"),
//		but is not also a non-breaking space (U+00A0 NBSP or U+2007 Figure Space
//		or U+202F Narrow NBSP).
// - It is U+0009 HORIZONTAL TABULATION.
// - It is U+000A LINE FEED.
// - It is U+000B VERTICAL TABULATION.
// - It is U+000C FORM FEED.
// - It is U+000D CARRIAGE RETURN.
// - It is U+001C FILE SEPARATOR.
// - It is U+001D GROUP SEPARATOR.
// - It is U+001E RECORD SEPARATOR.
// - It is U+001F UNIT SEPARATOR.
bool
BUnicodeChar::IsWhitespace(uint32 c)
{
	return u_isWhitespace(c);
}


// Determines whether the specified code point is a printable character.
// True for general categories other than "C" (controls).
bool
BUnicodeChar::IsPrintable(uint32 c)
{
	return u_isprint(c);
}


//	#pragma mark -

uint32
BUnicodeChar::ToLower(uint32 c)
{
	return u_tolower(c);
}


uint32
BUnicodeChar::ToUpper(uint32 c)
{
	return u_toupper(c);
}


uint32
BUnicodeChar::ToTitle(uint32 c)
{
	return u_totitle(c);
}


int32
BUnicodeChar::DigitValue(uint32 c)
{
	return u_digit(c, 10);
}


unicode_east_asian_width
BUnicodeChar::EastAsianWidth(uint32 c)
{
	return (unicode_east_asian_width)u_getIntPropertyValue(c,
			UCHAR_EAST_ASIAN_WIDTH);
}


void
BUnicodeChar::ToUTF8(uint32 c, char** out)
{
	int i = 0;
	U8_APPEND_UNSAFE(*out, i, c);
	*out += i;
}


uint32
BUnicodeChar::FromUTF8(const char** in)
{
	int i = 0;
	uint32 c = 0;
	U8_NEXT_UNSAFE(*in, i, c);
	*in += i;

	return c;
}


size_t
BUnicodeChar::UTF8StringLength(const char* string)
{
	size_t len = 0;
	while (*string) {
		FromUTF8(&string);
		len++;
	}
	return len;
}


size_t
BUnicodeChar::UTF8StringLength(const char* string, size_t maxLength)
{
	size_t len = 0;
	while (len < maxLength && *string) {
		FromUTF8(&string);
		len++;
	}
	return len;
}
