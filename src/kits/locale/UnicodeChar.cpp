/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/* Reads the information out of the data files created by (an edited version of)
 * IBM's ICU genprops utility. The BUnicodeChar class is mostly the counterpart
 * to ICU's uchar module, but is not as huge or broad as that one.
 *
 * Note, it probably won't be able to handle the output of the orginal genprops
 * tool and vice versa - only use the tool provided with this project to create
 * the Unicode property file.
 * However, the algorithmic idea behind the property file is still the same as
 * found in ICU - nothing important has been changed, so more recent versions
 * of genprops tool/data can probably be ported without too much effort.
 *
 * In case no property file can be found it will still provide basic services
 * for the Latin-1 part of the character tables.
 */


#include <OS.h>

#include <UnicodeChar.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define FLAG(n) ((uint32)1 << (n))
enum {
	UF_UPPERCASE		= FLAG(B_UNICODE_UPPERCASE_LETTER),
	UF_LOWERCASE		= FLAG(B_UNICODE_LOWERCASE_LETTER),
	UF_TITLECASE		= FLAG(B_UNICODE_TITLECASE_LETTER),
	UF_MODIFIER_LETTER	= FLAG(B_UNICODE_MODIFIER_LETTER),
	UF_OTHER_LETTER		= FLAG(B_UNICODE_OTHER_LETTER),
	UF_DECIMAL_NUMBER	= FLAG(B_UNICODE_DECIMAL_DIGIT_NUMBER),
	UF_OTHER_NUMBER		= FLAG(B_UNICODE_OTHER_NUMBER),
	UF_LETTER_NUMBER	= FLAG(B_UNICODE_LETTER_NUMBER)
};


static uint32 gStaticProps32Table[] = {
    /* 0x00 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x04 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x08 */	0x48f,		0x20c,		0x1ce,		0x20c,
    /* 0x0c */	0x24d,		0x1ce,		0x48f,		0x48f,
    /* 0x10 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x14 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x18 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x1c */	0x1ce,		0x1ce,		0x1ce,		0x20c,
    /* 0x20 */	0x24c,		0x297,		0x297,		0x117,
    /* 0x24 */	0x119,		0x117,		0x297,		0x297,
    /* 0x28 */	0x100a94,	0xfff00a95,	0x297,		0x118,
    /* 0x2c */	0x197,		0x113,		0x197,		0xd7,
    /* 0x30 */	0x89,		0x100089,	0x200089,	0x300089,
    /* 0x34 */	0x400089,	0x500089,	0x600089,	0x700089,
    /* 0x38 */	0x800089,	0x900089,	0x197,		0x297,
    /* 0x3c */	0x200a98,	0x298,		0xffe00a98,	0x297,
    /* 0x40 */	0x297,		0x2000001,	0x2000001,	0x2000001,
    /* 0x44 */	0x2000001,	0x2000001,	0x2000001,	0x2000001,
    /* 0x48 */	0x2000001,	0x2000001,	0x2000001,	0x2000001,
    /* 0x4c */	0x2000001,	0x2000001,	0x2000001,	0x2000001,
    /* 0x50 */	0x2000001,	0x2000001,	0x2000001,	0x2000001,
    /* 0x54 */	0x2000001,	0x2000001,	0x2000001,	0x2000001,
    /* 0x58 */	0x2000001,	0x2000001,	0x2000001,	0x200a94,
    /* 0x5c */	0x297,		0xffe00a95,	0x29a,		0x296,
    /* 0x60 */	0x29a,		0x2000002,	0x2000002,	0x2000002,
    /* 0x64 */	0x2000002,	0x2000002,	0x2000002,	0x2000002,
    /* 0x68 */	0x2000002,	0x2000002,	0x2000002,	0x2000002,
    /* 0x6c */	0x2000002,	0x2000002,	0x2000002,	0x2000002,
    /* 0x70 */	0x2000002,	0x2000002,	0x2000002,	0x2000002,
    /* 0x74 */	0x2000002,	0x2000002,	0x2000002,	0x2000002,
    /* 0x78 */	0x2000002,	0x2000002,	0x2000002,	0x200a94,
    /* 0x7c */	0x298,		0xffe00a95,	0x298,		0x48f,
    /* 0x80 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x84 */	0x48f,		0x1ce,		0x48f,		0x48f,
    /* 0x88 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x8c */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x90 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x94 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x98 */	0x48f,		0x48f,		0x48f,		0x48f,
    /* 0x9c */	0x48f,		0x48f,		0x48f,		0x48f
};

enum {
    INDEX_STAGE_2_BITS,
    INDEX_STAGE_3_BITS,
    INDEX_EXCEPTIONS,
    INDEX_STAGE_3_INDEX,
    INDEX_PROPS,
    INDEX_UCHARS
};

