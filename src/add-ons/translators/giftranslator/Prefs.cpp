////////////////////////////////////////////////////////////////////////////////
//
//	File: Prefs.cpp
//
//	Date: December 1999
//
//	Author: Daniel Switkin
//
//	Copyright 2003 (c) by Daniel Switkin. This file is made publically available
//	under the BSD license, with the stipulations that this complete header must
//	remain at the top of the file indefinitely, and credit must be given to the
//	original author in any about box using this software.
//
////////////////////////////////////////////////////////////////////////////////

#include "Prefs.h"
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>
#include <stdio.h>

extern bool debug;

Prefs::Prefs() {
	interlaced = usetransparent = usetransparentindex = false;
	usedithering = true;
	transparentindex = transparentred = transparentgreen = 
		transparentblue = palettemode = 0;
	
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("GIFTranslator_settings");
	file = new BFile(path.Path(), B_READ_WRITE | B_CREATE_FILE);
	
	if (file->InitCheck() != B_OK) {
		if (debug) printf("Bad filename for attributes\n");
		return;
	}
	
	bool db = false;
	if (!GetBool("interlaced", &interlaced, &db)) return;
	db = false;
	if (!GetBool("usetransparent", &usetransparent, &db)) return;
	db = false;
	if (!GetBool("usetransparentindex", &usetransparentindex, &db)) return;
	db = true;
	if (!GetBool("usedithering", &usedithering, &db)) return;
	int di = 0;
	if (!GetInt("palettemode", &palettemode, &di)) return;
	di = 0;
	if (!GetInt("transparentindex", &transparentindex, &di)) return;
	di = 0;
	if (!GetInt("transparentred", &transparentred, &di)) return;
	di = 0;
	if (!GetInt("transparentgreen", &transparentgreen, &di)) return;
	di = 0;
	if (!GetInt("transparentblue", &transparentblue, &di)) return;
}

bool Prefs::GetInt(char *name, int *value, int *defaultvalue) {
	status_t err = file->ReadAttr(name, B_INT32_TYPE, 0, value, 4);
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (file->WriteAttr(name, B_INT32_TYPE, 0, defaultvalue, 4) < 0) {
			if (debug) printf("WriteAttr on %s died\n", name);
			return false;
		}
	} else if  (err < 0) {
		if (debug) printf("Unknown error reading %s\n", name);
		return false;
	}
	return true;
}

bool Prefs::GetBool(char *name, bool *value, bool *defaultvalue) {
	status_t err = file->ReadAttr(name, B_BOOL_TYPE, 0, value, 1);
	if (err == B_ENTRY_NOT_FOUND) {
		*value = *defaultvalue;
		if (file->WriteAttr(name, B_BOOL_TYPE, 0, defaultvalue, 1) < 0) {
			if (debug) printf("WriteAttr on %s died\n", name);
			return false;
		}
	} else if (err < 0) {
		if (debug) printf("Unknown error reading %s\n", name);
		return false;
	}
	return true;
}

bool Prefs::PutInt(char *name, int *value) {
	status_t err = file->WriteAttr(name, B_INT32_TYPE, 0, value, 4);
	if (err < 0) {
		if (debug) printf("WriteAttr on %s died\n", name);
		return false;
	}
	return true;
}

bool Prefs::PutBool(char *name, bool *value) {
	status_t err = file->WriteAttr(name, B_BOOL_TYPE, 0, value, 1);
	if (err < 0) {
		if (debug) printf("WriteAttr on %s died\n", name);
		return false;
	}
	return true;
}

void Prefs::Save() {
	if (!PutBool("interlaced", &interlaced)) return;
	if (!PutBool("usetransparent", &usetransparent)) return;
	if (!PutBool("usetransparentindex", &usetransparentindex)) return;
	if (!PutBool("usedithering", &usedithering)) return;
	if (!PutInt("palettemode", &palettemode)) return;
	if (!PutInt("transparentindex", &transparentindex)) return;
	if (!PutInt("transparentred", &transparentred)) return;
	if (!PutInt("transparentgreen", &transparentgreen)) return;
	if (!PutInt("transparentblue", &transparentblue)) return;
}

Prefs::~Prefs() {
	delete file;
}

