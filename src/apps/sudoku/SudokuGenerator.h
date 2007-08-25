/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_GENERATOR_H
#define SUDOKU_GENERATOR_H


#include <vector>

#include <Messenger.h>
#include <SupportDefs.h>

class SudokuField;


class SudokuGenerator {
public:
	SudokuGenerator();
	~SudokuGenerator();

	void Generate(SudokuField* field, uint32 fieldsLeft,
		BMessenger progress, volatile bool *quit);

private:
	void _Progress(BMessenger progress, const char* text, float percent);
	bool _HasOnlyOneSolution(SudokuField& field);
};

#endif	// SUDOKU_GENERATOR_H
