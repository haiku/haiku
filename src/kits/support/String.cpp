//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		String.cpp
//	Author(s):		Marc Flerackers (mflerackers@androme.be)
//					Stefano Ceccherini (burton666@libero.it)
//
//	Description:	String class supporting common string operations.  
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algobase.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <String.h>


/*---- Construction --------------------------------------------------------*/
BString::BString()
	:_privateData(NULL)	
{
}


BString::BString(const char* str)
	:_privateData(NULL)
{
	if (str)
		_Init(str, strlen(str));
}


BString::BString(const BString &string)
	:_privateData(NULL)			
{
	_Init(string.String(), string.Length());
}


BString::BString(const char *str, int32 maxLength)
	:_privateData(NULL)		
{
	if (str) {
		int32 len = (int32)strlen(str);
		_Init(str, min(len, maxLength));
	}
}


/*---- Destruction ---------------------------------------------------------*/
BString::~BString()
{
	if (_privateData)
		free(_privateData - sizeof(int32));
}


/*---- Access --------------------------------------------------------------*/
int32
BString::CountChars() const
{
	int32 count = 0;
	char *ptr = _privateData;

	while (*ptr++)
	{
		count++;

		// Jump to next UTF8 character
		for (; (*ptr & 0xc0) == 0x80; ptr++);
	}

	return count;
}


/*---- Assignment ----------------------------------------------------------*/
BString&
BString::operator=(const BString &string)
{
	_DoAssign(string.String(), string.Length());
	return *this;
}


BString&
BString::operator=(const char *str)
{
	if (str) 
		_DoAssign(str, strlen(str));	
	return *this;
}


BString&
BString::operator=(char c)
{
	_DoAssign(&c, 1);
	return *this;
}


BString&
BString::SetTo(const char *str, int32 length)
{
	if (str) {
		int32 len = (int32)strlen(str);
		_DoAssign(str, min(length, len));
	}
	return *this;
}


BString&
BString::SetTo(const BString &from)
{
	_DoAssign(from.String(), from.Length());
	return *this;
}


BString&
BString::Adopt(BString &from)
{
	if (_privateData)
		free(_privateData - sizeof(int32));

	_privateData = from._privateData;
	from._privateData = NULL;

	return *this;
}


BString&
BString::SetTo(const BString &string, int32 length)
{
	_DoAssign(string.String(), min(length, string.Length()));
	return *this;
}


BString&
BString::Adopt(BString &from, int32 length)
{
	if (_privateData)
		free(_privateData - sizeof(int32));

	_privateData = from._privateData;
	from._privateData = NULL;

	if (length < Length())
		_privateData = _GrowBy(length - Length()); //Negative

	return *this;
}


BString&
BString::SetTo(char c, int32 count)
{
	char *tmp = (char*)malloc(count);
	memset(tmp, c, count);
	_DoAssign(tmp, count);
	free(tmp);
	
	return *this;	
}


/*---- Substring copying ---------------------------------------------------*/
BString &BString::CopyInto(BString &into, int32 fromOffset, int32 length) const
{
	into.SetTo(String() + fromOffset, length);
	return into;
}


void
BString::CopyInto(char *into, int32 fromOffset, int32 length) const
{
	if (into) {
		int32 len = min(Length() - fromOffset , length);
		strncpy(into, _privateData + fromOffset, len);
		into[len] = '\0'; 
	}
}


/*---- Appending -----------------------------------------------------------*/
BString&
BString::operator+=(const char *str)
{
	if (str)
		_DoAppend(str, strlen(str));
	return *this;
}


BString&
BString::operator+=(char c)
{
	_DoAppend(&c, 1);
	return *this;
}


BString&
BString::Append(const BString &string, int32 length)
{
	_DoAppend(string.String(), min(length, string.Length()));
	return *this;
}


BString&
BString::Append(const char *str, int32 length)
{
	if (str) {
		int32 len = (int32)strlen(str);
		_DoAppend(str, min(len, length));
	}	
	return *this;
}


