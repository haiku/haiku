/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
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
#include <String.h>
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
		fprintf(stderr, "strdup_to_utf8(%" B_PRId32 ", %" B_PRId32
			") dst allocate smalled(%" B_PRId32 ")\n", encode, length, dstLen);
	}
	return dup;
}


Grepper::Grepper(BString pattern, const Model* model,
		const BHandler* target, FileIterator* iterator)
	: fPattern(pattern),
	  fTarget(target),
	  fCaseSensitive(model->fCaseSensitive),
	  fEncoding(model->fEncoding),

	  fIterator(iterator),
	  fThreadId(-1),
	  fMustQuit(false)
{
}


Grepper::~Grepper()
{
	Cancel();
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

		BString contents;

		BFile file(&ref, B_READ_WRITE);
		off_t size;
		file.GetSize(&size);
		file.Seek(0, SEEK_SET);
		char* buffer = new char[size];
		file.Read(buffer, size);
		if (fEncoding > 0) {
			char* temp = strdup_to_utf8(fEncoding, buffer, size);
			contents.SetTo(temp, size);
			free(temp);
		} else
			contents.SetTo(buffer, size);
		delete[] buffer;

		int32 index = 0, lines = 1; // First line is 1 not 0
		while (true) {
			int32 newPos;
			if (fCaseSensitive)
				newPos = contents.FindFirst(fPattern, index);
			else
				newPos = contents.IFindFirst(fPattern, index);
			if (newPos == B_ERROR)
				break;

			lines += _CountLines(contents, index, newPos);
			BString linenoAndLine;
			linenoAndLine.SetToFormat("%" B_PRId32 ":%s", lines, _GetLine(contents, newPos).String());
			message.AddString("text", linenoAndLine);

			index = newPos + 1;
		}

		if (message.HasString("text") || fIterator->NotifyNegatives())
			fTarget.SendMessage(&message);
	}

	message.MakeEmpty();
	message.what = MSG_SEARCH_FINISHED;
	fTarget.SendMessage(&message);

	return 0;
}


int32
Grepper::_CountLines(BString& str, int32 startPos, int32 endPos)
{
	int32 ret = 0;
	for (int32 i = startPos; i < endPos; i++) {
		if (str[i] == '\n')
			ret++;
	}
	return ret;
}


BString
Grepper::_GetLine(BString& str, int32 pos)
{
	int32 startPos = 0;
	int32 endPos = str.Length();
	for (int32 i = pos; i > 0; i--) {
		if (str[i] == '\n') {
			startPos = i + 1;
			break;
		}
	}
	for (int32 i = pos; i < endPos; i++) {
		if (str[i] == '\n') {
			endPos = i;
			break;
		}
	}

	BString ret;
	return str.CopyInto(ret, startPos, endPos - startPos);
}
