/*
* Copyright 2001-2008, Haiku Inc. All Rights Reserved.
* Distributed under the terms of the MIT License.
*/
#ifndef __BSTRING__
#define __BSTRING__


#include <BeBuild.h>
#include <SupportDefs.h>
#include <string.h>


class BStringRef;


class BString {
public:
				BString();
				BString(const char* string);
				BString(const BString& string);
				BString(const char* string, int32 maxLength);
				~BString();

	// Access
	const char*	String() const;
	int32 		Length() const;
	int32		CountChars() const;
	int32 		ReferenceCount() const;

	// Assignment
	BString&	operator=(const BString& string);
	BString&	operator=(const char* string);
	BString&	operator=(char c);

	BString&	SetTo(const char* string);
	BString&	SetTo(const char* string, int32 maxLength);

	BString&	SetTo(const BString& string);
	BString&	Adopt(BString& from);

	BString&	SetTo(const BString& string, int32 maxLength);
	BString&	Adopt(BString& from, int32 maxLength);

	BString&	SetTo(char c, int32 count);

	// Substring copying
	BString&	CopyInto(BString& into, int32 fromOffset, int32 length) const;
	void		CopyInto(char* into, int32 fromOffset, int32 length) const;

	// Appending
	BString&	operator+=(const BString& string);
	BString&	operator+=(const char* string);
	BString&	operator+=(char c);

	BString&	Append(const BString& string);
	BString&	Append(const char* string);

	BString&	Append(const BString& string, int32 length);
	BString&	Append(const char* string, int32 length);
	BString&	Append(char c, int32 count);

	// Prepending
	BString&	Prepend(const char* string);
	BString&	Prepend(const BString& string);
	BString&	Prepend(const char* string, int32 length);
	BString&	Prepend(const BString& string, int32 length);
	BString&	Prepend(char c, int32 count);

	// Inserting
	BString&	Insert(const char* string, int32 position);
	BString&	Insert(const char* string, int32 length, int32 position);
	BString&	Insert(const char* string, int32 fromOffset, int32 length,
					int32 position);
	BString&	Insert(const BString& string, int32 position);
	BString&	Insert(const BString& string, int32 length,	int32 position);
	BString&	Insert(const BString& string, int32 fromOffset,	int32 length,
					int32 position);
	BString&	Insert(char c, int32 count, int32 position);

	// Removing
	BString&	Truncate(int32 newLength, bool lazy = true);
	BString&	Remove(int32 from, int32 length);

	BString&	RemoveFirst(const BString& string);
	BString&	RemoveLast(const BString& string);
	BString&	RemoveAll(const BString& string);

	BString&	RemoveFirst(const char* string);
	BString&	RemoveLast(const char* string);
	BString& 	RemoveAll(const char* string);

	BString& 	RemoveSet(const char* setOfCharsToRemove);

	BString& 	MoveInto(BString& into, int32 from, int32 length);
	void		MoveInto(char* into, int32 from, int32 length);

	// Compare functions
	bool		operator<(const BString& string) const;
	bool		operator<=(const BString& string) const;
	bool		operator==(const BString& string) const;
	bool		operator>=(const BString& string) const;
	bool		operator>(const BString& string) const;
	bool		operator!=(const BString& string) const;

	bool		operator<(const char* string) const;
	bool		operator<=(const char* string) const;
	bool		operator==(const char* string) const;
	bool		operator>=(const char* string) const;
	bool		operator>(const char* string) const;
	bool		operator!=(const char* string) const;

	// strcmp()-style compare functions
	int			Compare(const BString& string) const;
	int			Compare(const char* string) const;
	int			Compare(const BString& string, int32 length) const;
	int			Compare(const char* string, int32 length) const;
	int			ICompare(const BString& string) const;
	int			ICompare(const char* string) const;
	int			ICompare(const BString& string, int32 length) const;
	int			ICompare(const char* string, int32 length) const;

	// Searching
	int32		FindFirst(const BString& string) const;
	int32		FindFirst(const char* string) const;
	int32		FindFirst(const BString& string, int32 fromOffset) const;
	int32		FindFirst(const char* string, int32 fromOffset) const;
	int32		FindFirst(char c) const;
	int32		FindFirst(char c, int32 fromOffset) const;

