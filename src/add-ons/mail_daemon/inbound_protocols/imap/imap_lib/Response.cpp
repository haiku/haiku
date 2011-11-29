/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Response.h"

#include <stdlib.h>

#include "Protocol.h"
	// TODO: remove again once the ConnectionReader is out


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
	:
	fMessage(NULL)
{
}


ParseException::ParseException(const char* message)
	:
	fMessage(message)
{
}


// #pragma mark -


ExpectedParseException::ExpectedParseException(char expected, char instead)
{
	snprintf(fBuffer, sizeof(fBuffer), "Expected \"%c\", but got \"%c\"!",
		expected, instead);
	fMessage = fBuffer;
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
	fContinuation(false)
{
}


Response::~Response()
{
}


void
Response::Parse(ConnectionReader& reader, const char* line,
	LiteralHandler* handler) throw(ParseException)
{
	MakeEmpty();
	fReader = &reader;
	fLiteralHandler = handler;
	fTag = 0;
	fContinuation = false;

	if (line[0] == '*') {
		// Untagged response
		Consume(line, '*');
		Consume(line, ' ');
	} else if (line[0] == '+') {
		// Continuation
		Consume(line, '+');
		fContinuation = true;
	} else {
		// Tagged response
		Consume(line, 'A');
		fTag = strtoul(line, (char**)&line, 10);
		if (line == NULL)
			ParseException("Invalid tag!");
		Consume(line, ' ');
	}

	char c = ParseLine(*this, line);
	if (c != '\0')
		throw ExpectedParseException('\0', c);
}


bool
Response::IsCommand(const char* command) const
{
	return IsUntagged() && EqualsAt(0, command);
}


char
Response::ParseLine(ArgumentList& arguments, const char*& line)
{
	while (line[0] != '\0') {
		char c = line[0];
		switch (c) {
			case '(':
				ParseList(arguments, line, '(', ')');
				break;
			case '[':
				ParseList(arguments, line, '[', ']');
				break;
			case ')':
			case ']':
				Consume(line, c);
				return c;
			case '"':
				ParseQuoted(arguments, line);
				break;
			case '{':
				ParseLiteral(arguments, line);
				break;

			case ' ':
			case '\t':
				// whitespace
				Consume(line, c);
				break;

			case '\r':
				Consume(line, '\r');
				Consume(line, '\n');
				return '\0';
			case '\n':
				Consume(line, '\n');
				return '\0';

			default:
				ParseString(arguments, line);
				break;
		}
	}

	return '\0';
}


void
Response::Consume(const char*& line, char c)
{
	if (line[0] != c)
		throw ExpectedParseException(c, line[0]);

	line++;
}


void
Response::ParseList(ArgumentList& arguments, const char*& line, char start,
	char end)
{
	Consume(line, start);

	ListArgument* argument = new ListArgument(start);
	arguments.AddItem(argument);

	char c = ParseLine(argument->List(), line);
	if (c != end)
		throw ExpectedParseException(end, c);
}


void
Response::ParseQuoted(ArgumentList& arguments, const char*& line)
{
	Consume(line, '"');

	BString string;
	char* output = string.LockBuffer(strlen(line));
	int32 index = 0;

	while (line[0] != '\0') {
		char c = line[0];
		if (c == '\\') {
			line++;
			if (line[0] == '\0')
				break;
		} else if (c == '"') {
			line++;
			output[index] = '\0';
			string.UnlockBuffer(index);
			arguments.AddItem(new StringArgument(string));
			return;
		}

		output[index++] = c;
		line++;
	}

	throw ParseException("Unexpected end of qouted string!");
}


void
Response::ParseLiteral(ArgumentList& arguments, const char*& line)
{
	Consume(line, '{');
	off_t size = atoll(ExtractString(line));
	Consume(line, '}');
	Consume(line, '\r');
	Consume(line, '\n');

	if (fLiteralHandler != NULL)
		fLiteralHandler->HandleLiteral(*fReader, size);
	else {
		// The default implementation just throws the data away
		BMallocIO stream;
		TRACE("Trying to read literal with %llu bytes.\n", size);
		status_t status = fReader->ReadToStream(size, stream);
		if (status == B_OK) {
			TRACE("LITERAL: %-*s\n", (int)size, (char*)stream.Buffer());
		} else
			TRACE("Reading literal failed: %s\n", strerror(status));
	}
}


void
Response::ParseString(ArgumentList& arguments, const char*& line)
{
	arguments.AddItem(new StringArgument(ExtractString(line)));
}


BString
Response::ExtractString(const char*& line)
{
	const char* start = line;

	// TODO: parse modified UTF-7 as described in RFC 3501, 5.1.3
	while (line[0] != '\0') {
		char c = line[0];
		if (c <= ' ' || strchr("()[]{}\"", c) != NULL)
			return BString(start, line - start);

		line++;
	}

	throw ParseException("Unexpected end of string");
}


// #pragma mark -


ResponseParser::ResponseParser(ConnectionReader& reader)
	:
	fLiteralHandler(NULL)
{
	SetTo(reader);
}


ResponseParser::~ResponseParser()
{
}


void
ResponseParser::SetTo(ConnectionReader& reader)
{
	fReader = &reader;
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
	BString line;
	status_t status = fReader->GetNextLine(line, timeout);
	if (status != B_OK) {
		TRACE("S: read error %s", line.String());
		return status;
	}

	TRACE("S: %s", line.String());
	response.Parse(*fReader, line, fLiteralHandler);

	return B_OK;
}


}	// namespace IMAP
