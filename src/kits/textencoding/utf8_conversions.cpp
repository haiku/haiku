/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Bachmann
 */


#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <UTF8.h>

#include <errno.h>
#include <iconv.h>
#include <stdio.h>


//#define DEBUG_CONV 1

#ifdef DEBUG_CONV
#	define DEBPRINT(ARGS) printf ARGS;
#else
#	define DEBPRINT(ARGS) ;
#endif

using namespace BPrivate;

int iconvctl(iconv_t icd, int request, void* argument);


status_t
convert_encoding(const char* from, const char* to, const char* src,
	int32* srcLen, char* dst, int32* dstLen, int32* state,
	char substitute)
{
	if (*srcLen == 0) {
		// nothing to do!
		*dstLen = 0;
		return B_OK;
	}

	iconv_t conversion = iconv_open(to, from);
	if (conversion == (iconv_t)-1) {
		DEBPRINT(("iconv_open failed\n"));
		return B_ERROR;
	}
	if (state == NULL || *state == 0)
		iconv(conversion, 0, 0, 0, 0);

	char** inputBuffer = const_cast<char**>(&src);
	size_t inputLeft = *srcLen;
	size_t outputLeft = *dstLen;
	do {
		size_t nonReversibleConversions = iconv(conversion, inputBuffer,
			&inputLeft, &dst, &outputLeft);
		if (nonReversibleConversions == (size_t)-1) {
			if (errno == E2BIG) {
				// Not enough room in the output buffer for the next converted character
				// This is not a "real" error, we just quit out.
				break;
			}

			switch (errno) {
				case EILSEQ: // unable to generate a corresponding character
				{
					// discard the input character
					const int one = 1, zero = 0;
					iconvctl(conversion, ICONV_SET_DISCARD_ILSEQ, (void*)&one);
					iconv(conversion, inputBuffer, &inputLeft, &dst, &outputLeft);
					iconvctl(conversion, ICONV_SET_DISCARD_ILSEQ, (void*)&zero);

					// prepare to convert the substitute character to target encoding
					char* original = new char[1];
					original[0] = substitute;
					size_t len = 1;
					char* copy = original;

					// Perform the conversion
					// We ignore any errors during this as part of robustness/best-effort
					// We use ISO-8859-1 as a source because it is a single byte encoding
					// It also overlaps UTF-8 for the lower 128 characters.  It is also
					// likely to have a mapping to almost any target encoding.
					iconv_t iso8859_1to = iconv_open(to,"ISO-8859-1");
					if (iso8859_1to != (iconv_t)-1) {
						iconv(iso8859_1to, 0, 0, 0, 0);
						iconv(iso8859_1to, const_cast<char**>(&copy), &len, &dst,
							&outputLeft);
						iconv_close(iso8859_1to);
					}
					delete original;
					break;
				}

				case EINVAL: // incomplete multibyte sequence in the input
					// we just eat bad bytes, as part of robustness/best-effort
					inputBuffer++;
					inputLeft--;
					break;

				default:
					// unknown error, completely bail
					status_t status = errno;
					iconv_close(conversion);
					return status;
			}
		}
	} while (inputLeft > 0 && outputLeft > 0);

	*srcLen -= inputLeft;
	*dstLen -= outputLeft;
	iconv_close(conversion);

	return B_OK;
}


status_t
convert_to_utf8(uint32 srcEncoding, const char* src, int32* srcLen,
	char* dst, int32* dstLen, int32* state, char substitute)
{
	const BCharacterSet* charset = BCharacterSetRoster::GetCharacterSetByConversionID(
		srcEncoding);
	if (charset == NULL)
		return B_ERROR;

#if DEBUG_CONV
	fprintf(stderr, "convert_to_utf8(%s) : \"", charset->GetName());
	for (int i = 0 ; i < *srcLen ; i++) {
		fprintf(stderr, "%c", src[i]);
	}
	fprintf(stderr, "\"\n");
#endif

	return convert_encoding(charset->GetName(), "UTF-8", src, srcLen,
		dst, dstLen, state, substitute);
}


status_t
convert_from_utf8(uint32 dstEncoding, const char* src, int32* srcLen,
	char* dst, int32* dstLen, int32* state, char substitute)
{
	const BCharacterSet* charset = BCharacterSetRoster::GetCharacterSetByConversionID(
		dstEncoding);
	if (charset == NULL)
		return B_ERROR;

#if DEBUG_CONV
	fprintf(stderr, "convert_from_utf8(%s) : \"", charset->GetName());
	for (int i = 0 ; i < *srcLen ; i++) {
		fprintf(stderr, "%c", src[i]);
	}
	fprintf(stderr, "\"\n");
#endif

	return convert_encoding("UTF-8", charset->GetName(), src, srcLen,
		dst, dstLen, state, substitute);
}