BString&
BString::Append(char c, int32 count)
{
	char *tmp = (char*)malloc(count);
	memset(tmp, c, count);
	_DoAppend(tmp, count);
	free(tmp);	
	return *this;
}


/*---- Prepending ----------------------------------------------------------*/
BString&
BString::Prepend(const char *str)
{
	if (str)
		_DoPrepend(str, strlen(str));
	return *this;
}


BString&
BString::Prepend(const BString &string)
{
	if (&string != this)
		_DoPrepend(string.String(), string.Length());
	return *this;
}


BString&
BString::Prepend(const char *str, int32 length)
{
	if (str) {
		int32 len = (int32)strlen(str);
		_DoPrepend(str, min(len, length));		
	}
	return *this;
}


BString&
BString::Prepend(const BString &string, int32 len)
{
	if (&string != this)
		_DoPrepend(string.String(), min(len, string.Length()));
	return *this;
}


BString&
BString::Prepend(char c, int32 count)
{
	char *tmp = (char*)malloc(count);
	memset(tmp, c, count);
	_DoPrepend(tmp, count);
	free(tmp);	
	return *this;
}


/*---- Inserting ----------------------------------------------------------*/
BString&
BString::Insert(const char *str, int32 pos)
{
	if (str) {
		_privateData = _OpenAtBy(pos, strlen(str));
		memcpy(_privateData + pos, str, strlen(str));
	}
	return *this;
}


BString&
BString::Insert(const char *str, int32 length, int32 pos)
{
	if (str) {
		int32 len = (int32)strlen(str);
		len = min(len, length);
		_privateData = _OpenAtBy(pos, len);
		memcpy(_privateData + pos, str, len);
	}
	return *this;
}


BString&
BString::Insert(const char *str, int32 fromOffset, int32 length, int32 pos)
{
	if (str) {
		int32 len = (int32)strlen(str);
		len = min(len, length);
		_privateData = _OpenAtBy(pos, len);
		memcpy(_privateData + pos, str + fromOffset, len);
	}
	return *this;
}


BString&
BString::Insert(const BString &string, int32 pos)
{
	if (&string != this)
		Insert(string.String(), pos); 
	return *this;				  
}


BString&
BString::Insert(const BString &string, int32 length, int32 pos)
{
	if (&string != this)
		Insert(string.String(), length, pos);
	return *this;
}


BString&
BString::Insert(const BString &string, int32 fromOffset, int32 length, int32 pos)
{
	if (&string != this)
		Insert(string.String(), fromOffset, length, pos);
	return *this;
}


BString&
BString::Insert(char c, int32 count, int32 pos)
{
	char *tmp = (char*)malloc(count);
	memset(tmp, c, count);
	_privateData = _OpenAtBy(pos, count);
	memcpy(_privateData + pos, tmp, count);
	free(tmp);
	return *this;
}


/*---- Removing -----------------------------------------------------------*/
BString&
BString::Truncate(int32 newLength, bool lazy = true)
{
	if (newLength < 0)
		return *this;
		
	if (newLength < Length()) {
#if 0 
		if (lazy)
			; //ToDo: Implement
		else
#endif		
			_privateData = _GrowBy(newLength - Length()); //Negative	
		_privateData[Length()] = '\0';
	}
	return *this;
}


BString&
BString::Remove(int32 from, int32 length)
{
	_privateData = _ShrinkAtBy(from, length);
	return *this;
}


BString&
BString::RemoveFirst(const BString &string)
{
	int32 pos = _FindAfter(string.String(), 0, -1);
	
	if (pos >= 0)
		_privateData = _ShrinkAtBy(pos, string.Length());
	
	return *this;
}


BString&
BString::RemoveLast(const BString &string)
{
	int32 pos = _FindBefore(string.String(), Length(), -1);
	
	if (pos >= 0)
		_privateData = _ShrinkAtBy(pos, string.Length());
		
	return *this;
}


BString&
BString::RemoveAll(const BString &string)
{
	int32 pos = B_ERROR;
	while ((pos = _FindAfter(string.String(), 0, -1)) >= 0)
		_privateData = _ShrinkAtBy(pos, string.Length());
	return *this;
}


