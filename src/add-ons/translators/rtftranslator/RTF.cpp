/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "RTF.h"

#include <DataIO.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


static void
dump(RTFElement &element, int32 level = 0)
{
	printf("%03ld:", level);
	for (int32 i = 0; i < level; i++)
		printf("  ");

	if (RTFHeader *header = dynamic_cast<RTFHeader *>(&element)) {
		printf("<RTF header, major version %ld>\n", header->Version());
	} else if (RTFCommand *command = dynamic_cast<RTFCommand *>(&element)) {
		printf("<Command: %s", command->Name());
		if (command->HasOption())
			printf(", Option %ld", command->Option());
		puts(">");
	} else if (RTFText *text = dynamic_cast<RTFText *>(&element)) {
		printf("<Text>");
		puts(text->Text());
	} else
		puts("<Group>");

	for (uint32 i = 0; i < element.CountElements(); i++)
		dump(*element.ElementAt(i), level + 1);
}


//	#pragma mark -


RTFElement::RTFElement()
	:
	fParent(NULL),
	fDestination(RTF_OTHER)
{
}


RTFElement::~RTFElement()
{
	RTFElement *element;
	while ((element = (RTFElement *)fElements.RemoveItem(0L)) != NULL) {
		delete element;
	}
}


void
RTFElement::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	if (first == '\0')
		first = ReadChar(stream);

	if (first != '{')
		throw (status_t)B_BAD_TYPE;

	last = ReadChar(stream);
}


status_t
RTFElement::AddElement(RTFElement *element)
{
	if (element == NULL)
		return B_BAD_VALUE;

	if (fElements.AddItem(element)) {
		element->fParent = this;
		return B_OK;
	}

	return B_NO_MEMORY;
}


uint32
RTFElement::CountElements() const
{
	return (uint32)fElements.CountItems();
}


RTFElement *
RTFElement::ElementAt(uint32 index) const
{
	return static_cast<RTFElement *>(fElements.ItemAt(index));
}


RTFCommand *
RTFElement::FindDefinition(const char *name, int32 index) const
{
	if (index < 0)
		return NULL;

	RTFElement *element;
	int32 number = 0;
	for (uint32 i = 0; (element = ElementAt(i)) != NULL; i++) {
		if (RTFText *text = dynamic_cast<RTFText *>(element)) {
			// the ';' indicates the next definition
			if (!strcmp(text->Text(), ";"))
				number++;
		} else if (RTFCommand *command = dynamic_cast<RTFCommand *>(element)) {
			if (command != NULL
				&& !strcmp(name, command->Name())
				&& number == index)
				return command;
		}
	}

	return NULL;
}


RTFElement *
RTFElement::FindGroup(const char *name) const
{
	RTFElement *group;
	for (uint32 i = 0; (group = ElementAt(i)) != NULL; i++) {
		RTFCommand *command = dynamic_cast<RTFCommand *>(group->ElementAt(0));
		if (command != NULL && !strcmp(name, command->Name()))
			return group;
	}

	return NULL;
}


const char *
RTFElement::GroupName() const
{
	RTFCommand *command = dynamic_cast<RTFCommand *>(ElementAt(0));
	if (command != NULL)
		return command->Name();

	return NULL;
}


RTFElement *
RTFElement::Parent() const
{
	return fParent;
}


void
RTFElement::PrintToStream()
{
	dump(*this, 0);
}


void
RTFElement::DetermineDestination()
{
	const char *name = GroupName();
	if (name == NULL)
		fDestination = RTF_TEXT;

	if (!strcmp(name, "*")) {
		fDestination = RTF_COMMENT;
		return;
	}

	const char *texts[] = {"rtf", "sect", "par"};
	for (uint32 i = 0; i < sizeof(texts) / sizeof(texts[0]); i++) {
		if (!strcmp(texts[i], name)) {
			fDestination = RTF_TEXT;
			return;
		}
	}

	fDestination = RTF_OTHER;
}


rtf_destination
RTFElement::Destination() const
{
	return fDestination;
}


/* static */
char
RTFElement::ReadChar(BDataIO &stream, bool endOfFileAllowed) throw (status_t)
{
	char c;
	ssize_t bytesRead = stream.Read(&c, 1);
	
	if (bytesRead < B_OK)
		throw (status_t)bytesRead;

	if (bytesRead == 0 && !endOfFileAllowed)
		throw (status_t)B_ERROR;

	return c;
}


/* static */
int32
RTFElement::ParseInteger(char first, BDataIO &stream, char &_last) throw (status_t)
{
	int32 integer = 0;
	int32 count = 0;

	char digit = first;

	if (digit == '\0')
		digit = ReadChar(stream);

	while (true) {
		if (isdigit(digit)) {
			integer = integer * 10 + digit - '0';
			count++;
		} else {
			_last = digit;
			goto out;
		}

		digit = ReadChar(stream);
	}

out:
	if (count == 0)
		throw (status_t)B_BAD_TYPE;

	return integer;
}


//	#pragma mark -


RTFHeader::RTFHeader()
	:
	fVersion(0)
{
}


RTFHeader::~RTFHeader()
{
}


void
RTFHeader::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	int32 openBrackets = 1;

	// The stream has been picked up by the static RTFHeader::Parse(), so
	// the version follows in the stream -- let's pick it up

	fVersion = ParseInteger(first, stream, last);

	RTFElement *parent = this;
	char c = last;

	while (true) {
		RTFElement *element = NULL;

		switch (c) {
			case '{':
				openBrackets++;
				parent->AddElement(element = new RTFElement());
				parent = element;
				break;

			case '\\':
				parent->AddElement(element = new RTFCommand());
				break;

			case '}':
				openBrackets--;
				parent->DetermineDestination();
				parent = parent->Parent();
			case '\n':
			case '\r':
			{
				ssize_t bytesRead = stream.Read(&c, 1);
				if (bytesRead < B_OK)
					throw (status_t)bytesRead;
				else if (bytesRead != 1) {
					// this is the only valid exit status
					if (openBrackets == 0)
						return;

					throw B_ERROR;
				}
				continue;
			}

			default:
				parent->AddElement(element = new RTFText());
				break;
		}

		if (element == NULL)
			throw (status_t)B_ERROR;

		element->Parse(c, stream, last);
		c = last;
	}
}


