
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <StopWatch.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <Debug.h>

#include "SettingsHandler.h"

ArgvParser::ArgvParser(const char *name)
	:	file(0),
		buffer(0),
		pos(-1),
		argc(0),
		currentArgv(0),
		currentArgsPos(-1),
		sawBackslash(false),
		eatComment(false),
		inDoubleQuote(false),
		inSingleQuote(false),
		lineNo(0),
		fileName(name)
{
	file = fopen(fileName, "r");
	if (!file) {
		PRINT(("Error opening %s\n", fileName));
		return;
	}
	buffer = new char [kBufferSize];
	currentArgv = new char * [1024];
}


ArgvParser::~ArgvParser()
{
	delete [] buffer;

	MakeArgvEmpty();
	delete [] currentArgv;

	if (file)
		fclose(file);
}

void 
ArgvParser::MakeArgvEmpty()
{
	// done with current argv, free it up
	for (int32 index = 0; index < argc; index++)
		delete currentArgv[index];
	
	argc = 0;
}

status_t 
ArgvParser::SendArgv(ArgvHandler argvHandlerFunc, void *passThru)
{
	if (argc) {
		NextArgv();
		currentArgv[argc] = 0;
		const char *result = (argvHandlerFunc)(argc, currentArgv, passThru);
		if (result)
			printf("File %s; Line %ld # %s", fileName, lineNo, result);
		MakeArgvEmpty();
		if (result)
			return B_ERROR;
	}

	return B_NO_ERROR;
}

void 
ArgvParser::NextArgv()
{
	if (sawBackslash) {
		currentArgs[++currentArgsPos] = '\\';
		sawBackslash = false;
	}
	currentArgs[++currentArgsPos] = '\0';
	// terminate current arg pos
	
	// copy it as a string to the current argv slot
	currentArgv[argc] = new char [strlen(currentArgs) + 1];
	strcpy(currentArgv[argc], currentArgs);
	currentArgsPos = -1;
	argc++;
}

void 
ArgvParser::NextArgvIfNotEmpty()
{
	if (!sawBackslash && currentArgsPos < 0)
		return;

	NextArgv();
}

char 
ArgvParser::GetCh()
{
	if (pos < 0 || buffer[pos] == 0) {
		if (file == 0)
			return EOF;
		if (fgets(buffer, kBufferSize, file) == 0)
			return EOF;
		pos = 0;
	}
	return buffer[pos++];
}

status_t 
ArgvParser::EachArgv(const char *name, ArgvHandler argvHandlerFunc, void *passThru)
{
	ArgvParser parser(name);
	return parser.EachArgvPrivate(name, argvHandlerFunc, passThru);
}

status_t 
ArgvParser::EachArgvPrivate(const char *name, ArgvHandler argvHandlerFunc, void *passThru)
{
	status_t result;

	for (;;) {
		char ch = GetCh();
		if (ch == EOF) {
			// done with file
			if (inDoubleQuote || inSingleQuote) {
				printf("File %s # unterminated quote at end of file\n", name);
				result = B_ERROR;
				break;
			}
			result = SendArgv(argvHandlerFunc, passThru);
			break;
		}

		if (ch == '\n' || ch == '\r') {
			// handle new line
			eatComment = false;
			if (!sawBackslash && (inDoubleQuote || inSingleQuote)) {
				printf("File %s ; Line %ld # unterminated quote\n", name, lineNo);
				result = B_ERROR;
				break;
			}
			lineNo++; 
			if (sawBackslash) {
				sawBackslash = false;
				continue;
			}
			// end of line, flush all argv
			result = SendArgv(argvHandlerFunc, passThru);
			if (result != B_NO_ERROR)
				break;
			continue;
		}
		
		if (eatComment)
			continue;

		if (!sawBackslash) {
			if (!inDoubleQuote && !inSingleQuote) {
				if (ch == ';') {
					// semicolon is a command separator, pass on the whole argv
					result = SendArgv(argvHandlerFunc, passThru);
					if (result != B_NO_ERROR)
						break;
					continue;
				} else if (ch == '#') {
					// ignore everything on this line after this character
					eatComment = true;
					continue;
				} else if (ch == ' ' || ch == '\t') {
					// space or tab separates the individual arg strings
					NextArgvIfNotEmpty();
					continue;
				} else if (!sawBackslash && ch == '\\') {
					// the next character is escaped
					sawBackslash = true;
					continue;
				}
			}
			if (!inSingleQuote && ch == '"') {
				// enter/exit double quote handling
				inDoubleQuote = !inDoubleQuote;
				continue;
			}
			if (!inDoubleQuote && ch == '\'') {
				// enter/exit single quote handling
				inSingleQuote = !inSingleQuote;
				continue;
			}
		} else {
			// we just pass through the escape sequence as is
			currentArgs[++currentArgsPos] = '\\';
			sawBackslash = false;
		}
		currentArgs[++currentArgsPos] = ch;
	}

	return result;
}


