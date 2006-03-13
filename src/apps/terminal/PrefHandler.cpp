/*
 * Copyright (c) 2003-2006, Haiku, Inc. All Rights Reserved.
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Distributed unter the terms of the MIT License.
 */


#include "Coding.h"
#include "PrefHandler.h"
#include "TermConst.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Font.h>
#include <GraphicsDefs.h>
#include <Message.h>
#include <NodeInfo.h>
#include <Path.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * Startup preference settings.
 */
static const pref_defaults kTermDefaults[] = {
	{ PREF_COLS,				"80" },
	{ PREF_ROWS,				"25" },

	{ PREF_HALF_FONT_FAMILY,	"Courier10 BT" },
	{ PREF_HALF_FONT_SIZE,		"12" },
	{ PREF_FULL_FONT_FAMILY,	"Haru Tohaba" },
	{ PREF_FULL_FONT_SIZE,		"12" },

	{ PREF_TEXT_FORE_COLOR,		"  0,   0,   0" },
	{ PREF_TEXT_BACK_COLOR,		"255, 255, 255" },
	{ PREF_SELECT_FORE_COLOR,	"255, 255, 255" },
	{ PREF_SELECT_BACK_COLOR,	"  0,   0,   0" },
	{ PREF_CURSOR_FORE_COLOR,	"255, 255, 255" },
	{ PREF_CURSOR_BACK_COLOR,	"  0,   0,   0" },

	{ PREF_IM_FORE_COLOR,		"  0,   0,   0" },
	{ PREF_IM_BACK_COLOR,		"152, 203, 255" },
	{ PREF_IM_SELECT_COLOR,		"255, 152, 152" },

	{ PREF_SHELL,				"/bin/sh -login" },
	{ PREF_HISTORY_SIZE,		"500" },

	{ PREF_TEXT_ENCODING,		"UTF-8" },

	{ PREF_SELECT_MBUTTON,		"Button 1"},
	{ PREF_PASTE_MBUTTON,		"Button 2"},
	{ PREF_SUBMENU_MBUTTON,		"Button 3"},
	{ PREF_MOUSE_IMAGE,			"Hand cursor"},
	{ PREF_DRAGN_COPY,			"0"},

	{ PREF_GUI_LANGUAGE,		"English"},
	{ PREF_IM_AWARE,			"0"},
	{ NULL, NULL},
};


PrefHandler::PrefHandler()
	:
	fContainer('Pref')
{
	_LoadFromDefault(kTermDefaults);

	BPath path;
	GetDefaultPath(path);
	OpenText(path.Path());

	_ConfirmFont(PREF_HALF_FONT_FAMILY, be_fixed_font);
	_ConfirmFont(PREF_FULL_FONT_FAMILY, be_fixed_font);
}


PrefHandler::PrefHandler(const PrefHandler* p)
{
	fContainer = p->fContainer;
}


PrefHandler::~PrefHandler()
{
}


/* static */
status_t
PrefHandler::GetDefaultPath(BPath& path)
{
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return B_ERROR;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	path.Append("Terminal_settings");
#else
	path.Append("HaikuTerminal_settings");
#endif

	return B_OK;
}


status_t
PrefHandler::Open(const char *path)
{
	return _LoadFromFile(path);
}


status_t
PrefHandler::OpenText(const char *path)
{
	return _LoadFromTextFile(path);
}


status_t
PrefHandler::Save(const char *path)
{
	// make sure the target path exists
#if 0
	// TODO: currently not needed as we're reusing the standard directory
	BPath directoryPath(path);
	if (directoryPath.GetParent(&directoryPath) == B_OK)
		create_directory(directoryPath.Path(), 0755);
#endif

	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	return fContainer.Flatten(&file);
}


void
PrefHandler::SaveAsText(const char *path, const char *mimetype,
	const char *signature)
{
	// make sure the target path exists
#if 0
	BPath directoryPath(path);
	if (directoryPath.GetParent(&directoryPath) == B_OK)
		create_directory(directoryPath.Path(), 0755);
#endif

	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	char buffer[512];
	type_code type;
	const char *key;

	for (int32 i = 0;
#ifdef B_BEOS_VERSION_DANO
			fContainer.GetInfo(B_STRING_TYPE, i, &key, &type) == B_OK;
#else
			fContainer.GetInfo(B_STRING_TYPE, i, (char**)&key, &type) == B_OK;
#endif
			i++) {
		int len = snprintf(buffer, sizeof(buffer), "\"%s\" , \"%s\"\n", key, getString(key));
		file.Write(buffer, len);
	}

	if (mimetype != NULL){
		BNodeInfo info(&file);
		info.SetType(mimetype);
		info.SetPreferredApp(signature);
	}
}


int32 
PrefHandler::getInt32(const char *key)
{
	const char *value = fContainer.FindString(key);
	if (value == NULL)
		return 0;

	return atoi(value);
}


float 
PrefHandler::getFloat(const char *key)
{
	const char *value = fContainer.FindString(key);
	if (value == NULL)
		return 0;

	return atof(value);
}


const char* 
PrefHandler::getString(const char *key)
{
	const char *buffer;
	if (fContainer.FindString(key, &buffer) != B_OK)
		buffer = "Error!";

	//printf("%x GET %s: %s\n", this, key, buf);
	return buffer;
}


