/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "Grepper.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <Directory.h>
#include <List.h>
#include <Locale.h>
#include <NodeInfo.h>
#include <Path.h>
#include <UTF8.h>

#include "FileIterator.h"
#include "Model.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Grepper"


using std::nothrow;

// TODO: stippi: Check if this is a the best place to maintain a global
// list of files and folders for node monitoring. It should probably monitor
// every file that was grepped, as well as every visited (sub) folder.
// For the moment I don't know the life cycle of the Grepper object.


char*
strdup_to_utf8(uint32 encode, const char* src, int32 length)
{
	int32 srcLen = length;
	int32 dstLen = 2 * srcLen;
	// TODO: stippi: Why the duplicate copy? Why not just return
	// dst (and allocate with malloc() instead of new)? Is 2 * srcLen
	// enough space? Check return value of convert_to_utf8 and keep
	// converting if it didn't fit?
	char* dst = new (nothrow) char[dstLen + 1];
	if (dst == NULL)
		return NULL;
	int32 cookie = 0;
	convert_to_utf8(encode, src, &srcLen, dst, &dstLen, &cookie);
	dst[dstLen] = '\0';
	char* dup = strdup(dst);
	delete[] dst;
	if (srcLen != length) {
		fprintf(stderr, "strdup_to_utf8(%ld, %ld) dst allocate smalled(%ld)\n",
			encode, length, dstLen);
	}
	return dup;
}


char*
strdup_from_utf8(uint32 encode, const char* src, int32 length)
{
	int32 srcLen = length;
	int32 dstLen = srcLen;
	char* dst = new (nothrow) char[dstLen + 1];
	if (dst == NULL)
		return NULL;
	int32 cookie = 0;
	convert_from_utf8(encode, src, &srcLen, dst, &dstLen, &cookie);
	// TODO: See above.
	dst[dstLen] = '\0';
	char* dup = strdup(dst);
	delete[] dst;
	if (srcLen != length) {
		fprintf(stderr, "strdup_from_utf8(%ld, %ld) dst allocate "
			"smalled(%ld)\n", encode, length, dstLen);
	}
	return dup;
}


Grepper::Grepper(const char* pattern, const Model* model,
		const BHandler* target, FileIterator* iterator)
	: fPattern(NULL),
	  fTarget(target),
	  fEscapeText(model->fEscapeText),
	  fCaseSensitive(model->fCaseSensitive),
	  fEncoding(model->fEncoding),

	  fIterator(iterator),
	  fThreadId(-1),
	  fMustQuit(false)
{
	if (fEncoding > 0) {
		char* src = strdup_from_utf8(fEncoding, pattern, strlen(pattern));
		_SetPattern(src);
		free(src);
	} else
		_SetPattern(pattern);
}


Grepper::~Grepper()
{
	Cancel();
	free(fPattern);
	delete fIterator;
}


bool
Grepper::IsValid() const
{
	if (fIterator == NULL || !fIterator->IsValid())
		return false;
	return fPattern != NULL;
}


void
Grepper::Start()
{
	Cancel();

	fMustQuit = false;
	fThreadId = spawn_thread(
		_SpawnThread, "_GrepperThread", B_NORMAL_PRIORITY, this);

	resume_thread(fThreadId);
}


void
Grepper::Cancel()
{
	if (fThreadId < 0)
		return;

	fMustQuit = true;
	int32 exitValue;
	wait_for_thread(fThreadId, &exitValue);
	fThreadId = -1;
}


// #pragma mark - private


int32
Grepper::_SpawnThread(void* cookie)
{
	Grepper* self = static_cast<Grepper*>(cookie);
	return self->_GrepperThread();
}