/* constants and macros for access to the data */
enum {
    EXC_UPPERCASE,
    EXC_LOWERCASE,
    EXC_TITLECASE,
    EXC_DIGIT_VALUE,
    EXC_NUMERIC_VALUE,
    EXC_DENOMINATOR_VALUE,
    EXC_MIRROR_MAPPING,
    EXC_SPECIAL_CASING,
    EXC_CASE_FOLDING
};

enum {
    EXCEPTION_SHIFT	= 5,
    BIDI_SHIFT,
    MIRROR_SHIFT	= BIDI_SHIFT + 5,
    VALUE_SHIFT		= 20,

    VALUE_BITS		= 32 - VALUE_SHIFT
};

/* number of bits in an 8-bit integer value */
#define EXC_GROUP 8
static uint8 gFlagsOffset[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

#ifdef UCHAR_VARIABLE_TRIE_BITS
	// access values calculated from indices
	static uint16_t stage23Bits, stage2Mask, stage3Mask;
#	define sStage3Bits   indexes[INDEX_STAGE_3_BITS]
#else
    // Use hardcoded bit distribution for the trie table access
#	define sStage23Bits  10
#	define sStage2Mask   0x3f
#	define sStage3Mask   0xf
#	define sStage3Bits   4
#endif


/**	We need to change the char category for ISO 8 controls, since the
 *	genprops utility we got from IBM's ICU apparently changes it for
 *	some characters.
 */

static inline bool
isISO8Control(uint32 c)
{
	return ((uint32)c < 0x20 || (uint32)(c - 0x7f) <= 0x20);
}


static inline uint32
getProperties(uint32 c)
{
	if (c > 0x10ffff)
		return 0;

	// TODO : Data from unicode

	return c > 0x9f ? 0 : gStaticProps32Table[c];
}


static inline uint8
getCategory(uint32 properties)
{
	return properties & 0x1f;
}


static inline bool
propertyIsException(uint32 properties)
{
	return properties & (1UL << EXCEPTION_SHIFT);
}


static inline uint32
getUnsignedValue(uint32 properties)
{
	return properties >> VALUE_SHIFT;
}


static inline uint32
getSignedValue(uint32 properties)
{
	return (int32)properties >> VALUE_SHIFT;
}


static inline uint32 *
getExceptions(uint32 properties)
{
	// TODO : data from unicode
	return 0;
}


static inline bool
haveExceptionValue(uint32 flags,int16 index)
{
	return flags & (1UL << index);
}


static inline void
addExceptionOffset(uint32 &flags, int16 &index, uint32 **offset)
{
	if (index >= EXC_GROUP) {
		*offset += gFlagsOffset[flags & ((1 << EXC_GROUP) - 1)];
		flags >>= EXC_GROUP;
		index -= EXC_GROUP;
	}
	*offset += gFlagsOffset[flags & ((1 << index) - 1)];
}


//	#pragma mark -


BUnicodeChar::BUnicodeChar()
{
}


bool 
BUnicodeChar::IsAlpha(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (UF_UPPERCASE | UF_LOWERCASE | UF_TITLECASE | UF_MODIFIER_LETTER | UF_OTHER_LETTER)
		   ) != 0;
}


/** Returns the type code of the specified unicode character */
int8
BUnicodeChar::Type(uint32 c)
{
	BUnicodeChar();
	return (int8)getCategory(getProperties(c));
}


bool 
BUnicodeChar::IsLower(uint32 c)
{
	BUnicodeChar();
    return getCategory(getProperties(c)) == B_UNICODE_LOWERCASE_LETTER;
}


bool 
BUnicodeChar::IsUpper(uint32 c)
{
	BUnicodeChar();
	return getCategory(getProperties(c)) == B_UNICODE_UPPERCASE_LETTER;
}


bool 
BUnicodeChar::IsTitle(uint32 c)
{
	BUnicodeChar();
	return getCategory(getProperties(c)) == B_UNICODE_TITLECASE_LETTER;
}


bool 
BUnicodeChar::IsDigit(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (UF_DECIMAL_NUMBER | UF_OTHER_NUMBER | UF_LETTER_NUMBER)
		   ) != 0;
}


bool 
BUnicodeChar::IsAlNum(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (UF_DECIMAL_NUMBER | UF_OTHER_NUMBER | UF_LETTER_NUMBER | UF_UPPERCASE
			   | UF_LOWERCASE | UF_TITLECASE | UF_MODIFIER_LETTER | UF_OTHER_LETTER)
           ) != 0;
}


bool 
BUnicodeChar::IsDefined(uint32 c)
{
	BUnicodeChar();
	return getProperties(c) != 0;
}


