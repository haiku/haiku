/* mail util - header parsing
**
** Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <UTF8.h>
#include <Message.h>
#include <String.h>
#include <Locker.h>
#include <DataIO.h>
#include <List.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define __USE_GNU
#include <regex.h>
#include <ctype.h>
#include <errno.h>
#include <parsedate.h>

#include <mail_encoding.h>

#include <mail_util.h>

#include <CharacterSet.h>
#include <CharacterSetRoster.h>

using namespace BPrivate;

#define CRLF   "\r\n"

struct CharsetConversionEntry
{
	const char *charset;
	uint32 flavor;
};

extern const CharsetConversionEntry mail_charsets [] =
{
	// In order of authority, so when searching for the name for a particular
	// numbered conversion, start at the beginning of the array.
	{"iso-8859-1",  B_ISO1_CONVERSION}, // MIME STANDARD
	{"iso-8859-2",  B_ISO2_CONVERSION}, // MIME STANDARD
	{"iso-8859-3",  B_ISO3_CONVERSION}, // MIME STANDARD
	{"iso-8859-4",  B_ISO4_CONVERSION}, // MIME STANDARD
	{"iso-8859-5",  B_ISO5_CONVERSION}, // MIME STANDARD
	{"iso-8859-6",  B_ISO6_CONVERSION}, // MIME STANDARD
	{"iso-8859-7",  B_ISO7_CONVERSION}, // MIME STANDARD
	{"iso-8859-8",  B_ISO8_CONVERSION}, // MIME STANDARD
	{"iso-8859-9",  B_ISO9_CONVERSION}, // MIME STANDARD
	{"iso-8859-10", B_ISO10_CONVERSION}, // MIME STANDARD
	{"iso-8859-13", B_ISO13_CONVERSION}, // MIME STANDARD
	{"iso-8859-14", B_ISO14_CONVERSION}, // MIME STANDARD
	{"iso-8859-15", B_ISO15_CONVERSION}, // MIME STANDARD

	{"shift_jis",	B_SJIS_CONVERSION}, // MIME STANDARD
	{"shift-jis",	B_SJIS_CONVERSION},
	{"iso-2022-jp", B_JIS_CONVERSION}, // MIME STANDARD
	{"euc-jp",		B_EUC_CONVERSION}, // MIME STANDARD

	{"euc-kr",      B_EUC_KR_CONVERSION}, // Shift encoding 7 bit and KSC-5601 if bit 8 is on. // MIME STANDARD
	{"ksc5601",		B_EUC_KR_CONVERSION},    // Not sure if 7 or 8 bit. // COMPATIBLE?
	{"ks_c_5601-1987", B_EUC_KR_CONVERSION}, // Not sure if 7 or 8 bit. // COMPATIBLE with stupid MS software

	{"koi8-r",      B_KOI8R_CONVERSION},           // MIME STANDARD
	{"windows-1251",B_MS_WINDOWS_1251_CONVERSION}, // MIME STANDARD
	{"windows-1252",B_MS_WINDOWS_CONVERSION},      // MIME STANDARD

	{"dos-437",     B_MS_DOS_CONVERSION},     // WRONG NAME : MIME STANDARD NAME = NONE ( IBM437? )
	{"dos-866",     B_MS_DOS_866_CONVERSION}, // WRONG NAME : MIME STANDARD NAME = NONE ( IBM866? )
	{"x-mac-roman", B_MAC_ROMAN_CONVERSION},  // WRONG NAME : MIME STANDARD NAME = NONE ( macintosh? + x-mac-roman? )

    {"big5",        24}, // MIME STANDARD

    {"gb18030",     25}, // WRONG NAME : MIME STANDARD NAME = NONE ( GB18030? )
    {"gb2312",      25}, // COMPATIBLE
    {"gbk",         25}, // COMPATIBLE

	/* {"utf-16",		B_UNICODE_CONVERSION}, Might not work due to NULs in text, needs testing. */
	{"us-ascii",	B_MAIL_US_ASCII_CONVERSION},                                  // MIME STANDARD
	{"utf-8",		B_MAIL_UTF8_CONVERSION /* Special code for no conversion */}, // MIME STANDARD

	{NULL, (uint32) -1} /* End of list marker, NULL string pointer is the key. */
};


status_t
write_read_attr(BNode& node, read_flags flag)
{
	if (node.WriteAttr(B_MAIL_ATTR_READ, B_INT32_TYPE, 0, &flag, sizeof(int32))
		< 0)
		return B_ERROR;

#if R5_COMPATIBLE
	// manage the status string only if it currently has a "read" status
	BString currentStatus;
	if (node.ReadAttrString(B_MAIL_ATTR_STATUS, &currentStatus) == B_OK) {
		if (currentStatus.ICompare("New") != 0
			&& currentStatus.ICompare("Read") != 0
			&& currentStatus.ICompare("Seen") != 0)
			return B_OK;
	}

	const char* statusString = (flag == B_READ) ? "Read"
		: (flag  == B_SEEN) ? "Seen" : "New";
	if (node.WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, statusString,
		strlen(statusString)) < 0)
		return B_ERROR;
#endif
	return B_OK;
}


status_t
read_read_attr(BNode& node, read_flags& flag)
{
	if (node.ReadAttr(B_MAIL_ATTR_READ, B_INT32_TYPE, 0, &flag, sizeof(int32))
		== sizeof(int32))
		return B_OK;

#if R5_COMPATIBLE
	BString statusString;
	if (node.ReadAttrString(B_MAIL_ATTR_STATUS, &statusString) == B_OK) {
		if (statusString.ICompare("New"))
			flag = B_UNREAD;
		else
			flag = B_READ;

		return B_OK;
	}
#endif
	return B_ERROR;
}


// The next couple of functions are our wrapper around convert_to_utf8 and
// convert_from_utf8 so that they can also convert from UTF-8 to UTF-8 by
// specifying the B_MAIL_UTF8_CONVERSION constant as the conversion operation.  It
// also lets us add new conversions, like B_MAIL_US_ASCII_CONVERSION.

