#include "UdfString.h"

#include "ByteOrder.h"


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

/*! \brief Creates an empty string object.
*/
String::String()
	: fCs0String(NULL)
	, fUtf8String(NULL)
{
}

/*! \brief Creates a new String object from the given Utf8 string.
*/
String::String(const char *utf8)
	: fCs0String(NULL)
	, fUtf8String(NULL)
{
	SetTo(utf8);
}

/*! \brief Creates a new String object from the given Cs0 string.
*/
String::String(const char *cs0, uint32 length)
	: fCs0String(NULL)
	, fUtf8String(NULL)
{
	SetTo(cs0, length);
}

String::~String()
{
	DEBUG_INIT("String");	

	_Clear();
}

/*! \brief Assignment from a Utf8 string.
*/
void
String::SetTo(const char *utf8)
{
}

/*! \brief Assignment from a Cs0 string.
*/
void
String::SetTo(const char *cs0, uint32 length)
{
	DEBUG_INIT_ETC("String", ("cs0: %p, length: %ld", cs0, length));	

	_Clear();

	// The first byte of the CS0 string is the compression ID.
	// - 8: 1 byte characters
	// - 16: 2 byte, big endian characters
	// - 254: "CS0 expansion is empty and unique", 1 byte characters
	// - 255: "CS0 expansion is empty and unique", 2 byte, big endian characters
	PRINT(("compression ID: %d\n", cs0[0]));
	switch (reinterpret_cast<const uint8*>(cs0)[0]) {
		case 8:			
		case 254:
		{
			const uint8 *inputString = reinterpret_cast<const uint8*>(&(cs0[1]));
			int32 maxLength = length-1;				// Max length of input string in uint8 characters
			int32 allocationLength = maxLength*2+1;	// Need at most 2 utf8 chars per uint8 char
			fUtf8String = new char[allocationLength];	
			if (fUtf8String) {
				char *outputString = fUtf8String;
	
				for (int32 i = 0; i < maxLength && inputString[i]; i++) {
					unicode_to_utf8(inputString[i], &outputString);
				}
				outputString[0] = 0;
			} else {
				PRINT(("new fUtf8String[%ld] allocation failed\n", allocationLength));
			}
				
			break;
		}

		case 16:
		case 255:
		{
			const uint16 *inputString = reinterpret_cast<const uint16*>(&(cs0[1]));
			int32 maxLength = (length-1) / 2;		// Max length of input string in uint16 characters
			int32 allocationLength = maxLength*3+1;	// Need at most 3 utf8 chars per uint16 char
			fUtf8String = new char[allocationLength];
			if (fUtf8String) {
				char *outputString = fUtf8String;

				for (int32 i = 0; i < maxLength && inputString[i]; i++) {
					unicode_to_utf8(B_BENDIAN_TO_HOST_INT16(inputString[i]), &outputString);
				}
				outputString[0] = 0;
			} else {
				PRINT(("new fUtf8String[%ld] allocation failed\n", allocationLength));
			}
			
			break;
		}
		
		default:
			PRINT(("invalid compression id!\n"));
			break;
	}
}

void
String::_Clear()
{
	DEBUG_INIT("String");	

	delete [] fCs0String;
	fCs0String = NULL;
	delete [] fUtf8String;
	fUtf8String = NULL;
}
