/*
 * Copyright 2000, Georges-Edouard Berenger. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <Locker.h>
#include <Message.h>
#include <Window.h>


class Preferences : public BMessage, public BLocker {
	public:
		Preferences(const char* thename, const char* thesignature = NULL, bool doSave = true);
		Preferences(const entry_ref& ref, const char* thesignature = NULL, bool doSave = true);
		~Preferences();
		status_t	MakeEmpty();
		void		SaveWindowPosition(BWindow* window, const char* name);
		void		LoadWindowPosition(BWindow* window, const char* name);
		void		SaveWindowFrame(BWindow* window, const char* name);
		void		LoadWindowFrame(BWindow* window, const char* name);
		void		SaveInt32(int32 val, const char* name);
		bool		ReadInt32(int32& val, const char* name);
		void		SaveFloat(float val, const char* name);
		bool		ReadFloat(float& val, const char* name);
		void		SaveRect(BRect& rect, const char* name);
		BRect&		ReadRect(BRect& rect, const char* name);
		void		SaveString(BString& string, const char* name);
		void		SaveString(const char* string, const char* name);
		bool		ReadString(BString& string, const char* name);

	private:
		bool		fNewPreferences;
		bool		fSavePreferences;
		char*		fName;
		char*		fSignature;
		entry_ref*	fSettingsFile;
};

extern Preferences gPreferences;

// ggPreferences.LoadWindowPosition(this, kPosPrefName);
// ggPreferences.SaveWindowPosition(this, kPosPrefName);

// ggPreferences.LoadWindowFrame(this, frame);
// ggPreferences.SaveWindowFrame(this, frame);

#endif // PREFERENCES_H
