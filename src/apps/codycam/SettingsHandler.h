#ifndef SETTINGS_HANDLER_H
#define SETTINGS_HANDLER_H


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SupportDefs.h>


class BFile;
class BDirectory;
class BRect;
class Settings;


typedef const char* (*ArgvHandler)(int argc, const char *const *argv, void *params);
	// return 0 or error string if parsing failed


const int32 kBufferSize = 1024;


class ArgvParser {
	public:
		static status_t EachArgv(const char* name,
			ArgvHandler argvHandlerFunc, void* passThru);

	private:
		ArgvParser(const char* name);
		~ArgvParser();

		status_t EachArgvPrivate(const char* name,
			ArgvHandler argvHandlerFunc, void* passThru);
		char GetCh();

		status_t SendArgv(ArgvHandler argvHandlerFunc, void* passThru);
			// done with a whole line of argv, send it off and get ready
			// to build a new one

		void NextArgv();
			// done with current string, get ready to start building next
		void NextArgvIfNotEmpty();
			// as above, don't commint current string if empty

		void MakeArgvEmpty();

		FILE*	fFile;
		char*	fBuffer;
		int32	fPos;
		int32	fNumAvail;

		int		fArgc;
		char**	fCurrentArgv;

		int32	fCurrentArgsPos;
		char	fCurrentArgs[1024];

		bool	fSawBackslash;
		bool	fEatComment;
		bool	fInDoubleQuote;
		bool	fInSingleQuote;

		int32	fLineNo;
		const char* fFileName;
};


class SettingsArgvDispatcher {
		// base class for a single setting item
	public:
		SettingsArgvDispatcher(const char* name);
		virtual ~SettingsArgvDispatcher() {};

		void SaveSettings(Settings* settings, bool onlyIfNonDefault);

		const char* Name() const
		{
			return fName;
		}

		virtual const char* Handle(const char* const *argv) = 0;
			// override this adding an argv parser that reads in the
			// values in argv format for this setting
			// return a pointer to an error message or null if parsed OK

		bool HandleRectValue(BRect&, const char* const *argv,
			bool printError = true);

		// static bool HandleColorValue(rgb_color &, const char *const *argv, bool printError = true);
		void WriteRectValue(Settings*, BRect);
		// void WriteColorValue(BRect);

	protected:
		virtual void SaveSettingValue(Settings* settings) = 0;
			// override this to save the current value of this setting in a
			// text format

		virtual bool NeedsSaving() const
		{
			return true;
		}
		// override to return false if current value is equal to the default
		// and does not need saving
	private:
		const char* fName;
};


class Settings {
	public:
		Settings(const char* filename, const char* settingsDirName);
		~Settings();
		void TryReadingSettings();
		void SaveSettings(bool onlyIfNonDefault = true);

#ifdef SINGLE_SETTING_FILE
		static Settings* SettingsHandler()
		{
			return settingsHandler;
		}
#else
		Settings* SettingsHandler()
		{
			return this;
		}
#endif

		bool Add(SettingsArgvDispatcher *);

		void Write(const char* format, ...);
		void VSWrite(const char*, va_list);

#ifdef SINGLE_SETTING_FILE
		static Settings* settingsHandler;
#endif

	private:
		void _MakeSettingsDirectory(BDirectory*);

		SettingsArgvDispatcher* _Find(const char*);
		static const char* _ParseUserSettings(int, const char *const *argv, void*);
		void _SaveCurrentSettings(bool onlyIfNonDefault);

		const char* fFileName;
		const char* fSettingsDir;
		SettingsArgvDispatcher** fList;
		int32 fCount;
		int32 fListSize;
		BFile* fCurrentSettings;
};

#endif	// SETTINGS_HANDLER_H