int32
Grepper::_GrepperThread()
{
	BMessage message;

	char fileName[B_PATH_NAME_LENGTH];
	char tempString[B_PATH_NAME_LENGTH];
	char command[B_PATH_NAME_LENGTH + 32];

	BPath tempFile;
	sprintf(fileName, "/tmp/SearchText%ld", fThreadId);
	tempFile.SetTo(fileName);

	while (!fMustQuit && fIterator->GetNextName(fileName)) {

		message.MakeEmpty();
		message.what = MSG_REPORT_FILE_NAME;
		message.AddString("filename", fileName);
		fTarget.SendMessage(&message);

		message.MakeEmpty();
		message.what = MSG_REPORT_RESULT;
		message.AddString("filename", fileName);

		BEntry entry(fileName);
		entry_ref ref;
		entry.GetRef(&ref);
		message.AddRef("ref", &ref);

		if (!entry.Exists()) {
			if (fIterator->NotifyNegatives())
				fTarget.SendMessage(&message);
			continue;
		}

		if (!_EscapeSpecialChars(fileName, B_PATH_NAME_LENGTH)) {
			sprintf(tempString, B_TRANSLATE("%s: Not enough room to escape "
				"the filename."), fileName);

			message.MakeEmpty();
			message.what = MSG_REPORT_ERROR;
			message.AddString("error", tempString);
			fTarget.SendMessage(&message);
			continue;
		}

		sprintf(command, "grep -hn %s %s \"%s\" > \"%s\"",
			fCaseSensitive ? "" : "-i", fPattern, fileName, tempFile.Path());

		int res = system(command);

		if (res == 0 || res == 1) {
			FILE *results = fopen(tempFile.Path(), "r");

			if (results != NULL) {
				while (fgets(tempString, B_PATH_NAME_LENGTH, results) != 0) {
					if (fEncoding > 0) {
						char* tempdup = strdup_to_utf8(fEncoding, tempString,
							strlen(tempString));
						message.AddString("text", tempdup);
						free(tempdup);
					} else
						message.AddString("text", tempString);
				}

				if (message.HasString("text") || fIterator->NotifyNegatives())
					fTarget.SendMessage(&message);

				fclose(results);
				continue;
			}
		}

		sprintf(tempString, B_TRANSLATE("%s: There was a problem running grep."), fileName);

		message.MakeEmpty();
		message.what = MSG_REPORT_ERROR;
		message.AddString("error", tempString);
		fTarget.SendMessage(&message);
	}

	// We wait with removing the temporary file until after the
	// entire search has finished, to prevent a lot of flickering
	// if the Tracker window for /tmp/ might be open.

	remove(tempFile.Path());

	message.MakeEmpty();
	message.what = MSG_SEARCH_FINISHED;
	fTarget.SendMessage(&message);

	return 0;
}


void
Grepper::_SetPattern(const char* src)
{
	if (src == NULL)
		return;

	if (!fEscapeText) {
		fPattern = strdup(src);
		return;
	}

	// We will simply guess the size of the memory buffer
	// that we need. This should always be large enough.
	fPattern = (char*)malloc((strlen(src) + 1) * 3 * sizeof(char));
	if (fPattern == NULL)
		return;

	const char* srcPtr = src;
	char* dstPtr = fPattern;

	// Put double quotes around the pattern, so separate
	// words are considered to be part of a single string.
	*dstPtr++ = '"';

	while (*srcPtr != '\0') {
		char c = *srcPtr++;

		// Put a backslash in front of characters
		// that should be escaped.
		if ((c == '.')  || (c == ',')
			||  (c == '[')  || (c == ']')
			||  (c == '?')  || (c == '*')
			||  (c == '+')  || (c == '-')
			||  (c == ':')  || (c == '^')
			||  (c == '"')	|| (c == '`')) {
			*dstPtr++ = '\\';
		} else if ((c == '\\') || (c == '$')) {
			// Some characters need to be escaped
			// with *three* backslashes in a row.
			*dstPtr++ = '\\';
			*dstPtr++ = '\\';
			*dstPtr++ = '\\';
		}

		// Note: we do not have to escape the
		// { } ( ) < > and | characters.

		*dstPtr++ = c;
	}

	*dstPtr++ = '"';
	*dstPtr = '\0';
}


bool
Grepper::_EscapeSpecialChars(char* buffer, ssize_t bufferSize)
{
	char* copy = strdup(buffer);
	char* start = buffer;
	uint32 len = strlen(copy);
	bool result = true;
	for (uint32 count = 0; count < len; ++count) {
		if (copy[count] == '"' || copy[count] == '$')
			*buffer++ = '\\';
		if (buffer - start == bufferSize - 1) {
			result = false;
			break;
		}
		*buffer++ = copy[count];
	}
	*buffer = '\0';
	free(copy);
	return result;
}

