/*
 * Part of the MiniTerminal.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the Haiku License.
 */

#include "MiniWin.h"
#include "MiniView.h"

MiniWin::MiniWin(BRect bounds)
	:	BWindow(bounds, "MiniTerminal", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
{
	fView = new MiniView(bounds.OffsetToSelf(0, 0));
	AddChild(fView);
	fView->MakeFocus();
}


MiniWin::~MiniWin()
{
}


MiniView *
MiniWin::View()
{
	return fView;
}
