#include "sniffer/CharStream.h"

#include <sniffer/Err.h>

using namespace BPrivate::Storage::Sniffer;

//------------------------------------------------------------------------------
// CharStream
//------------------------------------------------------------------------------

/*! \brief Creates a new, initialized character stream

	\param string The character string to be streamed
*/
CharStream::CharStream(const std::string &string)
	: fString(string)
	, fPos(0)
	, fCStatus(B_OK)		
{
}

/*! \brief Creates a new, unitialized character stream

	Call SetTo() to initialize the stream.
*/
CharStream::CharStream()
	: fString("")
	, fPos(0)
	, fCStatus(B_NO_INIT)
{
}

/*! \brief Destroys the character stream
*/
CharStream::~CharStream() {
	Unset();
}

/*! \brief Reinitializes the character stream to the given string

	The stream position is reset to the beginning of the stream.

	\param string The new character string to be streamed
	\return Returns \c B_OK
*/
status_t
CharStream::SetTo(const std::string &string) {
	fString = string;
	fPos = 0;
	fCStatus = B_OK;
	return fCStatus;
}

/*! \brief Unitializes the stream
*/
void
CharStream::Unset() {
	fString = "";
	fPos = 0;
	fCStatus = B_NO_INIT;
}
	
/*! \brief Returns the current status of the stream

	\return
	- \c B_OK: Ready and initialized
	- \c B_NO_INIT: Unitialized
*/
status_t
CharStream::InitCheck() const {
	return fCStatus;
}

/*! \brief Returns \c true if there are no more characters in the stream.

	If the stream is unitialized, \c true is also returned.
*/
bool
CharStream::IsEmpty() const {
	return fPos >= fString.length();
}

/*! \brief Returns the current offset of the stream into the original string.

	If the stream is unitialized, zero is returned.
*/
size_t
CharStream::Pos() const {
	return fPos;
}

/*! \brief Returns the entire string being streamed.
*/
const std::string&
CharStream::String() const {
	return fString;
}

/*! Returns the next character in the stream.

	Also increments the position in the stream. Call Unget() to
	undo this operation.

	Throws a BPrivate::Storage::Sniffer::Err exception if the stream is
	unitialized. If the end of the stream has been reached, the position
	marker is still incremented, but an end of text	char (\c 0x03) is
	returned.
*/
char
CharStream::Get() {
	if (fCStatus != B_OK)
		throw new Err("Sniffer parser error: CharStream::Get() called on uninitialized CharStream object", -1);
	if (fPos < fString.length()) 
		return fString[fPos++];
	else {
		fPos++;		// Increment fPos to keep Unget()s consistent
		return 0x3;	// Return End-Of-Text char
	}
}

/*! Shifts the stream position back one character.

	Throws a BPrivate::Storage::Sniffer::Err exception if the stream is
	unitialized or there are no more characters to unget.
*/
void
CharStream::Unget() {
	if (fCStatus != B_OK)
		throw new Err("Sniffer parser error: CharStream::Unget() called on uninitialized CharStream object", -1);
	if (fPos > 0) 
		fPos--;
	else
		throw new Err("Sniffer parser error: CharStream::Unget() called at beginning of character stream", -1);
}