_EXPORT status_t mail_convert_to_utf8 (
	uint32 srcEncoding,
	const char *src,
	int32 *srcLen,
	char *dst,
	int32 *dstLen,
	int32 *state,
	char substitute)
{
	int32    copyAmount;
	char    *originalDst = dst;
	status_t returnCode = -1;

	if (srcEncoding == B_MAIL_UTF8_CONVERSION) {
		copyAmount = *srcLen;
		if (*dstLen < copyAmount)
			copyAmount = *dstLen;
		memcpy (dst, src, copyAmount);
		*srcLen = copyAmount;
		*dstLen = copyAmount;
		returnCode = B_OK;
	} else if (srcEncoding == B_MAIL_US_ASCII_CONVERSION) {
		int32 i;
		unsigned char letter;
		copyAmount = *srcLen;
		if (*dstLen < copyAmount)
			copyAmount = *dstLen;
		for (i = 0; i < copyAmount; i++) {
			letter = *src++;
			if (letter > 0x80U)
				// Invalid, could also use substitute, but better to strip high bit.
				*dst++ = letter - 0x80U;
			else if (letter == 0x80U)
				// Can't convert to 0x00 since that's NUL, which would cause problems.
				*dst++ = substitute;
			else
				*dst++ = letter;
		}
		*srcLen = copyAmount;
		*dstLen = copyAmount;
		returnCode = B_OK;
	} else
		returnCode = convert_to_utf8 (srcEncoding, src, srcLen,
			dst, dstLen, state, substitute);

	if (returnCode == B_OK) {
		// Replace spurious NUL bytes, which should normally not be in the
		// output of the decoding (not normal UTF-8 characters, and no NULs are
		// in our usual input strings).  They happen for some odd ISO-2022-JP
		// byte pair combinations which are improperly handled by the BeOS
		// routines.  Like "\e$ByD\e(B" where \e is the ESC character $1B, the
		// first ESC $ B switches to a Japanese character set, then the next
		// two bytes "yD" specify a character, then ESC ( B switches back to
		// the ASCII character set.  The UTF-8 conversion yields a NUL byte.
		int32 i;
		for (i = 0; i < *dstLen; i++)
			if (originalDst[i] == 0)
				originalDst[i] = substitute;
	}
	return returnCode;
}


_EXPORT status_t mail_convert_from_utf8 (
	uint32 dstEncoding,
	const char *src,
	int32 *srcLen,
	char *dst,
	int32 *dstLen,
	int32 *state,
	char substitute)
{
	int32		copyAmount;
	status_t	errorCode;
	int32		originalDstLen = *dstLen;
	int32		tempDstLen;
	int32		tempSrcLen;

	if (dstEncoding == B_MAIL_UTF8_CONVERSION)
	{
		copyAmount = *srcLen;
		if (*dstLen < copyAmount)
			copyAmount = *dstLen;
		memcpy (dst, src, copyAmount);
		*srcLen = copyAmount;
		*dstLen = copyAmount;
		return B_OK;
	}

	if (dstEncoding == B_MAIL_US_ASCII_CONVERSION)
	{
		int32			characterLength;
		int32			dstRemaining = *dstLen;
		unsigned char	letter;
		int32			srcRemaining = *srcLen;

		// state contains the number of source bytes to skip, left over from a
		// partial UTF-8 character split over the end of the buffer from last
		// time.
		if (srcRemaining <= *state) {
			*state -= srcRemaining;
			*dstLen = 0;
			return B_OK;
		}
		srcRemaining -= *state;
		src += *state;
		*state = 0;

		while (true) {
			if (srcRemaining <= 0 || dstRemaining <= 0)
				break;
			letter = *src;
			if (letter < 0x80)
				characterLength = 1; // Regular ASCII equivalent code.
			else if (letter < 0xC0)
				characterLength = 1; // Invalid in-between data byte 10xxxxxx.
			else if (letter < 0xE0)
				characterLength = 2;
			else if (letter < 0xF0)
				characterLength = 3;
			else if (letter < 0xF8)
				characterLength = 4;
			else if (letter < 0xFC)
				characterLength = 5;
			else if (letter < 0xFE)
				characterLength = 6;
			else
				characterLength = 1; // 0xFE and 0xFF are invalid in UTF-8.
			if (letter < 0x80)
				*dst++ = *src;
			else
				*dst++ = substitute;
			dstRemaining--;
			if (srcRemaining < characterLength) {
				// Character split past the end of the buffer.
				*state = characterLength - srcRemaining;
				srcRemaining = 0;
			} else {
				src += characterLength;
				srcRemaining -= characterLength;
			}
		}
		// Update with the amounts used.
		*srcLen = *srcLen - srcRemaining;
		*dstLen = *dstLen - dstRemaining;
		return B_OK;
	}

	errorCode = convert_from_utf8 (dstEncoding, src, srcLen, dst, dstLen, state, substitute);
	if (errorCode != B_OK)
		return errorCode;

	if (dstEncoding != B_JIS_CONVERSION)
		return B_OK;

	// B_JIS_CONVERSION (ISO-2022-JP) works by shifting between different
	// character subsets.  For E-mail headers (and other uses), it needs to be
	// switched back to ASCII at the end (otherwise the last character gets
	// lost or other weird things happen in the headers).  Note that we can't
	// just append the escape code since the convert_from_utf8 "state" will be
	// wrong.  So we append an ASCII letter and throw it away, leaving just the
	// escape code.  Well, it actually switches to the Roman character set, not
	// ASCII, but that should be OK.

	tempDstLen = originalDstLen - *dstLen;
	if (tempDstLen < 3) // Not enough space remaining in the output.
		return B_OK; // Sort of an error, but we did convert the rest OK.
	tempSrcLen = 1;
	errorCode = convert_from_utf8 (dstEncoding, "a", &tempSrcLen,
		dst + *dstLen, &tempDstLen, state, substitute);
	if (errorCode != B_OK)
		return errorCode;
	*dstLen += tempDstLen - 1 /* don't include the ASCII letter */;
	return B_OK;
}



