/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Grepper.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include <OS.h>
#include <image.h>

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


const char* kEOFTag = "//EOF";


using std::nothrow;

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
		fprintf(stderr, "strdup_from_utf8(%" B_PRId32 ", %" B_PRId32
			") dst allocate smalled(%" B_PRId32 ")\n", encode, length, dstLen);
	}
	return dup;
}


Grepper::Grepper(const char* pattern, const Model* model,
		const BHandler* target, FileIterator* iterator)
	: fPattern(NULL),
	  fTarget(target),
	  fRegularExpression(model->fRegularExpression),
	  fCaseSensitive(model->fCaseSensitive),
	  fEncoding(model->fEncoding),

	  fIterator(iterator),
	  fRunnerThreadId(-1),
	  fXargsInput(-1),
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
	fRunnerThreadId = spawn_thread(
		_SpawnRunnerThread, "Grep runner", B_NORMAL_PRIORITY, this);

	resume_thread(fRunnerThreadId);
}


void
Grepper::Cancel()
{
	if (fRunnerThreadId < 0)
		return;

	fMustQuit = true;
	int32 exitValue;
	wait_for_thread(fRunnerThreadId, &exitValue);
	fRunnerThreadId = -1;
}


// #pragma mark - private


int32
Grepper::_SpawnWriterThread(void* cookie)
{
	Grepper* self = static_cast<Grepper*>(cookie);
	return self->_WriterThread();
}


int32
Grepper::_WriterThread()
{
	BMessage message;
	char fileName[B_PATH_NAME_LENGTH*2];
	int count = 0;

	printf("paths_writer started.\n");

	while (!fMustQuit && fIterator->GetNextName(fileName)) {

		message.MakeEmpty();
		message.what = MSG_REPORT_FILE_NAME;
		message.AddString("filename", fileName);
//		fTarget.SendMessage(&message);

		BEntry entry(fileName);
		entry_ref ref;
		entry.GetRef(&ref);
		if (!entry.Exists()) {
			if (fIterator->NotifyNegatives()) {
				message.what = MSG_REPORT_RESULT;
				message.AddRef("ref", &ref);
				fTarget.SendMessage(&message);
			}
			continue;
		}

		if (!_EscapeSpecialChars(fileName, B_PATH_NAME_LENGTH)) {
			char tempString[B_PATH_NAME_LENGTH + 32];
			sprintf(tempString, B_TRANSLATE("%s: Not enough room to escape "
				"the filename."), fileName);
			message.MakeEmpty();
			message.what = MSG_REPORT_ERROR;
			message.AddString("error", tempString);
			fTarget.SendMessage(&message);
			continue;
		}

		// file exists, send it to xargs
		write(fXargsInput, fileName, strlen(fileName));
		write(fXargsInput, "\n", 1);
		// printf(">>>>>> %s\n", fileName);

		fTarget.SendMessage(&message);

		count++;
	}

	write(fXargsInput, kEOFTag, strlen(kEOFTag));
	write(fXargsInput, "\n", 1);
	close(fXargsInput);

	printf("paths_writer stopped (%d paths).\n", count);

	return 0;
}


int32
Grepper::_SpawnRunnerThread(void* cookie)
{
	Grepper* self = static_cast<Grepper*>(cookie);
	return self->_RunnerThread();
}


