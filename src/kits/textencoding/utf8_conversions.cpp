/*
 * Copyright 2003-2008, Haiku, Inc. All Rights Reserved.
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


static void
discard_invalid_input_character(iconv_t* conversion, char** inputBuffer,
	size_t* inputLeft)
{
	if (*inputLeft == 0)
		return;

	char outputBuffer[1];

	// skip the invalid input character only
	size_t left = 1;
	for (; left <= *inputLeft; left ++) {
		// reset internal state
		iconv(*conversion, NULL, NULL, NULL, NULL);

		char* buffer = *inputBuffer;
		char* output = outputBuffer;
		size_t outputLeft = 1;
		size_t size = iconv(*conversion, &buffer, &left,
			&output, &outputLeft);

		if (size != (size_t)-1) {
			// should not reach here
			break;
		}

		if (errno == EINVAL) {
			// too few input bytes provided,
			// increase input buffer size and try again
			continue;
		}

		if (errno == EILSEQ) {
			// minimal size of input buffer found
			break;
		}

		// should not reach here
	};

	*inputBuffer += left;
	*inputLeft -= left;
}


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

	// TODO: this doesn't work, as the state is reset every time!
	iconv_t conversion = iconv_open(to, from);
	if (conversion == (iconv_t)-1) {
		DEBPRINT(("iconv_open failed\n"));
		return B_ERROR;
	}

	size_t outputLeft = *dstLen;

	if (state == NULL || *state == 0) {
		if (state != NULL)
			*state = 1;

		iconv(conversion, NULL, NULL, &dst, &outputLeft);
	}

	char** inputBuffer = const_cast<char**>(&src);
	size_t inputLeft = *srcLen;
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
					discard_invalid_input_character(&conversion, inputBuffer,
						&inputLeft);

					// prepare to convert the substitute character to target encoding
					char original = substitute;
					size_t len = 1;
					char* copy = &original;

					// Perform the conversion
					// We ignore any errors during this as part of robustness/best-effort
					// We use ISO-8859-1 as a source because it is a single byte encoding
					// It also overlaps UTF-8 for the lower 128 characters.  It is also
					// likely to have a mapping to almost any target encoding.
					iconv_t iso8859_1to = iconv_open(to,"ISO-8859-1");
					if (iso8859_1to != (iconv_t)-1) {
						iconv(iso8859_1to, 0, 0, 0, 0);
						iconv(iso8859_1to, &copy, &len, &dst, &outputLeft);
						iconv_close(iso8859_1to);
					}
					break;
				}

				case EINVAL: // incomplete multibyte sequence at the end of the input
					// TODO inputLeft bytes from inputBuffer should
					// be stored in state variable, so that conversion
					// can continue when the caller provides the missing
					// bytes with the next call of this method

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

