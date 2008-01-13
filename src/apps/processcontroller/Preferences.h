/*
	
	Preferences.h
	
	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
	
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
