/*
 * Copyright 2007-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SudokuGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Catalog.h>

#include "ProgressWindow.h"
#include "SudokuField.h"
#include "SudokuSolver.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SudokuGenerator"


SudokuGenerator::SudokuGenerator()
{
}


SudokuGenerator::~SudokuGenerator()
{
}


bool
SudokuGenerator::_HasOnlyOneSolution(SudokuField& field)
{
	SudokuSolver solver(&field);
	solver.ComputeSolutions();

	return solver.CountSolutions() == 1;
}


void
SudokuGenerator::_Progress(BMessenger progress, const char* text,
	float percent)
{
	BMessage update(kMsgProgressStatusUpdate);
	if (text)
		update.AddString("message", text);
	update.AddFloat("percent", percent);
	progress.SendMessage(&update);
}


void
SudokuGenerator::Generate(SudokuField* target, uint32 fieldsLeft,
	BMessenger progress, volatile bool *quit)
{
	// first step: generate a solved field - random brute force style

	SudokuField field(target->BlockSize());
	uint32 inputCount = field.Size() * field.Size() / 3;
	_Progress(progress, B_TRANSLATE("Creating solvable field"), 5.f);

	while (!*quit) {
		field.Reset();

		// generate input field

		uint32 validMask = 0;

		for (uint32 i = 0; i < inputCount; i++) {
			uint32 x;
			uint32 y;
			do {
				x = rand() % field.Size();
				y = rand() % field.Size();
			} while (!*quit && field.ValueAt(x, y) != 0);

			validMask = field.ValidMaskAt(x, y);
			if (validMask == 0)
				break;

			uint32 value;
			do {
				value = rand() % field.Size();
			} while (!*quit && (validMask & (1UL << value)) == 0);

			field.SetValueAt(x, y, value + 1);
		}

		if (validMask == 0)
			continue;

		// try to solve it

		SudokuSolver solver(&field);
		solver.ComputeSolutions();
		if (solver.CountSolutions() > 0) {
			// choose a random solution
			field.SetTo(solver.SolutionAt(rand() % solver.CountSolutions()));
			break;
		}
	}

	if (*quit)
		return;

	// next step: try to remove as many fields as possible (and wished)
	// that still have only a single solution

	int32 removeCount = field.Size() * field.Size() - fieldsLeft;
	bool tried[field.Size() * field.Size()];
	int32 tries = field.Size() * field.Size() * 3 / 4;
	memset(tried, 0, sizeof(tried));
	_Progress(progress, B_TRANSLATE("Searching for removable values"), 30.f);

	while (!*quit && removeCount > 0 && tries-- > 0) {
		SudokuField copy(field);
		uint32 x;
		uint32 y;
		do {
			x = rand() % field.Size();
			y = rand() % field.Size();
		} while (copy.ValueAt(x, y) == 0 || tried[x + y * field.Size()]);

		tried[x + y * field.Size()] = true;
		copy.SetValueAt(x, y, 0);

		if (_HasOnlyOneSolution(copy)) {
			_Progress(progress, NULL, 100.f - (70.f * removeCount / 70.f));
			field.SetTo(&copy);
			removeCount--;
		}
	}

	if (*quit)
		return;

	if (tries <= 0) {
		puts("check all remove");
		for (uint32 y = 0; y < field.Size(); y++) {
			for (uint32 x = 0; x < field.Size(); x++) {
				if (tried[x + y * field.Size()])
					continue;

				SudokuField copy(field);
				copy.SetValueAt(x, y, 0);

				if (_HasOnlyOneSolution(copy)) {
					_Progress(progress, NULL,
						100.f - (70.f * removeCount / 70.f));
					field.SetTo(&copy);

					if (--removeCount <= 0 || *quit)
						break;
				}
			}

			if (removeCount <= 0 || *quit)
				break;
		}
		printf("  remove count = %ld\n", removeCount);
	}

	// set the remaining values to be initial values

	for (uint32 y = 0; y < field.Size(); y++) {
		for (uint32 x = 0; x < field.Size(); x++) {
			if (field.ValueAt(x, y))
				field.SetFlagsAt(x, y, kInitialValue);
		}
	}

	if (*quit)
		return;

	target->SetTo(&field);
}

