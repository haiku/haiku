/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Copyright (c) 2004 Daniel Furrer <assimil8or@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "Coding.h"
#include "PrefHandler.h"
#include "TermConst.h"

#include <GraphicsDefs.h>
#include <Message.h>
#include <File.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 * Startup preference settings.
   */
const prefDefaults  termDefaults[] ={
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
{
	fContainer.what = 'Pref';

	BPath path;
	GetDefaultPath(path);
	OpenText(path.Path(), termDefaults);
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

	// TODO: maybe just "Terminal"? (but this collides with the R5 Terminal settings file)
	path.Append("HaikuTerminal");
	path.Append("settings");

	return B_OK;
}


status_t
PrefHandler::Open(const char *path, const prefDefaults *defaults)
{
	BEntry entry(path);
	if (entry.Exists())
		return loadFromFile(&entry);

	return loadFromDefault(defaults);
}


status_t
PrefHandler::OpenText(const char *path, const prefDefaults *defaults)
{
	BEntry entry(path);
	if (entry.Exists())
		return loadFromTextFile(path);

	return loadFromDefault(defaults);
}


status_t
PrefHandler::Save(const char *path)
{
	// make sure the target path exists
	BPath directoryPath(path);
	if (directoryPath.GetParent(&directoryPath) == B_OK)
		create_directory(directoryPath.Path(), 0755);

	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	return fContainer.Flatten(&file);
}


void
PrefHandler::SaveAsText(const char *path, const char *mimetype,
	const char *signature)
{
	// make sure the target path exists
	BPath directoryPath(path);
	if (directoryPath.GetParent(&directoryPath) == B_OK)
		create_directory(directoryPath.Path(), 0755);

	BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	char buffer[512];
	type_code type;
	char *key;

	for (int32 i = 0; fContainer.GetInfo(B_STRING_TYPE, i, &key, &type) == B_OK; i++) {
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


status_t
PrefHandler::loadFromFile(BEntry *ent)
{
  // Future: It would be nice if we could simply use a flatened BMessage to
  // save the settings. (Who cares about compatibility in this case anyway?)

  BFile file (ent, B_READ_ONLY);
  //fContainer.MakeEmpty();
  //fContainer.Unflatten(&file);
  off_t size;
  if (file.GetSize(&size) != B_OK || size != sizeof(struct termprefs))
    return B_ERROR;

  struct termprefs prefs;
  file.Read(&prefs, size);
  if (prefs.magic != TP_MAGIC || prefs.version != TP_VERSION)
    return B_ERROR;

  //Valid settings file!
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
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////

status_t
PrefHandler::loadFromDefault(const prefDefaults* defaluts)
{
  if(defaluts == NULL) return B_ERROR;
  
  while(defaluts->key){
    this->setString(defaluts->key, defaluts->item);  
    ++defaluts;
  }
  return B_OK;
}
/////////////////////////////////////////////////////////////////////////////
//
// Text is "key","Content"
// Comment : Start with '#'
/////////////////////////////////////////////////////////////////////////////

status_t
PrefHandler::loadFromTextFile(const char * path)
{
char buf[1024];
char key[B_FIELD_NAME_LENGTH], data[512];
int n;
FILE *fp;

fp = fopen(path, "r");

  while(fgets(buf, sizeof(buf), fp) != NULL){
    if (*buf == '#') continue;
      n = sscanf(buf, "%*[\"]%[^\"]%*[\"]%*[^\"]%*[\"]%[^\"]", key, data);
      if (n == 2) this->setString(key, data);
  }

  fclose(fp);

  return B_OK;
}