BString&
BString::RemoveFirst(const char *str)
{
	if (str) {
		int32 pos = _FindAfter(str, 0, -1);
		if (pos >= 0)
			_privateData = _ShrinkAtBy(pos, strlen(str));
	}
	return *this;
}


BString&
BString::RemoveLast(const char *str)
{
	if (str) {
		int32 len = strlen(str);
		int32 pos = _FindBefore(str, len, -1);
		if (pos >= 0)
			_privateData = _ShrinkAtBy(pos, len);
	}
	return *this;
}


BString&
BString::RemoveAll(const char *str)
{
	if (str) {
		int32 pos = B_ERROR;
		int32 len = strlen(str);
		while ((pos = _FindAfter(str, 0, -1)) >= 0)
			_privateData = _ShrinkAtBy(pos, len);
	}
	return *this;
}


BString&
BString::RemoveSet(const char *setOfCharsToRemove)
{
	if (setOfCharsToRemove) {
		int32 pos;
		while ((pos = strcspn(String(), setOfCharsToRemove)) < Length())
			_privateData = _ShrinkAtBy(pos, 1);
	}		
	return *this;
}


BString&
BString::MoveInto(BString &into, int32 from, int32 length)
{
	int32 len = min(Length() - from, length);
	into.SetTo(String() + from, len);
	_privateData = _ShrinkAtBy(from, len);

	return *this;
}


void
BString::MoveInto(char *into, int32 from, int32 length)
{
	//TODO: Test
	if (into && (from + length <= Length())) {
		memcpy(into, String() + from, length);
		_privateData = _ShrinkAtBy(from, length);
		into[length] = '\0';
	}		 
}

/*---- Compare functions ---------------------------------------------------*/
// Implemented inline in the header file
/*---- strcmp-style compare functions --------------------------------------*/
int
BString::Compare(const BString &string) const
{
	return strcmp(_privateData, string.String());
}


int
BString::Compare(const char *string) const
{
	return strcmp(_privateData, string);
}


int
BString::Compare(const BString &string, int32 n) const
{
	return strncmp(_privateData, string.String(), n);
}


int
BString::Compare(const char *string, int32 n) const
{
	return strncmp(_privateData, string, n);
}


int
BString::ICompare(const BString &string) const
{
	return strcasecmp(_privateData, string.String());
}


int
BString::ICompare(const char *str) const
{
	return strcasecmp(_privateData, str);
}


int
BString::ICompare(const BString &string, int32 n) const
{
	return strncasecmp(_privateData, string.String(), n);
}


int
BString::ICompare(const char *str, int32 n) const
{
	return strncasecmp(_privateData, str, n);
}


/*---- Searching -----------------------------------------------------------*/
int32
BString::FindFirst(const BString &string) const
{
	return _FindAfter(string.String(), 0, -1);
}


int32
BString::FindFirst(const char *string) const
{
	return _FindAfter(string, 0, -1);
}


int32
BString::FindFirst(const BString &string, int32 fromOffset) const
{
	return _FindAfter(string.String(), fromOffset, -1);
}


int32
BString::FindFirst(const char *string, int32 fromOffset) const
{
	return _FindAfter(string, fromOffset, -1);
}


int32
BString::FindFirst(char c) const
{
	char tmp[2] = { c, '\0' };
	int32 result = _FindAfter(tmp, 0, -1);
	
	return result;	
}


int32
BString::FindFirst(char c, int32 fromOffset) const
{
	char tmp[2] = { c, '\0' };
	int32 result = _FindAfter(tmp, fromOffset, -1);
	
	return result;	
}


/*---- Replacing -----------------------------------------------------------*/
BString&
BString::ReplaceFirst(char replaceThis, char withThis)
{
	int32 pos = FindFirst(replaceThis, 0);
	if (pos >= 0)
		_privateData[pos] = withThis;
	return *this;
}