static int handle_non_rfc2047_encoding(char **buffer,size_t *bufferLength,size_t *sourceLength)
{
	char *string = *buffer;
	int32 length = *sourceLength;
	int32 i;

	// check for 8-bit characters
	for (i = 0;i < length;i++)
		if (string[i] & 0x80)
			break;
	if (i == length)
		return false;

	// check for groups of 8-bit characters - this code is not very smart;
	// it just can detect some sort of single-byte encoded stuff, the rest
	// is regarded as UTF-8

	int32 singletons = 0,doubles = 0;

	for (i = 0;i < length;i++)
	{
		if (string[i] & 0x80)
		{
			if ((string[i + 1] & 0x80) == 0)
				singletons++;
			else doubles++;
			i++;
		}
	}

	if (singletons != 0)	// can't be valid UTF-8 anymore, so we assume ISO-Latin-1
	{
		int32 state = 0;
		// just to be sure
		int32 destLength = length * 4 + 1;
		int32 destBufferLength = destLength;
		char *dest = (char*)malloc(destLength);
		if (dest == NULL)
			return 0;

		if (convert_to_utf8(B_ISO1_CONVERSION, string, &length,dest,
			&destLength, &state) == B_OK) {
			*buffer = dest;
			*bufferLength = destBufferLength;
			*sourceLength = destLength;
			return true;
		}
		free(dest);
		return false;
	}

	// we assume a valid UTF-8 string here, but yes, we don't check it
	return true;
}


_EXPORT ssize_t rfc2047_to_utf8(char **bufp, size_t *bufLen, size_t strLen)
{
	char *head, *tail;
	char *charset, *encoding, *end;
	ssize_t ret = B_OK;

	if (bufp == NULL || *bufp == NULL)
		return -1;

	char *string = *bufp;
	
	//---------Handle *&&^%*&^ non-RFC compliant, 8bit mail
	if (handle_non_rfc2047_encoding(bufp,bufLen,&strLen))
		return strLen;

	// set up string length
	if (strLen == 0)
		strLen = strlen(*bufp);
	char lastChar = (*bufp)[strLen];
	(*bufp)[strLen] = '\0';

	//---------Whew! Now for RFC compliant mail
	bool encodedWordFoundPreviously = false;
	for (head = tail = string;
		((charset = strstr(tail, "=?")) != NULL)
		&& (((encoding = strchr(charset + 2, '?')) != NULL)
			&& encoding[1] && (encoding[2] == '?') && encoding[3])
		&& (end = strstr(encoding + 3, "?=")) != NULL;
		// found "=?...charset...?e?...text...?=   (e == encoding)
		//        ^charset       ^encoding    ^end
		tail = end)
	{
		// Copy non-encoded text (from tail up to charset) to the output.
		// Ignore spaces between two encoded "words".  RFC2047 says the words
		// should be concatenated without the space (designed for Asian
		// sentences which have no spaces yet need to be broken into "words" to
		// keep within the line length limits).
		bool nonSpaceFound = false;
		for (int i = 0; i < charset-tail; i++) {
			if (!isspace (tail[i])) {
				nonSpaceFound = true;
				break;
			}
		}
		if (!encodedWordFoundPreviously || nonSpaceFound) {
			if (string != tail && tail != charset)
				memmove(string, tail, charset-tail);
			string += charset-tail;
		}
		tail = charset;
		encodedWordFoundPreviously = true;

		// move things to point at what they should:
		//   =?...charset...?e?...text...?=   (e == encoding)
		//     ^charset      ^encoding     ^end
		charset += 2;
		encoding += 1;
		end += 2;

		// find the charset this text is in now
		size_t		cLen = encoding - 1 - charset;
		bool		base64encoded = toupper(*encoding) == 'B';

		uint32 convert_id = B_MAIL_NULL_CONVERSION;
		char charset_string[cLen+1];
		memcpy(charset_string, charset, cLen);
		charset_string[cLen] = '\0';
		if (strcasecmp(charset_string, "us-ascii") == 0) {
			convert_id = B_MAIL_US_ASCII_CONVERSION;
		} else if (strcasecmp(charset_string, "utf-8") == 0) {
			convert_id = B_MAIL_UTF8_CONVERSION;
		} else {
			const BCharacterSet * cs = BCharacterSetRoster::FindCharacterSetByName(charset_string);
			if (cs != NULL) {
				convert_id = cs->GetConversionID();
			}
		}
		if (convert_id == B_MAIL_NULL_CONVERSION)
		{
			// unidentified charset
			// what to do? doing nothing skips the encoded text;
			// but we should keep it: we copy it to the output.
			if (string != tail && tail != end)
				memmove(string, tail, end-tail);
			string += end-tail;
			continue;
		}
		// else we've successfully identified the charset

		char *src = encoding+2;
		int32 srcLen = end - 2 - src;
		// encoded text: src..src+srcLen

		// decode text, get decoded length (reducing xforms)
		srcLen = !base64encoded ? decode_qp(src, src, srcLen, 1)
				: decode_base64(src, src, srcLen);

		// allocate space for the converted text
		int32 dstLen = end-string + *bufLen-strLen;
		char *dst = (char*)malloc(dstLen);
		int32 cvLen = srcLen;
		int32 convState = 0;

		//
		// do the conversion
		//
		ret = mail_convert_to_utf8(convert_id, src, &cvLen, dst, &dstLen, &convState);
		if (ret != B_OK)
		{
			// what to do? doing nothing skips the encoded text
			// but we should keep it: we copy it to the output.

			free(dst);

			if (string != tail && tail != end)
				memmove(string, tail, end-tail);
			string += end-tail;
			continue;
		}
		/* convert_to_ is either returning something wrong or my
		   test data is screwed up.  Whatever it is, Not Enough
		   Space is not the only cause of the below, so we just
		   assume it succeeds if it converts anything at all.
		else if (cvLen < srcLen)
		{
			// not enough room to convert the data;
			// grow *buf and retry

			free(dst);

			char *temp = (char*)realloc(*bufp, 2*(*bufLen + 1));
			if (temp == NULL)
			{
				ret = B_NO_MEMORY;
				break;
			}

			*bufp = temp;
			*bufLen = 2*(*bufLen + 1);

			string = *bufp + (string-head);
			tail = *bufp + (tail-head);
			charset = *bufp + (charset-head);
			encoding = *bufp + (encoding-head);
			end = *bufp + (end-head);
			src = *bufp + (src-head);
			head = *bufp;
			continue;
		}
		*/
		else
		{
			if (dstLen > end-string)
			{
				// copy the string forward...
				memmove(string+dstLen, end, strLen - (end-head) + 1);
				strLen += string+dstLen - end;
				end = string + dstLen;
			}

			memcpy(string, dst, dstLen);
			string += dstLen;
			free(dst);
			continue;
		}
	}

	// copy everything that's left
	size_t tailLen = strLen - (tail - head);
	memmove(string, tail, tailLen+1);
	string += tailLen;

	// replace the last char
	(*bufp)[strLen] = lastChar;

	return ret < B_OK ? ret : string-head;
}