int32
RTFHeader::Version() const
{
	return fVersion;
}


const char *
RTFHeader::Charset() const
{
	RTFCommand *command = dynamic_cast<RTFCommand *>(ElementAt(0));
	if (command == NULL)
		return NULL;

	return command->Name();
}


rgb_color 
RTFHeader::Color(int32 index)
{
	rgb_color color = {0, 0, 0, 255};

	RTFElement *colorTable = FindGroup("colortbl");

	if (colorTable != NULL) {
		if (RTFCommand *gun = colorTable->FindDefinition("red", index))
			color.red = gun->Option();
		if (RTFCommand *gun = colorTable->FindDefinition("green", index))
			color.green = gun->Option();
		if (RTFCommand *gun = colorTable->FindDefinition("blue", index))
			color.blue = gun->Option();
	}

	return color;
}


status_t
RTFHeader::Identify(BDataIO &stream)
{
	char header[5];

	if (stream.Read(header, sizeof(header)) < (ssize_t)sizeof(header))
		return B_IO_ERROR;

	return strncmp(header, "{\\rtf", 5) ? B_BAD_TYPE : B_OK;
}


status_t
RTFHeader::Parse(BDataIO &stream, RTFHeader &header, bool identified)
{
	if (!identified && Identify(stream) != B_OK)
		return B_BAD_TYPE;

	try {
		char last;
		header.Parse('\0', stream, last);
	} catch (status_t status) {
		return status;
	}

	return B_OK;
}


//	#pragma mark -


RTFText::RTFText()
{
}


RTFText::~RTFText()
{
	SetText(NULL);
}


void
RTFText::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	char c = first;
	if (c == '\0')
		c = ReadChar(stream);
	
	fText = "";

	while (true) {
		if (c == '\\' || c == '}')
			break;

		// ToDo: this is horribly inefficient with BStrings
		fText.Append(c, 1);
		c = ReadChar(stream);
	}

	// ToDo: add support for different charsets - right now, only ASCII is supported!
	//	To achieve this, we should just translate everything into UTF-8 here

	last = c;
}


status_t
RTFText::SetText(const char *text)
{
	return fText.SetTo(text) != NULL ? B_OK : B_NO_MEMORY;
}


const char *
RTFText::Text() const
{
	return fText.String();
}


uint32
RTFText::TextLength() const
{
	return fText.Length();
}


//	#pragma mark -


RTFCommand::RTFCommand()
	:
	fName(NULL),
	fHasOption(false),
	fOption(-1)
{
}


RTFCommand::~RTFCommand()
{
}


void
RTFCommand::Parse(char first, BDataIO &stream, char &last) throw (status_t)
{
	if (first == '\0')
		first = ReadChar(stream);

	if (first != '\\')
		throw B_BAD_TYPE;

	// get name
	char name[kRTFCommandLength];
	size_t length = 0;
	char c;
	while (isalpha(c = ReadChar(stream))) {
		name[length++] = c;
		if (length >= kRTFCommandLength - 1)
			throw B_BAD_TYPE;
	}

	if (length == 0) {
		if (c == '*') {
			// we're a comment!
			name[0] = 'c';
			length++;
		} else if (c == '\n') {
			// we're a hard return
			name[0] = '\n';
			length++;
		}
	}

	fName.SetTo(name, length);

	// parse numeric option

	if (c == '-')
		c = ReadChar(stream);

	last = c;

	if (isdigit(c))
		SetOption(ParseInteger(c, stream, last));

	// a space delimiter is eaten up by the command
	if (isspace(last))
		last = ReadChar(stream);
}


status_t
RTFCommand::SetName(const char *name)
{
	return fName.SetTo(name) != NULL ? B_OK : B_NO_MEMORY;
}


const char *
RTFCommand::Name()
{
	return fName.String();
}


void
RTFCommand::UnsetOption()
{
	fHasOption = false;
	fOption = -1;
}


void
RTFCommand::SetOption(int32 option)
{
	fOption = option;
	fHasOption = true;
}


bool
RTFCommand::HasOption() const
{
	return fHasOption;
}


int32
RTFCommand::Option() const
{
	return fOption;
}


//	#pragma mark -


RTFIterator::RTFIterator(RTFElement &start, rtf_destination destination)
{
	SetTo(start, destination);
}


void
RTFIterator::SetTo(RTFElement &start, rtf_destination destination)
{
	fStart = &start;
	fDestination = destination;

	Rewind();
}


void
RTFIterator::Rewind()
{
	fStack.MakeEmpty();
	fStack.Push(fStart);
}


bool
RTFIterator::HasNext() const
{
	return !fStack.IsEmpty();
}


RTFElement *
RTFIterator::Next()
{
	RTFElement *element;

	if (!fStack.Pop(&element))
		return NULL;

	// put this element's children on the stack in
	// reverse order, so that we iterate over the
	// tree in in-order

	for (int32 i = element->CountElements(); i-- > 0;) {
		RTFElement *child = element->ElementAt(i);

		if (fDestination == RTF_ALL_DESTINATIONS
			|| child->CountElements() == 0
			|| fDestination == element->Destination())
			fStack.Push(child);
	}

	return element;
}

