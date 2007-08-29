/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SUDOKU_SOLVER_H
#define SUDOKU_SOLVER_H


#include <vector>

#include <SupportDefs.h>

class SudokuField;

class SudokuSolver {
public:
	SudokuSolver(SudokuField* field);
	SudokuSolver();
	~SudokuSolver();

	void SetTo(SudokuField* field);

	void ComputeSolutions();

	uint32 CountSolutions();
	SudokuField* SolutionAt(uint32 index);

private:
	void _MakeEmpty();

	typedef std::vector<SudokuField*> SudokuList;

	SudokuField*	fField;
	SudokuList		fSolutions;
};

#endif	// SUDOKU_SOLVER_H