SettingsArgvDispatcher::SettingsArgvDispatcher(const char *name)
	:	name(name)
{
}

void 
SettingsArgvDispatcher::SaveSettings(Settings *settings, bool onlyIfNonDefault)
{
	if (!onlyIfNonDefault || NeedsSaving()) {
		settings->Write("%s ", Name());
		SaveSettingValue(settings);
		settings->Write("\n");
	}
}

bool 
SettingsArgvDispatcher::HandleRectValue(BRect &result, const char *const *argv,
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
SettingsArgvDispatcher::WriteRectValue(Settings *setting, BRect rect)
{
	setting->Write("%d %d %d %d", (int32)rect.left, (int32)rect.top,
		(int32)rect.right, (int32)rect.bottom);
}

#if 0
static int
CompareByNameOne(const SettingsArgvDispatcher *item1, const SettingsArgvDispatcher *item2)
{
	return strcmp(item1->Name(), item2->Name());
}
#endif

Settings::Settings(const char *filename, const char *settingsDirName)
	:	fileName(filename),
		settingsDir(settingsDirName),
		list(0),
		count(0),
		listSize(30),
		currentSettings(0)
{
#ifdef SINGLE_SETTING_FILE
	settingsHandler = this;
#endif
	list = (SettingsArgvDispatcher **)calloc(listSize, sizeof(SettingsArgvDispatcher *));
}


Settings::~Settings()
{
	for (int32 index = 0; index < count; index++)
		delete list[index];
	
	free(list);
}


const char *
Settings::ParseUserSettings(int, const char *const *argv, void *castToThis)
{
	if (!*argv)
		return 0;

#ifdef SINGLE_SETTING_FILE
	Settings *settings = settingsHandler;
#else
	Settings *settings = (Settings *)castToThis;
#endif

	SettingsArgvDispatcher *handler = settings->Find(*argv);
	if (!handler)
		return "unknown command";
	return handler->Handle(argv);
}


#if 0
static int
Compare(const SettingsArgvDispatcher *p1, const SettingsArgvDispatcher *p2)
{
	return strcmp(p1->Name(), p2->Name());
}
#endif

bool 
Settings::Add(SettingsArgvDispatcher *setting)
{
	// check for uniqueness
	if (Find(setting->Name()))
		return false;

	if (count >= listSize) {
		listSize += 30;
		list = (SettingsArgvDispatcher **)realloc(list,
			listSize * sizeof(SettingsArgvDispatcher *));
	}
	list[count++] = setting;
	return true;
}

SettingsArgvDispatcher *
Settings::Find(const char *name)
{
	for (int32 index = 0; index < count; index++)
		if (strcmp(name, list[index]->Name()) == 0)
			return list[index];

	return 0;
}

void 
Settings::TryReadingSettings()
{
	BPath prefsPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &prefsPath, true) == B_OK) {
		prefsPath.Append(settingsDir);

		BPath path(prefsPath);
		path.Append(fileName);
		ArgvParser::EachArgv(path.Path(), Settings::ParseUserSettings, this);
	}
}

void 
Settings::SaveSettings(bool onlyIfNonDefault)
{
	ASSERT(SettingsHandler());
	SettingsHandler()->SaveCurrentSettings(onlyIfNonDefault);
}

void 
Settings::MakeSettingsDirectory(BDirectory *resultingSettingsDir)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return;
	
	// make sure there is a directory
	path.Append(settingsDir);
	mkdir(path.Path(), 0777);
	resultingSettingsDir->SetTo(path.Path());
}

void 
Settings::SaveCurrentSettings(bool onlyIfNonDefault)
{
	BDirectory settingsDir;
	MakeSettingsDirectory(&settingsDir);

	if (settingsDir.InitCheck() != B_OK)
		return;
printf("+++++++++++ Settings::SaveCurrentSettings %s\n", fileName);
	// nuke old settings
	BEntry entry(&settingsDir, fileName);
	entry.Remove();
	
	BFile prefs(&entry, O_RDWR | O_CREAT);
	if (prefs.InitCheck() != B_OK)
		return;

	currentSettings = &prefs;
	for (int32 index = 0; index < count; index++) {
		list[index]->SaveSettings(this, onlyIfNonDefault);
	}

	currentSettings = 0;
}

void 
Settings::Write(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	VSWrite(format, args);
	va_end(args);
}

void 
Settings::VSWrite(const char *format, va_list arg)
{
	char buffer[2048];
	vsprintf(buffer, format, arg);
	ASSERT(currentSettings && currentSettings->InitCheck() == B_OK);
	currentSettings->Write(buffer, strlen(buffer));
}

#ifdef SINGLE_SETTING_FILE
Settings *Settings::settingsHandler = 0;
#endif