_EXPORT ssize_t utf8_to_rfc2047 (char **bufp, ssize_t length, uint32 charset, char encoding) {
	struct word {
		BString	originalWord;
		BString	convertedWord;
		bool	needsEncoding;

		// Convert the word from UTF-8 to the desired character set.  The
		// converted version also includes the escape codes to return to ASCII
		// mode, if relevant.  Also note if it uses unprintable characters,
		// which means it will need that special encoding treatment later.
		void ConvertWordToCharset (uint32 charset) {
			int32 state = 0;
			int32 originalLength = originalWord.Length();
			int32 convertedLength = originalLength * 5 + 1;
			char *convertedBuffer = convertedWord.LockBuffer (convertedLength);
			mail_convert_from_utf8 (charset, originalWord.String(),
				&originalLength, convertedBuffer, &convertedLength, &state);
			for (int i = 0; i < convertedLength; i++) {
				if ((convertedBuffer[i] & (1 << 7)) ||
					(convertedBuffer[i] >= 0 && convertedBuffer[i] < 32)) {
					needsEncoding = true;
					break;
				}
			}
			convertedWord.UnlockBuffer (convertedLength);
		};
	};
	struct word *currentWord;
	BList words;

	// Break the header into words.  White space characters (including tabs and
	// newlines) separate the words.  Each word includes any space before it as
	// part of the word.  Actually, quotes and other special characters
	// (",()<>@) are treated as separate words of their own so that they don't
	// get encoded (because MIME headers get the quotes parsed before character
	// set unconversion is done).  The reader is supposed to ignore all white
	// space between encoded words, which can be inserted so that older mail
	// parsers don't have overly long line length problems.

	const char *source = *bufp;
	const char *bufEnd = *bufp + length;
	const char *specialChars = "\"()<>@,";

	while (source < bufEnd) {
		currentWord = new struct word;
		currentWord->needsEncoding = false;

		int wordEnd = 0;

		// Include leading spaces as part of the word.
		while (source + wordEnd < bufEnd && isspace (source[wordEnd]))
			wordEnd++;

		if (source + wordEnd < bufEnd &&
			strchr (specialChars, source[wordEnd]) != NULL) {
			// Got a quote mark or other special character, which is treated as
			// a word in itself since it shouldn't be encoded, which would hide
			// it from the mail system.
			wordEnd++;
		} else {
			// Find the end of the word.  Leave wordEnd pointing just after the
			// last character in the word.
			while (source + wordEnd < bufEnd) {
				if (isspace(source[wordEnd]) ||
					strchr (specialChars, source[wordEnd]) != NULL)
					break;
				if (wordEnd > 51 /* Makes Base64 ISO-2022-JP "word" a multiple of 4 bytes */ &&
					0xC0 == (0xC0 & (unsigned int) source[wordEnd])) {
					// No English words are that long (46 is the longest),
					// break up what is likely Asian text (which has no spaces)
					// at the start of the next non-ASCII UTF-8 character (high
					// two bits are both ones).  Note that two encoded words in
					// a row get joined together, even if there is a space
					// between them in the final output text, according to the
					// standard.  Next word will also be conveniently get
					// encoded due to the 0xC0 test.
					currentWord->needsEncoding = true;
					break;
				}
				wordEnd++;
			}
		}
		currentWord->originalWord.SetTo (source, wordEnd);
		currentWord->ConvertWordToCharset (charset);
		words.AddItem(currentWord);
		source += wordEnd;
	}

	// Combine adjacent words which contain unprintable text so that the
	// overhead of switching back and forth between regular text and specially
	// encoded text is reduced.  However, the combined word must be shorter
	// than the maximum of 75 bytes, including character set specification and
	// all those delimiters (worst case 22 bytes of overhead).

	struct word *run;

	for (int32 i = 0; (currentWord = (struct word *) words.ItemAt (i)) != NULL; i++) {
		if (!currentWord->needsEncoding)
			continue; // No need to combine unencoded words.
		for (int32 g = i+1; (run = (struct word *) words.ItemAt (g)) != NULL; g++) {
			if (!run->needsEncoding)
				break; // Don't want to combine encoded and unencoded words.
			if ((currentWord->convertedWord.Length() + run->convertedWord.Length() <= 53)) {
				currentWord->originalWord.Append (run->originalWord);
				currentWord->ConvertWordToCharset (charset);
				words.RemoveItem(g);
				delete run;
				g--;
			} else // Can't merge this word, result would be too long.
				break;
		}
	}

	// Combine the encoded and unencoded words into one line, doing the
	// quoted-printable or base64 encoding.  Insert an extra space between
	// words which are both encoded to make word wrapping easier, since there
	// is normally none, and you're allowed to insert space (the receiver
	// throws it away if it is between encoded words).

	BString rfc2047;
	bool	previousWordNeededEncoding = false;

	const char *charset_dec = "none-bug";
	for (int32 i = 0; mail_charsets[i].charset != NULL; i++) {
		if (mail_charsets[i].flavor == charset) {
			charset_dec = mail_charsets[i].charset;
			break;
		}
	}

	while ((currentWord = (struct word *)words.RemoveItem(0L)) != NULL) {
		if ((encoding != quoted_printable && encoding != base64) ||
		!currentWord->needsEncoding) {
			rfc2047.Append (currentWord->convertedWord);
		} else {
			// This word needs encoding.  Try to insert a space between it and
			// the previous word.
			if (previousWordNeededEncoding)
				rfc2047 << ' '; // Can insert as many spaces as you want between encoded words.
			else {
				// Previous word is not encoded, spaces are significant.  Try
				// to move a space from the start of this word to be outside of
				// the encoded text, so that there is a bit of space between
				// this word and the previous one to enhance word wrapping
				// chances later on.
				if (currentWord->originalWord.Length() > 1 &&
					isspace (currentWord->originalWord[0])) {
					rfc2047 << currentWord->originalWord[0];
					currentWord->originalWord.Remove (0 /* offset */, 1 /* length */);
					currentWord->ConvertWordToCharset (charset);
				}
			}

			char *encoded = NULL;
			ssize_t encoded_len = 0;
			int32 convertedLength = currentWord->convertedWord.Length ();
			const char *convertedBuffer = currentWord->convertedWord.String ();

			switch (encoding) {
				case quoted_printable:
					encoded = (char *) malloc (convertedLength * 3);
					encoded_len = encode_qp (encoded, convertedBuffer, convertedLength, true /* headerMode */);
					break;
				case base64:
					encoded = (char *) malloc (convertedLength * 2);
					encoded_len = encode_base64 (encoded, convertedBuffer, convertedLength, true /* headerMode */);
					break;
				default: // Unknown encoding type, shouldn't happen.
					encoded = (char *) convertedBuffer;
					encoded_len = convertedLength;
					break;
			}

			rfc2047 << "=?" << charset_dec << '?' << encoding << '?';
			rfc2047.Append (encoded, encoded_len);
			rfc2047 << "?=";

			if (encoding == quoted_printable || encoding == base64)
				free(encoded);
		}
		previousWordNeededEncoding = currentWord->needsEncoding;
		delete currentWord;
	}

	free(*bufp);

	ssize_t finalLength = rfc2047.Length ();
	*bufp = (char *) (malloc (finalLength + 1));
	memcpy (*bufp, rfc2047.String(), finalLength);
	(*bufp)[finalLength] = 0;

	return finalLength;
}


