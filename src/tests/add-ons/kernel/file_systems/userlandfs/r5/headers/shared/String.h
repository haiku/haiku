// String.h

#ifndef STRING_H
#define STRING_H

#include <string.h>

#include <SupportDefs.h>


// string_hash
//
// from the Dragon Book: a slightly modified hashpjw()
static inline
uint32
string_hash(const char *name)
{
	uint32 h = 0;
	if (name) {
		for (; *name; name++) {
			uint32 g = h & 0xf0000000;
			if (g)
				h ^= g >> 24;
			h = (h << 4) + *name;
		}
	}
	return h;
}

#ifdef __cplusplus

namespace UserlandFSUtil {

// String
class String {
public:
	String();
	String(const String &string);
	String(const char *string, int32 length = -1);
	~String();

	bool SetTo(const char *string, int32 maxLength = -1);
	void Unset();

	void Truncate(int32 newLength);

	const char *GetString() const;
	int32 GetLength() const	{ return fLength; }

	uint32 GetHashCode() const	{ return string_hash(GetString()); }

	String &operator=(const String &string);
	bool operator==(const String &string) const;
	bool operator!=(const String &string) const { return !(*this == string); }

private:
	bool _SetTo(const char *string, int32 length);

private:
	int32	fLength;
	char	*fString;
};

}	// namespace UserlandFSUtil

using UserlandFSUtil::String;

#endif	// __cplusplus

#endif	// STRING_H
