#include <UTF8.h>
#include <iconv.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Errors.h>
#include <errno.h>
#include <stdio.h>

using namespace BPrivate;

typedef char ** input_buffer_t;

int iconvctl (iconv_t icd, int request, void* argument);

status_t
convert_encoding(const char * from, const char * to,
                 const char * src, int32 * srcLen,
                 char * dst, int32 * dstLen,
                 int32 * state, char substitute)
{
	status_t status;
	if (*srcLen == 0) {
		// nothing to do!
		return B_OK;
	}
	iconv_t conversion = iconv_open(to,from);
	if (conversion == (iconv_t)-1) {
		return B_ERROR;
	}
	if (state == 0) {
		return B_ERROR;
	}	
	if (*state == 0) {
		iconv(conversion,0,0,0,0);
	}	
	input_buffer_t inputBuffer = const_cast<input_buffer_t>(&src);
	size_t inputLeft = *srcLen;
	size_t outputLeft = *dstLen;
	do {
		size_t nonReversibleConversions = iconv(conversion,inputBuffer,&inputLeft,&dst,&outputLeft);
		if (nonReversibleConversions == (size_t)-1) {
			switch (errno) {
			case EILSEQ: // unable to generate a corresponding character
				{
				// discard the input character
				const int one = 1, zero = 0;
				iconvctl(conversion,ICONV_SET_DISCARD_ILSEQ,(void*)&one);
				iconv(conversion,inputBuffer,&inputLeft,&dst,&outputLeft);
				iconvctl(conversion,ICONV_SET_DISCARD_ILSEQ,(void*)&zero);
				// prepare to convert the substitute character to target encoding
				char * original = new char[1];
				original[0] = substitute;
				size_t len = 1;
				char * copy = original;
				// Perform the conversion
				// We ignore any errors during this as part of robustness/best-effort
				// We use ISO-8859-1 as a source because it is a single byte encoding
				// It also overlaps UTF-8 for the lower 128 characters.  It is also
				// likely to have a mapping to almost any target encoding.
				iconv_t iso8859_1to = iconv_open(to,"ISO-8859-1");
				if (iso8859_1to != (iconv_t)-1) {
					iconv(iso8859_1to,0,0,0,0);
					iconv(iso8859_1to,const_cast<input_buffer_t>(&copy),&len,&dst,&outputLeft);
					iconv_close(iso8859_1to);
				}
				delete original;
				}
				break;
			case EINVAL: // incomplete multibyte sequence in the input
				// we just eat bad bytes, as part of robustness/best-effort
				inputBuffer++;
				inputLeft--;
				break;
			case E2BIG:
				// not enough room in the output buffer for the next converted character
				break;
			default:
				// unknown error, completely bail
				status = errno;
				iconv_close(conversion);
				return status;
			}
		}
	} while ((inputLeft > 0) && (outputLeft > 0));
	*srcLen -= inputLeft;
	*dstLen -= outputLeft;
	iconv_close(conversion);
	if (*srcLen != 0) {
		// able to convert at least one character
		return B_OK;
	} else {
		// not able to convert at least one character
		return B_ERROR;
	}
}

status_t
convert_to_utf8(uint32 srcEncoding,
                const char * src, int32 * srcLen, 
                char * dst, int32 * dstLen,
                int32 * state, char substitute = B_SUBSTITUTE)
{
	const BCharacterSet * charset = BCharacterSetRoster::GetCharacterSetByConversionID(srcEncoding);
	if (charset == 0) {
		return B_ERROR;
	}
	return convert_encoding(charset->GetName(),"UTF-8",src,srcLen,dst,dstLen,state,substitute);
}

status_t
convert_from_utf8(uint32 dstEncoding,
                  const char * src, int32 * srcLen, 
                  char * dst, int32 * dstLen,
                  int32 * state, char substitute = B_SUBSTITUTE)
{
	const BCharacterSet * charset = BCharacterSetRoster::GetCharacterSetByConversionID(dstEncoding);
	if (charset == 0) {
		return B_ERROR;
	}	
	return convert_encoding("UTF-8",charset->GetName(),src,srcLen,dst,dstLen,state,substitute);
}
