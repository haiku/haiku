#include <UTF8.h>
#include <iconv.h>
#include <CharacterSet.h>
#include <CharacterSetRoster.h>
#include <Errors.h>
#include <errno.h>

using namespace BPrivate;

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
	iconv_t conversion = iconv_open("UTF-8",charset->GetName());
	if (conversion == (iconv_t)-1) {
		return B_ERROR;
	}
	const char ** inputBuffer = const_cast<const char**>(&src);
	size_t charsLeft = *dstLen;
	size_t inputLength = *srcLen;
	size_t bytesLeft = iconv(conversion,inputBuffer,&inputLength,&dst,&charsLeft);
	*srcLen = inputLength;
	*dstLen -= charsLeft;
	if ((bytesLeft != 0) && (errno != E2BIG) && (errno != EINVAL)) {
		return B_ERROR;
	}
	return B_OK;
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
	iconv_t conversion = iconv_open(charset->GetName(),"UTF-8");
	if (conversion == (iconv_t)-1) {
		return B_ERROR;
	}
	const char ** inputBuffer = const_cast<const char**>(&src);
	size_t charsLeft = *dstLen;
	size_t inputLength = *srcLen;
	size_t bytesLeft = iconv(conversion,inputBuffer,&inputLength,&dst,&charsLeft);
	*srcLen = inputLength;
	*dstLen -= charsLeft;
	if ((bytesLeft != 0) && (errno != E2BIG) && (errno != EINVAL)) {
		return B_ERROR;
	}
	return B_OK;
}
