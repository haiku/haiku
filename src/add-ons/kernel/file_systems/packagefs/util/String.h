/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRING_H
#define STRING_H


#include "StringPool.h"


class String {
public:
								String();
								String(const String& other);
								~String();

			bool				SetTo(const char* string);
			bool				SetTo(const char* string, size_t maxLength);
			bool				SetToExactLength(const char* string,
									size_t length);

			const char*			Data() const;
			uint32				Hash() const;

			bool				IsEmpty() const;

			String&				operator=(const String& other);

			bool				operator==(const String& other) const;
			bool				operator!=(const String& other) const;

								operator const char*() const;

private:
			StringData*			fData;
};


inline
String::String()
	:
	fData(StringData::GetEmpty())
{
}


inline
String::String(const String& other)
	:
	fData(other.fData)
{
	fData->AcquireReference();
}


inline
String::~String()
{
	fData->ReleaseReference();
}


inline bool
String::SetTo(const char* string)
{
	return SetToExactLength(string, strlen(string));
}


inline bool
String::SetTo(const char* string, size_t maxLength)
{
	return SetToExactLength(string, strnlen(string, maxLength));
}


inline const char*
String::Data() const
{
	return fData->String();
}


inline uint32
String::Hash() const
{
	return fData->Hash();
}


inline bool
String::IsEmpty() const
{
	return fData == StringData::Empty();
}


inline bool
String::operator==(const String& other) const
{
	return fData == other.fData;
}


inline bool
String::operator!=(const String& other) const
{
	return !(*this == other);
}


inline
String::operator const char*() const
{
	return fData->String();
}


#endif	// STRING_H
