/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "SudokuSolver.h"

#include "SudokuField.h"
#include "Stack.h"


struct SolutionStep {
public:
	SolutionStep(const SudokuField* field);
	SolutionStep(const SolutionStep& other);
	~SolutionStep();

	void ToFirstUnset();
	bool ToNextUnset();

	void SetSolvedFields();

	SudokuField* Field() { return fField; }
	uint32 X() { return fX; }
	uint32 Y() { return fY; }

private:
	SudokuField*	fField;
	uint32			fX;
	uint32			fY;
};

typedef std::vector<SolutionStep*> StepList;


uint32
bit_count(uint32 value)
{
	uint32 count = 0;
	while (value > 0) {
		if (value & 1)
			count++;
		value >>= 1;
	}
	return count;
}


//	#pragma mark -


SolutionStep::SolutionStep(const SudokuField* _field)
{
	fField = new SudokuField(*_field);
	fX = 0;
	fY = 0;
}


SolutionStep::SolutionStep(const SolutionStep& other)
{
	fField = new SudokuField(*other.fField);
	fX = other.fX;
	fY = other.fY;
}


SolutionStep::~SolutionStep()
{
	delete fField;
}


void
SolutionStep::ToFirstUnset()
{
	for (uint32 y = 0; y < fField->Size(); y++) {
		for (uint32 x = 0; x < fField->Size(); x++) {
			if (!fField->ValueAt(x, y)) {
				uint32 validMask = fField->ValidMaskAt(x, y);
				if (bit_count(validMask) == 1) {
					// If the chosen value is already solved, we set its
					// value here and go on - this makes sure the first
					// unset we return has actually more than one possible
					// value
					uint32 value = 0;
					while ((validMask & (1UL << value)) == 0) {
						value++;
					}
					fField->SetValueAt(x, y, value + 1, true);
					continue;
				}

				fX = x;
				fY = y;
				return;
			}
		}
	}
	
}


bool
SolutionStep::ToNextUnset()
{
	for (uint32 y = fY; y < fField->Size(); y++) {
		for (uint32 x = 0; x < fField->Size(); x++) {
			if (y == fY && x == 0) {
				x = fX;
				continue;
			}

			if (!fField->ValueAt(x, y)) {
				fX = x;
				fY = y;
				return true;
			}
		}
	}

	return false;
}


//	#pragma mark -


SudokuSolver::SudokuSolver(SudokuField* field)
	:
	fField(field)
{
}


SudokuSolver::SudokuSolver()
	:
	fField(NULL)
{
}


SudokuSolver::~SudokuSolver()
{
	// we don't own the field but the solutions
	_MakeEmpty();
}


void
SudokuSolver::_MakeEmpty()
{
	for (uint32 i = 0; i < fSolutions.size(); i++) {
		delete fSolutions[i];
	}
}


void
SudokuSolver::SetTo(SudokuField* field)
{
	fField = field;
}


void
SudokuSolver::ComputeSolutions()
{
	_MakeEmpty();

	// We need to check if generating a solution is affordable with a
	// brute force algorithm like this one
	uint32 set = 0;
	for (uint32 y = 0; y < fField->Size(); y++) {
		for (uint32 x = 0; x < fField->Size(); x++) {
			if (fField->ValueAt(x, y))
				set++;
		}
	}
	if (set < fField->Size() * fField->Size() / 6)
		return;

	Stack<SolutionStep*> stack;
	SolutionStep* step = new SolutionStep(fField);
	step->ToFirstUnset();

	stack.Push(step);
	uint32 count = 0;

	// brute force version

	while (stack.Pop(&step)) {
		uint32 x = step->X();
		uint32 y = step->Y();
		uint32 validMask = step->Field()->ValidMaskAt(x, y);

		count++;

		if (step->ToNextUnset()) {
			if (validMask != 0) {
				// generate further steps
				for (uint32 i = 0; i < fField->Size(); i++) {
					if ((validMask & (1UL << i)) == 0)
						continue;

					SolutionStep* next = new SolutionStep(*step);
					next->Field()->SetValueAt(x, y, i + 1, true);
					stack.Push(next);
				}
			}
		} else if (step->Field()->IsSolved())
			fSolutions.push_back(new SudokuField(*step->Field()));

		delete step;
	}

	//printf("evaluated %lu steps\n", count);
}


uint32
SudokuSolver::CountSolutions()
{
	return fSolutions.size();
}


SudokuField*
SudokuSolver::SolutionAt(uint32 index)
{
	return fSolutions[index];
}