/** Returns true if the specified unicode character is a base
 *	form character that can be used with a diacritic.
 *	This doesn't mean that the character has to be distinct,
 *	though.
 */

bool 
BUnicodeChar::IsBase(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (UF_DECIMAL_NUMBER | UF_OTHER_NUMBER | UF_LETTER_NUMBER 
			   | UF_UPPERCASE | UF_LOWERCASE | UF_TITLECASE
			   | UF_MODIFIER_LETTER | UF_OTHER_LETTER | FLAG(B_UNICODE_NON_SPACING_MARK)
			   | FLAG(B_UNICODE_ENCLOSING_MARK) | FLAG(B_UNICODE_COMBINING_SPACING_MARK))
		   ) != 0;
}


/** Returns true if the specified unicode character is a
 *	control character.
 */

bool 
BUnicodeChar::IsControl(uint32 c)
{
	BUnicodeChar();
	return isISO8Control(c)
			|| (FLAG(getCategory(getProperties(c)))
				& (FLAG(B_UNICODE_CONTROL_CHAR) | FLAG(B_UNICODE_FORMAT_CHAR)
					| FLAG(B_UNICODE_LINE_SEPARATOR) | FLAG(B_UNICODE_PARAGRAPH_SEPARATOR))
			   ) != 0;
}


/** Returns true if the specified unicode character is a
 *	punctuation character.
 */

bool
BUnicodeChar::IsPunctuation(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (FLAG(B_UNICODE_DASH_PUNCTUATION)
				| FLAG(B_UNICODE_START_PUNCTUATION)
				| FLAG(B_UNICODE_END_PUNCTUATION)
				| FLAG(B_UNICODE_CONNECTOR_PUNCTUATION)
				| FLAG(B_UNICODE_OTHER_PUNCTUATION))
			) != 0;
}


/** Returns true if the specified unicode character is some
 *	kind of a space character.
 */

bool 
BUnicodeChar::IsSpace(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (FLAG(B_UNICODE_SPACE_SEPARATOR)
				| FLAG(B_UNICODE_LINE_SEPARATOR)
				| FLAG(B_UNICODE_PARAGRAPH_SEPARATOR))
		   ) != 0;
}


/** Returns true if the specified unicode character is a white
 *	space character.
 *	This is essentially the same as IsSpace(), but excludes all
 *	non-breakable spaces.
 */

bool 
BUnicodeChar::IsWhitespace(uint32 c)
{
	BUnicodeChar();
	return (FLAG(getCategory(getProperties(c)))
			& (FLAG(B_UNICODE_SPACE_SEPARATOR)
				| FLAG(B_UNICODE_LINE_SEPARATOR)
				| FLAG(B_UNICODE_PARAGRAPH_SEPARATOR))
		   ) != 0 && c != 0xa0 && c != 0x202f && c != 0xfeff; // exclude non-breakable spaces
}


/** Returns true if the specified unicode character is printable.
 */

bool 
BUnicodeChar::IsPrintable(uint32 c)
{
	BUnicodeChar();
	return !isISO8Control(c)
			&& (FLAG(getCategory(getProperties(c)))
				& ~(FLAG(B_UNICODE_UNASSIGNED) | FLAG(B_UNICODE_CONTROL_CHAR)
					| FLAG(B_UNICODE_FORMAT_CHAR) | FLAG(B_UNICODE_PRIVATE_USE_CHAR)
					| FLAG(B_UNICODE_SURROGATE) | FLAG(B_UNICODE_GENERAL_OTHER_TYPES)
					| FLAG(31))
				   ) != 0;
}


//	#pragma mark -


/** Transforms the specified unicode character to lowercase.
 */

uint32 
BUnicodeChar::ToLower(uint32 c)
{
	BUnicodeChar();

	uint32 props = getProperties(c);

	if (!propertyIsException(props)) {
		if (FLAG(getCategory(props)) & (UF_UPPERCASE | UF_TITLECASE))
			return c + getSignedValue(props);
	} else {
		uint32 *exceptions = getExceptions(props);
		uint32 firstExceptionValue = *exceptions;

		if (haveExceptionValue(firstExceptionValue, EXC_LOWERCASE)) {
			int16 index = EXC_LOWERCASE;
			addExceptionOffset(firstExceptionValue, index, &++exceptions);
			return *exceptions;
		}
	}
	// no mapping found, just return the character unchanged
	return c;
}


/** Transforms the specified unicode character to uppercase.
 */

