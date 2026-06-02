/*
 * Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef _DOSFS_ENCODINGS_H_
#define _DOSFS_ENCODINGS_H_


#ifndef FS_SHELL
#include <ByteOrder.h>
#else // FS_SHELL
typedef unsigned char uchar;

#include "sys/types.h"
#include "fssh_api_wrapper.h"
#endif // FS_SHELL


#ifdef __cplusplus
extern "C" {
#endif


#define BEGINS_UTF8CHAR(byte) (((byte) & 0xc0) != 0x80)

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define UTF16HE_TO_UTF8(...) utf16le_to_utf8(__VA_ARGS__)
#define UTF8_TO_UTF16HE(...) utf8_to_utf16le(__VA_ARGS__)
#else
#define UTF16HE_TO_UTF8(...) utf16be_to_utf8(__VA_ARGS__)
#define UTF8_TO_UTF16HE(...) utf8_to_utf16be(__VA_ARGS__)
#endif // BYTE_ORDER

// Evaluates to true if the given host-endian UTF-16 unit is one of a pair of units
// that together represent a single character.
#define is_high_surrogate(word) ((word) >= 0xD800 && (word) <= 0xDBFF)
#define is_low_surrogate(word) ((word) >= 0xDC00 && (word) <= 0xDFFF)


	/*!	Returns true if \a c is the first byte of a multibyte UTF-8 character. Returns false if
		\a c is a continuation byte or a single-byte character.
	*/
	static inline bool begins_utf8_multibyte_char(u_char c)
	{
		if (BEGINS_UTF8CHAR(c) && ((c & 0x80) != 0))
			return true;
		else
			return false;
	}


	/*!	Determine the byte length of a UTF-8 character, based on the value of the initial byte.
		@return If \a c is a valid initial UTF-8 byte, returns the advertised character length;
		otherwise, returns 1 to indicate that the client should treat this as an invalid character
		of length one.
	*/
	static inline uint32 utf8_char_len(u_char c)
	{
		if (c < 0xC0)
			return 1;
		else if (c < 0xE0)
			return 2;
		else if (c < 0xF0)
			return 3;
		else if (c < 0xF8)
			return 4;
		else if (c < 0xFC)
			return 5;
		else
			return 6;
	}


	/*! Unlike utf8_char_len, if continuation bytes are missing, this does not count the missing
		bytes. If using the count to advance a pointer, this avoids skipping over possibly valid
		bytes that follow malformed UTF-8.
		@param limit Number of bytes left in the buffer, including \a c. This ensures we don't read
		past the end of the buffer if continuation bytes are missing.
	*/
	static inline uint32 utf8_corrected_char_len(const u_char* c, uint32 limit)
	{
		uint32 len = utf8_char_len(*c);
		if (len != 1) {
			uint32 i;
			for (i = 1; i < len; ++i) {
				if (i >= limit || BEGINS_UTF8CHAR(c[i])) {
					len = i;
					break;
				}
			}
		}

		return len;
	}


	static inline bool is_unicode_japanese(uint16 c)
	{
		if (((c >= 0x3000) && (c <= 0x30ff)) ||
				((c >= 0x3200) && (c <= 0x3400)) ||
				((c >= 0x4e00) && (c <= 0x9fff)) ||
				((c >= 0xf900) && (c <= 0xfaff)) ||
				((c >= 0xfe30) && (c <= 0xfe6f)) ||
				((c >= 0xff00) && (c <= 0xffef))) {
			return true;
		}

		return false;
	}


	static inline bool is_initial_sjis_byte(uchar c)
	{
		return ((c >= 0x81) && (c <= 0x9F)) || ((c >= 0xE0) && (c <= 0xEF));
	}


	status_t generate_short_name_sjis(const uchar* utf8, uchar nshort[11]);
	int sjis_to_utf8(uchar* sjis, uchar* utf8, int toLower);
	bool utf8_contains_japanese(const u_char* utf8, size_t utf8Len);
	int utf8_to_winentry(const u_char* utf8Str, size_t utf8Len, struct winentry* entry, int count,
		int checkSum, struct msdosfsmount* fatVolume, uint16_t* unpaired, size_t* totalParsed);
	int winentry_to_utf8(struct mbnambuf* nameBuf, const struct winentry* entry, int chkSum,
		uint16* unpaired);
	bool possibly_miscoded_longname(uint8* longName, size_t nameUnits);
	status_t fix_miscoded_longname(char* name, size_t nameLength, size_t bufferSize);
	status_t fix_miscoded_mbnambuf(struct mbnambuf* mbnambuf);

#ifdef __cplusplus
}
#endif

#endif // _DOSFS_ENCODINGS_H_
