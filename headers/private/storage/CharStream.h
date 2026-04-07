/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHAR_STREAM_H
#define _CHAR_STREAM_H


#include <String.h>


namespace BPrivate {


//! Manages a stream of characters.
class CharStream {
public:
	CharStream(const BString& source)
		:
		fString(source),
		fPos(0)
	{
	}

	inline char Get();
	inline void Unget();

	inline status_t SetTo(const BString& source);
	inline bool IsEmpty() const;
	
	inline int32 Pos() const { return fPos; }

private:
	BString fString;
	int32 fPos;
};


/*! Returns the next character in the stream.

	Also increments the position in the stream. Call Unget() to
	undo this operation.

	If the end of the stream has been reached, the position marker
	is still incremented, but an end of text char (\c 0x03) is returned.
*/
char
CharStream::Get()
{
	if (fPos < fString.Length())
		return fString[fPos++];

	fPos++;
	return 0x3;
}


/*! Shifts the stream position back one character. */
void
CharStream::Unget()
{
	if (fPos > 0) {
		fPos--;
		return;
	}

	throw (status_t)B_BUFFER_OVERFLOW;
}


/*! \brief Reinitializes the character stream to the given string

	The stream position is reset to the beginning of the stream.

	\param string The new character string to be streamed
	\return Returns \c B_OK
*/
status_t
CharStream::SetTo(const BString& string)
{
	fString = string;
	fPos = 0;
	return B_OK;
}


/*! \brief Returns \c true if there are no more characters in the stream.

	If the stream is unitialized, \c true is also returned.
*/
bool
CharStream::IsEmpty() const
{
	return fPos >= fString.Length();
}


}	// namespace BPrivate


#endif // _CHAR_STREAM_H
