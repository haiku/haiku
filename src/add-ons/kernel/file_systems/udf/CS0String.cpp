#include "CS0String.h"

/*! \brief Converts the given unicode character to utf8.
*/
void
Udf::unicode_to_utf8(uint32 c, char **out)
{
	char *s = *out;

	if (c < 0x80)
		*(s++) = c;
	else if (c < 0x800) {
		*(s++) = 0xc0 | (c>>6);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c < 0x10000) {
		*(s++) = 0xe0 | (c>>12);
		*(s++) = 0x80 | ((c>>6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	} else if (c <= 0x10ffff) {
		*(s++) = 0xf0 | (c>>18);
		*(s++) = 0x80 | ((c>>12) & 0x3f);
		*(s++) = 0x80 | ((c>>6) & 0x3f);
		*(s++) = 0x80 | (c & 0x3f);
	}
	*out = s;
}

using namespace Udf;

CS0String::CS0String()
	: fUtf8String(NULL)
{
}

CS0String::~CS0String()
{
	DEBUG_INIT(CF_HELPER | CF_HIGH_VOLUME, "CS0String");	

	_Clear();
}

void
CS0String::_Clear()
{
	DEBUG_INIT(CF_HELPER | CF_HIGH_VOLUME, "CS0String");	

	delete [] fUtf8String;
	fUtf8String = NULL;
}
