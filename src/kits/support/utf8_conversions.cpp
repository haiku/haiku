#include <UTF8.h>
#include <iconv.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Errors.h>
#include <errno.h>

using namespace BPrivate;

typedef char ** input_buffer_t;

status_t
convert_encoding(const char * from, const char * to,
                 const char * src, int32 * srcLen,
                 char * dst, int32 * dstLen,
                 int32 * state)
{
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
	size_t nonReversibleConversions = iconv(conversion,inputBuffer,&inputLeft,&dst,&outputLeft);
	*srcLen -= inputLeft;
	*dstLen -= outputLeft;
	iconv_close(conversion);
	if (nonReversibleConversions == -1) {
		switch (errno) {
		case EILSEQ: // invalid multibyte sequence in the source
			return B_ERROR;
		case EINVAL: // incomplete multibyte sequence in the input
			return B_OK;
		case E2BIG: // not enough room in the output buffer for the next converted character
			return B_OK;
		default:
			// unknown error
			int err = errno;
		}
	}
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
	return convert_encoding(charset->GetName(),"UTF-8",src,srcLen,dst,dstLen,state);
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
	return convert_encoding("UTF-8",charset->GetName(),src,srcLen,dst,dstLen,state);
}