	int32		FindLast(const BString& string) const;
	int32		FindLast(const char* string) const;
	int32		FindLast(const BString& string, int32 beforeOffset) const;
	int32		FindLast(const char* string, int32 beforeOffset) const;
	int32		FindLast(char c) const;
	int32		FindLast(char c, int32 beforeOffset) const;

	int32		IFindFirst(const BString& string) const;
	int32		IFindFirst(const char* string) const;
	int32		IFindFirst(const BString& string, int32 fromOffset) const;
	int32		IFindFirst(const char* string, int32 fromOffset) const;

	int32		IFindLast(const BString& string) const;
	int32		IFindLast(const char* string) const;
	int32		IFindLast(const BString& string, int32 beforeOffset) const;
	int32		IFindLast(const char* string, int32 beforeOffset) const;

	// Replacing
	BString&	ReplaceFirst(char replaceThis, char withThis);
	BString&	ReplaceLast(char replaceThis, char withThis);
	BString&	ReplaceAll(char replaceThis, char withThis, int32 fromOffset = 0);
	BString&	Replace(char replaceThis, char withThis, int32 maxReplaceCount,
					int32 fromOffset = 0);
	BString&	ReplaceFirst(const char* replaceThis, const char* withThis);
	BString&	ReplaceLast(const char* replaceThis, const char* withThis);
	BString&	ReplaceAll(const char* replaceThis, const char* withThis,
					int32 fromOffset = 0);
	BString&	Replace(const char* replaceThis, const char* withThis,
					int32 maxReplaceCount, int32 fromOffset = 0);

	BString&	IReplaceFirst(char replaceThis, char withThis);
	BString&	IReplaceLast(char replaceThis, char withThis);
	BString&	IReplaceAll(char replaceThis, char withThis, int32 fromOffset = 0);
	BString&	IReplace(char replaceThis, char withThis, int32 maxReplaceCount,
					int32 fromOffset = 0);
	BString&	IReplaceFirst(const char* replaceThis, const char* withThis);
	BString&	IReplaceLast(const char* replaceThis, const char* withThis);
	BString&	IReplaceAll(const char* replaceThis, const char* withThis,
					int32 fromOffset = 0);
	BString&	IReplace(const char* replaceThis, const char* withThis,
					int32 maxReplaceCount, int32 fromOffset = 0);

	BString&	ReplaceSet(const char* setOfChars, char with);
	BString&	ReplaceSet(const char* setOfChars, const char* with);

	// Unchecked char access
	char	 	operator[](int32 index) const;

#if __GNUC__ > 3
	BStringRef  operator[](int32 index);
#else
	char& 		operator[](int32 index);
#endif

	// Checked char access
	char		ByteAt(int32 index) const;

	// Fast low-level manipulation
	char*		LockBuffer(int32 maxLength);
	BString&	UnlockBuffer(int32 length = -1);

	// Upercase <-> Lowercase
	BString&	ToLower();
	BString&	ToUpper();

	BString&	Capitalize();
	BString&	CapitalizeEachWord();

	// Escaping and De-escaping
	BString&	CharacterEscape(const char* original, const char* setOfCharsToEscape,
					char escapeWith);
	BString&	CharacterEscape(const char* setOfCharsToEscape, char escapeWith);
	BString&	CharacterDeescape(const char* original, char escapeChar);
	BString&	CharacterDeescape(char escapeChar);

	// Insert
	BString&	operator<<(const char* string);
	BString&	operator<<(const BString& string);
	BString&	operator<<(char c);
	BString&	operator<<(int value);
	BString&	operator<<(unsigned int value);
	BString&	operator<<(uint32 value);
	BString&	operator<<(int32 value);
	BString&	operator<<(uint64 value);
	BString&	operator<<(int64 value);
	// float output hardcodes %.2f style formatting
	BString&	operator<<(float value);

private:
	class PosVect;
	friend class BStringRef;

	// Management
	status_t	_Detach();
	char*		_Realloc(int32 length);
	void		_Init(const char* src, int32 length);
	char*		_Clone(const char* data, int32 length);
	char*		_OpenAtBy(int32 offset, int32 length);
	char*		_ShrinkAtBy(int32 offset, int32 length);
	status_t	_DetachWith(const char* string, int32 length);

