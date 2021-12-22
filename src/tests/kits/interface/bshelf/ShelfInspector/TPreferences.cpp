//
// TPreferences
//
// Class for saving and loading preference information
// via BMessages.
//
// Eric Shepherd
//

/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
	
	Modified by H. Reh Dec. 2001
*/

#include <Message.h>
#include <Messenger.h>
#include <File.h>
#include <FindDirectory.h>
#include <Beep.h>		// wieder entfernen

#include "TPreferences.h"


//
// TPreferences::TPreferences
//
// Open the settings file and read the data in.
//
TPreferences::TPreferences(char *filename) : BMessage('pref') 
// TPreferences von BMessage ableiten, 'pref' = Message-Konstante
{
	BFile file;
	
	status = find_directory(B_COMMON_SETTINGS_DIRECTORY, &path);
	// B_COMMON_SETTINGS_DIRECTORY  = Ordner in dem Settings stehen
	if (status != B_OK) 
	{
		return;
	}
	
	path.Append(filename);									// Filenamen anhängen
	
	path.GetParent(&parent);								// übergeordnetes Verzeichnis suchen
	create_directory(parent.Path(), 0777);	// Verzeichnis anlegen
	parent.Unset();													// File schließen
	
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) 
	{
		status = Unflatten(&file);
		// da file eine Datei ist, ruft Unflatten sofort Read() auf
	}
}


//
// TPreferences::~TPreferences
//
// Write the preferences to disk.
//
TPreferences::~TPreferences() 
{
	BFile file;
	
	if (file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE) == B_OK) 
	{
		Flatten(&file);
		// da file eine Datei ist, ruft Flatten sofort Write() auf
	}
}


status_t TPreferences::SetBool(const char *name, bool b) 
{
	if (HasBool(name)) 									// Testen auf Typ bool
	{
		return ReplaceBool(name, 0, b);		// Wenn Variable schon existiert -> überschreiben -> Funktion beenden
	}
	return AddBool(name, b);						// Wenn Variable nicht existiert -> neu anlegen -> Funktion beenden
}

status_t TPreferences::SetInt8(const char *name, int8 i) 
{
	if (HasInt8(name)) 
	{
		return ReplaceInt8(name, 0, i);
	}
	return AddInt8(name, i);
}

status_t TPreferences::SetInt16(const char *name, int16 i) 
{
	if (HasInt16(name)) 
	{
		return ReplaceInt16(name, 0, i);
	}
	return AddInt16(name, i);
}

status_t TPreferences::SetInt32(const char *name, int32 i) 
{
	if (HasInt32(name)) 
	{
		return ReplaceInt32(name, 0, i);
	}
	return AddInt32(name, i);
}

status_t TPreferences::SetInt64(const char *name, int64 i) 
{
	if (HasInt64(name)) 
	{
		return ReplaceInt64(name, 0, i);
	}
	return AddInt64(name, i);
}

status_t TPreferences::SetFloat(const char *name, float f) 
{
	if (HasFloat(name)) 
	{
		return ReplaceFloat(name, 0, f);
	}
	return AddFloat(name, f);
}

status_t TPreferences::SetDouble(const char *name, double f) 
{
	if (HasDouble(name)) 
	{
		return ReplaceDouble(name, 0, f);
	}
	return AddDouble(name, f);
}

status_t TPreferences::SetString(const char *name, const char *s) 
{
	if (HasString(name)) 
	{
		return ReplaceString(name, 0, s);
	}
	return AddString(name, s);
}

status_t TPreferences::SetPoint(const char *name, BPoint p) 
{
	if (HasPoint(name)) 
	{
		return ReplacePoint(name, 0, p);
	}
	return AddPoint(name, p);
}

status_t TPreferences::SetRect(const char *name, BRect r) 
{
	if (HasRect(name)) 
	{
		return ReplaceRect(name, 0, r);
	}
	return AddRect(name, r);
}

status_t TPreferences::SetMessage(const char *name, const BMessage *message) 
{
	if (HasMessage(name)) 
	{
		return ReplaceMessage(name, 0, message);
	}
	return AddMessage(name, message);
}

status_t TPreferences::SetFlat(const char *name, const BFlattenable *obj) 
{
	if (HasFlat(name, obj)) 
	{
		return ReplaceFlat(name, 0, (BFlattenable *) obj);
	}
	return AddFlat(name, (BFlattenable *) obj);
}

status_t TPreferences::SetData(const char *name, type_code type,const void *data, ssize_t numBytes) 
{
	if (HasData(name, type)) 
	{
		return ReplaceData(name, type, 0, data, numBytes);
	}
	return AddData(name, type, data, numBytes);
}

status_t TPreferences::SetRef(const char *name, entry_ref *ref) 
{
	if (HasRef(name)) 
	{
		return ReplaceRef(name, 0, ref);
	}
	return AddRef(name, ref);
}