bool
PrefHandler::getBool(const char *key)
{
	const char *value = fContainer.FindString(key);
	if (value == NULL)
		return false;

	return !strcmp(value, PREF_TRUE);
}


/** Returns RGB data from given key. */

rgb_color
PrefHandler::getRGB(const char *key)
{
	rgb_color col;
	int r, g, b;

	if (const char *s = fContainer.FindString(key)) {
		sscanf(s, "%d, %d, %d", &r, &g, &b);
	} else {
		fprintf(stderr, "PrefHandler::getRGB(%s) - key not found\n", key);
		r = g = b = 0;
	}

	col.red = r;
	col.green = g;
	col.blue = b;
	col.alpha = 255;
	return col;
}


/** Setting Int32 data with key. */

void
PrefHandler::setInt32(const char *key, int32 data)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%d", (int)data);
	setString(key, buffer);
}


/** Setting Float data with key */

void
PrefHandler::setFloat(const char *key, float data)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%g", data);
	setString(key, buffer);
}


/** Setting Bool data with key */

void
PrefHandler::setBool(const char *key, bool data)
{
	if (data)
		setString(key, PREF_TRUE);
	else
		setString(key, PREF_FALSE);
}


/** Setting CString data with key */

void
PrefHandler::setString(const char *key, const char *data)
{
	//printf("%x SET %s: %s\n", this, key, data);
	fContainer.RemoveName(key);
	fContainer.AddString(key, data);
}


/** Setting RGB data with key */

void
PrefHandler::setRGB(const char *key, const rgb_color data)
{
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%d, %d, %d", data.red, data.green, data.blue);
	setString(key, buffer);
}


/** Check any peference stored or not. */

bool
PrefHandler::IsEmpty() const
{
	return fContainer.IsEmpty();
}


void
PrefHandler::_ConfirmFont(const char *key, const BFont *fallback)
{
	int32 count = count_font_families();
	const char *font = getString(key);
	if (font == NULL)
		count = 0;

	font_family family;

	for (int32 i = 0; i < count; i++) {
		if (get_font_family(i, &family) != B_OK)
			continue;

		if (!strcmp(family, font)) {
			// found font family: we can safely use this font
			return;
		}
	}

	// use fall-back font

	fallback->GetFamilyAndStyle(&family, NULL);
	setString(key, family);
}


status_t
PrefHandler::_LoadFromFile(const char* path)
{
	// Future: It would be nice if we could simply use a flatened BMessage to
	// save the settings. (Who cares about compatibility in this case anyway?)

	BFile file(path, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	//fContainer.MakeEmpty();
	//fContainer.Unflatten(&file);

	off_t size;
	if (file.GetSize(&size) != B_OK || size != sizeof(struct termprefs))
		return B_ERROR;

	struct termprefs prefs;
	file.Read(&prefs, size);
	if (prefs.magic != TP_MAGIC || prefs.version != TP_VERSION)
		return B_ERROR;

	// Valid settings file!

	setInt32(PREF_COLS, prefs.cols);
	setInt32(PREF_ROWS, prefs.rows);
	setInt32(PREF_HALF_FONT_SIZE, prefs.font_size);
	setInt32(PREF_FULL_FONT_SIZE, prefs.font_size);
	char *font_family = strtok(prefs.font, "/");
	char *font_style = strtok(NULL, "");
	setString(PREF_FULL_FONT_FAMILY, font_family);
	setString(PREF_FULL_FONT_STYLE, font_style);
	setString(PREF_HALF_FONT_FAMILY, font_family);
	setString(PREF_HALF_FONT_STYLE, font_style);
	setRGB(PREF_TEXT_BACK_COLOR, prefs.bg);
	setRGB(PREF_TEXT_FORE_COLOR, prefs.fg);
	setRGB(PREF_CURSOR_BACK_COLOR, prefs.curbg);
	setRGB(PREF_CURSOR_FORE_COLOR, prefs.curfg);
	setRGB(PREF_SELECT_BACK_COLOR, prefs.selbg);
	setRGB(PREF_SELECT_FORE_COLOR, prefs.selfg);
	setString(PREF_TEXT_ENCODING, encoding_table[prefs.encoding].name);

	return B_OK;
}


status_t
PrefHandler::_LoadFromDefault(const pref_defaults* defaults)
{
	if (defaults == NULL)
		return B_ERROR;

	while (defaults->key) {
		setString(defaults->key, defaults->item);
		++defaults;
	}

	return B_OK;
}


/**	Text is "key","Content"
 *	Comment : Start with '#'
 */

status_t
PrefHandler::_LoadFromTextFile(const char * path)
{
	char buffer[1024];
	char key[B_FIELD_NAME_LENGTH], data[512];
	int n;
	FILE *file;

	file = fopen(path, "r");
	if (file == NULL)
		return B_ENTRY_NOT_FOUND;

	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		if (*buffer == '#')
			continue;

		n = sscanf(buffer, "%*[\"]%[^\"]%*[\"]%*[^\"]%*[\"]%[^\"]", key, data);
		if (n == 2)
			setString(key, data);
	}

	fclose(file);
	return B_OK;
}
