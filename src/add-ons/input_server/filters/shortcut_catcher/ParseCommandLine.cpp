/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */


#include "ParseCommandLine.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <List.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <SupportKit.h>


const char* kTrackerSignature = "application/x-vnd.Be-TRAK";


// This char is used to hold words together into single words...
#define GUNK_CHAR 0x01


// Turn all spaces that are not-to-be-counted-as-spaces into GUNK_CHAR chars.
static void
GunkSpaces(char* string)
{
	bool insideQuote = false;
	bool afterBackslash = false;

	while (*string != '\0') {
		switch(*string) {
			case '\"':
				if (!afterBackslash) {
					// toggle escapement mode
					insideQuote = !insideQuote;
				}
			break;

			case ' ':
			case '\t':
				if ((insideQuote)||(afterBackslash))
					*string = GUNK_CHAR;
			break;
		}

		afterBackslash = (*string == '\\') ? !afterBackslash : false;
		string++;
	}
}


// Removes all un-escaped quotes and backslashes from the string, in place
static void
RemoveQuotes(char* string)
{
	bool afterBackslash = false;
	char* endString = strchr(string, '\0');
	char* to = string;

	while (*string != '\0') {
		bool temp = (*string == '\\') ? !afterBackslash : false;
		switch(*string) {
			case '\"':
			case '\\':
				if (afterBackslash)
					*(to++) = *string;
				break;

			case 'n':
				*(to++) = afterBackslash ? '\n' : *string;
				break;

			case 't':
				*(to++) = afterBackslash ? '\t' : *string;
				break;

			default:
				*(to++) = *string;
		}

		afterBackslash = temp;
		string++;
	}
	*to = '\0';

	if (to < endString) {
		// needs to be double-terminated!
		*(to + 1) = '\0';
	}
}


static bool
IsValidChar(char c)
{
	return ((c > ' ')||(c == '\n')||(c == '\t'));
}


// Returns true if it is returning valid results into (*setBegin) & (*setEnd).
// If returning true, (*setBegin) now points to the first char in a new word,
// and (*setEnd) now points to the char after the last char in the word, which
// has been set to a NUL byte.
static bool
GetNextWord(char** setBegin, char** setEnd)
{
	char* next = *setEnd;
		// we'll start one after the end of the last one...

	while (next++) {
		if (*next == '\0') {
			// no words left!
			return false;
		}
		else if ((IsValidChar(*next) == false) && (*next != GUNK_CHAR))
			*next = '\0';
		else {
			// found a non-whitespace char!
			break;
		}
	}

	*setBegin = next;
		// we found the first char!

	while (next++) {
		if ((IsValidChar(*next) == false) && (*next != GUNK_CHAR)) {
			*next = '\0';
				// terminate the word
			*setEnd = next;
			return true;
		}
	}

	return false;
		// should never get here, actually
}


// Turns the gunk back into spaces
static void
UnGunk(char* str)
{
	char* temp = str;
	while (*temp) {
		if (*temp == GUNK_CHAR)
			*temp = ' ';

		temp++;
	}
}


char**
ParseArgvFromString(const char* command, int32& argc)
{
	// make our own copy of the string...
	int length = strlen(command);

	// need an extra \0 byte to get GetNextWord() to stop
	char* cmd = new char[length + 2];
	strcpy(cmd, command);
	cmd[length + 1] = '\0';
		// zero out the second nul byte

	GunkSpaces(cmd);
	RemoveQuotes(cmd);

	BList wordlist;
	char* beginWord = NULL, *endWord = cmd - 1;

	while (GetNextWord(&beginWord, &endWord))
		wordlist.AddItem(beginWord);

	argc = wordlist.CountItems();
	char** argv = new char* [argc + 1];
	for (int i = 0; i < argc; i++) {
		char* temp = (char*) wordlist.ItemAt(i);
		argv[i] = new char[strlen(temp) + 1];
		strcpy(argv[i], temp);

		// turn space-markers back into real spaces...
		UnGunk(argv[i]);
	}
	argv[argc] = NULL;
		// terminate the array
	delete[] cmd;
		// don't need our local copy any more

	return argv;
}


void
FreeArgv(char** argv)
{
	if (argv != NULL) {
		int i = 0;
		while (argv[i] != NULL) {
			delete[] argv[i];
			i++;
		}
	}

	delete[] argv;
}


// Make new, independent clone of an argv array and its strings.
char**
CloneArgv(char** argv)
{
	int argc = 0;
	while (argv[argc] != NULL)
		argc++;

	char** newArgv = new char* [argc + 1];
	for (int i = 0; i < argc; i++) {
		newArgv[i] = new char[strlen(argv[i]) + 1];
		strcpy(newArgv[i], argv[i]);
	}
	newArgv[argc] = NULL;

	return newArgv;
}


BString
ParseArgvZeroFromString(const char* command)
{
	char* array = NULL;

	// make our own copy of the array...
	int length = strlen(command);

	// need an extra nul byte to get GetNextWord() to stop
	char* cmd = new char[length + 2];
	strcpy(cmd, command);
	cmd[length + 1] = '\0';
		// zero out the second \0 byte

	GunkSpaces(cmd);
	RemoveQuotes(cmd);

	char* beginWord = NULL, *endWord = cmd - 1;
	if (GetNextWord(&beginWord, &endWord)) {
		array = new char[strlen(beginWord) + 1];
		strcpy(array, beginWord);
		UnGunk(array);
	}
	delete[] cmd;

	BString string(array != NULL ? array : "");
	delete[] array;

	return string;
}


bool
DoStandardEscapes(BString& string)
{
	bool escape = false;

	// Escape any characters that might mess us up
	// note: check this first, or we'll detect the slashes WE put in!
	escape |= EscapeChars(string, '\\');
	escape |= EscapeChars(string, '\"');
	escape |= EscapeChars(string, ' ');
	escape |= EscapeChars(string, '\t');

	return escape;
}


// Modifies (string) so that each instance of (badChar) in it is preceded by a
// backslash. Returns true iff modifications were made.
bool
EscapeChars(BString& string, char badChar)
{
	if (string.FindFirst(badChar) == -1)
		return false;

	BString temp;
	int stringLen = string.Length();
	for (int i = 0; i < stringLen; i++) {
		char next = string[i];
		if (next == badChar)
			temp += '\\';
		temp += next;
	}

	string = temp;
	return true;
}


// Launch the given app/project file. Put here so that Shortcuts and
// BartLauncher can share this code!
status_t
LaunchCommand(char** argv, int32 argc)
{
	BEntry entry(argv[0], true);
	if (entry.Exists()) {
		// See if it's a directory. If it is, ask Tracker to open it, rather
		// than launch.
		BDirectory testDir(&entry);
		if (testDir.InitCheck() == B_OK) {
			entry_ref ref;
			status_t status = entry.GetRef(&ref);
			if (status < B_OK)
				return status;

			BMessenger target(kTrackerSignature);
			BMessage message(B_REFS_RECEIVED);
			message.AddRef("refs", &ref);

			return target.SendMessage(&message);
		} else {
			// It's not a directory, must be a file.
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				if (argc > 1)
					be_roster->Launch(&ref, argc - 1, &argv[1]);
				else
					be_roster->Launch(&ref);
				return B_OK;
			}
		}
	}

	return B_ERROR;
}
