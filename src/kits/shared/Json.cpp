/*
 * Copyright 2017-2023, Andrew Lindesay <apl@lindesay.co.nz>
 * Copyright 2014-2017, Augustin Cavalier (waddlesplash)
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include "Json.h"

#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cerrno>

#include <AutoDeleter.h>
#include <DataIO.h>
#include <UnicodeChar.h>

#include "JsonEventListener.h"
#include "JsonMessageWriter.h"


// #pragma mark - Public methods

namespace BPrivate {

/*!	A buffer is used to assemble strings into. This will be the initial size
	of this buffer.
*/

static const size_t kInitialAssemblyBufferSize = 64;

/*!	A buffer is used to assemble strings into. This buffer starts off small
	but is able to grow as the string it needs to process as encountered. To
	avoid frequent reallocation of the buffer, the buffer will be retained
	between strings. This is the maximum size of buffer that will be retained.
*/

static const size_t kRetainedAssemblyBufferSize = 32 * 1024;

static const size_t kAssemblyBufferSizeIncrement = 256;

static const size_t kMaximumUtf8SequenceLength = 7;


class JsonParseAssemblyBuffer {
public:
	JsonParseAssemblyBuffer()
		:
		fAssemblyBuffer(NULL),
		fAssemblyBufferAllocatedSize(0),
		fAssemblyBufferUsedSize(0)
	{
		fAssemblyBuffer = (char*) malloc(kInitialAssemblyBufferSize);
		if (fAssemblyBuffer != NULL)
			fAssemblyBufferAllocatedSize = kInitialAssemblyBufferSize;
	}

	~JsonParseAssemblyBuffer()
	{
		if (fAssemblyBuffer != NULL)
			free(fAssemblyBuffer);
	}

	const char* Buffer() const
	{
		return fAssemblyBuffer;
	}

	/*! This method should be used each time that the assembly buffer has
		been finished with by some section of logic.
	*/

	status_t Reset()
	{
		fAssemblyBufferUsedSize = 0;

		if (fAssemblyBufferAllocatedSize > kRetainedAssemblyBufferSize) {
			fAssemblyBuffer = (char*) realloc(fAssemblyBuffer, kRetainedAssemblyBufferSize);
			if (fAssemblyBuffer == NULL) {
				fAssemblyBufferAllocatedSize = 0;
				return B_NO_MEMORY;
			}
			fAssemblyBufferAllocatedSize = kRetainedAssemblyBufferSize;
		}

		return B_OK;
	}

	status_t AppendCharacter(char c)
	{
		status_t result = _EnsureAssemblyBufferAllocatedSize(fAssemblyBufferUsedSize + 1);

		if (result == B_OK) {
			fAssemblyBuffer[fAssemblyBufferUsedSize] = c;
			fAssemblyBufferUsedSize++;
		}

		return result;
	}

	status_t AppendCharacters(char* str, size_t len)
	{
		status_t result = _EnsureAssemblyBufferAllocatedSize(fAssemblyBufferUsedSize + len);

		if (result == B_OK) {
			memcpy(&fAssemblyBuffer[fAssemblyBufferUsedSize], str, len);
			fAssemblyBufferUsedSize += len;
		}

		return result;
	}

	status_t AppendUnicodeCharacter(uint32 c)
	{
		status_t result = _EnsureAssemblyBufferAllocatedSize(
			fAssemblyBufferUsedSize + kMaximumUtf8SequenceLength);
		if (result == B_OK) {
			char* insertPtr = &fAssemblyBuffer[fAssemblyBufferUsedSize];
			char* ptr = insertPtr;
			BUnicodeChar::ToUTF8(c, &ptr);
			size_t sequenceLength = static_cast<uint32>(ptr - insertPtr);
			fAssemblyBufferUsedSize += sequenceLength;
		}

		return result;
	}

private:

	/*!	This method will return the assembly buffer ensuring that it has at
		least `minimumSize` bytes available.
	*/