	// Data
	void		_SetLength(int32 length);
	void		_SetReferenceCount(int32 count);
	bool		_DoAppend(const char* string, int32 length);
	bool		_DoPrepend(const char* string, int32 length);
	bool		_DoInsert(const char* string, int32 offset, int32 length);

	// Search
	int32		_ShortFindAfter(const char* string, int32 len) const;
	int32		_FindAfter(const char* string, int32 offset, int32 strlen) const;
	int32		_IFindAfter(const char* string, int32 offset, int32 strlen) const;

	int32		_FindBefore(const char* string, int32 offset, int32 strlen) const;
	int32		_IFindBefore(const char* string, int32 offset, int32 strlen) const;

	// Escape
	BString&    _DoCharacterEscape(const char* string,
					const char *setOfCharsToEscape, char escapeChar);
	BString&    _DoCharacterDeescape(const char* string, char escapeChar);

	// Replace
	BString&	_DoReplace(const char* findThis, const char* replaceWith,
					int32 maxReplaceCount, int32 fromOffset, bool ignoreCase);
	void		_ReplaceAtPositions(const PosVect* positions, int32 searchLen,
					const char* with, int32 withLen);

private:
	char*	            fPrivateData;
};


// Commutative compare operators
bool operator<(const char* a, const BString& b);
bool operator<=(const char* a, const BString& b);
bool operator==(const char* a, const BString& b);
bool operator>(const char* a, const BString& b);
bool operator>=(const char* a, const BString& b);
bool operator!=(const char* a, const BString& b);


// Non-member compare for sorting, etc.
int Compare(const BString& a, const BString& b);
int ICompare(const BString& a, const BString& b);
int Compare(const BString* a, const BString* b);
int ICompare(const BString* a, const BString* b);


inline int32
BString::Length() const
{
	// the most significant bit is reserved; accessing
	// it in any way will cause the computer to explode
	return fPrivateData ? (*(((int32 *)fPrivateData) - 1) & 0x7fffffff) : 0;
}


inline const char*
BString::String() const
{
	if (!fPrivateData)
		return "";
	return fPrivateData;
}


inline int32
BString::ReferenceCount() const
{
	return fPrivateData ? (*(((int32 *)fPrivateData) - 2)) : 1;
}


inline BString &
BString::SetTo(const char* string)
{
	return operator=(string);
}


inline char
BString::operator[](int32 index) const
{
	return fPrivateData[index];
}


inline char
BString::ByteAt(int32 index) const
{
	if (!fPrivateData || index < 0 || index > Length())
		return 0;
	return fPrivateData[index];
}


inline BString &
BString::operator+=(const BString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}


inline BString &
BString::Append(const BString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}


inline BString &
BString::Append(const char* string)
{
	return operator+=(string);
}


inline bool
BString::operator==(const BString &string) const
{
	return strcmp(String(), string.String()) == 0;
}


inline bool
BString::operator<(const BString &string) const
{
	return strcmp(String(), string.String()) < 0;
}


inline bool
BString::operator<=(const BString &string) const
{
	return strcmp(String(), string.String()) <= 0;
}


inline bool
BString::operator>=(const BString &string) const
{
	return strcmp(String(), string.String()) >= 0;
}


inline bool
BString::operator>(const BString &string) const
{
	return strcmp(String(), string.String()) > 0;
}


inline bool
BString::operator!=(const BString &string) const
{
	return strcmp(String(), string.String()) != 0;
}


inline bool
BString::operator!=(const char* string) const
{
	return !operator==(string);
}


inline bool
operator<(const char *str, const BString &string)
{
	return string > str;
}


inline bool
operator<=(const char *str, const BString &string)
{
	return string >= str;
}


inline bool
operator==(const char *str, const BString &string)
{
	return string == str;
}


inline bool
operator>(const char *str, const BString &string)
{
	return string < str;
}


inline bool
operator>=(const char *str, const BString &string)
{
	return string <= str;
}


inline bool
operator!=(const char *str, const BString &string)
{
	return string != str;
}


//	#pragma mark - BStringRef


class BStringRef
{
public:
	BStringRef(BString& string, int32 position);
	~BStringRef() {}
	
	operator char() const;

	char* operator&();
	const char* operator&() const;

	BStringRef& operator=(char c);
	BStringRef &operator=(const BStringRef &rc);

private:
	BString&	fString;
	int32		fPosition;
};

#endif	// __BSTRING__
