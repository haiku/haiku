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
#ifndef PREFHANDLER_H_INCLUDED
#define PREFHANDLER_H_INCLUDED

#include <SupportDefs.h>
#include <Errors.h>
#include <GraphicsDefs.h>
#include <Message.h>

#define TP_MAGIC 0xf1f2f3f4
#define TP_VERSION 0x02
#define TP_FONT_NAME_SZ 128

struct termprefs {
        uint32 magic;
        uint32 version;
        float x;
        float y;
        uint32 cols;
        uint32 rows;
        uint32 tab_width;
        uint32 font_size;
        char font[TP_FONT_NAME_SZ]; // "Family/Style"
        uint32 cursor_blink_rate; // blinktime in µs = 1000000
        uint32 refresh_rate; // ??? = 0
        rgb_color bg;
        rgb_color fg;
        rgb_color curbg;
        rgb_color curfg;
        rgb_color selbg;
        rgb_color selfg;
        char encoding; // index in the menu (0 = UTF-8)
        char unknown[3];
};

struct prefDefaults
{
  const char *key;
  char *item;
};

#define PREF_TRUE "true"
#define PREF_FALSE "false"

class BMessage;
class BEntry;

class PrefHandler{
public:
  PrefHandler();
  PrefHandler(const PrefHandler* p);

  ~PrefHandler();

  status_t    Open(const char *name, const prefDefaults *defaults = NULL);
  status_t    OpenText(const char *path, const prefDefaults *defaults = NULL);
  status_t    Save(const char *name);
  void        SaveAsText(const char *path, const char *minmtype = NULL,
			 const char *signature = NULL);

  int32       getInt32(const char *key);
  float       getFloat(const char *key);
  const char* getString(const char *key);
  bool        getBool(const char *key);
  rgb_color   getRGB(const char *key);

  void        setInt32(const char *key, int32 data);
  void        setFloat(const char *key, float data);
  void        setString(const char *key, const char *data);
  void        setBool(const char *key, bool data);
  void        setRGB(const char *key, const rgb_color data);

  bool        IsEmpty() const;

private:
  status_t    loadFromFile(BEntry *ent);
  status_t    loadFromDefault(const prefDefaults* defaluts = NULL);
  status_t    loadFromTextFile(const char * path);


  BMessage    mPrefContainer;


};

#endif //PREFHANDLER_H_INCLUDED
