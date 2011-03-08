/*
 * Copyright 2007-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Sudoku.h"

#include "SudokuWindow.h"

#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <TextView.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Sudoku"


const char* kSignature = "application/x-vnd.Haiku-Sudoku";


Sudoku::Sudoku()
	:
	BApplication(kSignature)
{
}


Sudoku::~Sudoku()
{
}


void
Sudoku::ReadyToRun()
{
	fWindow = new SudokuWindow();
	fWindow->Show();
}


void
Sudoku::RefsReceived(BMessage* message)
{
	fWindow->PostMessage(message);
}


void
Sudoku::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
}


void
Sudoku::AboutRequested()
{
	Sudoku::DisplayAbout();
}


void
Sudoku::DisplayAbout()
{
	BAlert *alert = new BAlert(B_TRANSLATE("About"), B_TRANSLATE("Sudoku\n"
		"\twritten by Axel Dörfler\n"
		"\tCopyright 2007, Haiku Inc.\n"), B_TRANSLATE("OK"));
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE);
	view->SetFontAndColor(0, strlen(B_TRANSLATE("Sudoku")), &font);

	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	srand(system_time() % INT_MAX);

	Sudoku sudoku;
	sudoku.Run();

	return 0;
}