//====================================================================

void FoldLineAtWhiteSpaceAndAddCRLF (BString &string)
{
	int			inputLength = string.Length();
	int			lineStartIndex;
	const int	maxLineLength = 78; // Doesn't include CRLF.
	BString		output;
	int			splitIndex;
	int			tempIndex;

	lineStartIndex = 0;
	while (true) {
		// If we don't need to wrap the text, just output the remainder, if any.

		if (lineStartIndex + maxLineLength >= inputLength) {
			if (lineStartIndex < inputLength) {
				output.Insert (string, lineStartIndex /* source offset */,
					inputLength - lineStartIndex /* count */,
					output.Length() /* insert at */);
				output.Append (CRLF);
			}
			break;
		}

		// Look ahead for a convenient spot to split it, between a comma and
		// space, which you often see between e-mail addresses like this:
		// "Joe Who" joe@dot.com, "Someone Else" else@blot.com

		tempIndex = lineStartIndex + maxLineLength;
		if (tempIndex > inputLength)
			tempIndex = inputLength;
		splitIndex = string.FindLast (", ", tempIndex);
		if (splitIndex >= lineStartIndex)
			splitIndex++; // Point to the space character.

		// If none of those exist, try splitting at any white space.

		if (splitIndex <= lineStartIndex)
			splitIndex = string.FindLast (" ", tempIndex);
		if (splitIndex <= lineStartIndex)
			splitIndex = string.FindLast ("\t", tempIndex);

		// If none of those exist, allow for a longer word - split at the next
		// available white space.

		if (splitIndex <= lineStartIndex)
			splitIndex = string.FindFirst (" ", lineStartIndex + 1);
		if (splitIndex <= lineStartIndex)
			splitIndex = string.FindFirst ("\t", lineStartIndex + 1);

		// Give up, the whole rest of the line can't be split, just dump it
		// out.

		if (splitIndex <= lineStartIndex) {
			if (lineStartIndex < inputLength) {
				output.Insert (string, lineStartIndex /* source offset */,
					inputLength - lineStartIndex /* count */,
					output.Length() /* insert at */);
				output.Append (CRLF);
			}
			break;
		}

		// Do the split.  The current line up to but not including the space
		// gets output, followed by a CRLF.  The space remains to become the
		// start of the next line (and that tells the message reader that it is
		// a continuation line).

		output.Insert (string, lineStartIndex /* source offset */,
			splitIndex - lineStartIndex /* count */,
			output.Length() /* insert at */);
		output.Append (CRLF);
		lineStartIndex = splitIndex;
	}
	string.SetTo (output);
}


//====================================================================

_EXPORT ssize_t readfoldedline(FILE *file, char **buffer, size_t *buflen)
{
	ssize_t len = buflen && *buflen ? *buflen : 0;
	char * buf = buffer && *buffer ? *buffer : NULL;
	ssize_t cnt = 0; // Number of characters currently in the buffer.
	int c;

	while (true)
	{
		// Make sure there is space in the buffer for two more characters (one
		// for the next character, and one for the end of string NUL byte).
		if (buf == NULL || cnt + 2 >= len)
		{
			char *temp = (char *)realloc(buf, len + 64);
			if (temp == NULL) {
				// Out of memory, however existing buffer remains allocated.
				cnt = ENOMEM;
				break;
			}
			len += 64;
			buf = temp;
		}

		// Read the next character, or end of file, or IO error.
		if ((c = fgetc(file)) == EOF) {
			if (ferror (file)) {
				cnt = errno;
				if (cnt >= 0)
					cnt = -1; // Error codes must be negative.
			} else {
				// Really is end of file.  Also make it end of line if there is
				// some text already read in.  If the first thing read was EOF,
				// just return an empty string.
				if (cnt > 0) {
					buf[cnt++] = '\n';
					if (buf[cnt-2] == '\r') {
						buf[cnt-2] = '\n';
						--cnt;
					}
				}
			}
			break;
		}

		buf[cnt++] = c;

		if (c == '\n') {
			// Convert CRLF end of line to just a LF.  Do it before folding, in
			// case we don't need to fold.
			if (cnt >= 2 && buf[cnt-2] == '\r') {
				buf[cnt-2] = '\n';
				--cnt;
			}
			// If the current line is empty then return it (so that empty lines
			// don't disappear if the next line starts with a space).
			if (cnt <= 1)
				break;
			// Fold if first character on the next line is whitespace.
			c = fgetc(file); // Note it's OK to read EOF and ungetc it too.
			if (c == ' ' || c == '\t')
				buf[cnt-1] = c; // Replace \n with the white space character.
			else {
				// Not folding, we finished reading a line; break out of the loop
				ungetc(c,file);
				break;
			}
		}
	}


	if (buf != NULL && cnt >= 0)
		buf[cnt] = '\0';

	if (buffer)
		*buffer = buf;
	else if (buf)
		free(buf);

	if (buflen)
		*buflen = len;

	return cnt;
}


//====================================================================