	status_t _EnsureAssemblyBufferAllocatedSize(size_t minimumSize)
	{
		if (fAssemblyBufferAllocatedSize < minimumSize) {
			size_t requestedSize = minimumSize;

			// if the requested quantity of memory is less than the retained buffer size then
			// it makes sense to request a wee bit more in order to reduce the number of small
			// requests to increment the buffer over time.

			if (requestedSize < kRetainedAssemblyBufferSize - kAssemblyBufferSizeIncrement) {
				requestedSize = ((requestedSize / kAssemblyBufferSizeIncrement) + 1)
					* kAssemblyBufferSizeIncrement;
			}

			fAssemblyBuffer = (char*) realloc(fAssemblyBuffer, requestedSize);
			if (fAssemblyBuffer == NULL) {
				fAssemblyBufferAllocatedSize = 0;
				return B_NO_MEMORY;
			}
			fAssemblyBufferAllocatedSize = requestedSize;
		}
		return B_OK;
	}

private:
	char*					fAssemblyBuffer;
	size_t					fAssemblyBufferAllocatedSize;
	size_t					fAssemblyBufferUsedSize;
};


class JsonParseAssemblyBufferResetter {
public:
	JsonParseAssemblyBufferResetter(JsonParseAssemblyBuffer* assemblyBuffer)
		:
		fAssemblyBuffer(assemblyBuffer)
	{
	}

	~JsonParseAssemblyBufferResetter()
	{
		fAssemblyBuffer->Reset();
	}

private:
	JsonParseAssemblyBuffer*
							fAssemblyBuffer;
};


/*! This class carries state around the parsing process. */

class JsonParseContext {
public:
	JsonParseContext(BDataIO* data, BJsonEventListener* listener)
		:
		fListener(listener),
		fData(data),
		fLineNumber(1), // 1 is the first line
		fPushbackChar(0),
		fHasPushbackChar(false),
		fAssemblyBuffer(new JsonParseAssemblyBuffer())
	{
	}


	~JsonParseContext()
	{
		delete fAssemblyBuffer;
	}


	BJsonEventListener* Listener() const
	{
		return fListener;
	}


	BDataIO* Data() const
	{
		return fData;
	}


	int LineNumber() const
	{
		return fLineNumber;
	}


	void IncrementLineNumber()
	{
		fLineNumber++;
	}

	status_t NextChar(char* buffer)
	{
		if (fHasPushbackChar) {
			buffer[0] = fPushbackChar;
			fHasPushbackChar = false;
			return B_OK;
		}

		return Data()->ReadExactly(buffer, 1);
	}

	void PushbackChar(char c)
	{
		if (fHasPushbackChar)
			debugger("illegal state - more than one character pushed back");
		fPushbackChar = c;
		fHasPushbackChar = true;
	}


	JsonParseAssemblyBuffer* AssemblyBuffer()
	{
		return fAssemblyBuffer;
	}


private:
	BJsonEventListener*		fListener;
	BDataIO*				fData;
	uint32					fLineNumber;
	char					fPushbackChar;
	bool					fHasPushbackChar;
	JsonParseAssemblyBuffer*
							fAssemblyBuffer;
};


status_t
BJson::Parse(const BString& JSON, BMessage& message)
{
	return Parse(JSON.String(), message);
}


status_t
BJson::Parse(const char* JSON, size_t length, BMessage& message)
{
	BMemoryIO* input = new BMemoryIO(JSON, length);
	ObjectDeleter<BMemoryIO> inputDeleter(input);
	BJsonMessageWriter* writer = new BJsonMessageWriter(message);
	ObjectDeleter<BJsonMessageWriter> writerDeleter(writer);

	Parse(input, writer);
	status_t result = writer->ErrorStatus();

	return result;
}


status_t
BJson::Parse(const char* JSON, BMessage& message)
{
	return Parse(JSON, strlen(JSON), message);
}


/*! The data is read as a stream of JSON data.  As the JSON is read, events are
    raised such as;
     - string
     - number
     - true
     - array start
     - object end
    Each event is sent to the listener to process as required.
*/

void
BJson::Parse(BDataIO* data, BJsonEventListener* listener)
{
	JsonParseContext context(data, listener);
	ParseAny(context);
	listener->Complete();
}


// #pragma mark - Specific parse logic.


bool
BJson::NextChar(JsonParseContext& jsonParseContext, char* c)
{
	status_t result = jsonParseContext.NextChar(c);

	switch (result) {
		case B_OK:
			return true;

		case B_PARTIAL_READ:
		{
			jsonParseContext.Listener()->HandleError(B_BAD_DATA,
				jsonParseContext.LineNumber(), "unexpected end of input");
			return false;
		}

		default:
		{
			jsonParseContext.Listener()->HandleError(result, -1,
				"io related read error");
			return false;
		}
	}
}


