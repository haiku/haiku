/*
 * Copyright 2004-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "RTF.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DataIO.h>


//#define TRACE_RTF
#ifdef TRACE_RTF
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


static const char *kDestinationControlWords[] = {
	"aftncn", "aftnsep", "aftnsepc", "annotation", "atnauthor", "atndate",
	"atnicn", "atnid", "atnparent", "atnref", "atntime", "atrfend",
	"atrfstart", "author", "background", "bkmkend", "buptim", "colortbl",
	"comment", "creatim", "do", "doccomm", "docvar", "fonttbl", "footer",
	"footerf", "footerl", "footerr", "footnote", "ftncn", "ftnsep",
	"ftnsepc", "header", "headerf", "headerl", "headerr", "info",
	"keywords", "operator", "pict", "printim", "private1", "revtim",
	"rxe", "stylesheet", "subject", "tc", "title", "txe", "xe",
};

static char read_char(BDataIO &stream, bool endOfFileAllowed = false) throw (status_t);
static int32 parse_integer(char first, BDataIO &stream, char &_last, int32 base = 10) throw (status_t);


using namespace RTF;


static char
read_char(BDataIO &stream, bool endOfFileAllowed) throw (status_t)
{
	char c;
	ssize_t bytesRead = stream.Read(&c, 1);

	if (bytesRead < B_OK)
		throw (status_t)bytesRead;

	if (bytesRead == 0 && !endOfFileAllowed)
		throw (status_t)B_ERROR;

	return c;
}


static int32
parse_integer(char first, BDataIO &stream, char &_last, int32 base)
	throw (status_t)
{
	const char *kDigits = "0123456789abcdef";
	int32 integer = 0;
	int32 count = 0;

	char digit = first;

	if (digit == '\0')
		digit = read_char(stream);

	while (true) {
		int32 pos = 0;
		for (; pos < base; pos++) {
			if (kDigits[pos] == tolower(digit)) {
				integer = integer * base + pos;
				count++;
				break;
			}
		}
		if (pos == base) {
			_last = digit;
			goto out;
		}

		digit = read_char(stream);
	}

out:
	if (count == 0)
		throw (status_t)B_BAD_TYPE;

	return integer;
}


static int
string_array_compare(const char *key, const char **array)
{
	return strcmp(key, array[0]);
}


static void
dump(Element &element, int32 level = 0)
{
	printf("%03" B_PRId32 " (%p):", level, &element);
	for (int32 i = 0; i < level; i++)
		printf("  ");

	if (RTF::Header *header = dynamic_cast<RTF::Header *>(&element)) {
		printf("<RTF header, major version %" B_PRId32 ">\n", header->Version());
	} else if (RTF::Command *command = dynamic_cast<RTF::Command *>(&element)) {
		printf("<Command: %s", command->Name());
		if (command->HasOption())
			printf(", Option %" B_PRId32, command->Option());
		puts(">");
	} else if (RTF::Text *text = dynamic_cast<RTF::Text *>(&element)) {
		printf("<Text>");
		puts(text->String());
	} else if (RTF::Group *group = dynamic_cast<RTF::Group *>(&element))
		printf("<Group \"%s\">\n", group->Name());

	if (RTF::Group *group = dynamic_cast<RTF::Group *>(&element)) {
		for (uint32 i = 0; i < group->CountElements(); i++)
			dump(*group->ElementAt(i), level + 1);
	}
}


//	#pragma mark -


Parser::Parser(BPositionIO &stream)
	:
	fStream(&stream, 65536, false),
	fIdentified(false)
{
}


status_t
Parser::Identify()
{
	char header[5];
	if (fStream.Read(header, sizeof(header)) < (ssize_t)sizeof(header))
		return B_IO_ERROR;

	if (strncmp(header, "{\\rtf", 5))
		return B_BAD_TYPE;

	fIdentified = true;
	return B_OK;
}


status_t
Parser::Parse(Header &header)
{
	if (!fIdentified && Identify() != B_OK)
		return B_BAD_TYPE;

	try {
		int32 openBrackets = 1;

		// since we already preparsed parts of the RTF header, the header
		// is handled here directly
		char last;
		header.Parse('\0', fStream, last);

		Group *parent = &header;
		char c = last;

		while (true) {
			Element *element = NULL;

			// we'll just ignore the end of the stream
			if (parent == NULL)
				return B_OK;

			switch (c) {
				case '{':
					openBrackets++;
					parent->AddElement(element = new Group());
					parent = static_cast<Group *>(element);
					break;

				case '\\':
					parent->AddElement(element = new Command());
					break;

				case '}':
					openBrackets--;
					parent->DetermineDestination();
					parent = parent->Parent();
					// supposed to fall through
				case '\n':
				case '\r':
				{
					ssize_t bytesRead = fStream.Read(&c, 1);
					if (bytesRead < B_OK)
						throw (status_t)bytesRead;
					else if (bytesRead != 1) {
						// this is the only valid exit status
						if (openBrackets == 0)
							return B_OK;

						throw (status_t)B_ERROR;
					}
					continue;
				}

				default:
					parent->AddElement(element = new Text());
					break;
			}

			if (element == NULL)
				throw (status_t)B_ERROR;

			element->Parse(c, fStream, last);
			c = last;
		}
	} catch (status_t status) {
		return status;
	}

	return B_OK;
}


//	#pragma mark -


Element::Element()
	:
	fParent(NULL)
{
}


Element::~Element()
{
}


void
Element::SetParent(Group *parent)
{
	fParent = parent;
}


Group *
Element::Parent() const
{
	return fParent;
}


bool
Element::IsDefinitionDelimiter()
{
	return false;
}


void
Element::PrintToStream(int32 level)
{
	dump(*this, level);
}


//	#pragma mark -


Group::Group()
	:
	fDestination(TEXT_DESTINATION)
{
}


Group::~Group()
{
	Element *element;
	while ((element = (Element *)fElements.RemoveItem((int32)0)) != NULL) {
		delete element;
	}
}


void
Group::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	if (first == '\0')
		first = read_char(stream);

	if (first != '{')
		throw (status_t)B_BAD_TYPE;

	last = read_char(stream);
}


status_t
Group::AddElement(Element *element)
{
	if (element == NULL)
		return B_BAD_VALUE;

	if (fElements.AddItem(element)) {
		element->SetParent(this);
		return B_OK;
	}

	return B_NO_MEMORY;
}


uint32
Group::CountElements() const
{
	return (uint32)fElements.CountItems();
}


Element *
Group::ElementAt(uint32 index) const
{
	return static_cast<Element *>(fElements.ItemAt(index));
}


Element *
Group::FindDefinitionStart(int32 index, int32 *_startIndex) const
{
	if (index < 0)
		return NULL;

	Element *element;
	int32 number = 0;
	for (uint32 i = 0; (element = ElementAt(i)) != NULL; i++) {
		if (number == index) {
			if (_startIndex)
				*_startIndex = i;
			return element;
		}

		if (element->IsDefinitionDelimiter())
			number++;
	}

	return NULL;
}


Command *
Group::FindDefinition(const char *name, int32 index) const
{
	int32 startIndex;
	Element *element = FindDefinitionStart(index, &startIndex);
	if (element == NULL)
		return NULL;

	for (uint32 i = startIndex; (element = ElementAt(i)) != NULL; i++) {
		if (element->IsDefinitionDelimiter())
			break;

		if (Command *command = dynamic_cast<Command *>(element)) {
			if (command != NULL && !strcmp(name, command->Name()))
				return command;
		}
	}

	return NULL;
}


Group *
Group::FindGroup(const char *name) const
{
	Element *element;
	for (uint32 i = 0; (element = ElementAt(i)) != NULL; i++) {
		Group *group = dynamic_cast<Group *>(element);
		if (group == NULL)
			continue;

		Command *command = dynamic_cast<Command *>(group->ElementAt(0));
		if (command != NULL && !strcmp(name, command->Name()))
			return group;
	}

	return NULL;
}


const char *
Group::Name() const
{
	Command *command = dynamic_cast<Command *>(ElementAt(0));
	if (command != NULL)
		return command->Name();

	return NULL;
}


void
Group::DetermineDestination()
{
	const char *name = Name();
	if (name == NULL)
		return;

	if (!strcmp(name, "*")) {
		fDestination = COMMENT_DESTINATION;
		return;
	}

	// binary search for destination control words

	if (bsearch(name, kDestinationControlWords,
			sizeof(kDestinationControlWords) / sizeof(kDestinationControlWords[0]),
			sizeof(kDestinationControlWords[0]),
			(int (*)(const void *, const void *))string_array_compare) != NULL)
		fDestination = OTHER_DESTINATION;
}


group_destination
Group::Destination() const
{
	return fDestination;
}


//	#pragma mark -


Header::Header()
	:
	fVersion(0)
{
}


Header::~Header()
{
}


void
Header::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	// The stream has been peeked into by the parser already, and
	// only the version follows in the stream -- let's pick it up

	fVersion = parse_integer(first, stream, last);

	// recreate "rtf" command to name this group

	Command *command = new Command();
	command->SetName("rtf");
	command->SetOption(fVersion);

	AddElement(command);
}


int32
Header::Version() const
{
	return fVersion;
}


const char *
Header::Charset() const
{
	Command *command = dynamic_cast<Command *>(ElementAt(1));
	if (command == NULL)
		return NULL;

	return command->Name();
}


rgb_color
Header::Color(int32 index)
{
	rgb_color color = {0, 0, 0, 255};

	Group *colorTable = FindGroup("colortbl");

	if (colorTable != NULL) {
		if (Command *gun = colorTable->FindDefinition("red", index))
			color.red = gun->Option();
		if (Command *gun = colorTable->FindDefinition("green", index))
			color.green = gun->Option();
		if (Command *gun = colorTable->FindDefinition("blue", index))
			color.blue = gun->Option();
	}

	return color;
}


//	#pragma mark -


Text::Text()
{
}


Text::~Text()
{
	SetTo(NULL);
}


bool
Text::IsDefinitionDelimiter()
{
	return fText == ";";
}


void
Text::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	char c = first;
	if (c == '\0')
		c = read_char(stream);

	if (c == ';') {
		// definition delimiter
		fText.SetTo(";");
		last = read_char(stream);
		return;
	}

	const size_t kBufferSteps = 1;
	size_t maxSize = kBufferSteps;
	char *text = fText.LockBuffer(maxSize);
	if (text == NULL)
		throw (status_t)B_NO_MEMORY;

	size_t position = 0;

	while (true) {
		if (c == '\\' || c == '}' || c == '{' || c == ';' || c == '\n' || c == '\r')
			break;

		if (position >= maxSize) {
			fText.UnlockBuffer(position);
			text = fText.LockBuffer(maxSize += kBufferSteps);
			if (text == NULL)
				throw (status_t)B_NO_MEMORY;
		}

		text[position++] = c;

		c = read_char(stream);
	}
	fText.UnlockBuffer(position);

	// ToDo: add support for different charsets - right now, only ASCII is supported!
	//	To achieve this, we should just translate everything into UTF-8 here

	last = c;
}


status_t
Text::SetTo(const char *text)
{
	return fText.SetTo(text) != NULL ? B_OK : B_NO_MEMORY;
}


const char *
Text::String() const
{
	return fText.String();
}


uint32
Text::Length() const
{
	return fText.Length();
}


//	#pragma mark -


Command::Command()
	:
	fName(NULL),
	fHasOption(false),
	fOption(-1)
{
}


Command::~Command()
{
}


void
Command::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	if (first == '\0')
		first = read_char(stream);

	if (first != '\\')
		throw (status_t)B_BAD_TYPE;

	// get name
	char name[kCommandLength];
	size_t length = 0;
	char c;
	while (isalpha(c = read_char(stream))) {
		name[length++] = c;
		if (length >= kCommandLength - 1)
			throw (status_t)B_BAD_TYPE;
	}

	if (length == 0) {
		if (c == '\n' || c == '\r') {
			// we're a hard return
			fName.SetTo("par");
		} else
			fName.SetTo(c, 1);

		// read over character
		c = read_char(stream);
	} else
		fName.SetTo(name, length);

	TRACE("command: %s\n", fName.String());

	// parse numeric option

	if (c == '-')
		c = read_char(stream);

	last = c;

	if (fName == "'") {
		// hexadecimal
		char bytes[2];
		bytes[0] = read_char(stream);
		bytes[1] = '\0';
		BMemoryIO memory(bytes, 2);

		SetOption(parse_integer(c, memory, last, 16));
		last = read_char(stream);
	} else {
		// decimal
		if (isdigit(c))
			SetOption(parse_integer(c, stream, last));

		// a space delimiter is eaten up by the command
		if (isspace(last))
			last = read_char(stream);
	}

	if (HasOption())
		TRACE("  option: %ld\n", fOption);
}


status_t
Command::SetName(const char *name)
{
	return fName.SetTo(name) != NULL ? B_OK : B_NO_MEMORY;
}


const char *
Command::Name()
{
	return fName.String();
}


void
Command::UnsetOption()
{
	fHasOption = false;
	fOption = -1;
}


void
Command::SetOption(int32 option)
{
	fOption = option;
	fHasOption = true;
}


bool
Command::HasOption() const
{
	return fHasOption;
}


int32
Command::Option() const
{
	return fOption;
}


//	#pragma mark -


Iterator::Iterator(Element &start, group_destination destination)
{
	SetTo(start, destination);
}


void
Iterator::SetTo(Element &start, group_destination destination)
{
	fStart = &start;
	fDestination = destination;

	Rewind();
}


void
Iterator::Rewind()
{
	fStack.MakeEmpty();
	fStack.Push(fStart);
}


bool
Iterator::HasNext() const
{
	return !fStack.IsEmpty();
}


Element *
Iterator::Next()
{
	Element *element;

	if (!fStack.Pop(&element))
		return NULL;

	Group *group = dynamic_cast<Group *>(element);
	if (group != NULL
		&& (fDestination == ALL_DESTINATIONS
			|| fDestination == group->Destination())) {
		// put this group's children on the stack in
		// reverse order, so that we iterate over
		// the tree in in-order

		for (int32 i = group->CountElements(); i-- > 0;) {
			fStack.Push(group->ElementAt(i));
		}
	}

	return element;
}


//	#pragma mark -


Worker::Worker(RTF::Header &start)
	:
	fStart(start)
{
}


Worker::~Worker()
{
}


void
Worker::Dispatch(Element *element)
{
	if (RTF::Group *group = dynamic_cast<RTF::Group *>(element)) {
		fSkip = false;
		Group(group);

		if (fSkip)
			return;

		for (int32 i = 0; (element = group->ElementAt(i)) != NULL; i++)
			Dispatch(element);

		GroupEnd(group);
	} else if (RTF::Command *command = dynamic_cast<RTF::Command *>(element)) {
		Command(command);
	} else if (RTF::Text *text = dynamic_cast<RTF::Text *>(element)) {
		Text(text);
	}
}


void
Worker::Work() throw (status_t)
{
	Dispatch(&fStart);
}


void
Worker::Group(RTF::Group *group)
{
}


void
Worker::GroupEnd(RTF::Group *group)
{
}


void
Worker::Command(RTF::Command *command)
{
}


void
Worker::Text(RTF::Text *text)
{
}


RTF::Header &
Worker::Start()
{
	return fStart;
}


void
Worker::Skip()
{
	fSkip = true;
}


void
Worker::Abort(status_t status)
{
	throw status;
}