BString&
BString::ReplaceLast(char replaceThis, char withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::ReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	int32 pos = B_ERROR;
	while ((pos = FindFirst(replaceThis, fromOffset)) >= 0)
		_privateData[pos] = withThis;
	return *this;
}


BString&
BString::Replace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::ReplaceFirst(const char *replaceThis, const char *withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::ReplaceLast(const char *replaceThis, const char *withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::ReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::Replace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplaceFirst(char replaceThis, char withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplaceLast(char replaceThis, char withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplaceFirst(const char *replaceThis, const char *withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplaceLast(const char *replaceThis, const char *withThis)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::IReplace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	//TODO: Implement
	return *this;
}


BString&
BString::ReplaceSet(const char *setOfChars, char with)
{
	int32 pos = Length();
	while ((pos = strcspn(String(), setOfChars)) < Length())
		_privateData[pos] = with;
	return *this;
}


BString&
BString::ReplaceSet(const char *setOfChars, const char *with)
{
	//TODO: Implement
	return *this;
}


/*---- Fast low-level manipulation -----------------------------------------*/
char*
BString::LockBuffer(int32 maxLength)
{
	if (maxLength > Length())
		_privateData = _GrowBy(maxLength - Length());

	return _privateData;
}


BString&
BString::UnlockBuffer(int32 length)
{
	int32 len = length;

	if (len < 0)
		len = strlen(_privateData);

	if (len != Length())
		_privateData = _GrowBy(len - Length());
		
	return *this;
}


/*---- Uppercase<->Lowercase ------------------------------------------------*/
BString&
BString::ToLower()
{
	for (int count = 0; count < Length(); count++)
			_privateData[count] = tolower(_privateData[count]);
	
	return *this;
}


BString&
BString::ToUpper()
{	
	for (int count = 0; count < Length(); count++)
			_privateData[count] = toupper(_privateData[count]);
	
	return *this;
}


BString&
BString::Capitalize()
{
	
	for (int count = 0; count < Length(); count++)
		if (isalpha(_privateData[count])) {
			_privateData[count] = toupper(_privateData[count]);
			break;
		}
	return *this;
}


BString&
BString::CapitalizeEachWord()
{
	//TODO: Implement
	return *this;
}


/*----- Escaping and Deescaping --------------------------------------------*/
BString&
BString::CharacterEscape(const char *original, const char *setOfCharsToEscape, char escapeWith)
{
	//TODO: Implement
	return *this;
}


BString&
BString::CharacterEscape(const char *setOfCharsToEscape, char escapeWith)
{
	//TODO: Implement
	return *this;
}


BString&
BString::CharacterDeescape(const char *original, char escapeChar)
{
	//TODO: Implement
	return *this;
}


BString&
BString::CharacterDeescape(char escapeChar)
{
	//TODO: Implement
	return *this;
}


/*---- Simple sprintf replacement calls ------------------------------------*/
/*---- Slower than sprintf but type and overflow safe ----------------------*/
BString&
BString::operator<<(const char *str)
{
	if (str)
		_DoAppend(str, strlen(str));
	return *this;	
}


BString&
BString::operator<<(const BString &string)
{
	_DoAppend(string.String(), string.Length());
	return *this;
}


BString&
BString::operator<<(char c)
{
	_DoAppend(&c, 1);	
	return *this;
}


BString&
BString::operator<<(int i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(unsigned int i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(uint32 i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(int32 i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(uint64 i)
{
	//TODO: implement
	return *this;
}


BString&
BString::operator<<(int64 i)
{
	//TODO: implement
	return *this;
}

BString&
BString::operator<<(float f)
{
	//TODO: implement
	return *this;
}


/*---- Private or Reserved ------------------------------------------------*/
void
BString::_Init(const char* str, int32 len)
{
	assert(str != NULL);
	_privateData = _GrowBy(len);
	memcpy(_privateData, str, len);
}


void
BString::_DoAssign(const char *str, int32 len)
{
	assert(str != NULL);	
	int32 curLen = Length();
	
	if (len != curLen)
		_privateData = _GrowBy(len - curLen);
		
	memcpy(_privateData, str, len);
}


void
BString::_DoAppend(const char *str, int32 len)
{
	assert(str != NULL);
	
	int32 length = Length();
	_privateData = _GrowBy(len);
	memcpy(_privateData + length, str, len);
}


char*
BString::_GrowBy(int32 size)
{		
	int32 curLen = Length(); 	
	assert(curLen + size >= 0);
	
	if (_privateData) {
		_privateData -= sizeof(int32);
		_privateData = (char*)realloc(_privateData,
			 curLen + size + sizeof(int32) + 1);
	} else 
		_privateData = (char*)malloc(size + sizeof(int32) + 1);
		
	_privateData += sizeof(int32);
	
	_SetLength(curLen + size);	
	_privateData[Length()] = '\0';
	
	return _privateData;
}


char*
BString::_OpenAtBy(int32 offset, int32 length)
{
	int32 oldLength = Length();
	
	_privateData -= sizeof(int32);
	_privateData = (char*)realloc(_privateData , oldLength + length + sizeof(int32) + 1);
	_privateData += sizeof(int32);
	memmove(_privateData + offset + length, _privateData + offset,
			oldLength - offset);
	
	_SetLength(oldLength + length);
	_privateData[Length()] = '\0';
	
	return _privateData;	
}


char*
BString::_ShrinkAtBy(int32 offset, int32 length)
{
	assert(offset + length <= Length());
	int32 oldLength = Length();
	
	memmove(_privateData + offset, _privateData + offset + length,
		Length() - offset - length);
	
	_privateData -= sizeof(int32);	
	_privateData = (char*)realloc(_privateData, oldLength - length + sizeof(int32) + 1);
	_privateData += sizeof(int32);
	
	_SetLength(oldLength - length);	
	_privateData[Length()] = '\0';
	
	return _privateData;
}


void
BString::_DoPrepend(const char *str, int32 count)
{
	assert(str != NULL);
	_privateData = _OpenAtBy(0, count);
	memcpy(_privateData, str, count);
}


int32
BString::_FindAfter(const char *str, int32 offset, int32) const
{	
	assert(str != NULL);
	
	if (offset > Length())
		return B_ERROR;

	char *ptr = strstr(String() + offset, str);

	if (ptr != NULL)
		return ptr - (String() + offset);
	
	return B_ERROR;
}


int32
BString::_IFindAfter(const char *str, int32 offset, int32 ) const
{
	assert(str != NULL);
	if (offset > Length())
		return B_ERROR;

	char *ptr = strcasestr(String() + offset, str);

	if (ptr != NULL)
		return ptr - (String() + offset);

	return B_ERROR;
}


int32
BString::_ShortFindAfter(const char *str, int32 ) const
{
	assert(str != NULL);
	//TODO: Implement?
	return B_ERROR;
}


int32
BString::_FindBefore(const char *str, int32 offset, int32 ) const
{
	assert(str != NULL);
	if (offset <= 0)
		return B_ERROR;
	
	//TODO: Implement
	
	return B_ERROR;	
}


int32
BString::_IFindBefore(const char *str, int32 , int32 ) const
{
	assert(str != NULL);
	//TODO: Implement
	return B_ERROR;
}


void
BString::_SetLength(int32 length)
{
	*((int32*)_privateData - 1) = length & 0x7fffffff;
}


#if DEBUG
// TODO: Implement?
BString::_SetUsingAsCString(bool) {}
BString::_AssertNotUsingAsCString() {}
#endif


/*----- Non-member compare for sorting, etc. ------------------------------*/
int
Compare(const BString &string1, const BString &string2)
{
	return strcmp(string1.String(), string2.String());
}


int
ICompare(const BString &string1, const BString &string2)
{
	return strcasecmp(string1.String(), string2.String());
}


int
Compare(const BString *string1, const BString *string2)
{
	return strcmp(string1->String(), string2->String());
}


int
ICompare(const BString *string1, const BString *string2)
{
	return strcasecmp(string1->String(), string2->String());
}
