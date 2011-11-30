/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Response.h"

#include <stdlib.h>


#define TRACE_IMAP
#ifdef TRACE_IMAP
#	define TRACE(...) printf(__VA_ARGS__)
#else
#	define TRACE(...) ;
#endif


namespace IMAP {


ArgumentList::ArgumentList()
	:
	BObjectList<Argument>(5, true)
{
}


ArgumentList::~ArgumentList()
{
}


bool
ArgumentList::Contains(const char* string) const
{
	for (int32 i = 0; i < CountItems(); i++) {
		if (StringArgument* argument
				= dynamic_cast<StringArgument*>(ItemAt(i))) {
			if (argument->String().ICompare(string) == 0)
				return true;
		}
	}
	return false;
}


BString
ArgumentList::StringAt(int32 index) const
{
	if (index >= 0 && index < CountItems()) {
		if (StringArgument* argument
				= dynamic_cast<StringArgument*>(ItemAt(index)))
			return argument->String();
	}
	return "";
}


bool
ArgumentList::IsStringAt(int32 index) const
{
	if (index >= 0 && index < CountItems()) {
		if (dynamic_cast<StringArgument*>(ItemAt(index)) != NULL)
			return true;
	}
	return false;
}


bool
ArgumentList::EqualsAt(int32 index, const char* string) const
{
	return StringAt(index).ICompare(string) == 0;
}


ArgumentList&
ArgumentList::ListAt(int32 index) const
{
	if (index >= 0 && index < CountItems()) {
		if (ListArgument* argument = dynamic_cast<ListArgument*>(ItemAt(index)))
			return argument->List();
	}

	static ArgumentList empty;
	return empty;
}


bool
ArgumentList::IsListAt(int32 index) const
{
	return index >= 0 && index < CountItems()
		&& dynamic_cast<ListArgument*>(ItemAt(index)) != NULL;
}


bool
ArgumentList::IsListAt(int32 index, char kind) const
{
	if (index >= 0 && index < CountItems()) {
		if (ListArgument* argument = dynamic_cast<ListArgument*>(ItemAt(index)))
			return argument->Kind() == kind;
	}
	return false;
}


uint32
ArgumentList::NumberAt(int32 index) const
{
	return atoul(StringAt(index).String());
}


bool
ArgumentList::IsNumberAt(int32 index) const
{
	BString string = StringAt(index);
	for (int32 i = 0; i < string.Length(); i++) {
		if (!isdigit(string.ByteAt(i)))
			return false;
	}
	return string.Length() > 0;
}


BString
ArgumentList::ToString() const
{
	BString string;

	for (int32 i = 0; i < CountItems(); i++) {
		if (i > 0)
			string += ", ";
		string += ItemAt(i)->ToString();
	}
	return string;
}


// #pragma mark -


Argument::Argument()
{
}


Argument::~Argument()
{
}


// #pragma mark -


ListArgument::ListArgument(char kind)
	:
	fKind(kind)
{
}


BString
ListArgument::ToString() const
{
	BString string(fKind == '[' ? "[" : "(");
	string += fList.ToString();
	string += fKind == '[' ? "]" : ")";

	return string;
}


// #pragma mark -


StringArgument::StringArgument(const BString& string)
	:
	fString(string)
{
}


StringArgument::StringArgument(const StringArgument& other)
	:
	fString(other.fString)
{
}


BString
StringArgument::ToString() const
{
	return fString;
}


// #pragma mark -


ParseException::ParseException()
{
	fBuffer[0] = '\0';
}


ParseException::ParseException(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(fBuffer, sizeof(fBuffer), format, args);
	va_end(args);
}


// #pragma mark -


StreamException::StreamException(status_t status)
	:
	ParseException("Error from stream: %s", status)
{
}


// #pragma mark -


ExpectedParseException::ExpectedParseException(char expected, char instead)
{
	char bufferA[8];
	char bufferB[8];
	snprintf(fBuffer, sizeof(fBuffer), "Expected %s, but got %s instead!",
		CharToString(bufferA, sizeof(bufferA), expected),
		CharToString(bufferB, sizeof(bufferB), instead));
}


const char*
ExpectedParseException::CharToString(char* buffer, size_t size, char c)
{
	snprintf(buffer, size, isprint(c) ? "\"%c\"" : "(%x)", c);
	return buffer;
}


// #pragma mark -


LiteralHandler::LiteralHandler()
{
}


LiteralHandler::~LiteralHandler()
{
}


// #pragma mark -


Response::Response()
	:
	fTag(0),
	fContinuation(false),
	fHasNextChar(false)
{
}


Response::~Response()
{
}


void
Response::Parse(BDataIO& stream, LiteralHandler* handler) throw(ParseException)
{
	MakeEmpty();
	fLiteralHandler = handler;
	fTag = 0;
	fContinuation = false;
	fHasNextChar = false;

	char begin = Next(stream);
	if (begin == '*') {
		// Untagged response
		Consume(stream, ' ');
	} else if (begin == '+') {
		// Continuation
		fContinuation = true;
	} else if (begin == 'A') {
		// Tagged response
		fTag = ExtractNumber(stream);
		Consume(stream, ' ');
	} else
		throw ParseException("Unexpected response begin");

	char c = ParseLine(*this, stream);
	if (c != '\0')
		throw ExpectedParseException('\0', c);
}


bool
Response::IsCommand(const char* command) const
{
	return IsUntagged() && EqualsAt(0, command);
}


char
Response::ParseLine(ArgumentList& arguments, BDataIO& stream)
{
	while (true) {
		char c = Peek(stream);
		if (c == '\0')
			break;

		switch (c) {
			case '(':
				ParseList(arguments, stream, '(', ')');
				break;
			case '[':
				ParseList(arguments, stream, '[', ']');
				break;
			case ')':
			case ']':
				Consume(stream, c);
				return c;
			case '"':
				ParseQuoted(arguments, stream);
				break;
			case '{':
				ParseLiteral(arguments, stream);
				break;

			case ' ':
			case '\t':
				// whitespace
				Consume(stream, c);
				break;

			case '\r':
				Consume(stream, '\r');
				Consume(stream, '\n');
				return '\0';
			case '\n':
				Consume(stream, '\n');
				return '\0';

			default:
				ParseString(arguments, stream);
				break;
		}
	}

	return '\0';
}


void
Response::ParseList(ArgumentList& arguments, BDataIO& stream, char start,
	char end)
{
	Consume(stream, start);

	ListArgument* argument = new ListArgument(start);
	arguments.AddItem(argument);

	char c = ParseLine(argument->List(), stream);
	if (c != end)
		throw ExpectedParseException(end, c);
}


void
Response::ParseQuoted(ArgumentList& arguments, BDataIO& stream)
{
	Consume(stream, '"');

	BString string;
	while (true) {
		char c = Next(stream);
		if (c == '\\') {
			c = Next(stream);
		} else if (c == '"') {
			arguments.AddItem(new StringArgument(string));
			return;
		}
		if (c == '\0')
			break;

		string += c;
	}

	throw ParseException("Unexpected end of qouted string!");
}


void
Response::ParseLiteral(ArgumentList& arguments, BDataIO& stream)
{
	Consume(stream, '{');
	size_t size = ExtractNumber(stream);
	Consume(stream, '}');
	Consume(stream, '\r');
	Consume(stream, '\n');

	if (fLiteralHandler != NULL)
		fLiteralHandler->HandleLiteral(stream, size);
	else {
		// The default implementation just adds the data as a string
		TRACE("Trying to read literal with %" B_PRIuSIZE " bytes.\n", size);
		BString string;
		char* buffer = string.LockBuffer(size);
		if (buffer == NULL) {
			throw ParseException("Not enough memory for literal of %"
				B_PRIuSIZE " bytes.", size);
		}

		size_t totalRead = 0;
		while (totalRead < size) {
			ssize_t bytesRead = stream.Read(buffer + totalRead,
				size - totalRead);
			if (bytesRead == 0)
				throw ParseException("Unexpected end of literal");
			if (bytesRead < 0)
				throw StreamException(bytesRead);

			totalRead += bytesRead;
		}

		string.UnlockBuffer(size);
		arguments.AddItem(new StringArgument(string));
	}
}


void
Response::ParseString(ArgumentList& arguments, BDataIO& stream)
{
	arguments.AddItem(new StringArgument(ExtractString(stream)));
}


BString
Response::ExtractString(BDataIO& stream)
{
	BString string;

	// TODO: parse modified UTF-7 as described in RFC 3501, 5.1.3
	while (true) {
		char c = Peek(stream);
		if (c == '\0')
			break;
		if (c <= ' ' || strchr("()[]{}\"", c) != NULL)
			return string;

		string += Next(stream);
	}

	throw ParseException("Unexpected end of string");
}


size_t
Response::ExtractNumber(BDataIO& stream)
{
	BString string = ExtractString(stream);

	const char* end;
	size_t number = strtoul(string.String(), (char**)&end, 10);
	if (end == NULL || end[0] != '\0')
		ParseException("Invalid number!");

	return number;
}


void
Response::Consume(BDataIO& stream, char expected)
{
	char c = Next(stream);
	if (c != expected)
		throw ExpectedParseException(expected, c);
}


char
Response::Next(BDataIO& stream)
{
	if (fHasNextChar) {
		fHasNextChar = false;
		return fNextChar;
	}

	return Read(stream);
}


char
Response::Peek(BDataIO& stream)
{
	if (fHasNextChar)
		return fNextChar;

	fNextChar = Read(stream);
	fHasNextChar = true;

	return fNextChar;
}


char
Response::Read(BDataIO& stream)
{
	char c;
	ssize_t bytesRead = stream.Read(&c, 1);
	if (bytesRead == 1) {
		printf("%c", c);
		return c;
	}

	if (bytesRead == 0)
		throw ParseException("Unexpected end of string");

	throw StreamException(bytesRead);
}


// #pragma mark -


ResponseParser::ResponseParser(BDataIO& stream)
	:
	fLiteralHandler(NULL)
{
	SetTo(stream);
}


ResponseParser::~ResponseParser()
{
}


void
ResponseParser::SetTo(BDataIO& stream)
{
	fStream = &stream;
}


void
ResponseParser::SetLiteralHandler(LiteralHandler* handler)
{
	fLiteralHandler = handler;
}


status_t
ResponseParser::NextResponse(Response& response, bigtime_t timeout)
	throw(ParseException)
{
	response.Parse(*fStream, fLiteralHandler);
	return B_OK;
}


}	// namespace IMAP