_EXPORT ssize_t readfoldedline(BPositionIO &in, char **buffer, size_t *buflen)
{
	ssize_t len = buflen && *buflen ? *buflen : 0;
	char * buf = buffer && *buffer ? *buffer : NULL;
	ssize_t cnt = 0; // Number of characters currently in the buffer.
	char c;
	status_t errorCode;

	while (true)
	{
		// Make sure there is space in the buffer for two more characters (one
		// for the next character, and one for the end of string NUL byte).
		if (buf == NULL || cnt + 2 >= len)
		{
			char *temp = (char *)realloc(buf, len + 64);
			if (temp == NULL) {
				// Out of memory, however existing buffer remains allocated.
				cnt = ENOMEM;
				break;
			}
			len += 64;
			buf = temp;
		}

		errorCode = in.Read (&c,1); // A really slow way of reading - unbuffered.
		if (errorCode != 1) {
			if (errorCode < 0) {
				cnt = errorCode; // IO error encountered, just return the code.
			} else {
				// Really is end of file.  Also make it end of line if there is
				// some text already read in.  If the first thing read was EOF,
				// just return an empty string.
				if (cnt > 0) {
					buf[cnt++] = '\n';
					if (buf[cnt-2] == '\r') {
						buf[cnt-2] = '\n';
						--cnt;
					}
				}
			}
			break;
		}

		buf[cnt++] = c;

		if (c == '\n') {
			// Convert CRLF end of line to just a LF.  Do it before folding, in
			// case we don't need to fold.
			if (cnt >= 2 && buf[cnt-2] == '\r') {
				buf[cnt-2] = '\n';
				--cnt;
			}
			// If the current line is empty then return it (so that empty lines
			// don't disappear if the next line starts with a space).
			if (cnt <= 1)
				break;
			// if first character on the next line is whitespace, fold lines
			errorCode = in.Read(&c,1);
			if (errorCode == 1) {
				if (c == ' ' || c == '\t')
					buf[cnt-1] = c; // Replace \n with the white space character.
				else {
					// Not folding, we finished reading a whole line.
					in.Seek(-1,SEEK_CUR); // Undo the look-ahead character read.
					break;
				}
			} else if (errorCode < 0) {
				cnt = errorCode;
				break;
			} else // No next line; at the end of the file.  Return the line.
				break;
		}
	}

	if (buf != NULL && cnt >= 0)
		buf[cnt] = '\0';

	if (buffer)
		*buffer = buf;
	else if (buf)
		free(buf);

	if (buflen)
		*buflen = len;

	return cnt;
}


_EXPORT ssize_t
nextfoldedline(const char** header, char **buffer, size_t *buflen)
{
	ssize_t len = buflen && *buflen ? *buflen : 0;
	char * buf = buffer && *buffer ? *buffer : NULL;
	ssize_t cnt = 0; // Number of characters currently in the buffer.
	char c;

	while (true)
	{
		// Make sure there is space in the buffer for two more characters (one
		// for the next character, and one for the end of string NUL byte).
		if (buf == NULL || cnt + 2 >= len)
		{
			char *temp = (char *)realloc(buf, len + 64);
			if (temp == NULL) {
				// Out of memory, however existing buffer remains allocated.
				cnt = ENOMEM;
				break;
			}
			len += 64;
			buf = temp;
		}

		// Read the next character, or end of file.
		if ((c = *(*header)++) == 0) {
			// End of file.  Also make it end of line if there is some text
			// already read in.  If the first thing read was EOF, just return
			// an empty string.
			if (cnt > 0) {
				buf[cnt++] = '\n';
				if (buf[cnt-2] == '\r') {
					buf[cnt-2] = '\n';
					--cnt;
				}
			}
			break;
		}

		buf[cnt++] = c;

		if (c == '\n') {
			// Convert CRLF end of line to just a LF.  Do it before folding, in
			// case we don't need to fold.
			if (cnt >= 2 && buf[cnt-2] == '\r') {
				buf[cnt-2] = '\n';
				--cnt;
			}
			// If the current line is empty then return it (so that empty lines
			// don't disappear if the next line starts with a space).
			if (cnt <= 1)
				break;
			// if first character on the next line is whitespace, fold lines
			c = *(*header)++;
			if (c == ' ' || c == '\t')
				buf[cnt-1] = c; // Replace \n with the white space character.
			else {
				// Not folding, we finished reading a line; break out of the loop
				(*header)--; // Undo read of the non-whitespace.
				break;
			}
		}
	}


	if (buf != NULL && cnt >= 0)
		buf[cnt] = '\0';

	if (buffer)
		*buffer = buf;
	else if (buf)
		free(buf);

	if (buflen)
		*buflen = len;

	return cnt;
}


_EXPORT void
trim_white_space(BString &string)
{
	int32 i;
	int32 length = string.Length();
	char *buffer = string.LockBuffer(length + 1);

	while (length > 0 && isspace(buffer[length - 1]))
		length--;
	buffer[length] = '\0';

	for (i = 0; buffer[i] && isspace(buffer[i]); i++) {}
	if (i != 0) {
		length -= i;
		memmove(buffer,buffer + i,length + 1);
	}
	string.UnlockBuffer(length);
}


/** Tries to return a human-readable name from the specified
 *	header parameter (should be from "To:" or "From:").
 *	Tries to return the name rather than the eMail address.
 */

_EXPORT void
extract_address_name(BString &header)
{
	BString name;
	const char *start = header.String();
	const char *stop = start + strlen (start);

	// Find a string S in the header (email foo) that matches:
	//   Old style name in brackets: foo@bar.com (S)
	//   New style quotes: "S" <foo@bar.com>
	//   New style no quotes if nothing else found: S <foo@bar.com>
	//   If nothing else found then use the whole thing: S

	for (int i = 0; i <= 3; i++) {
		// Set p1 to the first letter in the name and p2 to just past the last
		// letter in the name.  p2 stays NULL if a name wasn't found in this
		// pass.
		const char *p1 = NULL, *p2 = NULL;

		switch (i) {
			case 0: // foo@bar.com (S)
				if ((p1 = strchr(start,'(')) != NULL) {
					p1++; // Advance to first letter in the name.
					size_t nest = 1; // Handle nested brackets.
					for (p2 = p1; p2 < stop; ++p2)
					{
						if (*p2 == ')')
							--nest;
						else if (*p2 == '(')
							++nest;
						if (nest <= 0)
							break;
					}
					if (nest != 0)
						p2 = NULL; // False alarm, no terminating bracket.
				}
				break;
			case 1: // "S" <foo@bar.com>
				if ((p1 = strchr(start, '\"')) != NULL)
					p2 = strchr(++p1, '\"');
				break;
			case 2: // S <foo@bar.com>
				p1 = start;
				if (name.Length() == 0)
					p2 = strchr(start, '<');
				break;
			case 3: // S
				p1 = start;
				if (name.Length() == 0)
					p2 = stop;
				break;
		}

		// Remove leading and trailing space-like characters and save the
		// result if it is longer than any other likely names found.
		if (p2 != NULL) {
			while (p1 < p2 && (isspace (*p1)))
				++p1;

			while (p1 < p2 && (isspace (p2[-1])))
				--p2;

			int newLength = p2 - p1;
			if (name.Length() < newLength)
				name.SetTo(p1, newLength);
		}
	}

	int32 lessIndex = name.FindFirst('<');
	int32 greaterIndex = name.FindLast('>');

	if (lessIndex == 0) {
		// Have an address of the form <address> and nothing else, so remove
		// the greater and less than signs, if any.
		if (greaterIndex > 0)
			name.Remove(greaterIndex, 1);
		name.Remove(lessIndex, 1);
	} else if (lessIndex > 0 && lessIndex < greaterIndex) {
		// Yahoo stupidly inserts the e-mail address into the name string, so
		// this bit of code fixes: "Joe <joe@yahoo.com>" <joe@yahoo.com>
		name.Remove(lessIndex, greaterIndex - lessIndex + 1);
	}

	trim_white_space(name);
	header = name;
}



