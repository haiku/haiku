/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
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


static bool
b_jsonparse_is_hex(char c)
{
	return isdigit(c)
		|| (c > 0x41 && c <= 0x46)
		|| (c > 0x61 && c <= 0x66);
}


static bool
b_jsonparse_all_hex(const char* c)
{
	for (int i = 0; i < 4; i++) {
		if (!b_jsonparse_is_hex(c[i]))
			return false;
	}

	return true;
}


/*! This class carries state around the parsing process. */

class JsonParseContext {
public:
	JsonParseContext(BDataIO* data, BJsonEventListener* listener)
		:
		fListener(listener),
		fData(data),
		fLineNumber(1), // 1 is the first line
		fPushbackChar(0),
		fHasPushbackChar(false)
	{
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


		// TODO; there is considerable opportunity for performance improvements
		// here by buffering the input and then feeding it into the parse
		// algorithm character by character.

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
		fPushbackChar = c;
		fHasPushbackChar = true;
	}

private:
	BJsonEventListener*		fListener;
	BDataIO*				fData;
	uint32					fLineNumber;
	char					fPushbackChar;
	bool					fHasPushbackChar;
};


status_t
BJson::Parse(const BString& JSON, BMessage& message)
{
	return Parse(JSON.String(), message);
}


status_t
BJson::Parse(const char* JSON, BMessage& message)
{
	BMemoryIO* input = new BMemoryIO(JSON, strlen(JSON));
	ObjectDeleter<BMemoryIO> inputDeleter(input);
	BJsonMessageWriter* writer = new BJsonMessageWriter(message);
	ObjectDeleter<BJsonMessageWriter> writerDeleter(writer);

	Parse(input, writer);
	status_t result = writer->ErrorStatus();

	return result;
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
BJson::ParseEscapeUnicodeSequence(JsonParseContext& jsonParseContext,
	BString& stringResult)
{
	char buffer[5];
	buffer[4] = 0;

	if (!NextChar(jsonParseContext, &buffer[0])
		|| !NextChar(jsonParseContext, &buffer[1])
		|| !NextChar(jsonParseContext, &buffer[2])
		|| !NextChar(jsonParseContext, &buffer[3])) {
		return false;
	}

	if (!b_jsonparse_all_hex(buffer)) {
		BString errorMessage;
		errorMessage.SetToFormat(
			"malformed unicode sequence [%s] in string parsing",
			buffer);
		jsonParseContext.Listener()->HandleError(B_BAD_DATA,
			jsonParseContext.LineNumber(), errorMessage.String());
		return false;
	}

	uint intValue;

	if (sscanf(buffer, "%4x", &intValue) != 1) {
		BString errorMessage;
		errorMessage.SetToFormat(
			"unable to process unicode sequence [%s] in string "
			" parsing", buffer);
		jsonParseContext.Listener()->HandleError(B_BAD_DATA,
			jsonParseContext.LineNumber(), errorMessage.String());
		return false;
	}

	char character[7];
	char* ptr = character;
	BUnicodeChar::ToUTF8(intValue, &ptr);
	int32 sequenceLength = ptr - character;
	stringResult.Append(character, sequenceLength);

	return true;
}


bool
BJson::ParseStringEscapeSequence(JsonParseContext& jsonParseContext,
	BString& stringResult)
{
	char c;

	if (!NextChar(jsonParseContext, &c))
		return false;

	switch (c) {
		case 'n':
			stringResult += "\n";
			break;
		case 'r':
			stringResult += "\r";
			break;
		case 'b':
			stringResult += "\b";
			break;
		case 'f':
			stringResult += "\f";
			break;
		case '\\':
			stringResult += "\\";
			break;
		case '/':
			stringResult += "/";
			break;
		case 't':
			stringResult += "\t";
			break;
		case '"':
			stringResult += "\"";
			break;
		case 'u':
		{
				// unicode escape sequence.
			if (!ParseEscapeUnicodeSequence(jsonParseContext,
					stringResult)) {
				return false;
			}
			break;
		}
		default:
		{
			BString errorMessage;
			errorMessage.SetToFormat(
				"unexpected escaped character [%c] in string parsing",
				c);
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
	BString stringResult;

	while(true) {
		if (!NextChar(jsonParseContext, &c))
    		return false;

		switch (c) {
			case '"':
			{
					// terminates the string assembled so far.
				jsonParseContext.Listener()->Handle(
					BJsonEvent(eventType, stringResult.String()));
				return true;
			}

			case '\\':
			{
				if (!ParseStringEscapeSequence(jsonParseContext,
					stringResult)) {
					return false;
				}
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

				stringResult.Append(&c, 1);
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
BJson::IsValidNumber(BString& number)
{
	int32 offset = 0;
	int32 len = number.Length();

	if (offset < len && number[offset] == '-')
		offset++;

	if (offset >= len)
		return false;

	if (isdigit(number[offset]) && number[offset] != '0') {
		while (offset < len && isdigit(number[offset]))
			offset++;
	} else {
		if (number[offset] == '0')
			offset++;
		else
			return false;
	}

	if (offset < len && number[offset] == '.') {
		offset++;

		if (offset >= len)
			return false;

		while (offset < len && isdigit(number[offset]))
			offset++;
	}

	if (offset < len && (number[offset] == 'E' || number[offset] == 'e')) {
		offset++;

		if(offset < len && (number[offset] == '+' || number[offset] == '-'))
		 	offset++;

		if (offset >= len)
			return false;

		while (offset < len && isdigit(number[offset]))
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
	BString value;

	while (true) {
		char c;
		status_t result = jsonParseContext.NextChar(&c);

		switch (result) {
			case B_OK:
			{
				if (isdigit(c)) {
					value += c;
					break;
				}

				if (NULL != strchr("+-eE.", c)) {
					value += c;
					break;
				}

				jsonParseContext.PushbackChar(c);
				// intentional fall through
			}
			case B_PARTIAL_READ:
			{
				errno = 0;

				if (!IsValidNumber(value)) {
					jsonParseContext.Listener()->HandleError(B_BAD_DATA,
						jsonParseContext.LineNumber(), "malformed number");
					return false;
				}

				jsonParseContext.Listener()->Handle(BJsonEvent(B_JSON_NUMBER,
					value.String()));

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