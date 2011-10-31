/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Response.h"


namespace IMAP {


ArgumentList::ArgumentList()
	:
	fArguments(5, true)
{
}


ArgumentList::~ArgumentList()
{
}


BString
ArgumentList::StringAt(int32 index) const
{
	if (index >= 0 && index < fArguments.CountItems()) {
		if (StringArgument* argument = dynamic_cast<StringArgument*>(
				fArguments.ItemAt(index)))
			return argument->String();
	}
	return "";
}


bool
ArgumentList::IsStringAt(int32 index) const
{
	if (index >= 0 && index < fArguments.CountItems()) {
		if (dynamic_cast<StringArgument*>(fArguments.ItemAt(index)) != NULL)
			return true;
	}
	return false;
}


bool
ArgumentList::EqualsAt(int32 index, const char* string) const
{
	return StringAt(index).ICompare(string);
}


const ArgumentList&
ArgumentList::ListAt(int32 index) const
{
	if (index >= 0 && index < fArguments.CountItems()) {
		if (ListArgument* argument = dynamic_cast<ListArgument*>(
				fArguments.ItemAt(index)))
			return argument->List();
	}

	static ArgumentList empty(0, true);
	return empty;
}


bool
ArgumentList::IsListAt(int32 index) const
{
	if (index >= 0 && index < fArguments.CountItems()) {
		if (ListArgument* argument = dynamic_cast<ListArgument*>(
				fArguments.ItemAt(index)))
			return true;
	}
	return false;
}


bool
ArgumentList::IsListAt(int32 index, char kind) const
{
	if (index >= 0 && index < fArguments.CountItems()) {
		if (ListArgument* argument = dynamic_cast<ListArgument*>(
				fArguments.ItemAt(index)))
			return argument->Kind() == kind;
	}
	return false;
}


int32
ArgumentList::IntegerAt(int32 index) const
{
	return atoi(StringAt(index).String());
}


bool
ArgumentList::IsIntegerAt(int32 index) const
{
	BString string = StringAt(index);
	for (int32 i = 0; i < string.Length(); i++) {
		if (!isdigit(string.ByteAt(i)))
			return false;
	}
	return string.Length() > 0;
}


// #pragma mark -


Argument::Argument()
{
}


Argument::~Argument()
{
}


/*static*/ BString
Argument::ToString(const ArgumentList& arguments)
{
	BString string;

	for (int32 i = 0; i < arguments.CountItems(); i++) {
		if (i > 0)
			string += ", ";
		string += arguments.ItemAt(i)->ToString();
	}
	return string;
}


bool
Argument::Contains(const ArgumentList& arguments, const char* string) const
{
	for (int32 i = 0; i < arguments.CountItems(); i++) {
		if (StringArgument* argument = dynamic_cast<StringArgument*>(
				arguments.ItemAt(i))) {
			if (argument->String().ICompare(string))
				return true;
		}
	}
	return false;
}


// #pragma mark -


ListArgument::ListArgument()
	:
	fList(5, true)
{
}


BString
ListArgument::ToString() const
{
	BString string("(");
	string += Argument::ToString(response.Arguments());
	string += ")";

	return string;
}


// #pragma mark -


StringArgument::StringArgument(const BString& string)
	:
	fString(string)
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


ParseException::~ParseException()
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


Response::Response()
	:
	fTag(0),
	fArguments(5, true),
	fContinued(false)
{
}


Response::~Response()
{
}


void
Response::SetTo(const char* line) throw(ParseException)
{
	MakeEmpty();
	fTag = 0;
	fContinued = false;

	if (line[0] == '*') {
		// Untagged response
		Consume(line, '*');
		Consume(line, ' ');
	} else if (line[0] == '+') {
		// Continuation
		Consume(line, '+');
		fContinued = true;
	} else {
		// Tagged response
		Consume(line, 'A');
		fTag = strtoul(line, (char**)&line, 10);
		if (line == NULL)
			ParseException("Invalid tag!");
		Consume(line, ' ');
	}

	char c = ParseLine(this, line);
	if (c != '\0')
		throw ExpectedParseException('\0', c);
}


bool
Response::IsCommand(const char* command) const
{
	return IsStringAt(0, command);
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
	// TODO!
	throw ParseException("Literals are not yet supported!");
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

	while (line[0] != '\0') {
		char c = line[0];
		if (c <= ' ' || strchr("()[]{}\"", c) != NULL)
			return BString(start, line - start);

		line++;
	}

	throw ParseException("Unexpected end of string");
}


}	// namespace IMAP