// Given a subject in a BString, remove the extraneous RE: re: and other stuff
// to get down to the core subject string, which should be identical for all
// messages posted about a topic.  The input string is modified in place to
// become the output core subject string.

static int32				gLocker = 0;
static size_t				gNsub = 1;
static re_pattern_buffer	gRe;
static re_pattern_buffer   *gRebuf = NULL;
static unsigned char					gTranslation[256];

_EXPORT void SubjectToThread (BString &string)
{
// a regex that matches a non-ASCII UTF8 character:
#define U8C \
	"[\302-\337][\200-\277]" \
	"|\340[\302-\337][\200-\277]" \
	"|[\341-\357][\200-\277][\200-\277]" \
	"|\360[\220-\277][\200-\277][\200-\277]" \
	"|[\361-\367][\200-\277][\200-\277][\200-\277]" \
	"|\370[\210-\277][\200-\277][\200-\277][\200-\277]" \
	"|[\371-\373][\200-\277][\200-\277][\200-\277][\200-\277]" \
	"|\374[\204-\277][\200-\277][\200-\277][\200-\277][\200-\277]" \
	"|\375[\200-\277][\200-\277][\200-\277][\200-\277][\200-\277]"

#define PATTERN \
	"^ +" \
	"|^(\\[[^]]*\\])(\\<|  +| *(\\<(\\w|" U8C "){2,3} *(\\[[^\\]]*\\])? *:)+ *)" \
	"|^(  +| *(\\<(\\w|" U8C "){2,3} *(\\[[^\\]]*\\])? *:)+ *)" \
	"| *\\(fwd\\) *$"

	if (gRebuf == NULL && atomic_add(&gLocker,1) == 0)
	{
		// the idea is to compile the regexp once to speed up testing

		for (int i=0; i<256; ++i) gTranslation[i]=i;
		for (int i='a'; i<='z'; ++i) gTranslation[i]=toupper(i);

		gRe.translate = gTranslation;
		gRe.regs_allocated = REGS_FIXED;
		re_syntax_options = RE_SYNTAX_POSIX_EXTENDED;

		const char *pattern = PATTERN;
		// count subexpressions in PATTERN
		for (unsigned int i=0; pattern[i] != 0; ++i)
		{
			if (pattern[i] == '\\')
				++i;
			else if (pattern[i] == '(')
				++gNsub;
		}

		const char *err = re_compile_pattern(pattern,strlen(pattern),&gRe);
		if (err == NULL)
			gRebuf = &gRe;
		else
			fprintf(stderr, "Failed to compile the regex: %s\n", err);
	}
	else
	{
		int32 tries = 200;
		while (gRebuf == NULL && tries-- > 0)
			snooze(10000);
	}

	if (gRebuf)
	{
		struct re_registers regs;
		// can't be static if this function is to be thread-safe

		regs.num_regs = gNsub;
		regs.start = (regoff_t*)malloc(gNsub*sizeof(regoff_t));
		regs.end = (regoff_t*)malloc(gNsub*sizeof(regoff_t));

		for (int start=0;
		    (start=re_search(gRebuf, string.String(), string.Length(),
							0, string.Length(), &regs)) >= 0;
			)
		{
			//
			// we found something
			//

			// don't delete [bemaildaemon]...
			if (start == regs.start[1])
				start = regs.start[2];

			string.Remove(start,regs.end[0]-start);
			if (start) string.Insert(' ',1,start);

			// TODO: for some subjects this results in an endless loop, check
			// why this happen.
			if (regs.end[0] - start <= 1)
				break;
		}

		free(regs.start);
		free(regs.end);
	}

	// Finally remove leading and trailing space.  Some software, like
	// tm-edit 1.8, appends a space to the subject, which would break
	// threading if we left it in.
	trim_white_space(string);
}



// Converts a date to a time.  Handles numeric time zones too, unlike
// parsedate.  Returns -1 if it fails.

