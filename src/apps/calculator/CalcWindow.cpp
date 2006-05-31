/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#include "Calculator.h"
#include "CalcWindow.h"
#include "CalcView.h"
#include "Prefs.h"

typedef struct
{
	int h;
	int v;
} Prefs;


CalcWindow::CalcWindow(BPoint pt)
 : BWindow(CalcView::TheRect(pt), "Calculator", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	BRect r = CalcView::TheRect();

	SetSizeLimits(r.right, r.right, r.bottom, r.bottom+CalcView::ExtraHeight());
	SetZoomLimits(r.right, r.bottom+CalcView::ExtraHeight());

	calc_view = new CalcView(r);
	AddChild(calc_view);

	BView *extraView = calc_view->AddExtra();
	AddChild(extraView);
}


bool CalcWindow::QuitRequested()
{
	Prefs prefs;
	prefs.h = (int)(Frame().left);
	prefs.v = (int)(Frame().top);

	WritePrefs("calculator.prefs", &prefs, sizeof(prefs));

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
