/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_H
#define SUDOKU_H

#include <Application.h>

class BMessage;
class SudokuWindow;

class Sudoku : public BApplication {
public:
	Sudoku();
	virtual ~Sudoku();

	virtual void ReadyToRun();

	virtual void RefsReceived(BMessage *message);
	virtual void MessageReceived(BMessage *message);

	virtual void AboutRequested();
	static void DisplayAbout();

private:
	SudokuWindow* fWindow;
};

extern const char* kSignature;

#endif	// SUDOKU_H
