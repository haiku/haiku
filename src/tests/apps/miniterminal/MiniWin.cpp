/*
 * Part of the MiniTerminal.
 *
 * Copyright 2005 Michael Lotz. All rights reserved.
 * Distributed under the MIT License.
 */

#include "Arguments.h"
#include "MiniView.h"
#include "MiniWin.h"

MiniWin::MiniWin(const Arguments &args)
	:	BWindow(args.Bounds(), args.Title(), B_TITLED_WINDOW,
			B_QUIT_ON_WINDOW_CLOSE)
{
	fView = new MiniView(args);
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
