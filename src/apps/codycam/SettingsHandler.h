#ifndef __SETTINGS_FILE__
#define __SETTINGS_FILE__

#include <SupportDefs.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class BFile;
class BDirectory;
class BRect;
class Settings;


typedef const char *(*ArgvHandler)(int argc, const char *const *argv, void *params);
	// return 0 or error string if parsing failed
	

const int32 kBufferSize = 1024;

class ArgvParser {
	// this class opens a text file and passes the context in argv
	// format to a specified handler
public:
	static status_t EachArgv(const char *name,
		ArgvHandler argvHandlerFunc, void *passThru);

private:
	ArgvParser(const char *name);
	~ArgvParser();

	status_t EachArgvPrivate(const char *name,
		ArgvHandler argvHandlerFunc, void *passThru);
	char GetCh();
	
	status_t SendArgv(ArgvHandler argvHandlerFunc, void *passThru);
		// done with a whole line of argv, send it off and get ready
		// to build a new one

	void NextArgv();
		// done with current string, get ready to start building next
	void NextArgvIfNotEmpty();
		// as above, don't commint current string if empty

	void MakeArgvEmpty();

	FILE *file;
	char *buffer;
	int32 pos;
	int32 numAvail;
	
	int argc;
	char **currentArgv;

	int32 currentArgsPos;
	char currentArgs [1024];

	bool sawBackslash;
	bool eatComment;
	bool inDoubleQuote;
	bool inSingleQuote;

	int32 lineNo;
	const char *fileName;
};

class SettingsArgvDispatcher {
	// base class for a single setting item
public:
	SettingsArgvDispatcher(const char *name);

	void SaveSettings(Settings *settings, bool onlyIfNonDefault);
	
	const char *Name() const
		{ return name; }
		// name as it appears in the settings file

	virtual const char *Handle(const char *const *argv) = 0;
		// override this adding an argv parser that reads in the
		// values in argv format for this setting
		// return a pointer to an error message or null if parsed OK

	
	// some handy reader/writer calls
	bool HandleRectValue(BRect &, const char *const *argv, bool printError = true);
	// static bool HandleColorValue(rgb_color &, const char *const *argv, bool printError = true);
	void WriteRectValue(Settings *, BRect);
	// void WriteColorValue(BRect);

protected:
	virtual void SaveSettingValue(Settings *settings) = 0;
		// override this to save the current value of this setting in a
		// text format
		
	virtual bool NeedsSaving() const
		{ return true; }
		// override to return false if current value is equal to the default
		// and does not need saving
private:
	const char *name;
};

class Settings {
	// this class is a list of all the settings handlers, reads and
	// saves the settings file
public:
	Settings(const char *filename, const char *settingsDirName);
	~Settings();
	void TryReadingSettings();
	void SaveSettings(bool onlyIfNonDefault = true);

#ifdef SINGLE_SETTING_FILE
	static Settings *SettingsHandler()
		{ return settingsHandler; }
#else
	Settings *SettingsHandler()
		{ return this; }
#endif

	bool Add(SettingsArgvDispatcher *);
		// return false if argv dispatcher with the same name already
		// registered

	void Write(const char *format, ...);
	void VSWrite(const char *, va_list);

#ifdef SINGLE_SETTING_FILE
	static Settings *settingsHandler;
#endif

private:
	void MakeSettingsDirectory(BDirectory *);

	SettingsArgvDispatcher *Find(const char *);
	static const char *ParseUserSettings(int, const char *const *argv, void *);
	void SaveCurrentSettings(bool onlyIfNonDefault);

	const char *fileName;
	const char *settingsDir;
	SettingsArgvDispatcher **list;
	int32 count;
	int32 listSize;
	BFile *currentSettings;
};


#endif