bool
BJson::NextNonWhitespaceChar(JsonParseContext& jsonParseContext, char* c)
{
	while (true) {
		if (!NextChar(jsonParseContext, c))
			return false;

		switch (*c) {
			case 0x0a: // newline
			case 0x0d: // cr
				jsonParseContext.IncrementLineNumber();
			case ' ': // space
					// swallow whitespace as it is not syntactically
					// significant.
				break;

			default:
				return true;
		}
	}
}


bool
BJson::ParseAny(JsonParseContext& jsonParseContext)
{
	char c;

	if (!NextNonWhitespaceChar(jsonParseContext, &c))
		return false;

	switch (c) {
		case 'f': // [f]alse
			return ParseExpectedVerbatimStringAndRaiseEvent(
				jsonParseContext, "alse", 4, 'f', B_JSON_FALSE);

		case 't': // [t]rue
			return ParseExpectedVerbatimStringAndRaiseEvent(
				jsonParseContext, "rue", 3, 't', B_JSON_TRUE);

		case 'n': // [n]ull
			return ParseExpectedVerbatimStringAndRaiseEvent(
				jsonParseContext, "ull", 3, 'n', B_JSON_NULL);

		case '"':
			return ParseString(jsonParseContext, B_JSON_STRING);

		case '{':
			return ParseObject(jsonParseContext);

		case '[':
			return ParseArray(jsonParseContext);

		case '+':
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			jsonParseContext.PushbackChar(c); // keeps the parse simple
			return ParseNumber(jsonParseContext);

		default:
		{
			BString errorMessage;
			if (c >= 0x20 && c < 0x7f) {
				errorMessage.SetToFormat("unexpected character [%" B_PRIu8 "]"
					" (%c) when parsing element", static_cast<uint8>(c), c);
			} else {
				errorMessage.SetToFormat("unexpected character [%" B_PRIu8 "]"
					" when parsing element", (uint8) c);
			}
			jsonParseContext.Listener()->HandleError(B_BAD_DATA,
				jsonParseContext.LineNumber(), errorMessage.String());
			return false;
		}
	}

	return true;
}


/*! This method captures an object name, a separator ':' and then any value. */

bool
BJson::ParseObjectNameValuePair(JsonParseContext& jsonParseContext)
{
	bool didParseName = false;
	char c;

	while (true) {
		if (!NextNonWhitespaceChar(jsonParseContext, &c))
			return false;

		switch (c) {
			case '\"': // name of the object
			{
				if (!didParseName) {
					if (!ParseString(jsonParseContext, B_JSON_OBJECT_NAME))
						return false;

					didParseName = true;
				} else {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "unexpected"
							" [\"] character when parsing object name-"
							" value separator");
					return false;
				}
				break;
			}

			case ':': // separator
			{
				if (didParseName) {
					if (!ParseAny(jsonParseContext))
						return false;
					return true;
				} else {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "unexpected"
							" [:] character when parsing object name-"
							" value pair");
					return false;
				}
			}

			default:
			{
				BString errorMessage;
				errorMessage.SetToFormat(
					"unexpected character [%c] when parsing object"
					" name-value pair",
					c);
				jsonParseContext.Listener()->HandleError(B_BAD_DATA,
					jsonParseContext.LineNumber(), errorMessage.String());
				return false;
			}
		}
	}
}


bool
BJson::ParseObject(JsonParseContext& jsonParseContext)
{
	if (!jsonParseContext.Listener()->Handle(
			BJsonEvent(B_JSON_OBJECT_START))) {
		return false;
	}

	char c;
	bool firstItem = true;

	while (true) {
		if (!NextNonWhitespaceChar(jsonParseContext, &c))
			return false;

		switch (c) {
			case '}': // terminate the object
			{
				if (!jsonParseContext.Listener()->Handle(
						BJsonEvent(B_JSON_OBJECT_END))) {
					return false;
				}
				return true;
			}

			case ',': // next value.
			{
				if (firstItem) {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "unexpected"
							" item separator when parsing start of"
							" object");
					return false;
				}

				if (!ParseObjectNameValuePair(jsonParseContext))
					return false;
				break;
			}

			default:
			{
				if (firstItem) {
					jsonParseContext.PushbackChar(c);
					if (!ParseObjectNameValuePair(jsonParseContext))
						return false;
					firstItem = false;
				} else {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "expected"
							" separator when parsing an object");
				}
			}
		}
	}

	return true;
}