_EXPORT time_t ParseDateWithTimeZone (const char *DateString)
{
	time_t	currentTime;
	time_t	dateAsTime;
	char	tempDateString [80];
	char	tempZoneString [6];
	time_t	zoneDeltaTime;
	int		zoneIndex;
	char   *zonePntr;

	// See if we can remove the time zone portion.  parsedate understands time
	// zone 3 letter names, but doesn't understand the numeric +9999 time zone
	// format.  To do: see if a newer parsedate exists.

	strncpy (tempDateString, DateString, sizeof (tempDateString));
	tempDateString[sizeof (tempDateString) - 1] = 0;

	// Remove trailing spaces.
	zonePntr = tempDateString + strlen (tempDateString) - 1;
	while (zonePntr >= tempDateString && isspace (*zonePntr))
		*zonePntr-- = 0;
	if (zonePntr < tempDateString)
		return -1; // Empty string.

	// Remove the trailing time zone in round brackets, like in
	// Fri, 22 Feb 2002 15:22:42 EST (-0500)
	// Thu, 25 Apr 1996 11:44:19 -0400 (EDT)
	if (tempDateString[strlen(tempDateString)-1] == ')')
	{
		zonePntr = strrchr (tempDateString, '(');
		if (zonePntr != NULL)
		{
			*zonePntr-- = 0; // Zap the '(', then remove trailing spaces.
			while (zonePntr >= tempDateString && isspace (*zonePntr))
				*zonePntr-- = 0;
			if (zonePntr < tempDateString)
				return -1; // Empty string.
		}
	}
	
	// Look for a numeric time zone like  Tue, 30 Dec 2003 05:01:40 +0000
	for (zoneIndex = strlen (tempDateString); zoneIndex >= 0; zoneIndex--)
	{
		zonePntr = tempDateString + zoneIndex;
		if (zonePntr[0] == '+' || zonePntr[0] == '-')
		{
			if (zonePntr[1] >= '0' && zonePntr[1] <= '9' &&
				zonePntr[2] >= '0' && zonePntr[2] <= '9' &&
				zonePntr[3] >= '0' && zonePntr[3] <= '9' &&
				zonePntr[4] >= '0' && zonePntr[4] <= '9')
				break;
		}
	}
	if (zoneIndex >= 0)
	{
		// Remove the zone from the date string and any following time zone
		// letter codes.  Also put in GMT so that the date gets parsed as GMT.
		memcpy (tempZoneString, zonePntr, 5);
		tempZoneString [5] = 0;
		strcpy (zonePntr, "GMT");
	}
	else // No numeric time zone found.
		strcpy (tempZoneString, "+0000");

	time (&currentTime);
	dateAsTime = parsedate (tempDateString, currentTime);
	if (dateAsTime == (time_t) -1)
		return -1; // Failure.

	zoneDeltaTime = 60 * atol (tempZoneString + 3); // Get the last two digits - minutes.
	tempZoneString[3] = 0;
	zoneDeltaTime += atol (tempZoneString + 1) * 60 * 60; // Get the first two digits - hours.
	if (tempZoneString[0] == '+')
		zoneDeltaTime = 0 - zoneDeltaTime;
	dateAsTime += zoneDeltaTime;

	return dateAsTime;
}


/** Parses a mail header and fills the headers BMessage
 */

_EXPORT status_t
parse_header(BMessage &headers, BPositionIO &input)
{
	char *buffer = NULL;
	size_t bufferSize = 0;
	int32 length;

	while ((length = readfoldedline(input, &buffer, &bufferSize)) >= 2) {
		--length;
			// Don't include the \n at the end of the buffer.

		// convert to UTF-8 and null-terminate the buffer
		length = rfc2047_to_utf8(&buffer, &bufferSize, length);
		buffer[length] = '\0';

		const char *delimiter = strstr(buffer, ":");
		if (delimiter == NULL)
			continue;

		BString header(buffer, delimiter - buffer);
		header.CapitalizeEachWord();
			// unified case for later fetch

		delimiter++; // Skip the colon.
		while (isspace (*delimiter))
			delimiter++; // Skip over leading white space and tabs.  To do: (comments in brackets).

		// ToDo: implement joining of multiple header tags (i.e. multiple "Cc:"s)
		headers.AddString(header.String(), delimiter);
	}
	free(buffer);

	return B_OK;
}


_EXPORT status_t
extract_from_header(const BString& header, const BString& field,
	BString& target)
{
	int32 headerLength = header.Length();
	int32 fieldEndPos = 0;
	while (true) {
		int32 pos = header.IFindFirst(field, fieldEndPos);
		if (pos < 0)
			return B_BAD_VALUE;
		fieldEndPos = pos + field.Length();
		
		if (pos != 0 && header.ByteAt(pos - 1) != '\n')
			continue;
		if (header.ByteAt(fieldEndPos) == ':')
			break;
	}
	fieldEndPos++;

	int32 crPos = fieldEndPos;
	while (true) {
		fieldEndPos = crPos;
		crPos = header.FindFirst('\n', crPos);
		if (crPos < 0)
			crPos = headerLength;
		BString temp;
		header.CopyInto(temp, fieldEndPos, crPos - fieldEndPos);
		if (header.ByteAt(crPos - 1) == '\r') {
			temp.Truncate(temp.Length() - 1);
			temp += " ";
		}
		target += temp;
		crPos++;
		if (crPos >= headerLength)
			break;
		char nextByte = header.ByteAt(crPos);
		if (nextByte != ' ' && nextByte != '\t')
			break;
		crPos++;
	}

	size_t bufferSize = target.Length();
	char* buffer = target.LockBuffer(bufferSize);
	size_t length = rfc2047_to_utf8(&buffer, &bufferSize, bufferSize);
	target.UnlockBuffer(length);

	return B_OK;
}


_EXPORT void
extract_address(BString &address)
{
	const char *string = address.String();
	int32 first;

	// first, remove all quoted text
	
	if ((first = address.FindFirst('"')) >= 0) {
		int32 last = first + 1;
		while (string[last] && string[last] != '"')
			last++;

		if (string[last] == '"')
			address.Remove(first, last + 1 - first);
	}

	// try to extract the address now

	if ((first = address.FindFirst('<')) >= 0) {
		// the world likes us and we can just get the address the easy way...
		int32 last = address.FindFirst('>');
		if (last >= 0) {
			address.Truncate(last);
			address.Remove(0, first + 1);

			return;
		}
	}

	// then, see if there is anything in parenthesis to throw away

	if ((first = address.FindFirst('(')) >= 0) {
		int32 last = first + 1;
		while (string[last] && string[last] != ')')
			last++;

		if (string[last] == ')')
			address.Remove(first, last + 1 - first);
	}

	// now, there shouldn't be much else left

	trim_white_space(address);
}


_EXPORT void
get_address_list(BList &list, const char *string, void (*cleanupFunc)(BString &))
{
	if (string == NULL || !string[0])
		return;

	const char *start = string;

	while (true) {
		if (string[0] == '"') {
			const char *quoteEnd = ++string;

			while (quoteEnd[0] && quoteEnd[0] != '"')
				quoteEnd++;

			if (!quoteEnd[0])	// string exceeds line!
				quoteEnd = string;

			string = quoteEnd + 1;
		}

		if (string[0] == ',' || string[0] == '\0') {
			BString address(start, string - start);
			trim_white_space(address);

			if (cleanupFunc)
				cleanupFunc(address);

			list.AddItem(strdup(address.String()));

			start = string + 1;
		}

		if (!string[0])
			break;

		string++;
	}
}

