/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
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
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <GraphicsDefs.h>
#include <Message.h>
#include <File.h>
#include <Entry.h>
#include <NodeInfo.h>

#include "Coding.h"
#include "PrefHandler.h"
#include "TermConst.h"

/*
 * Startup preference settings.
   */
const prefDefaults  termDefaults[] ={
  { PREF_COLS,			"80" },
  { PREF_ROWS,			"25" },

  { PREF_HALF_FONT_FAMILY,	"Courier10 BT" },
  { PREF_HALF_FONT_SIZE,	"12" },
  { PREF_FULL_FONT_FAMILY,	"Haru Tohaba" },
  { PREF_FULL_FONT_SIZE,	"12" },

  { PREF_TEXT_FORE_COLOR,	"  0,   0,   0" },
  { PREF_TEXT_BACK_COLOR,	"255, 255, 255" },
  { PREF_SELECT_FORE_COLOR,	"255, 255, 255" },
  { PREF_SELECT_BACK_COLOR,	"  0,   0,   0" },
  { PREF_CURSOR_FORE_COLOR,	"255, 255, 255" },
  { PREF_CURSOR_BACK_COLOR,	"  0,   0,   0" },

  { PREF_IM_FORE_COLOR,		"  0,   0,   0" },
  { PREF_IM_BACK_COLOR,		"152, 203, 255" },
  { PREF_IM_SELECT_COLOR,	"255, 152, 152" },

  { PREF_SHELL,			"/bin/sh -login" },
  { PREF_HISTORY_SIZE,		"500" },

  { PREF_TEXT_ENCODING,		"UTF-8" },

  { PREF_SELECT_MBUTTON,	"Button 1"},
  { PREF_PASTE_MBUTTON,		"Button 2"},
  { PREF_SUBMENU_MBUTTON,	"Button 3"},
  { PREF_MOUSE_IMAGE,		"Hand cursor"},
  { PREF_DRAGN_COPY,		"0"},

  { PREF_GUI_LANGUAGE,		"English"},
  { PREF_IM_AWARE,		"0"},
  { NULL, NULL},
};

/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
PrefHandler::PrefHandler()
{
  mPrefContainer.what = 'Pref';
  OpenText(TERM_PREF, termDefaults);
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
PrefHandler::PrefHandler(const PrefHandler* p)
{
  mPrefContainer = p->mPrefContainer;
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
PrefHandler::~PrefHandler()
{
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
status_t
PrefHandler::Open(const char *path, const prefDefaults *defaults)
{
  BEntry ent(path);
  if(ent.Exists()){
    return loadFromFile(&ent);
  }else{
    return loadFromDefault(defaults);
  }
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
status_t
PrefHandler::OpenText(const char *path,  const prefDefaults *defaults)
{
  BEntry ent(path);
  if(ent.Exists()){
    return loadFromTextFile(path);
  }else{
    return loadFromDefault(defaults);
  }
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
status_t
PrefHandler::Save(const char *path)
{
  status_t sts;
  BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
  sts = mPrefContainer.Flatten(&file);
  return sts;
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
void
PrefHandler::SaveAsText(const char *path, const char *mimetype,
			const char *signature)
{
  BFile file(path, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
  type_code type;
  char *key;
  char buf[256];
  
  for (int32 i = 0; mPrefContainer.GetInfo(B_STRING_TYPE, i, &key, &type) == B_OK; i++){  
    int len = sprintf(buf, "\"%s\" , \"%s\"\n", key, this->getString(key));
    file.Write(buf, len);
  }
  
  if(mimetype != NULL){
    BNodeInfo info(&file);
    info.SetType(mimetype);
    info.SetPreferredApp (signature);
  }  
  
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
int32 
PrefHandler::getInt32(const char *key)
{
  return atoi(mPrefContainer.FindString(key));
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
float 
PrefHandler::getFloat(const char *key)
{
  return atof(mPrefContainer.FindString(key));
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
const char* 
PrefHandler::getString(const char *key)
{
  const char *buf;

  if (mPrefContainer.FindString(key, &buf) != B_OK)
    buf = "Error!";

  //printf("%x GET %s: %s\n", this, key, buf);
  return buf;
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////
bool
PrefHandler::getBool(const char *key)
{
  const char *s = mPrefContainer.FindString(key);
  if (!strcmp(s, PREF_TRUE)) return true;
  return false;
}
/////////////////////////////////////////////////////////////////////////////
// getRGB
// Returns RGB data from given key. 
/////////////////////////////////////////////////////////////////////////////
rgb_color
PrefHandler::getRGB(const char *key)
{
  int r, g, b;
  rgb_color col;
  const char *s = mPrefContainer.FindString(key);
  sscanf(s, "%d, %d, %d", &r, &g, &b);
  col.red = r;
  col.green = g;
  col.blue = b;
  col.alpha = 0;
  return col;
}
/////////////////////////////////////////////////////////////////////////////
// setInt32
// Setting Int32 data with key.
/////////////////////////////////////////////////////////////////////////////
void
PrefHandler::setInt32(const char *key, int32 data)
{
  char buf[20];
  sprintf(buf, "%d", (int)data);
  this->setString(key, buf);
}
/////////////////////////////////////////////////////////////////////////////
// setFloat
// Setting Float data with key
/////////////////////////////////////////////////////////////////////////////
void
PrefHandler::setFloat(const char *key, float data)
{
  char buf[20];
  sprintf(buf, "%g", data);
  this->setString(key, buf);
}
/////////////////////////////////////////////////////////////////////////////
// setBool
// Setting Bool data with key
/////////////////////////////////////////////////////////////////////////////
void
PrefHandler::setBool(const char *key, bool data)
{
  if(data){
    this->setString(key, PREF_TRUE);
  }else{
    this->setString(key, PREF_FALSE);
  }
}
/////////////////////////////////////////////////////////////////////////////
// this->setString
// Setting CString data with key
/////////////////////////////////////////////////////////////////////////////
void
PrefHandler::setString(const char *key, const char *data)
{
  //printf("%x SET %s: %s\n", this, key, data);
  mPrefContainer.RemoveName(key);
  mPrefContainer.AddString(key, data);
}
/////////////////////////////////////////////////////////////////////////////
// setRGB
// Setting RGB data with key
/////////////////////////////////////////////////////////////////////////////
void
PrefHandler::setRGB(const char *key, const rgb_color data)
{
  char buf[20];
  sprintf(buf, "%d, %d, %d", data.red, data.green, data.blue);
  this->setString(key, buf);
}
/////////////////////////////////////////////////////////////////////////////
// IsEmpty
// Check any peference stored or not.
/////////////////////////////////////////////////////////////////////////////
bool
PrefHandler::IsEmpty() const
{
  return mPrefContainer.IsEmpty();
}
/////////////////////////////////////////////////////////////////////////////
//
//
/////////////////////////////////////////////////////////////////////////////

status_t
PrefHandler::loadFromFile(BEntry *ent)
{
  // Future: It would be nice if we could simply use a flatened BMessage to
  // save the settings. (Who cares about compatibility in this case anyway?)

  BFile file (ent, B_READ_ONLY);
  //mPrefContainer.MakeEmpty();
  //mPrefContainer.Unflatten(&file);
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
