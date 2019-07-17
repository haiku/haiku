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
#ifndef _SETTINGS_FILE_H
#define _SETTINGS_FILE_H


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>


class BFile;
class BDirectory;
class BRect;

namespace BPrivate {

class Settings;

typedef const char* (*ArgvHandler)(int argc, const char* const *argv,
	void* params);
	// return 0 or error string if parsing failed

const int32 kBufferSize = 1024;


class ArgvParser {
	// this class opens a text file and passes the context in argv
	// format to a specified handler
public:
	static status_t EachArgv(const char* name,
		ArgvHandler argvHandlerFunc, void* passThru);

private:
	ArgvParser(const char* name);
	~ArgvParser();

	status_t EachArgvPrivate(const char* name,
		ArgvHandler argvHandlerFunc, void* passThru);
	int GetCh();

	status_t SendArgv(ArgvHandler argvHandlerFunc, void* passThru);
		// done with a whole line of argv, send it off and get ready
		// to build a new one

	void NextArgv();
		// done with current string, get ready to start building next
	void NextArgvIfNotEmpty();
		// as above, don't commint current string if empty

	void MakeArgvEmpty();

	FILE* fFile;
	char* fBuffer;
	int32 fPos;

	int fArgc;
	char** fCurrentArgv;

	int32 fCurrentArgsPos;
	char fCurrentArgs[1024];

	bool fSawBackslash;
	bool fEatComment;
	bool fInDoubleQuote;
	bool fInSingleQuote;

	int32 fLineNo;
	const char* fFileName;
};


class SettingsArgvDispatcher {
	// base class for a single setting item
public:
	SettingsArgvDispatcher(const char* name);
	virtual ~SettingsArgvDispatcher() {};

	void SaveSettings(Settings* settings, bool onlyIfNonDefault);

	const char* Name() const { return fName; }
		// name as it appears in the settings file

	virtual const char* Handle(const char* const *argv) = 0;
		// override this adding an argv parser that reads in the
		// values in argv format for this setting
		// return a pointer to an error message or null if parsed OK

	// some handy reader/writer calls
	bool HandleRectValue(BRect&, const char* const *argv,
		bool printError = true);
	void WriteRectValue(Settings*, BRect);

protected:
	virtual void SaveSettingValue(Settings* settings) = 0;
		// override this to save the current value of this setting in a
		// text format

	virtual bool NeedsSaving() const { return true; }
		// override to return false if current value is equal to the default
		// and does not need saving

private:
	const char* fName;
};


class Settings {
	// this class is a list of all the settings handlers, reads and
	// saves the settings file
public:
	Settings(const char* filename, const char* settingsDirName);
	~Settings();
	void TryReadingSettings();
	void SaveSettings(bool onlyIfNonDefault = true);

	bool Add(SettingsArgvDispatcher*);
		// return false if argv dispatcher with the same name already
		// registered

	void Write(const char* format, ...);
	void VSWrite(const char*, va_list);

private:
	void MakeSettingsDirectory(BDirectory*);

	SettingsArgvDispatcher* Find(const char*);
	static const char* ParseUserSettings(int, const char* const *argv, void*);
	void SaveCurrentSettings(bool onlyIfNonDefault);

	const char* fFileName;
	const char* fSettingsDir;
		// currently unused
	SettingsArgvDispatcher** fList;
	int32 fCount;
	int32 fListSize;
	BFile* fCurrentSettings;
};

}	// namespace BPrivate

using namespace BPrivate;


#endif	// _SETTINGS_FILE_H
