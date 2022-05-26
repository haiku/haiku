#include "UdfString.h"

#include <ByteOrder.h>

#include <AutoDeleter.h>


using std::nothrow;


/*! \brief Converts the given unicode character to utf8.

	\param c The unicode character.
	\param out Pointer to a C-string of at least 4 characters
	           long into which the output utf8 characters will
	           be written. The string that is pointed to will
	           be incremented to reflect the number of characters
	           written, i.e. if \a out initially points to a pointer
	           to the first character in string named \c str, and
	           the function writes 4 characters to \c str, then
	           upon returning, out will point to a pointer to
	           the fifth character in \c str.
*/
static void
unicode_to_utf8(uint32 c, char **out)
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

/*! \brief Converts the given utf8 character to 4-byte unicode.

	\param in Pointer to a C-String from which utf8 characters
	          will be read. *in will be incremented to reflect
	          the number of characters read, similarly to the
	          \c out parameter for unicode_to_utf8().

	\return The 4-byte unicode character, or **in if passed an
	        invalid character, or 0 if passed any NULL pointers.
*/
static uint32
utf8_to_unicode(const char **in)
{
	if (!in)
		return 0;
	uint8 *bytes = (uint8 *)*in;
	if (!bytes)
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


// #pragma mark -


/*! \brief Creates an empty string object. */
UdfString::UdfString()
	:
	fCs0String(NULL),
	fUtf8String(NULL)
{
}


/*! \brief Creates a new UdfString object from the given Utf8 string. */
UdfString::UdfString(const char *utf8)
	:
	fCs0String(NULL),
	fUtf8String(NULL)
{
	SetTo(utf8);
}


/*! \brief Creates a new UdfString object from the given Cs0 string. */
UdfString::UdfString(const char *cs0, uint32 length)
	:
	fCs0String(NULL),
	fUtf8String(NULL)
{
	SetTo(cs0, length);
}


UdfString::~UdfString()
{
	_Clear();
}


/*! \brief Assignment from a Utf8 string. */
void
UdfString::SetTo(const char *utf8)
{
	TRACE(("UdfString::SetTo: utf8 = `%s', strlen(utf8) = %ld\n",
		utf8, utf8 ? strlen(utf8) : 0));
	_Clear();

	if (utf8 == NULL) {
		TRACE_ERROR(("UdfString::SetTo: passed NULL utf8 string\n"));
		return;
	}

	uint32 length = strlen(utf8);
	// First copy the utf8 string
	fUtf8String = new(nothrow) char[length + 1];
	if (fUtf8String == NULL) {
		TRACE_ERROR(("UdfString::SetTo: fUtf8String[%" B_PRIu32
			"] allocation failed\n", length + 1));
		return;
	}

	memcpy(fUtf8String, utf8, length + 1);
	// Next convert to raw 4-byte unicode. Then we'll do some
	// analysis to figure out if we have any invalid characters,
	// and whether we can get away with compressed 8-bit unicode,
	// or have to use burly 16-bit unicode.
	uint32 *raw = new(nothrow) uint32[length];
	if (raw == NULL) {
		TRACE_ERROR(("UdfString::SetTo: uint32 raw[%" B_PRIu32 "] temporary"
			" string allocation failed\n", length));
		_Clear();
		return;
	}

	ArrayDeleter<uint32> rawDeleter(raw);

	const char *in = utf8;
	uint32 rawLength = 0;
	for (uint32 i = 0; i < length && uint32(in - utf8) < length; i++, rawLength++)
		raw[i] = utf8_to_unicode(&in);

	// Check for invalids.
	uint32 mask = 0xffff0000;
	for (uint32 i = 0; i < rawLength; i++) {
		if (raw[i] & mask) {
			TRACE(("WARNING: utf8 string contained a multi-byte sequence which "
			       "was converted into a unicode character larger than 16-bits; "
			       "character will be converted to an underscore character for "
			       "safety.\n"));
			raw[i] = '_';
		}
	}
	// See if we can get away with 8-bit compressed unicode
	mask = 0xffffff00;
	bool canUse8bit = true;
	for (uint32 i = 0; i < rawLength; i++) {
		if (raw[i] & mask) {
			canUse8bit = false;
			break;
		}
	}
	// Build our cs0 string
	if (canUse8bit) {
		fCs0Length = rawLength + 1;
		fCs0String = new(nothrow) char[fCs0Length];
		if (fCs0String != NULL) {
			fCs0String[0] = '\x08';	// 8-bit compressed unicode
			for (uint32 i = 0; i < rawLength; i++)
				fCs0String[i + 1] = raw[i] % 256;
		} else {
			TRACE_ERROR(("UdfString::SetTo: fCs0String[%" B_PRIu32
				"] allocation failed\n", fCs0Length));
			_Clear();
			return;
		}
	} else {
		fCs0Length = rawLength * 2 + 1;
		fCs0String = new(nothrow) char[fCs0Length];
		if (fCs0String != NULL) {
			uint32 pos = 0;
			fCs0String[pos++] = '\x10';	// 16-bit unicode
			for (uint32 i = 0; i < rawLength; i++) {
				// 16-bit unicode chars must be written big endian
				uint16 value = uint16(raw[i]);
				uint8 high = uint8(value >> 8 & 0xff);
				uint8 low = uint8(value & 0xff);
				fCs0String[pos++] = high;
				fCs0String[pos++] = low;
			}
		} else {
			TRACE_ERROR(("UdfString::SetTo: fCs0String[%" B_PRIu32
				"] allocation failed\n", fCs0Length));
			_Clear();
			return;
		}
	}
}


/*! \brief Assignment from a Cs0 string. */
void
UdfString::SetTo(const char *cs0, uint32 length)
{
	DEBUG_INIT_ETC("UdfString", ("cs0: %p, length: %" B_PRIu32, cs0, length));

	_Clear();
	if (length == 0)
		return;
	if (!cs0) {
		PRINT(("passed NULL cs0 string\n"));
		return;
	}

	// First copy the Cs0 string and length
	fCs0String = new(nothrow) char[length];
	if (fCs0String) {
		memcpy(fCs0String, cs0, length);
		fCs0Length = length;
	} else {
		PRINT(("new fCs0String[%" B_PRIu32 "] allocation failed\n", length));
		return;
	}

	// Now convert to utf8

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
			fUtf8String = new(nothrow) char[allocationLength];
			if (fUtf8String) {
				char *outputString = fUtf8String;

				for (int32 i = 0; i < maxLength && inputString[i]; i++) {
					unicode_to_utf8(inputString[i], &outputString);
				}
				outputString[0] = 0;
			} else {
				PRINT(("new fUtf8String[%" B_PRId32 "] allocation failed\n",
					allocationLength));
			}

			break;
		}

		case 16:
		case 255:
		{
			const uint16 *inputString = reinterpret_cast<const uint16*>(&(cs0[1]));
			int32 maxLength = (length-1) / 2;		// Max length of input string in uint16 characters
			int32 allocationLength = maxLength*3+1;	// Need at most 3 utf8 chars per uint16 char
			fUtf8String = new(nothrow) char[allocationLength];
			if (fUtf8String) {
				char *outputString = fUtf8String;

				for (int32 i = 0; i < maxLength && inputString[i]; i++) {
					unicode_to_utf8(B_BENDIAN_TO_HOST_INT16(inputString[i]), &outputString);
				}
				outputString[0] = 0;
			} else {
				PRINT(("new fUtf8String[%" B_PRId32 "] allocation failed\n",
					allocationLength));
			}

			break;
		}

		default:
			PRINT(("invalid compression id!\n"));
			break;
	}
}

void
UdfString::_Clear()
{
	DEBUG_INIT("UdfString");

	delete [] fCs0String;
	fCs0String = NULL;
	delete [] fUtf8String;
	fUtf8String = NULL;
}