bool
BJson::ParseArray(JsonParseContext& jsonParseContext)
{
	if (!jsonParseContext.Listener()->Handle(
			BJsonEvent(B_JSON_ARRAY_START))) {
		return false;
	}

	char c;
	bool firstItem = true;

	while (true) {
		if (!NextNonWhitespaceChar(jsonParseContext, &c))
			return false;

		switch (c) {
			case ']': // terminate the array
			{
				if (!jsonParseContext.Listener()->Handle(
						BJsonEvent(B_JSON_ARRAY_END))) {
					return false;
				}
				return true;
			}

			case ',': // next value.
			{
				if (firstItem) {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "unexpected"
							" item separator when parsing start of"
							" array");
				}

				if (!ParseAny(jsonParseContext))
					return false;
				break;
			}

			default:
			{
				if (firstItem) {
					jsonParseContext.PushbackChar(c);
					if (!ParseAny(jsonParseContext))
						return false;
					firstItem = false;
				} else {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "expected"
							" separator when parsing an array");
				}
			}
		}
	}

	return true;
}


bool
BJson::ParseEscapeUnicodeSequence(JsonParseContext& jsonParseContext)
{
	char ch;
	uint32 unicodeCh = 0;

	for (int i = 3; i >= 0; i--) {
		if (!NextChar(jsonParseContext, &ch)) {
			jsonParseContext.Listener()->HandleError(B_ERROR, jsonParseContext.LineNumber(),
				"unable to read unicode sequence");
			return false;
		}

		if (ch >= '0' && ch <= '9')
			unicodeCh |= static_cast<uint32>(ch - '0') << (i * 4);
		else if (ch >= 'a' && ch <= 'f')
			unicodeCh |= (10 + static_cast<uint32>(ch - 'a')) << (i * 4);
		else if (ch >= 'A' && ch <= 'F')
			unicodeCh |= (10 + static_cast<uint32>(ch - 'A')) << (i * 4);
		else {
			BString errorMessage;
			errorMessage.SetToFormat(
				"malformed hex character [%c] in unicode sequence in string parsing", ch);
			jsonParseContext.Listener()->HandleError(B_BAD_DATA, jsonParseContext.LineNumber(),
				errorMessage.String());
			return false;
		}
	}

	JsonParseAssemblyBuffer* assemblyBuffer = jsonParseContext.AssemblyBuffer();
	status_t result = assemblyBuffer->AppendUnicodeCharacter(unicodeCh);

	if (result != B_OK) {
		jsonParseContext.Listener()->HandleError(result, jsonParseContext.LineNumber(),
			"unable to store unicode char as utf-8");
		return false;
	}

	return true;
}


bool
BJson::ParseStringEscapeSequence(JsonParseContext& jsonParseContext)
{
	char c;

	if (!NextChar(jsonParseContext, &c))
		return false;

	JsonParseAssemblyBuffer* assemblyBuffer = jsonParseContext.AssemblyBuffer();

	switch (c) {
		case 'n':
			assemblyBuffer->AppendCharacter('\n');
			break;
		case 'r':
			assemblyBuffer->AppendCharacter('\r');
			break;
		case 'b':
			assemblyBuffer->AppendCharacter('\b');
			break;
		case 'f':
			assemblyBuffer->AppendCharacter('\f');
			break;
		case '\\':
			assemblyBuffer->AppendCharacter('\\');
			break;
		case '/':
			assemblyBuffer->AppendCharacter('/');
			break;
		case 't':
			assemblyBuffer->AppendCharacter('\t');
			break;
		case '"':
			assemblyBuffer->AppendCharacter('"');
			break;
		case 'u':
		{
				// unicode escape sequence.
			if (!ParseEscapeUnicodeSequence(jsonParseContext)) {
				return false;
			}
			break;
		}
		default:
		{
			BString errorMessage;
			errorMessage.SetToFormat("unexpected escaped character [%c] in string parsing", c);
			jsonParseContext.Listener()->HandleError(B_BAD_DATA,
				jsonParseContext.LineNumber(), errorMessage.String());
			return false;
		}
	}

	return true;
}


