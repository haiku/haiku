#ifndef __MOREUTF8
#define __MOREUTF8


static inline bool
IsInsideGlyph(uchar ch)
{
	return (ch & 0xC0) == 0x80;
}


static inline uint32
UTF8NextCharLen(const char *text)
{
	const char *ptr = text;
	
	if (ptr == NULL || *ptr == 0)
		return 0;
	
	do {
		ptr++;
	} while (IsInsideGlyph(*ptr));
				
	return ptr - text;
}


static inline uint32
UTF8PreviousCharLen(const char *text, const char *limit)
{
	const char *ptr = text;
	
	if (ptr == NULL || limit == NULL)
		return 0;

	do {
		if (ptr == limit)
			break;
		ptr--;
	} while (IsInsideGlyph(*ptr));
				
	return text - ptr;
}

#endif
