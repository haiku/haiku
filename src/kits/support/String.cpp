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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// System Includes -------------------------------------------------------------
#include <Debug.h>
#define DEBUG 1
#include <String.h>

// Temporary Includes
#include "string_helper.h"

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
	else
		_GrowBy(-Length()); // Empties the string
	
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
	} else 
		_GrowBy(-Length()); // Empties the string
	
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
	_GrowBy(count - Length());
	memset(_privateData, c, count);
	
	return *this;	
}


/*---- Substring copying ---------------------------------------------------*/
BString &BString::CopyInto(BString &into, int32 fromOffset, int32 length) const
{
	if (&into != this)
		into.SetTo(String() + fromOffset, length);
	return into;
}


void
BString::CopyInto(char *into, int32 fromOffset, int32 length) const
{
	if (into) {
		int32 len = min(Length() - fromOffset , length);
		strncpy(into, _privateData + fromOffset, len);
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
	int32 len = Length();
	_GrowBy(count);
	memset(_privateData + len, c, count);

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
	_OpenAtBy(0, count);
	memset(_privateData, c, count);
	
	return *this;
}


/*---- Inserting ----------------------------------------------------------*/
BString&
BString::Insert(const char *str, int32 pos)
{
	if (str) {
		if (pos < 0) {
			str -= pos;
			pos = 0;
		}
		_privateData = _OpenAtBy(pos, strlen(str));
		memcpy(_privateData + pos, str, strlen(str));
	}
	return *this;
}


BString&
BString::Insert(const char *str, int32 length, int32 pos)
{
	if (str) {
		if (pos < 0) {
			str -= pos;
			pos = 0;
		}
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
		len = min(len - fromOffset, length);
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
	_OpenAtBy(pos, count);
	memset(_privateData + pos, c, count);
	
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
			; //ToDo: Implement?
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

// These are implemented inline in the header file

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
	
	return _FindAfter(tmp, 0, -1);	
}


int32
BString::FindFirst(char c, int32 fromOffset) const
{
	char tmp[2] = { c, '\0' };
	
	return _FindAfter(tmp, fromOffset, -1);	
}


int32
BString::FindLast(const BString &string) const
{
	return _FindBefore(string.String(), Length(), -1);
}


int32
BString::FindLast(const char *str) const
{
	return _FindBefore(str, Length(), -1);
}


int32
BString::FindLast(const BString &string, int32 beforeOffset) const
{
	return _FindBefore(string.String(), beforeOffset, -1); 
}


int32
BString::FindLast(char c) const
{
	char tmp[2] = { c, '\0' };
	
	return _FindBefore(tmp, Length(), -1);
}


int32
BString::IFindFirst(const BString &string) const
{
	return _IFindAfter(string.String(), 0, -1);
}


int32
BString::IFindFirst(const char *string) const
{
	return _IFindAfter(string, 0, -1);
}


int32
BString::IFindFirst(const BString &string, int32 fromOffset) const
{
	return _IFindAfter(string.String(), fromOffset, -1);
}


int32
BString::IFindFirst(const char *string, int32 fromOffset) const
{
	return _IFindAfter(string, fromOffset, -1);
}


int32
BString::IFindLast(const BString &string) const
{
	return _IFindBefore(string.String(), Length(), -1);
}


int32
BString::IFindLast(const char *string) const
{
	return _IFindBefore(string, Length(), -1);
}


int32
BString::IFindLast(const BString &string, int32 beforeOffset) const
{
	return _IFindBefore(string.String(), beforeOffset, -1);
}


int32
BString::IFindLast(const char *string, int32 beforeOffset) const
{
	return _IFindBefore(string, beforeOffset, -1);
}


/*---- Replacing -----------------------------------------------------------*/
BString&
BString::ReplaceFirst(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };
	int32 pos = _FindAfter(tmp, 0, -1);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	return *this;
}


BString&
BString::ReplaceLast(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };
	int32 pos = _FindBefore(tmp, Length(), -1);
	
	if (pos >= 0)
		_privateData[pos] = withThis;
	return *this;
}


BString&
BString::ReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	int32 pos = B_ERROR;
	char tmp[2] = { replaceThis, '\0' };
	
	while ((pos = _FindAfter(tmp, fromOffset, -1)) >= 0) {
		_privateData[pos] = withThis;
		fromOffset += pos;
	}
	return *this;
}


