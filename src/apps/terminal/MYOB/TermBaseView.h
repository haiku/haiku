/* Hey Emacs, this file is -*- c++ -*- 

  Multilingual Terminal Emulator "MuTerminal".
 
 Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. ALL RIGHT RESERVED
  
 This file is part of MuTerminal.

 MuTerminal is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.

 TermBaseView.h,v 2.3 1999/10/13 15:00:59 kaz Exp
 TermBaseView.h,v: Interface for BView sub class.

***************************************************************************/

#ifndef TERMBASEVIEW_H_INCLUDED
#define TERMBASEVIEW_H_INCLUDED
#include <View.h>

class TermView;

class TermBaseView : public BView
{
public:
  //
  // Constructor and Desctructor
  //
  TermBaseView (BRect frame, TermView *);
  ~TermBaseView ();

private:
  //
  // Hook Functions.
  //
  void SetBaseColor (rgb_color);
  
  void FrameResized (float, float);

  TermView *fTermView;
  
};

#endif /* TERMBASEVIEW_H_INCLUDED */
