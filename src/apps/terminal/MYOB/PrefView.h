/* Hey Emacs, this file is -*- c++ -*- 

  Multilingual Terminal Emulator "MuTerminal".
 
 Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. ALL RIGHT RESERVED
  
 This file is part of MuTerminal.

 MuTerminal is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 PrefView.h,v 2.8 1999/10/02 15:17:30 kaz Exp
 PrefView.h,v: Interface for Preference Tab Container View.

***************************************************************************/

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
