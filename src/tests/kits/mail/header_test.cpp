/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BufferIO.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <String.h>

#include <mail_util.h>


struct mail_header_field {
	const char*	rfc_name;
	const char*	attr_name;
	type_code	attr_type;
	// currently either B_STRING_TYPE and B_TIME_TYPE
};


static const mail_header_field gDefaultFields[] = {
	{ "To",				B_MAIL_ATTR_TO,			B_STRING_TYPE },
	{ "From",         	B_MAIL_ATTR_FROM,		B_STRING_TYPE },
	{ "Cc",				B_MAIL_ATTR_CC,			B_STRING_TYPE },
	{ "Date",         	B_MAIL_ATTR_WHEN,		B_TIME_TYPE   },
	{ "Delivery-Date",	B_MAIL_ATTR_WHEN,		B_TIME_TYPE   },
	{ "Reply-To",     	B_MAIL_ATTR_REPLY,		B_STRING_TYPE },
	{ "Subject",      	B_MAIL_ATTR_SUBJECT,	B_STRING_TYPE },
	{ "X-Priority",		B_MAIL_ATTR_PRIORITY,	B_STRING_TYPE },
		// Priorities with prefered
	{ "Priority",		B_MAIL_ATTR_PRIORITY,	B_STRING_TYPE },
		// one first - the numeric
	{ "X-Msmail-Priority", B_MAIL_ATTR_PRIORITY, B_STRING_TYPE },
		// one (has more levels).
	{ "Mime-Version",	B_MAIL_ATTR_MIME,		B_STRING_TYPE },
	{ "STATUS",       	B_MAIL_ATTR_STATUS,		B_STRING_TYPE },
	{ "THREAD",       	B_MAIL_ATTR_THREAD,		B_STRING_TYPE },
	{ "NAME",       	B_MAIL_ATTR_NAME,		B_STRING_TYPE },
	{ NULL,				NULL,					0 }
};


enum mode {
	PARSE_HEADER,
	EXTRACT_FROM_HEADER,
	PARSE_FIELDS
};
static mode gParseMode = PARSE_HEADER;


void
format_filter(BFile& file)
{
	BMessage message;

	BString header;
	off_t size;
	if (file.GetSize(&size) == B_OK) {
		if (size > 8192)
			size = 8192;
		char* buffer = header.LockBuffer(size);
		file.Read(buffer, size);
		header.UnlockBuffer(size);
	}

	for (int i = 0; gDefaultFields[i].rfc_name; ++i) {
		BString target;
		if (extract_from_header(header, gDefaultFields[i].rfc_name, target)
				== B_OK)
			message.AddString(gDefaultFields[i].rfc_name, target);
	}
}


status_t
parse_fields(BPositionIO& input, size_t maxSize)
{
	char* buffer = (char*)malloc(maxSize);
	if (buffer == NULL)
		return B_NO_MEMORY;

	BMessage header;

	ssize_t bytesRead = input.Read(buffer, maxSize);
	for (int pos = 0; pos < bytesRead; pos++) {
		const char* target = NULL;
		int fieldIndex = 0;
		int fieldStart = 0;

		// Test for fields we should retrieve
		for (int i = 0; gDefaultFields[i].rfc_name; i++) {
			size_t fieldLength = strlen(gDefaultFields[i].rfc_name);
			if (!memcmp(&buffer[pos], gDefaultFields[i].rfc_name,
					fieldLength) && buffer[pos + fieldLength] == ':') {
				target = gDefaultFields[i].rfc_name;
				pos += fieldLength + 1;
				fieldStart = pos;
				fieldIndex = i;
				break;
			}
		}

		// Find end of line
		while (pos < bytesRead && buffer[pos] != '\n')
			pos++;

		// Fill in field
		if (target != NULL) {
			// Skip white space
			while (isspace(buffer[fieldStart]) && fieldStart < pos)
				fieldStart++;

			int end = pos - 1;
			while (isspace(buffer[end]) && fieldStart < end - 1)
				end--;

			char* start = &buffer[fieldStart];
			size_t sourceLength = end + 1 - fieldStart;
			size_t length = rfc2047_to_utf8(&start, &sourceLength,
				sourceLength);
			start[length] = '\0';

			header.AddString(gDefaultFields[fieldIndex].rfc_name, start);
			target = NULL;
		}
	}

	free(buffer);
	return B_OK;
}


void
process_file(const BEntry& entry)
{
	BFile file(&entry, B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		fprintf(stderr, "could not open file: %s\n",
			strerror(file.InitCheck()));
		return;
	}

	switch (gParseMode) {
		case PARSE_HEADER:
		{
			BBufferIO bufferIO(&file, 8192, false);
			BMessage headers;
			parse_header(headers, bufferIO);
			break;
		}
		case EXTRACT_FROM_HEADER:
			format_filter(file);
			break;
		case PARSE_FIELDS:
			parse_fields(file, 8192);
			break;
	}
}


void
process_directory(const BEntry& directoryEntry)
{
	BDirectory directory(&directoryEntry);
	BEntry entry;
	while (directory.GetNextEntry(&entry, false) == B_OK) {
		if (entry.IsDirectory())
			process_directory(entry);
		else
			process_file(entry);
	}
}


int
main(int argc, char** argv)
{
	if (argc < 3) {
		fprintf(stderr, "Expected: <parse|extract|fields> <path-to-mails>\n");
		return 1;
	}

	if (!strcmp(argv[1], "parse"))
		gParseMode = PARSE_HEADER;
	else if (!strcmp(argv[1], "extract"))
		gParseMode = EXTRACT_FROM_HEADER;
	else if (!strcmp(argv[1], "fields"))
		gParseMode = PARSE_FIELDS;
	else {
		fprintf(stderr,
			"Invalid type. Must be one of parse, extract, or fields.\n");
		return 1;
	}

	for (int i = 2; i < argc; i++) {
		BEntry entry(argv[i]);
		if (entry.IsDirectory())
			process_directory(entry);
		else
			process_file(entry);
	}
}
