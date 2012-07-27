/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <StopWatch.h>

#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


#include "SettingsHandler.h"

ArgvParser::ArgvParser(const char* name)
	:	fFile(0),
		fBuffer(NULL),
		fPos(-1),
		fArgc(0),
		fCurrentArgv(0),
		fCurrentArgsPos(-1),
		fSawBackslash(false),
		fEatComment(false),
		fInDoubleQuote(false),
		fInSingleQuote(false),
		fLineNo(0),
		fFileName(name)
{
	fFile = fopen(fFileName, "r");
	if (!fFile) {
		PRINT(("Error opening %s\n", fFileName));
		return;
	}
	fBuffer = new char [kBufferSize];
	fCurrentArgv = new char * [1024];
}


ArgvParser::~ArgvParser()
{
	delete [] fBuffer;

	MakeArgvEmpty();
	delete [] fCurrentArgv;

	if (fFile)
		fclose(fFile);
}


void
ArgvParser::MakeArgvEmpty()
{
	// done with current argv, free it up
	for (int32 index = 0; index < fArgc; index++)
		delete[] fCurrentArgv[index];
	
	fArgc = 0;
}


status_t
ArgvParser::SendArgv(ArgvHandler argvHandlerFunc, void* passThru)
{
	if (fArgc) {
		NextArgv();
		fCurrentArgv[fArgc] = 0;
		const char* result = (argvHandlerFunc)(fArgc, fCurrentArgv, passThru);
		if (result)
			printf("File %s; Line %ld # %s", fFileName, fLineNo, result);
		MakeArgvEmpty();
		if (result)
			return B_ERROR;
	}

	return B_OK;
}


void
ArgvParser::NextArgv()
{
	if (fSawBackslash) {
		fCurrentArgs[++fCurrentArgsPos] = '\\';
		fSawBackslash = false;
	}
	fCurrentArgs[++fCurrentArgsPos] = '\0';
	// terminate current arg pos
	
	// copy it as a string to the current argv slot
	fCurrentArgv[fArgc] = new char [strlen(fCurrentArgs) + 1];
	strcpy(fCurrentArgv[fArgc], fCurrentArgs);
	fCurrentArgsPos = -1;
	fArgc++;
}


void
ArgvParser::NextArgvIfNotEmpty()
{
	if (!fSawBackslash && fCurrentArgsPos < 0)
		return;

	NextArgv();
}


char
ArgvParser::GetCh()
{
	if (fPos < 0 || fBuffer[fPos] == 0) {
		if (fFile == 0)
			return EOF;
		if (fgets(fBuffer, kBufferSize, fFile) == 0)
			return EOF;
		fPos = 0;
	}
	return fBuffer[fPos++];
}


status_t
ArgvParser::EachArgv(const char* name, ArgvHandler argvHandlerFunc,
	void* passThru)
{
	ArgvParser parser(name);
	return parser.EachArgvPrivate(name, argvHandlerFunc, passThru);
}


status_t
ArgvParser::EachArgvPrivate(const char* name, ArgvHandler argvHandlerFunc,
	void* passThru)
{
	status_t result;

	for (;;) {
		char ch = GetCh();
		if (ch == EOF) {
			// done with fFile
			if (fInDoubleQuote || fInSingleQuote) {
				printf("File %s # unterminated quote at end of file\n", name);
				result = B_ERROR;
				break;
			}
			result = SendArgv(argvHandlerFunc, passThru);
			break;
		}

		if (ch == '\n' || ch == '\r') {
			// handle new line
			fEatComment = false;
			if (!fSawBackslash && (fInDoubleQuote || fInSingleQuote)) {
				printf("File %s ; Line %ld # unterminated quote\n", name, fLineNo);
				result = B_ERROR;
				break;
			}

			fLineNo++;
			if (fSawBackslash) {
				fSawBackslash = false;
				continue;
			}

			// end of line, flush all argv
			result = SendArgv(argvHandlerFunc, passThru);

			continue;
		}

		if (fEatComment)
			continue;

		if (!fSawBackslash) {
			if (!fInDoubleQuote && !fInSingleQuote) {
				if (ch == ';') {
					// semicolon is a command separator, pass on the whole argv
					result = SendArgv(argvHandlerFunc, passThru);
					if (result != B_OK)
						break;
					continue;
				} else if (ch == '#') {
					// ignore everything on this line after this character
					fEatComment = true;
					continue;
				} else if (ch == ' ' || ch == '\t') {
					// space or tab separates the individual arg strings
					NextArgvIfNotEmpty();
					continue;
				} else if (!fSawBackslash && ch == '\\') {
					// the next character is escaped
					fSawBackslash = true;
					continue;
				}
			}
			if (!fInSingleQuote && ch == '"') {
				// enter/exit double quote handling
				fInDoubleQuote = !fInDoubleQuote;
				continue;
			}
			if (!fInDoubleQuote && ch == '\'') {
				// enter/exit single quote handling
				fInSingleQuote = !fInSingleQuote;
				continue;
			}
		} else {
			// we just pass through the escape sequence as is
			fCurrentArgs[++fCurrentArgsPos] = '\\';
			fSawBackslash = false;
		}
		fCurrentArgs[++fCurrentArgsPos] = ch;
	}

	return result;
}