uint32 
BUnicodeChar::ToUpper(uint32 c)
{
	BUnicodeChar();

	uint32 props = getProperties(c);

	if (!propertyIsException(props)) {
		if (getCategory(props) == B_UNICODE_LOWERCASE_LETTER)
			return c - getSignedValue(props);
	} else {
		uint32 *exceptions = getExceptions(props);
		uint32 firstExceptionValue = *exceptions;

		if (haveExceptionValue(firstExceptionValue, EXC_UPPERCASE)) {
			int16 index = EXC_UPPERCASE;
			++exceptions;
			addExceptionOffset(firstExceptionValue, index, &exceptions);
			return *exceptions;
		}
    }
	// no mapping found, just return the character unchanged
	return c;
}


/** Transforms the specified unicode character to title case.
 */

uint32 
BUnicodeChar::ToTitle(uint32 c)
{
	BUnicodeChar();

	uint32 props = getProperties(c);

	if (!propertyIsException(props)) {
		if (getCategory(props) == B_UNICODE_LOWERCASE_LETTER) {
			// here, titlecase is the same as uppercase
			return c - getSignedValue(props);
		}
	} else {
		uint32 *exceptions = getExceptions(props);
		uint32 firstExceptionValue = *exceptions;

		if (haveExceptionValue(firstExceptionValue, EXC_TITLECASE)) {
			int16 index = EXC_TITLECASE;
			addExceptionOffset(firstExceptionValue, index, &++exceptions);
			return (uint32)*exceptions;
		} else if (haveExceptionValue(firstExceptionValue, EXC_UPPERCASE)) {
			// here, titlecase is the same as uppercase
			int16 index = EXC_UPPERCASE;
			addExceptionOffset(firstExceptionValue, index, &++exceptions);
			return *exceptions;
		}
	}
	// no mapping found, just return the character unchanged
	return c;
}


int32 
BUnicodeChar::DigitValue(uint32 c)
{
	BUnicodeChar();

	uint32 props = getProperties(c);

	if (!propertyIsException(props)) {
		if (getCategory(props) == B_UNICODE_DECIMAL_DIGIT_NUMBER)
			return getSignedValue(props);
	} else {
		uint32 *exceptions = getExceptions(props);
		uint32 firstExceptionValue = *exceptions;

		if (haveExceptionValue(firstExceptionValue, EXC_DIGIT_VALUE)) {
			int16 index = EXC_DIGIT_VALUE;
			addExceptionOffset(firstExceptionValue, index, &++exceptions);

			int32 value = (int32)(int16)*exceptions;
				 // the digit value is in the lower 16 bits
			if (value != -1)
				return value;
		}
	}

    // If there is no value in the properties table,
    // then check for some special characters
	switch (c) {
		case 0x3007:	return 0;
		case 0x4e00:	return 1;
		case 0x4e8c:	return 2;
		case 0x4e09:	return 3;
		case 0x56d8:	return 4;
		case 0x4e94:	return 5;
		case 0x516d:	return 6;
		case 0x4e03:	return 7;
		case 0x516b:	return 8;
		case 0x4e5d:	return 9;
		default:		return -1;
	}
}


void
BUnicodeChar::ToUTF8(uint32 c, char **out)
{
	char *s = *out;

	if (c < 0x80)
		*(s++) = c;
	else if (c < 0x800) {
		*(s++) = 0xc0 | (c >> 6);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
		*(s++) = 0xe0 | (c >> 12);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c <= 0x10ffff) {
		*(s++) = 0xf0 | (c >> 18);
		*(s++) = 0x80 | ((c >> 12) & 0x3f);
		*(s++) = 0x80 | ((c >> 6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	*out = s;
}


uint32 
BUnicodeChar::FromUTF8(const char **in)
{
	uint8 *bytes = (uint8 *)*in;
	if (bytes == NULL)
		return 0;

	int32 length;
	uint8 mask = 0x1f;

	switch (bytes[0] & 0xf0) {
		case 0xc0:
		case 0xd0:	length = 2; break;
		case 0xe0:	length = 3; break;
		case 0xf0:
			mask = 0x0f;
			length = 4;
			break;
		default:
			// valid 1-byte character
			// and invalid characters
			(*in)++;
			return bytes[0];
	}
	uint32 c = bytes[0] & mask;
	int32 i = 1;
	for (;i < length && (bytes[i] & 0x80) > 0;i++)
		c = (c << 6) | (bytes[i] & 0x3f);

	if (i < length) {
		// invalid character
		(*in)++;
		return (uint32)bytes[0];
	}
	*in += length;
	return c;
}

size_t
BUnicodeChar::UTF8StringLength(const char *str)
{
	size_t len = 0;
	while (*str) {
		FromUTF8(&str);
		len++;
	}
	return len;
}

size_t
BUnicodeChar::UTF8StringLength(const char *str, size_t maxLength)
{
	size_t len = 0;
	while (len < maxLength && *str) {
		FromUTF8(&str);
		len++;
	}
	return len;
}

