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

#include <Menu.h>
#include <string.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Font.h>
#include <stdio.h>

#include "MenuUtil.h"
#include "TermConst.h"
#include "PrefHandler.h"
#include "Coding.h"

extern PrefHandler *gTermPref;

#define LOCALE_FILE_DIR PREF_FOLDER"menu/"

//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
BPopUpMenu *
MakeFontMenu(ulong msg, const char *defaultFontName)
{
  BPopUpMenu *menu = new BPopUpMenu("font");
  int32 numFamilies = count_font_families(); 
  
  for ( int32 i = 0; i < numFamilies; i++ ) { 
    font_family family; 
    uint32 flags; 

    if ( get_font_family(i, &family, &flags) == B_OK ) { 
      menu->AddItem(new BMenuItem(family, new BMessage(msg)));
      if(!strcmp(defaultFontName, family)){
        (menu->ItemAt(i))->SetMarked(true);
      }
    }
  }
  
  return menu;
}
//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
BPopUpMenu *
MakeMenu(ulong msg, const char **items, const char *defaultItemName)
{
  BPopUpMenu *menu = new BPopUpMenu("");

  int32 i = 0;
  while(*items) {  
    menu->AddItem(new BMenuItem(*items, new BMessage(msg)));
    if(!strcmp(*items, defaultItemName)){
      (menu->ItemAt(i))->SetMarked(true);
    }
    items++;
    i++;
  }
  
  return menu;
}

////////////////////////////////////////////////////////////////////////////
// 
// 
////////////////////////////////////////////////////////////////////////////
int
longname2op(const char *longname)
{
int op = M_UTF8;
const etable *s = encoding_table;

  for (int i = 0; s->name; s++, i++){
    if(!strcmp(s->name, longname)) {
      op = s->op;
      break;
    }
  }
  
  return op;
}

////////////////////////////////////////////////////////////////////////////
// 
// 
////////////////////////////////////////////////////////////////////////////
const char*
op2longname(int op)
{
  return encoding_table[op].name;
}

//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
void
MakeEncodingMenu(BMenu *eMenu, int coding, bool flag)
{
  const etable *e = encoding_table;
  int i = 0;
  while(e->name){
    BMessage *msg = new BMessage(MENU_ENCODING);
    msg->AddInt32("op", (int32)e->op);
    if (flag) {
      eMenu->AddItem(new BMenuItem(e->name, msg, e->shortcut));
    }
    else {
      eMenu->AddItem(new BMenuItem(e->name, msg));
    }
      
    if ( i == coding) {
      (eMenu->ItemAt(i))->SetMarked(true);
    }
    e++;
    i++;
  }
}

//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////
void
LoadLocaleFile (PrefHandler *pref)
{

  const char *locale;
  char buf[B_PATH_NAME_LENGTH];
  status_t sts;
  
  /*
   * Open menu localize file.
   */
  locale = gTermPref->getString (PREF_GUI_LANGUAGE);
  sprintf (buf, "%s%s", LOCALE_FILE_DIR, locale);

  sts = pref->OpenText (buf);

  if (sts == B_ERROR)
    pref->OpenText (LOCALE_FILE_DEFAULT);

  return;
  
}
