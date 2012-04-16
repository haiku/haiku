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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Sudoku"


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


//	#pragma mark -


int
main(int /*argc*/, char** /*argv*/)
{
	srand(system_time() % INT_MAX);

	Sudoku sudoku;
	sudoku.Run();

	return 0;
}