int32
Grepper::_RunnerThread()
{
	BMessage message;
	char fileName[B_PATH_NAME_LENGTH];

	const char* argv[32];
	int argc = 0;
	argv[argc++] = "xargs";

	// can't use yet the --null mode due to pipe issue
	// the xargs stdin input pipe closure is not detected
	// by xargs. Instead, we use eof-string mode

	// argv[argc++] = "--null";
	argv[argc++] = "-E";
	argv[argc++] = kEOFTag;

	// Enable parallel mode
	// Retrieve cpu count for to parallel xargs via -P argument
	char cpuCount[8];
	system_info sys_info;
	get_system_info(&sys_info);
	snprintf(cpuCount, sizeof(cpuCount), "%" B_PRIu32, sys_info.cpu_count);
	argv[argc++] = "-P";
	argv[argc++] = cpuCount;

	// grep command driven by xargs dispatcher
	argv[argc++] = "grep";
	argv[argc++] = "-n"; // need matching line(s) number(s)
	argv[argc++] = "-H"; // need filename prefix
	if (! fCaseSensitive)
		argv[argc++] = "-i";
	if (! fRegularExpression)
		argv[argc++] = "-F";	 // no a regexp: force fixed string, 
	argv[argc++] = fPattern;
	argv[argc] = NULL;

	// prepare xargs to run with stdin, stdout and stderr pipes

	int oldStdIn, oldStdOut, oldStdErr;
	oldStdIn  = dup(STDIN_FILENO);
	oldStdOut = dup(STDOUT_FILENO);
	oldStdErr = dup(STDERR_FILENO);

	int fds[2];
	pipe(fds); dup2(fds[0], STDIN_FILENO); close(fds[0]);
	fXargsInput = fds[1];	// write to in, appears on command's stdin

	pipe(fds); dup2(fds[1], STDOUT_FILENO);	close(fds[1]);
	int out = fds[0]; // read from out, taken from command's stdout

	pipe(fds);dup2(fds[1], STDERR_FILENO); close(fds[1]);
	int err = fds[0]; // read from err, taken from command's stderr

	// "load" command
	thread_id xargsThread = load_image(argc, argv,
		const_cast<const char**>(environ));
	// xargsThread is suspended after loading

	// restore our previous stdin, stdout and stderr
	close(STDIN_FILENO); dup(oldStdIn);	close(oldStdIn);
	close(STDOUT_FILENO); dup(oldStdOut); close(oldStdOut);
	close(STDERR_FILENO); dup(oldStdErr); close(oldStdErr);

	if (xargsThread < B_OK) {
		message.MakeEmpty();
		message.what = MSG_REPORT_ERROR;
		message.AddString("error",
			B_TRANSLATE("Failed to start xargs program!"));
		fTarget.SendMessage(&message);
		return 0;
	}
	set_thread_priority(xargsThread, B_LOW_PRIORITY);

	thread_id writerThread = spawn_thread(_SpawnWriterThread,
		"Grep writer", B_LOW_PRIORITY, this);
	// let's go!
	resume_thread(xargsThread);
	resume_thread(writerThread);

	// Listen on xargs's stdout and stderr via select()
	printf("Running: ");
	for (int i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");

	int fdl[2] = { out, err };
	int maxfd = 0;
	for (int i = 0; i < 2; i++) {
		if (maxfd < fdl[i])
			maxfd = fdl[i];
	}

	fd_set readSet;
	char line[B_PATH_NAME_LENGTH * 2];

	FILE* output = fdopen(out, "r");
	FILE* errors = fdopen(err, "r");

	char currentFileName[B_PATH_NAME_LENGTH];
	currentFileName[0] = '\0';
	bool canReadOutput, canReadErrors;
	canReadOutput = canReadErrors = true;
	while (!fMustQuit && (canReadOutput || canReadErrors)) {
		FD_ZERO(&readSet);
		if (canReadOutput) {
			FD_SET(out, &readSet);
		}
		if (canReadErrors) {
			FD_SET(err, &readSet);
		}

		int result = select(maxfd + 1, &readSet, NULL, NULL, NULL);
		if (result < 0) {
			printf("select(): %d (%s)\n", result, strerror(errno));
			break;
		}

		if (canReadOutput && FD_ISSET(out, &readSet)) {
			if (fgets(line, sizeof(line), output) != NULL) {
				// parse grep output
				int lineNumber = -1;
				int textPos = -1;
				sscanf(line, "%[^\n:]:%d:%n", fileName, &lineNumber, &textPos);
				// printf("sscanf(\"%s\") -> %s %d %d\n", line, fileName,
				//		lineNumber, textPos);
				if (textPos > 0) {
					if (strcmp(fileName, currentFileName) != 0) {
						fTarget.SendMessage(&message);

						strncpy(currentFileName, fileName, 
							sizeof(currentFileName));

						message.MakeEmpty();
						message.what = MSG_REPORT_RESULT;
						message.AddString("filename", fileName);

						BEntry entry(fileName);
						entry_ref ref;
						entry.GetRef(&ref);
						message.AddRef("ref", &ref);
					}

					char* text = &line[strlen(fileName)+1];
					printf("[%s] %s", fileName, text);
					if (fEncoding > 0) {
						char* tempdup = strdup_to_utf8(fEncoding, text,
							strlen(text));
						message.AddString("text", tempdup);
						free(tempdup);
					} else {
						message.AddString("text", text);
					}
					message.AddInt32("line", lineNumber);
				}
			} else {
				canReadOutput = false;
			}
		}
		if (canReadErrors && FD_ISSET(err, &readSet)) {
			if (fgets(line, sizeof(line), errors) != NULL) {

				if (message.HasString("text"))
					fTarget.SendMessage(&message);
				currentFileName[0] = '\0';

				message.MakeEmpty();
				message.what = MSG_REPORT_ERROR;
				message.AddString("error", line);
				fTarget.SendMessage(&message);
			} else {
				canReadErrors = false;
			}
		}
	}

	// send last pending message, if any
	if (message.HasString("text"))
		fTarget.SendMessage(&message);

	printf("Done.\n");
	fclose(output);
	fclose(errors);

	close(out);
	close(err);

	fMustQuit = true;
	int32 exitValue;
	wait_for_thread(xargsThread, &exitValue);
	wait_for_thread(writerThread, &exitValue);

	// We wait with removing the temporary file until after the
	// entire search has finished, to prevent a lot of flickering
	// if the Tracker window for /tmp/ might be open.

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

	fPattern = strdup(src);
}


bool
Grepper::_EscapeSpecialChars(char* buffer, ssize_t bufferSize)
{
	char* copy = strdup(buffer);
	char* start = buffer;
	uint32 len = strlen(copy);
	bool result = true;
	for (uint32 count = 0; count < len; ++count) {
		if (copy[count] == ' ')
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
