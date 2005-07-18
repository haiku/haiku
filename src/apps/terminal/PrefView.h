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

#ifndef PREFVIEW_H_INCLUDED
#define PREFVIEW_H_INCLUDED
#include <View.h>
#include <ColorControl.h>
#include <String.h>

class PrefHandler;
class BRect;
class BBox;
class TTextControl;
class BStringView;
class BRadioButton;
class BButton;
class BCheckBox;
class BMenuField;

enum {
  PREF_APP_VIEW,
  PREF_SHELL_VIEW,
  PREF_LANG_VIEW
};

//
// Appearance Message
//
const ulong MSG_HALF_FONT_CHANGED	= 'mchf';
const ulong MSG_HALF_SIZE_CHANGED	= 'mchs';
const ulong MSG_FULL_FONT_CHANGED	= 'mcff';
const ulong MSG_FULL_SIZE_CHANGED	= 'mcfs';
const ulong MSG_COLOR_FIELD_CHANGED	= 'mccf';
const ulong MSG_COLOR_CHANGED		= 'mcbc';

//
// ShellMessage
//
const ulong MSG_COLS_CHANGED		= 'mccl';
const ulong MSG_ROWS_CHANGED		= 'mcrw';
const ulong MSG_HISTORY_CHANGED		= 'mhst';

const ulong MSG_LANG_CHANGED		= 'mclg';
const ulong MSG_INPUT_METHOD_CHANGED	= 'mcim';

//
// TextControl Modification Message.
//
const ulong MSG_TEXT_MODIFIED		= 'tcmm';
const ulong MSG_PREF_MODIFIED		= 'mpmo';


extern PrefHandler *gTermPref;

class PrefView : public BView
{
	public:
		PrefView (BRect frame, const char *name);
  		virtual       ~PrefView();
  		const char *  ViewLabel (void) const;
  		virtual bool  CanApply ();
  		virtual void	MessageReceived (BMessage *msg);

  		BColorControl* SetupBColorControl	(BPoint p, color_control_layout layout, float cell_size, ulong msg);

  	private:
  		BString fLabel;

};

#endif //PREFVIEW_H_INCLUDED
