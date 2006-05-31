/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#include <iostream.h>

#include <Alert.h>
#include <Window.h>
#include <Application.h>
#include <Beep.h>
#include <Screen.h>

#include "Calculator.h"
#include "CalcWindow.h"
#include "Prefs.h"

typedef struct
{
	int h;
	int v;
} Prefs;

int main(int argc, char **argv)
{
	// If we have command-line arguments, calculate and
	// return.  Otherwise, run the graphical interface.
	if(argc > 1) {
		for (int i=1; i<argc; i++) {
			char *str = argv[i];
			CalcEngine engine;
	
			while (*str != '\0')
				engine.DoCommand(*str++);
			
			engine.DoCommand('='); // send an extra one, just in case
			printf("%.15g\n", engine.GetValue());
		}
	}
	else {
		CalculatorApp app;
		app.Run();
	}

	return B_OK;
}


CalculatorApp::CalculatorApp(void)
 : BApplication("application/x-vnd.Haiku-Calculator")
{
	Prefs prefs;
	if (ReadPrefs("calculator.prefs", &prefs, sizeof(prefs)) != true) {
		prefs.h = 20;
		prefs.v = 70;
	}

	BScreen screen;
	BRect r = screen.Frame();
	r.left += 3;
	r.top += 23;
	r.right -= (20-5);
	r.bottom -= -3;
	
	if(prefs.h < r.left)
		prefs.h = (int)r.left;
	if(prefs.h > r.right)
		prefs.h = (int)r.right;
	if(prefs.v < r.top)
		prefs.v = (int)r.top;
	if(prefs.v > r.bottom)
		prefs.v = (int)r.bottom;
	

	wind = new CalcWindow(BPoint(prefs.h, prefs.v));
	wind->Show();
}


void CalculatorApp::AboutRequested()
{
	CalcView::About();
}