SettingsArgvDispatcher::SettingsArgvDispatcher(const char* name)
	:	name(name)
{
}


void
SettingsArgvDispatcher::SaveSettings(Settings* settings, bool onlyIfNonDefault)
{
	if (!onlyIfNonDefault || NeedsSaving()) {
		settings->Write("%s ", Name());
		SaveSettingValue(settings);
		settings->Write("\n");
	}
}


bool
SettingsArgvDispatcher::HandleRectValue(BRect &result, const char* const* argv,
	bool printError)
{
	if (!*argv) {
		if (printError)
			printf("rect left expected");
		return false;
	}
	result.left = atoi(*argv);

	if (!*++argv) {
		if (printError)
			printf("rect top expected");
		return false;
	}
	result.top = atoi(*argv);

	if (!*++argv) {
		if (printError)
			printf("rect right expected");
		return false;
	}
	result.right = atoi(*argv);

	if (!*++argv) {
		if (printError)
			printf("rect bottom expected");
		return false;
	}
	result.bottom = atoi(*argv);

	return true;
}


void
SettingsArgvDispatcher::WriteRectValue(Settings* setting, BRect rect)
{
	setting->Write("%d %d %d %d", (int32)rect.left, (int32)rect.top,
		(int32)rect.right, (int32)rect.bottom);
}


Settings::Settings(const char* filename, const char* settingsDirName)
	:	fFileName(filename),
		fSettingsDir(settingsDirName),
		fList(0),
		fCount(0),
		fListSize(30),
		fCurrentSettings(0)
{
	fList = (SettingsArgvDispatcher**)calloc((size_t)fListSize,
		sizeof(SettingsArgvDispatcher*));
}


Settings::~Settings()
{
	for (int32 index = 0; index < fCount; index++)
		delete fList[index];

	free(fList);
}


const char*
Settings::ParseUserSettings(int, const char* const* argv, void* castToThis)
{
	if (!*argv)
		return 0;
	
	SettingsArgvDispatcher* handler = ((Settings*)castToThis)->Find(*argv);
	if (!handler)
		return "unknown command";

	return handler->Handle(argv);
}


bool
Settings::Add(SettingsArgvDispatcher* setting)
{
	// check for uniqueness
	if (Find(setting->Name()))
		return false;

	if (fCount >= fListSize) {
		fListSize += 30;
		fList = (SettingsArgvDispatcher**)realloc(fList,
			fListSize * sizeof(SettingsArgvDispatcher*));
	}
	fList[fCount++] = setting;
	return true;
}


SettingsArgvDispatcher*
Settings::Find(const char* name)
{
	for (int32 index = 0; index < fCount; index++)
		if (strcmp(name, fList[index]->Name()) == 0)
			return fList[index];

	return NULL;
}


void
Settings::TryReadingSettings()
{
	BPath prefsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefsPath, true) == B_OK) {
		prefsPath.Append(fSettingsDir);

		BPath path(prefsPath);
		path.Append(fFileName);
		ArgvParser::EachArgv(path.Path(), Settings::ParseUserSettings, this);
	}
}


void
Settings::SaveSettings(bool onlyIfNonDefault)
{
	SaveCurrentSettings(onlyIfNonDefault);
}


void
Settings::MakeSettingsDirectory(BDirectory* resultingSettingsDir)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	
	// make sure there is a directory
	// mkdir() will only make one leaf at a time, unfortunately
	path.Append(fSettingsDir);
	char* ptr = (char *)alloca(strlen(path.Path()) + 1);
	strcpy(ptr, path.Path());
	char* end = ptr+strlen(ptr);
	char* mid = ptr+1;
	while (mid < end) {
		mid = strchr(mid, '/');
		if (!mid) break;
		*mid = 0;
		mkdir(ptr, 0777);
		*mid = '/';
		mid++;
	}
	mkdir(ptr, 0777);
	resultingSettingsDir->SetTo(path.Path());
}


void
Settings::SaveCurrentSettings(bool onlyIfNonDefault)
{
	BDirectory settingsDir;
	MakeSettingsDirectory(&settingsDir);

	if (settingsDir.InitCheck() != B_OK)
		return;

	// nuke old settings
	BEntry entry(&settingsDir, fFileName);
	entry.Remove();

	BFile prefs(&entry, O_RDWR | O_CREAT);
	if (prefs.InitCheck() != B_OK)
		return;

	fCurrentSettings = &prefs;
	for (int32 index = 0; index < fCount; index++)
		fList[index]->SaveSettings(this, onlyIfNonDefault);

	fCurrentSettings = NULL;
}


void
Settings::Write(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	VSWrite(format, args);
	va_end(args);
}


void
Settings::VSWrite(const char* format, va_list arg)
{
	char fBuffer[2048];
	vsprintf(fBuffer, format, arg);
	ASSERT(fCurrentSettings && fCurrentSettings->InitCheck() == B_OK);
	fCurrentSettings->Write(fBuffer, strlen(fBuffer));
}
