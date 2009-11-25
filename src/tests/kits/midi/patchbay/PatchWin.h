// PatchWin.h
// ----------
// The main PatchBay window class.
//
// Copyright 1999, Be Incorporated.   All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.

#ifndef _PatchWin_h
#define _PatchWin_h

#include <Window.h>

class PatchView;

class PatchWin : public BWindow
{
public:
	PatchWin();
	bool QuitRequested();
private:
	PatchView* m_patchView;
};

#endif /* _PatchWin_h */