BString&
BString::Replace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	int32 pos = B_ERROR;
	int32 replaceCount = 0;
	char tmp[2] = { replaceThis, '\0' };
	
	if (maxReplaceCount <= 0)
		return *this;
		
	while ((pos = _FindAfter(tmp, fromOffset, -1)) >= 0) {
		_privateData[pos] = withThis;
		fromOffset += pos;
		
		if (++replaceCount == maxReplaceCount)
			break;
	}
	return *this;
}


BString&
BString::ReplaceFirst(const char *replaceThis, const char *withThis)
{
	if (replaceThis == NULL)
		return *this;
	
	int32 pos = _FindAfter(replaceThis, 0, -1);
	if (pos >= 0) {
		int32 len = (withThis ? strlen(withThis) : 0);
		int32 difference = len - strlen(replaceThis);
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BString&
BString::ReplaceLast(const char *replaceThis, const char *withThis)
{
	if (replaceThis == NULL)
		return *this;
		
	int32 pos = _FindBefore(replaceThis, Length(), -1);
	if (pos >= 0) {
		int32 len = (withThis ? strlen(withThis) : 0);
		int32 difference = len - strlen(replaceThis);
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BString&
BString::ReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	if (replaceThis == NULL)
		return *this;
	
	int32 len = (withThis ? strlen(withThis) : 0);
	int32 difference = len - strlen(replaceThis);
	
	int32 pos; 
	while ((pos = _FindAfter(replaceThis, fromOffset, -1)) >= 0) {
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
		fromOffset += pos;
	}

	return *this;
}


BString&
BString::Replace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	if (replaceThis == NULL || maxReplaceCount <= 0)
		return *this;
	
	int32 replaceCount = 0;	
	int32 len = (withThis ? strlen(withThis) : 0);
	int32 difference = len - strlen(replaceThis);
	
	int32 pos; 
	while ((pos = _FindAfter(replaceThis, fromOffset, -1)) >= 0) {
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
		fromOffset += pos;
		
		if (++replaceCount == maxReplaceCount)
			break;
	}
	return *this;
}


BString&
BString::IReplaceFirst(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };
	int32 pos = _FindAfter(tmp, 0, -1);
	if (pos >= 0)
		_privateData[pos] = withThis;

	return *this;
}


BString&
BString::IReplaceLast(char replaceThis, char withThis)
{
	char tmp[2] = { replaceThis, '\0' };	
	int32 pos = _IFindBefore(tmp, Length(), -1);
	if (pos >= 0)
		_privateData[pos] = withThis;
	
	return *this;
}


BString&
BString::IReplaceAll(char replaceThis, char withThis, int32 fromOffset)
{
	int32 pos = B_ERROR;
	char tmp[2] = { replaceThis, '\0' };
	
	while ((pos = _IFindAfter(tmp, fromOffset, -1)) >= 0) {
		_privateData[pos] = withThis;
		fromOffset += pos;
	}
	return *this;
}


BString&
BString::IReplace(char replaceThis, char withThis, int32 maxReplaceCount, int32 fromOffset)
{
	int32 pos = B_ERROR;
	int32 replaceCount = 0;
	char tmp[2] = { replaceThis, '\0' };
	
	if (_privateData == NULL || maxReplaceCount <= 0)
		return *this;
		
	while ((pos = _FindAfter(tmp, fromOffset, -1)) >= 0) {
		_privateData[pos] = withThis;
		fromOffset += pos;
		
		if (++replaceCount == maxReplaceCount)
			break;
	}
	return *this;
}


BString&
BString::IReplaceFirst(const char *replaceThis, const char *withThis)
{
	if (replaceThis == NULL)
		return *this;
		
	int32 pos = _IFindAfter(replaceThis, 0, -1);
	if (pos >= 0) {
		int32 len = (withThis ? strlen(withThis) : 0);
		int32 difference = len - strlen(replaceThis);
		
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BString&
BString::IReplaceLast(const char *replaceThis, const char *withThis)
{
	if (replaceThis == NULL)
		return *this;
		
	int32 pos = _FindBefore(replaceThis, Length(), -1);
	if (pos >= 0) {
		int32 len = (withThis ? strlen(withThis) : 0);
		int32 difference = len - strlen(replaceThis);
		
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
	}
		
	return *this;
}


BString&
BString::IReplaceAll(const char *replaceThis, const char *withThis, int32 fromOffset)
{
	if (replaceThis == NULL)
		return *this;
	
	int32 len = (withThis ? strlen(withThis) : 0);
	int32 difference = len - strlen(replaceThis);
	
	int32 pos; 
	while ((pos = _IFindAfter(replaceThis, fromOffset, -1))>= 0) {
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
		fromOffset += pos;
	}

	return *this;
}


BString&
BString::IReplace(const char *replaceThis, const char *withThis, int32 maxReplaceCount, int32 fromOffset)
{
	if (replaceThis == NULL || maxReplaceCount <= 0)
		return *this;
	
	int32 replaceCount = 0;	
	int32 len = (withThis ? strlen(withThis) : 0);
	int32 difference = len - strlen(replaceThis);
	
	int32 pos; 
	while ((pos = _IFindAfter(replaceThis, fromOffset, -1)) >= 0) {
		if (difference > 0)
			_OpenAtBy(pos, difference);
		else if (difference < 0)
			_ShrinkAtBy(pos, -difference);
		memcpy(_privateData + pos, withThis, len);
		fromOffset += pos;
		
		if (++replaceCount == maxReplaceCount)
			break;
	}
	return *this;
}


BString&
BString::ReplaceSet(const char *setOfChars, char with)
{
	int32 pos;
	int32 length = Length();
	while ((pos = strcspn(String(), setOfChars)) < length)
		_privateData[pos] = with;
	return *this;
}


BString&
BString::ReplaceSet(const char *setOfChars, const char *with)
{
	//TODO: Implement
	return *this;
}


/*---- Unchecked char access -----------------------------------------------*/
char &
BString::operator[](int32 index)
{
	return _privateData[index];
}


/*---- Fast low-level manipulation -----------------------------------------*/
char*
BString::LockBuffer(int32 maxLength)
{
	_SetUsingAsCString(true); //debug
	
	int32 len = Length();
	
	if (maxLength > len)
		_privateData = _GrowBy(maxLength - len);

	return _privateData;
}


BString&
BString::UnlockBuffer(int32 length)
{
	_SetUsingAsCString(false); //debug
	
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
	int32 length = Length();
	for (int32 count = 0; count < length; count++)
			_privateData[count] = tolower(_privateData[count]);
	
	return *this;
}


BString&
BString::ToUpper()
{			
	int32 length = Length();
	for (int32 count = 0; count < length; count++)
			_privateData[count] = toupper(_privateData[count]);
	
	return *this;
}


BString&
BString::Capitalize()
{
	if (_privateData == NULL)
		return *this;
		
	_privateData[0] = toupper(_privateData[0]);
	int32 length = Length();
		
	for (int32 count = 1; count < length; count++)
			_privateData[count] = tolower(_privateData[count]);

	return *this;
}


BString&
BString::CapitalizeEachWord()
{
	if (_privateData == NULL)
		return *this;
		
	int32 count = 0;
	int32 length = Length();
		
	do {
		// Find the first alphabetical character	
		for(; count < length; count++) {
			if (isalpha(_privateData[count])) {
				_privateData[count] = toupper(_privateData[count]);
				count++;
				break;
			}
		}
		// Find the first non-alphabetical character,
		// and meanwhile, turn to lowercase all the alphabetical ones
		for(; count < length; count++) {
			if (isalpha(_privateData[count]))
				_privateData[count] = tolower(_privateData[count]);
			else
				break;
		}
	} while (count < length);
				
	return *this;
}


/*----- Escaping and Deescaping --------------------------------------------*/
BString&
BString::CharacterEscape(const char *original, const char *setOfCharsToEscape, char escapeWith)
{
	SetTo(original);
	CharacterEscape(setOfCharsToEscape, escapeWith);
	
	return *this;
}


BString&
BString::CharacterEscape(const char *setOfCharsToEscape, char escapeWith)
{
	if (setOfCharsToEscape == NULL || _privateData == NULL)
		return *this;
	
	int32 pos, offset = 0;
	
	for(;;) {
		pos = strcspn(_privateData + offset, setOfCharsToEscape);
		offset += pos;
		if (offset >= Length())
			break;
		_OpenAtBy(offset, 1);
		memset(_privateData + offset, escapeWith, 1);
		offset += 2;
	}
		
	return *this;
}


BString&
BString::CharacterDeescape(const char *original, char escapeChar)
{
	SetTo(original);	
	CharacterDeescape(escapeChar);
		
	return *this;
}


BString&
BString::CharacterDeescape(char escapeChar)
{
	if (_privateData == NULL)
		return *this;
		
	char tmp[2] = { escapeChar, '\0' };
	RemoveAll(tmp);
	
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
	char num[64];
	sprintf(num, "%d", i);
	
	return *this << num;
}


BString&
BString::operator<<(unsigned int i)
{
	char num[64];
	sprintf(num, "%u", i);
	
	return *this << num;
}


BString&
BString::operator<<(uint32 i)
{
	char num[64];
	sprintf(num, "%lu", i);
	
	return *this << num;
}


BString&
BString::operator<<(int32 i)
{
	char num[64];
	sprintf(num, "%ld", i);
	
	return *this << num;
}


BString&
BString::operator<<(uint64 i)
{
	char num[64];
	sprintf(num, "%llu", i);
	
	return *this << num;
}


BString&
BString::operator<<(int64 i)
{
	char num[64];
	sprintf(num, "%lld", i);
	
	return *this << num;
}


BString&
BString::operator<<(float f)
{
	char num[64];
	sprintf(num, "%.2f", f);
	
	return *this << num;
}


/*---- Private or Reserved ------------------------------------------------*/
void
BString::_Init(const char* str, int32 len)
{
	ASSERT(str != NULL);
	_privateData = _GrowBy(len);
	memcpy(_privateData, str, len);
}


void
BString::_DoAssign(const char *str, int32 len)
{
	ASSERT(str != NULL);	
	int32 curLen = Length();
	
	if (len != curLen)
		_privateData = _GrowBy(len - curLen);
		
	memcpy(_privateData, str, len);
}


void
BString::_DoAppend(const char *str, int32 len)
{
	ASSERT(str != NULL);
	
	int32 length = Length();
	_privateData = _GrowBy(len);
	memcpy(_privateData + length, str, len);
}


char*
BString::_GrowBy(int32 size)
{		
	int32 curLen = Length(); 	
	ASSERT(curLen + size >= 0);
	
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
	ASSERT(offset >= 0);
			
	int32 oldLength = Length();
	
	if (_privateData != NULL)
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
	ASSERT(offset + length <= Length());
	
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
	ASSERT(str != NULL);
	_privateData = _OpenAtBy(0, count);
	memcpy(_privateData, str, count);
}


int32
BString::_FindAfter(const char *str, int32 offset, int32) const
{	
	if (!str)
		return 0;
	
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
	if (!str)
		return 0;

	if (offset > Length())
		return B_ERROR;

	char *ptr = strcasestr(String() + offset, str);

	if (ptr != NULL)
		return ptr - (String() + offset);

	return B_ERROR;
}


int32
BString::_ShortFindAfter(const char *str, int32) const
{
	ASSERT(str != NULL);
	//TODO: Implement?
	return B_ERROR;
}


int32
BString::_FindBefore(const char *str, int32 offset, int32) const
{
	if (!str)
		return 0;
	
	if (offset <= 0)
		return B_ERROR;
	
	int len2 = strlen(str);

	if (len2 == 0)
		return 0;
		
	char *ptr1 = _privateData + offset - len2;
	
	while (ptr1 >= _privateData) {
		if (!memcmp(ptr1, str, len2))
			return ptr1 - _privateData; 
		ptr1--;
	}
	
	return B_ERROR;
}


int32
BString::_IFindBefore(const char *str, int32 offset, int32) const
{
	if (!str)
		return 0;
	
	if (offset <= 0)
		return B_ERROR;
	
	int len2 = strlen(str);

	if (len2 == 0)
		return 0;
		
	char *ptr1 = _privateData + offset - len2;
	
	while (ptr1 >= _privateData) {
		if (!strncasecmp(ptr1, str, len2))
			return ptr1 - _privateData; 
		ptr1--;
	}
	
	return B_ERROR;
}


void
BString::_SetLength(int32 length)
{
	*((int32*)_privateData - 1) = length & 0x7fffffff;
}


#if DEBUG
// AFAIK, these are not implemented in BeOS R5
void
BString::_SetUsingAsCString(bool state)
{
}


void
BString::_AssertNotUsingAsCString() const
{
}
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
