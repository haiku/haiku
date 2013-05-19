/* PatchWin.h
 * ----------
 * The main PatchBay window class.
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef PATCHWIN_H
#define PATCHWIN_H

#include <Window.h>

class PatchView;

class PatchWin : public BWindow
{
public:
	PatchWin();
	bool QuitRequested();
private:
	PatchView* fPatchView;
};

#endif /* PATCHWIN_H */