bool
BJson::ParseString(JsonParseContext& jsonParseContext,
	json_event_type eventType)
{
	char c;
	JsonParseAssemblyBuffer* assemblyBuffer = jsonParseContext.AssemblyBuffer();
	JsonParseAssemblyBufferResetter assembleBufferResetter(assemblyBuffer);

	while(true) {
		if (!NextChar(jsonParseContext, &c))
    		return false;

		switch (c) {
			case '"':
			{
					// terminates the string assembled so far.
				assemblyBuffer->AppendCharacter(0);
				jsonParseContext.Listener()->Handle(
					BJsonEvent(eventType, assemblyBuffer->Buffer()));
				return true;
			}

			case '\\':
			{
				if (!ParseStringEscapeSequence(jsonParseContext))
					return false;
				break;
			}

			default:
			{
				uint8 uc = static_cast<uint8>(c);

				if(uc < 0x20) { // control characters are not allowed
					BString errorMessage;
					errorMessage.SetToFormat("illegal control character"
						" [%" B_PRIu8 "] when parsing a string", uc);
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(),
						errorMessage.String());
					return false;
				}

				assemblyBuffer->AppendCharacter(c);
				break;
			}
		}
	}
}


bool
BJson::ParseExpectedVerbatimStringAndRaiseEvent(
	JsonParseContext& jsonParseContext, const char* expectedString,
	size_t expectedStringLength, char leadingChar,
	json_event_type jsonEventType)
{
	if (ParseExpectedVerbatimString(jsonParseContext, expectedString,
			expectedStringLength, leadingChar)) {
		if (!jsonParseContext.Listener()->Handle(BJsonEvent(jsonEventType)))
			return false;
	}

	return true;
}

/*! This will make sure that the constant string is available at the input. */

bool
BJson::ParseExpectedVerbatimString(JsonParseContext& jsonParseContext,
	const char* expectedString, size_t expectedStringLength, char leadingChar)
{
	char c;
	size_t offset = 0;

	while (offset < expectedStringLength) {
		if (!NextChar(jsonParseContext, &c))
			return false;

		if (c != expectedString[offset]) {
			BString errorMessage;
			errorMessage.SetToFormat("malformed json primative literal; "
				"expected [%c%s], but got [%c] at position %" B_PRIdSSIZE,
				leadingChar, expectedString, c, offset);
			jsonParseContext.Listener()->HandleError(B_BAD_DATA,
				jsonParseContext.LineNumber(), errorMessage.String());
			return false;
		}

		offset++;
	}

	return true;
}


/*! This function checks to see that the supplied string is a well formed
    JSON number.  It does this from a string rather than a stream for
    convenience.  This is not anticipated to impact performance because
    the string values are short.
*/

bool
BJson::IsValidNumber(const char* value)
{
	int32 offset = 0;
	int32 len = strlen(value);

	if (offset < len && value[offset] == '-')
		offset++;

	if (offset >= len)
		return false;

	if (isdigit(value[offset]) && value[offset] != '0') {
		while (offset < len && isdigit(value[offset]))
			offset++;
	} else {
		if (value[offset] == '0')
			offset++;
		else
			return false;
	}

	if (offset < len && value[offset] == '.') {
		offset++;

		if (offset >= len)
			return false;

		while (offset < len && isdigit(value[offset]))
			offset++;
	}

	if (offset < len && (value[offset] == 'E' || value[offset] == 'e')) {
		offset++;

		if(offset < len && (value[offset] == '+' || value[offset] == '-'))
		 	offset++;

		if (offset >= len)
			return false;

		while (offset < len && isdigit(value[offset]))
			offset++;
	}

	return offset == len;
}


/*! Note that this method hits the 'NextChar' method on the context directly
    and handles any end-of-file state itself because it is feasible that the
    entire JSON payload is a number and because (unlike other structures, the
    number can take the end-of-file to signify the end of the number.
*/

bool
BJson::ParseNumber(JsonParseContext& jsonParseContext)
{
	JsonParseAssemblyBuffer* assemblyBuffer = jsonParseContext.AssemblyBuffer();
	JsonParseAssemblyBufferResetter assembleBufferResetter(assemblyBuffer);

	while (true) {
		char c;
		status_t result = jsonParseContext.NextChar(&c);

		switch (result) {
			case B_OK:
			{
				if (isdigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E' || c == '+') {
					assemblyBuffer->AppendCharacter(c);
					break;
				}

				jsonParseContext.PushbackChar(c);
				// intentional fall through
			}
			case B_PARTIAL_READ:
			{
				errno = 0;
				assemblyBuffer->AppendCharacter(0);

				if (!IsValidNumber(assemblyBuffer->Buffer())) {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "malformed number");
					return false;
				}

				jsonParseContext.Listener()->Handle(BJsonEvent(B_JSON_NUMBER,
					assemblyBuffer->Buffer()));

				return true;
			}
			default:
			{
				jsonParseContext.Listener()->HandleError(result, -1,
					"io related read error");
				return false;
			}
		}
	}
}

} // namespace BPrivate
