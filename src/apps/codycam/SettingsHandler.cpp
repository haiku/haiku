#include "SettingsHandler.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <Path.h>
#include <StopWatch.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SettingsHandler"


#if 0
static int
Compare(const SettingsArgvDispatcher* p1, const SettingsArgvDispatcher* p2)
{
	return strcmp(p1->Name(), p2->Name());
}
#endif


#if 0
static int
CompareByNameOne(const SettingsArgvDispatcher* item1,
	const SettingsArgvDispatcher* item2)
{
	return strcmp(item1->Name(), item2->Name());
}
#endif


/*! \class ArgvParser
	ArgvParser class opens a text file and passes the context in argv
	format to a specified handler
*/


ArgvParser::ArgvParser(const char* name)
	:
	fFile(0),
	fBuffer(0),
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
		PRINT((B_TRANSLATE("Error opening %s\n"), fFileName));
		return;
	}
	fBuffer = new char [kBufferSize];
	fCurrentArgv = new char* [1024];
}


ArgvParser::~ArgvParser()
{
	delete[] fBuffer;

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
		delete fCurrentArgv[index];

	fArgc = 0;
}


status_t
ArgvParser::SendArgv(ArgvHandler argvHandlerFunc, void* passThru)
{
	if (fArgc) {
		NextArgv();
		fCurrentArgv[fArgc] = 0;
		const char *result = (argvHandlerFunc)(fArgc, fCurrentArgv, passThru);
		if (result)
			printf(B_TRANSLATE("File %s; Line %ld # %s"), fFileName, fLineNo, result);
		MakeArgvEmpty();
		if (result)
			return B_ERROR;
	}

	return B_NO_ERROR;
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
			// done with file
			if (fInDoubleQuote || fInSingleQuote) {
				printf(B_TRANSLATE("File %s # unterminated quote at end of "
					"file\n"), name);
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
				printf(B_TRANSLATE("File %s ; Line %ld # unterminated "
					"quote\n"), name, fLineNo);
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
			if (result != B_NO_ERROR)
				break;
			continue;
		}

		if (fEatComment)
			continue;

		if (!fSawBackslash) {
			if (!fInDoubleQuote && !fInSingleQuote) {
				if (ch == ';') {
					// semicolon is a command separator, pass on the whole argv
					result = SendArgv(argvHandlerFunc, passThru);
					if (result != B_NO_ERROR)
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


//	#pragma mark -


SettingsArgvDispatcher::SettingsArgvDispatcher(const char* name)
	:
	fName(name)
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
SettingsArgvDispatcher::HandleRectValue(BRect &result, const char* const *argv,
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


//	#pragma mark -


/*!	\class Settings
	this class represents a list of all the settings handlers, reads and
	saves the settings file
*/


Settings::Settings(const char* filename, const char* settingsDirName)
	:
	fFileName(filename),
	fSettingsDir(settingsDirName),
	fList(0),
	fCount(0),
	fListSize(30),
	fCurrentSettings(0)
{
#ifdef SINGLE_SETTING_FILE
	settingsHandler = this;
#endif
	fList = (SettingsArgvDispatcher**)calloc(fListSize, sizeof(SettingsArgvDispatcher *));
}


Settings::~Settings()
{
	for (int32 index = 0; index < fCount; index++)
		delete fList[index];

	free(fList);
}


const char*
Settings::_ParseUserSettings(int, const char* const *argv, void* castToThis)
{
	if (!*argv)
		return 0;

#ifdef SINGLE_SETTING_FILE
	Settings* settings = settingsHandler;
#else
	Settings* settings = (Settings*)castToThis;
#endif

	SettingsArgvDispatcher* handler = settings->_Find(*argv);
	if (!handler)
		return B_TRANSLATE("unknown command");
	return handler->Handle(argv);
}


/*!
	Returns false if argv dispatcher with the same name already
	registered
*/
bool
Settings::Add(SettingsArgvDispatcher* setting)
{
	// check for uniqueness
	if (_Find(setting->Name()))
		return false;

	if (fCount >= fListSize) {
		fListSize += 30;
		fList = (SettingsArgvDispatcher **)realloc(fList,
			fListSize * sizeof(SettingsArgvDispatcher *));
	}
	fList[fCount++] = setting;
	return true;
}


SettingsArgvDispatcher*
Settings::_Find(const char* name)
{
	for (int32 index = 0; index < fCount; index++)
		if (strcmp(name, fList[index]->Name()) == 0)
			return fList[index];

	return 0;
}


void
Settings::TryReadingSettings()
{
	BPath prefsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefsPath, true) == B_OK) {
		prefsPath.Append(fSettingsDir);

		BPath path(prefsPath);
		path.Append(fFileName);
		ArgvParser::EachArgv(path.Path(), Settings::_ParseUserSettings, this);
	}
}


void
Settings::SaveSettings(bool onlyIfNonDefault)
{
	ASSERT(SettingsHandler());
	SettingsHandler()->_SaveCurrentSettings(onlyIfNonDefault);
}


void
Settings::_MakeSettingsDirectory(BDirectory *resultingSettingsDir)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;

	// make sure there is a directory
	path.Append(fSettingsDir);
	mkdir(path.Path(), 0777);
	resultingSettingsDir->SetTo(path.Path());
}


void
Settings::_SaveCurrentSettings(bool onlyIfNonDefault)
{
	BDirectory fSettingsDir;
	_MakeSettingsDirectory(&fSettingsDir);

	if (fSettingsDir.InitCheck() != B_OK)
		return;

	printf("+++++++++++ Settings::_SaveCurrentSettings %s\n", fFileName);
	// nuke old settings
	BEntry entry(&fSettingsDir, fFileName);
	entry.Remove();

	BFile prefs(&entry, O_RDWR | O_CREAT);
	if (prefs.InitCheck() != B_OK)
		return;

	fCurrentSettings = &prefs;
	for (int32 index = 0; index < fCount; index++) {
		fList[index]->SaveSettings(this, onlyIfNonDefault);
	}

	fCurrentSettings = 0;
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
	char buffer[2048];
	vsprintf(buffer, format, arg);
	ASSERT(fCurrentSettings && fCurrentSettings->InitCheck() == B_OK);
	fCurrentSettings->Write(buffer, strlen(buffer));
}


#ifdef SINGLE_SETTING_FILE
Settings* Settings::settingsHandler = 0;
#endif
